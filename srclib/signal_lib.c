/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  signal_lib.c - Archivo con las funciones necesarias para     *
 *                    manejar las señales entre threads          *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/signal_lib.h"

#include <signal.h>
#include <stdio.h>

/********
 * FUNCIÓN: int establece_manejador(int signal, void (*handler))
 * ARGS_IN: int signal - Señal a manejar
 *          void (*handler) - Funcion que se asigna a la señal
 * DESCRIPCIÓN: Establece el manejador de la señal proporcionada.
 *              En caso de que handler sea NULL, se bloque la señal indicada
 *              si tiene valor positivo y se desbloquea si tiene valor negativo
 * ARGS_OUT: int - devuelve 0 en caso de éxito y -1 en caso contrario
 ********/
int establece_manejador(int signal, void(*handler)) {
  struct sigaction act;
  sigset_t set;
  int block;

  if (handler != NULL) {
    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;
    act.sa_handler = handler;
    if (sigaction(signal, &act, NULL) < 0) {
      perror("sigaction");
      return -1;
    }
  } else {
    if (signal > 0) {
      block = SIG_BLOCK;
    } else {
      block = SIG_UNBLOCK;
      signal *= -1;
    }

    /* Máscara que bloqueará o desbloqueará la señal */
    sigemptyset(&set);
    sigaddset(&set, signal);

    if (pthread_sigmask(block, &set, NULL) < 0) {
      perror("sigprocmask");
      return -1;
    }
  }

  return 0;
}
