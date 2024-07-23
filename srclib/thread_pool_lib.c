/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  thread_pool_lib.c - Implementa el manejo de threads en lotes *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/thread_pool_lib.h"
#include "../includes/client_conn_lib.h"
#include "../includes/signal_lib.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>

/* Estructura de un trabajo para ejecutar holder_execution_job */
typedef struct HolderJob {
  int id;        // batch id
  int position;  // posicion en el batch
  void *jobInfo; // informacion del trabajo creado
} HolderJob;

/* Estructura de una lista doblemente enlazada e
 * informacion del lote, conteniendo informacion para cada hilo */
typedef struct ThreadBatch {
  struct ThreadBatch *prevBatch;
  int id;
  pthread_t threads[THREADBATCHCOUNT];
  pthread_mutex_t mutex[THREADBATCHCOUNT];
  HolderJob holderJobs[THREADBATCHCOUNT];
  struct ThreadBatch *nextBatch;
} ThreadBatch;

/* Funcion que se llama al iniciar un trabajo */
void *(*client_function)(void *);
/* Funcion que se llama al limpiar un trabajo */
void (*cleanup_function)(void *);

/* Tengo garantia de que se inicializa a 0 todo, porque es global */
/* Primer lote necesario para el pool de hilos */
ThreadBatch firstBatch;
/* Variable que indica si los hilos deben de suicidarse */
u_int8_t suicide;

/********
 * FUNCIÓN: static ThreadBatch *find_batch(int id)
 * ARGS_IN: int id - identificador del batch
 * DESCRIPCIÓN: Busca el batch con el identificador id
 * ARGS_OUT: ThreadBatch * - devuelve la batch con el idendificador especificado
 ********/
static ThreadBatch *find_batch(int id) {
  ThreadBatch *batch = &firstBatch;
  for (int i = 0; i < id; i++) {
    batch = batch->nextBatch;
  }
  return batch;
}

/********
 * FUNCIÓN: static void empty_function(void *arg)
 * ARGS_IN: void *arg - No usado
 * DESCRIPCIÓN: Funcion auxiliar vacía para asignar pthread_cleanup_push
 *              en caso de que no se pase ninguna funcion al iniciar el pool
 ********/
static void empty_function(void *arg) {}

/********
 * FUNCIÓN: static void signal_usr1_handler()
 * DESCRIPCIÓN: Maneja la señal usr1
 ********/
static void signal_usr1_handler() {}

/********
 * FUNCIÓN: static void *holder_execution_job(void *args)
 * ARGS_IN: void *args - Puntero a HolderJob que contiene informacion para el hilo
 * DESCRIPCIÓN: Se mantiene en espera hasta que se desbloquea para ejecutar la funcion del servidor
 * ARGS_OUT: void * - No devuelve informacion util. NULL
 ********/
static void *holder_execution_job(void *args) {
  HolderJob *hJob = (HolderJob *)args;
  ThreadBatch *myBatch = find_batch(hJob->id);

  pthread_cleanup_push(cleanup_function, &hJob->jobInfo);

  while (hJob->jobInfo == NULL) {
    int ret = pthread_mutex_lock(&myBatch->mutex[hJob->position]);

    // comprobamos si suicide esta activo porque pthread_mutex_lock no se puede interrumpir nunca, entonces
    // cuando quiero matarlos, la función destroy_all_job pone jobInfo a NULL y hace unlock del mutex. 
    if (ret == -1 || suicide) {
      break;
    }
    // llamada de la funcion del cliente
    (*client_function)(hJob->jobInfo);

    // como ha terminado, jobInfo se pone a NULL
    hJob->jobInfo = NULL;
  }

  pthread_cleanup_pop(1);
  return NULL;
}

/********
 * FUNCIÓN: static int initialize_batch(ThreadBatch *batch, int id)
 * ARGS_IN: ThreadBatch *batch - Lote que se rellena con la informacion necesaria
 *          int id - Identificador a asignar al lote
 * DESCRIPCIÓN: Inicializa el lote
 * ARGS_OUT: int - Devuelve 0 en caso de exito, -1 en caso de error
 ********/
static int initialize_batch(ThreadBatch *batch, int id) {
  batch->id = id;

  for (int i = 0; i < THREADBATCHCOUNT; i++) {
    batch->holderJobs[i].id = id;
    batch->holderJobs[i].position = i;
    int ret = pthread_mutex_init(&batch->mutex[i], NULL);
    if (ret) {
      pthread_kill(batch->threads[i], SIGKILL);
      for (int j = 0; j < i; j++) {
        pthread_kill(batch->threads[j], SIGKILL);
        pthread_mutex_destroy(&batch->mutex[j]);
      }
      return -1;
    }
    pthread_mutex_lock(&batch->mutex[i]);

    ret = pthread_create(&batch->threads[i], NULL, &holder_execution_job, &batch->holderJobs[i]);
    if (ret) {
      for (int j = 0; j < i; j++) {
        pthread_kill(batch->threads[j], SIGKILL);
        pthread_mutex_destroy(&batch->mutex[j]);
      }
      return -1;
    }
  }
  return 0;
}

/********
 * FUNCIÓN: static ThreadBatch *create_new_batch(ThreadBatch *lastBatch)
 * ARGS_IN: ThreadBatch *lastBatch - (opcional) ultima batch de la lista enlazada.
 *                                   Permite asignar la nueva lista más rápido
 * DESCRIPCIÓN: Crea un nuevo lote
 * ARGS_OUT: ThreadBatch * - Devuelve la nueva batch en caso de éxito. NULL en caso de error
 ********/
