/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  server.c - Implementacion del servidor con pool de hilos     *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/server.h"
#include "../includes/client_conn_lib.h"
#include "../includes/confuse.h"
#include "../includes/signal_lib.h"
#include "../includes/socket_lib.h"
#include "../includes/thread_pool_lib.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h>

/* Variable para saber si hemos recibido una señal SIGINT (Ctrl-C) */
static volatile u_int8_t got_sigint = 0x00;
/* Manejador para la señal SIGINT */
static void signal_int_handler() { got_sigint = 0x01; }

/* Parametros de configuracion leidos por otros archivos e inicializado en el main */
ConfigParameters configParams;

/* Semaphore to limit the max number of connections */
sem_t numConnections;

/* Funciones privadas para leer el config */
int read_config(cfg_t **cfg);
void free_config(cfg_t *cfg);
void do_daemon(void);

/********
 * FUNCIÓN: int main(int argc, char *argv[])
 * ARGS_IN: int argc - numero de argumentos pasados por parametros
 *          char *argv[] - los argumentos como tal
 * DESCRIPCIÓN: Es la funcion que se ejecuta cuando se llama al programa. 
 *              Crea el servidor y maneja todas las peticiones.
 * ARGS_OUT: int - devuelve 0 en caso de éxito y -1 en caso contrario
 ********/
int main(int argc, char *argv[]) {
  int serverfd, connfd;
  syslog(LOG_INFO, "Initiating new server.");

  if (argc > 1) {
    if (strcmp(argv[1], "--daemon") == 0 || strcmp(argv[1], "-d") == 0) {
      syslog(LOG_INFO, "Trying to convert to daemon");
      do_daemon();
    }
  }

  cfg_t *cfg;
  if (read_config(&cfg) != 0)
    return -1;

  if (sem_init(&numConnections, 0, configParams.maxClients) == -1){
    perror("sem_init");
    free_config(cfg);
    return -1;
  }

  establece_manejador(SIGINT, signal_int_handler);
  establece_manejador(SIGPIPE, NULL);
  /* Contiene las llamadas a socket(), bind() y listen() */
  serverfd = initiate_server();
  if (serverfd < 0) {
    printf("Error iniciando servidor\n");
    sem_destroy(&numConnections);
    free_config(cfg);
    return -1;
  }
  printf("Iniciando servidor\n");

  if (initialize_pool(manage_client, free_thread_resources) == -1) {
    close(serverfd);
    sem_destroy(&numConnections);
    free_config(cfg);
    return -1;
  }

  while(got_sigint == 0x00) {
    if (sem_wait(&numConnections) == -1) break;
    connfd = accept_connection(serverfd);
    if (got_sigint == 0x01) break;
    
    struct ClientConnection *cliConn;
    cliConn = (struct ClientConnection *)calloc(1, sizeof(struct ClientConnection));
    if (!cliConn) {
      syslog(LOG_ERR, "Error allocating memory. Exiting");
      close(serverfd);
      break;
    }
    // printf("Iniciada nueva conexion\n");
    cliConn->connfd = connfd;
    if (add_job((void *)cliConn) == -1) {
      syslog(LOG_ERR, "Error adding a job");
      break;
    }
  }
  syslog(LOG_INFO, "Got signal. Terminating");

  terminate_pool();
  sem_destroy(&numConnections);
  close(serverfd);
  free_config(cfg);

  return 0;
}

/********
 * FUNCIÓN: int read_config
 * ARGS_IN: cfg_t **cfg - (output) memoria del puntero de cfg_t, necesario para leer la configuracion
 * DESCRIPCIÓN: Lee los parametros del archivo de configuracion, 
 *              que está en el directorio de trabajo y lo guarda en
 *              la variable global configParams
 * ARGS_OUT: int - devuelve 0 en caso de éxito y otro valor en caso contrario
 ********/
