/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  client_conn_lib.c - Implementación de funciones para         *
 *                      manejar las conexiones con un cliente    *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/client_conn_lib.h"
#include "../includes/client_process_functions.h"
#include "../includes/picohttpparser.h"
#include "../includes/server.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <time.h>
#include <unistd.h>

/* Calcula el minimo */
#define min(a, b) (a < b) ? a : b

/********
 * FUNCIÓN: static int create_and_send_response(ClientConnection *cliConn, RequestContent *request, char *sendBuffer)
 * ARGS_IN: ClientConnection *cliConn - Estructura conteniendo información sobre la conexión
 *          RequestContent *request - Estructura que contiene informacion de la request actual
 *          char *sendBuffer - Buffer donde poder almacenar la respuesta
 * DESCRIPCIÓN: Funcion que llama a la función correspondiente
 *              en funcion del método que contenga la request.
 * ARGS_OUT: int - devuelve 0 en caso de éxito y -1 en caso contrario
 ********/
static int create_and_send_response(ClientConnection *cliConn, RequestContent *request, char *sendBuffer) {
  char *method = request->method;
  int methodLen = request->methodLen;
  int minorVersion = request->minorVersion;

  if (minorVersion != 0 && minorVersion != 1)
    return process_error(request, sendBuffer, cliConn->connfd, HTTP_VER_NOT_SUPP);

  /* OPTIONS only supported in 1.1 */
  if (minorVersion == 1 && strncmp(method, "OPTIONS", min(methodLen, 7)) == 0)
    return process_OPTIONS(request, sendBuffer, cliConn->connfd);

  else if (strncmp(method, "GET", min(methodLen, 3)) == 0)
    return process_GET(request, sendBuffer, cliConn->connfd, &cliConn->fcloseVar);
  else if (strncmp(method, "POST", min(methodLen, 4)) == 0)
    return process_POST(request, sendBuffer, cliConn->connfd, &cliConn->fcloseVar);
  else if (strncmp(method, "HEAD", min(methodLen, 4)) == 0)
    return process_HEAD(request, sendBuffer, cliConn->connfd);

  return process_error(request, sendBuffer, cliConn->connfd, NOT_IMPLEMENTED);
}

/********
 * FUNCIÓN: static int my_parse_request(RequestContent *request, char *recvBuffer, int recvLen)
 * ARGS_IN: RequestContent *request - Estructura a rellenar con informacion de la request
 * DESCRIPCIÓN: Funcion wrapper para parsear una request utilizando la libreria picohttpparser
 * ARGS_OUT: int - devuelve el número de bytes que tiene la cabecera de request en caso de éxito
 *                 y -1 o -2 en caso contrario
 ********/
static int my_parse_request(RequestContent *request) {
  int pRet;
  request->numHeaders = MAXNUMHEADERS;
  pRet = phr_parse_request(request->completeRequest, request->totalLen, (const char **)&request->method, &request->methodLen,
                           (const char **)&request->path, &request->pathLen, &request->minorVersion, request->headers, &request->numHeaders, 0);

  /* Valor que nos permite obtener el puntero al body.
   * Para mas detalles ver: https://github.com/h2o/picohttpparser/issues/59 */
  request->requestLen = pRet;

  // syslog(LOG_DEBUG, "HTTP version: 1.%d", request->minorVersion);
  return pRet;
}

/********
 * FUNCIÓN: void free_thread_resources(void *arg)
 * ARGS_IN: void *arg - Memoria de un puntero a ClientConnection. Es void* pues
 *                      se usa con pthread_cancel_push
 * DESCRIPCIÓN: Funcion encargada de liberar la memoria reservada y archivos abiertos
 *              en cualquier punto del programa.
 ********/
void free_thread_resources(void *arg) {
  // arg contiene un puntero al la memoria creada por server de Client Connection
  if (!arg) {
    return;
  }
  if (!(*(void **)arg)) {
    return;
  }
  ClientConnection *cliConn = *(ClientConnection **)arg;
  if (cliConn->freeVar) {
    free(cliConn->freeVar);
  }
  if (cliConn->closeVar > 0) {
    close(cliConn->closeVar);
  }
  if (cliConn->fcloseVar) {
    fclose(cliConn->fcloseVar);
  }
  free(cliConn);
}

/********
 * FUNCIÓN: void *manage_client(void *cliConnVoid)
 * ARGS_IN: void *cliConnVoid - puntero a ClientConnection, con informacion de la conexion
 * DESCRIPCIÓN: Funcion que recibe, parsea y procesa una request,
 *              creando posteriormente la respuesta adecuada.
 *              Esta funcion se ejecuta en un thread independiente.
 * ARGS_OUT: void * - No retorna ningun valor de utilidad. NULL
 ********/
void *manage_client(void *cliConnVoid) {
  int recvLen = 0, pRet;
  char *recvBuffer = NULL;
  char sendBuffer[RESPONSE_LEN];
  RequestContent request;
  ClientConnection *cliConn = (ClientConnection *)cliConnVoid;
  cliConn->freeVar = NULL;
  cliConn->closeVar = 0;
  cliConn->fcloseVar = NULL;

  // Allocamos el buffer de recepcion
  recvBuffer = (char *)calloc(configParams.recvBufferLen + 1, 1);
  cliConn->freeVar = recvBuffer;
  if (!recvBuffer) {
    syslog(LOG_ERR, "Error allocating buffer. Client not managed.");
    close((int)cliConn->connfd);
    return (NULL);
  }

  // Bucle principal que se queda esperando a nuevas requests
  while ((recvLen = recv(cliConn->connfd, recvBuffer, configParams.recvBufferLen, 0)) > 0) {
    memset(&request, 0, sizeof(RequestContent));
    // Parseo la request recibida y la devuelvo en la estructura request
    request.totalLen = recvLen;
    request.completeRequest = recvBuffer;
    pRet = my_parse_request(&request);
    recvBuffer[recvLen] = 0;
    if (pRet < 0) {
      syslog(LOG_ERR, "Error parsing request. Closing connection. pRet = %d", pRet);

      /* Enviamos al navegador una respuesta de error 400, ya que si hay fallo en el parsing
       * sería error suyo (request mal formulada). Aún así nos ha llegado ha dar algún fallo
       * la librería de parseo, pero consideramos que esta es la respuesta más apropiada, ya que
       * el primer caso es el caso más frecuente */
      process_error(&request, sendBuffer, cliConn->connfd, BAD_REQUEST);
      break;
    }

    // Enviar respuesta al cliente
    if (create_and_send_response(cliConn, &request, sendBuffer) == -1) {
      syslog(LOG_ERR, "Error creating response. Closing connection");
      break;
    }
  }

  free_thread_resources(&cliConn);
  sem_post(&numConnections);
  return (NULL);
}