static ThreadBatch *create_new_batch(ThreadBatch *lastBatch) {
  ThreadBatch *batch = &firstBatch;
  if (lastBatch) {
    // encuentra el ultimo batch mas rapido
    batch = lastBatch;
  }
  int id;
  for (; batch->nextBatch != NULL; batch = batch->nextBatch) {
  }
  id = batch->id + 1;

  ThreadBatch *newBatch = (ThreadBatch *)calloc(1, sizeof(ThreadBatch));
  if (!newBatch) {
    return NULL;
  }

  ThreadBatch *prevBatch = batch;

  batch = newBatch;

  batch->prevBatch = prevBatch;
  prevBatch->nextBatch = batch;

  if (initialize_batch(batch, id) == -1) {
    return NULL;
  }

  return batch;
}

/********
 * FUNCIÓN: static void printInfo()
 * DESCRIPCIÓN: USADO DURANTE EL PROCESO DE DEBUGGING
 *              Imprime el estado actual de los lotes,
 *              teniendo valor 1 si está ejecutandose y 0 si esta esperando
 ********/
/*
static void printInfo() {
  ThreadBatch *batch = &firstBatch;
  printf("\nPrint Info\n");
  for (; batch != NULL; batch = batch->nextBatch) {
    for (int i = 0; i < THREADBATCHCOUNT; i++) {
      int val = batch->holderJobs[i].jobInfo != NULL;
      printf("%d, ", val);
    }
    printf("\n");
  }
}
*/

/********
 * FUNCIÓN: static void destroy_all_job()
 * DESCRIPCIÓN: Destruye todos los trabajos que han sido creados
 ********/
static void destroy_all_job() {
  ThreadBatch *batch = &firstBatch;
  for (; batch != NULL; batch = batch->nextBatch) {
    int i = 0;
    for (HolderJob *job = batch->holderJobs; i < THREADBATCHCOUNT; job++, i++) {
      suicide = 0x01;
      pthread_cancel(batch->threads[i]);
      pthread_mutex_unlock(&batch->mutex[i]);
    }
  }
}


/********
 * FUNCIÓN: int initialize_pool(void *(*client_fun)(void *), void (*cleanup_fun)(void *))
 * ARGS_IN: void *(*client_fun)(void *) - Funcion que recibe un parametro void *.
 *                                        Esta es la funcion que se llamará al añadir un trabajo
 *          void *(*cleanup_fun)(void *) - Funcion que recibe un parametro void *.
 *                                         Esta es la funcion que se llamará al destruir un hilo
 * DESCRIPCIÓN: Inicializa la pool de hilos, estableciendo a su vez las funciones del cliente y limpieza
 * ARGS_OUT: int - Devuelve 0 en caso de exito, -1 en caso de error
 ********/
int initialize_pool(void *(*client_fun)(void *), void (*cleanup_fun)(void *)) {
  client_function = client_fun;
  if (!cleanup_fun)
    cleanup_function = empty_function;
  else
    cleanup_function = cleanup_fun;
  ThreadBatch *batch = &firstBatch;
  establece_manejador(SIGUSR1, signal_usr1_handler);

  return initialize_batch(batch, 0);
}

/********
 * FUNCIÓN: int add_job(void *info)
 * ARGS_IN: void *info - Puntero a la informacion que se quiere delegar al trabajo
 *                       asignado en initialize_pool
 * DESCRIPCIÓN: Inicia un trabajo
 * ARGS_OUT: int - Devuelve el id trabajo (valor no negativo), -1 en caso de error
 ********/
int add_job(void *info) {
  ThreadBatch *batch = &firstBatch;
  u_int8_t createBatch = 0x01;
  do {

    for (int i = 0; i < THREADBATCHCOUNT; i++) {
      // Asegurandonos de que no se esta suicidando
      if (batch->holderJobs[i].jobInfo == NULL) {
        batch->holderJobs[i].jobInfo = info;
        pthread_mutex_unlock(&batch->mutex[i]);
        return THREADBATCHCOUNT * batch->id + i;
      }
    }

    if (batch->nextBatch) {
      batch = batch->nextBatch;
    } else if (createBatch) {
      createBatch = 0x00;
      batch = create_new_batch(batch);
      if (!batch) {
        syslog(LOG_ERR, "Error creating batch\n");
        return -1;
      }
    }

  } while (batch != NULL);

  return -1;
}

/********
 * FUNCIÓN: void terminate_pool()
 * DESCRIPCIÓN: Destruye todos los trabajos activos y libera la memoria
 ********/
void terminate_pool() {
  // printInfo();
  destroy_all_job();
  ThreadBatch *batch = &firstBatch;
  ThreadBatch *lastBatch = batch;
  for (; batch != NULL; batch = batch->nextBatch) {
    for (int i = 0; i < THREADBATCHCOUNT; i++) {
      pthread_kill(batch->threads[i], SIGUSR1);
      pthread_join(batch->threads[i], NULL);
      pthread_mutex_destroy(&batch->mutex[i]);
    }
    if (batch->nextBatch == NULL) {
      lastBatch = batch;
    }
  }
  batch = lastBatch;
  // En este momento batch es el ultimo
  for (; batch->prevBatch != NULL;) {
    ThreadBatch *prevBatch = batch->prevBatch;
    free(batch);
    batch = prevBatch;
  }
  memset(&firstBatch, 0, sizeof(ThreadBatch));
}