int read_config(cfg_t **cfg) {
  char tmpDir[128], mkDir[128];

  cfg_opt_t opts[] = {// opciones obligatorias que aparecen en el enunciado
                      CFG_SIMPLE_STR("server_root", &configParams.rootPath), CFG_SIMPLE_INT("max_clients", &configParams.maxClients),
                      CFG_SIMPLE_INT("listen_port", &configParams.port), CFG_SIMPLE_STR("server_signature", &configParams.serverSignature),

                      // opciones nuevas que hemos añadido
                      CFG_SIMPLE_STR("base_file", &configParams.baseFile), CFG_SIMPLE_INT("recv_buffer_length", &configParams.recvBufferLen),
                      CFG_SIMPLE_INT("timeout", &configParams.timeout), CFG_SIMPLE_STR("exe_python", &configParams.exe_scripts.python),
                      CFG_SIMPLE_STR("exe_php", &configParams.exe_scripts.php),

                      CFG_END()};

  sprintf(tmpDir, "/tmp/httpserver_%ld/", time(NULL));

  strcpy(mkDir, "mkdir ");
  strcat(mkDir, tmpDir);
  if (system(mkDir) == -1) {
    syslog(LOG_ERR, "No se pudo crear directorio temporal");
    return -1;
  }

  // valores por defecto de la config
  configParams.rootPath = strdup("./www");
  configParams.maxClients = 128;
  configParams.port = 34567;
  configParams.serverSignature = strdup("Redes2Server v0.1 alpha");
  configParams.baseFile = strdup("index.html");
  configParams.recvBufferLen = 8192;
  configParams.timeout = 5 * 60; // 5 minutos de timeout
  configParams.tmpDirectory = strdup(tmpDir);
  configParams.exe_scripts.python = strdup("/usr/bin/python");
  configParams.exe_scripts.php = strdup("/usr/bin/php");

  *cfg = cfg_init(opts, 0);
  if (cfg_parse(*cfg, "server.conf") != CFG_SUCCESS) {
    syslog(LOG_ERR, "No se pudo leer el archivo de configuración");
    return -1;
  }
  return 0;
}

/********
 * FUNCIÓN: void free_config
 * ARGS_IN: cfg_t *cfg - memoria del puntero de cfg_t, necesario para leer la configuracion
 * DESCRIPCIÓN: Libera la memoria asociada a los parametros de configuracion
 ********/
void free_config(cfg_t *cfg) {
  // char removeTmpDir[1024];
  // strcpy(removeTmpDir, "rm -rf ");
  // strcat(removeTmpDir, tmpDir);
  // system(removeTmpDir);
  cfg_free(cfg);
  if (configParams.rootPath)
    free(configParams.rootPath);
  if (configParams.serverSignature)
    free(configParams.serverSignature);
  if (configParams.baseFile)
    free(configParams.baseFile);
  if (configParams.tmpDirectory)
    free(configParams.tmpDirectory);
  if (configParams.exe_scripts.python)
    free(configParams.exe_scripts.python);
  if (configParams.exe_scripts.php)
    free(configParams.exe_scripts.php);
}

/********
 * FUNCIÓN: void do_daemon
 * DESCRIPCIÓN: Demoniza el programa
 ********/
void do_daemon() {
  pid_t pid;

  pid = fork(); /* Fork off the parent process */
  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS); /* Exiting the parent process. */

  pid_t childPID = getpid();

  umask(0);                       /* Change the file mode mask */
  setlogmask(LOG_UPTO(LOG_INFO)); /* Open logs here */
  openlog("Server system messages:", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL3);

  if (setsid() < 0) { /* Create a new SID for the child process */
    syslog(LOG_ERR, "Error creating a new SID for the child process.");
    exit(EXIT_FAILURE);
  }

  printf("Iniciando daemon. PID = %d\n", childPID);
  // syslog(LOG_INFO, "Closing standard file descriptors");
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO); /* Close out the dard file descriptors */
  syslog(LOG_INFO, "Daemon created successfuly. PID = %d", childPID);
  return;
}
