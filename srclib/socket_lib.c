/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  socketLib.c - Implementa las funciones básicas de sockets    *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/socket_lib.h"
#include "../includes/server.h"

#include <netinet/in.h>
#include <strings.h>
#include <sys/syslog.h>

/********
 * FUNCIÓN: int initiate_server()
 * DESCRIPCIÓN: Devuelve el descriptor del socket del servidor (o EXIT_FAILURE en caso de error)
 * ARGS_OUT: int - Devuelve el descriptor del servidor en caso de éxito y -1 en caso contrario
 ********/
int initiate_server() {
  int sockval;
  struct sockaddr_in Direccion;

  syslog(LOG_INFO, "Creating socket");
  if ((sockval = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    syslog(LOG_ERR, "Error creating socket");
    return -1;
  }
  /* Violates tcp/ip protocol, as someone could get packets
   * that were sent to the client that was on that connection previously.
   * TODO: change */
  int iSetOption = 1;
  setsockopt(sockval, SOL_SOCKET, SO_REUSEADDR, (char *)&iSetOption, sizeof(iSetOption));

  Direccion.sin_family = AF_INET;                /* TCP/IP family */
  Direccion.sin_port = htons(configParams.port); /* Asigning port */
  Direccion.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept all adresses */
  bzero((void *)&(Direccion.sin_zero), 8);

  syslog(LOG_INFO, "Binding socket");
  if (bind(sockval, (struct sockaddr *)&Direccion, sizeof(Direccion)) < 0) {
    syslog(LOG_ERR, "Error binding socket");
    return -1;
  }

  syslog(LOG_INFO, "Listening connections");
  if (listen(sockval, LISTENMAXCONNECTIONS) < 0) {
    syslog(LOG_ERR, "Error listenining");
    return -1;
  }
  return sockval;
}

/********
 * FUNCIÓN: int establece_manejador(int signal, void (*handler))
 * ARGS_IN: int sockval - Socket del servidor por el que se acepta una conexión
 * DESCRIPCIÓN: Aceptar una nueva conexión de un cliente.
 * ARGS_OUT: int - Devuelve el descriptor del socket del cliente, -1 en caso de error
 ********/
int accept_connection(int sockval) {
  int desc;
  socklen_t len;
  struct sockaddr Conexion;

  len = sizeof(Conexion);

  if ((desc = accept(sockval, &Conexion, &len)) < 0) {
    syslog(LOG_ERR, "Error accepting connection");
    return -1;
  }
  struct timeval tv;
  tv.tv_sec = configParams.timeout;
  tv.tv_usec = 0;
  if (tv.tv_sec > 0)
    setsockopt(desc, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
  return desc;
}
