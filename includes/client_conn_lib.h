/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  client_conn_lib.h - Archivo .h para client_conn_lib.c        *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

/* Para incluir FILE */
#include "../includes/picohttpparser.h"
#include <stdio.h>

/* Longitud maximo de respuesta con headers */
#define RESPONSE_LEN 8192
/* Maximo numero de headers */
#define MAXNUMHEADERS 128

/* Estructura con informacion del socket del cliente
 * e informacion a liberar cuando se destruya */
typedef struct ClientConnection {
  int connfd;
  void *freeVar;
  int closeVar;
  FILE *fcloseVar;
} ClientConnection;

/*
 * Estructura que contiene los campos necesarios para
 * trabajar con HTTPPicoParser
 * También tiene campos útiles para nuestra
 * implementacion del servidor
 */
typedef struct RequestContent {
  char *completeRequest;
  char *method;
  size_t methodLen;
  char *path;
  size_t pathLen;
  int minorVersion;
  struct phr_header headers[MAXNUMHEADERS];
  size_t numHeaders;
  size_t requestLen; // longitud hasta el los headers
  size_t totalLen;
} RequestContent;

/********
 * FUNCIÓN: void *manage_client(void *cliConnVoid)
 * ARGS_IN: void *cliConnVoid - puntero a ClientConnection, con informacion de la conexion
 * DESCRIPCIÓN: Funcion que recibe, parsea y procesa una request,
 *              creando posteriormente la respuesta adecuada.
 *              Esta funcion se ejecuta en un thread independiente.
 * ARGS_OUT: void * - No retorna ningun valor de utilidad. NULL
 ********/
void *manage_client(void *cli_conn_arg);

/********
 * FUNCIÓN: void free_thread_resources(void *arg)
 * ARGS_IN: void *arg - Memoria de un puntero a ClientConnection. Es void* pues
 *                      se usa con pthread_cancel_push
 * DESCRIPCIÓN: Funcion encargada de liberar la memoria reservada y archivos abiertos
 *              en cualquier punto del programa.
 ********/
void free_thread_resources(void *arg);
