/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  client_process_functions.h - Archivo .h para                 *
 *                                  client_process_functions.c   *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include "../includes/client_conn_lib.h"
#include "../includes/http_codes.h"
#include "../includes/server.h"

/********
 * FUNCIÓN: int process_OPTIONS(RequestContent *request, char *sendBuffer, int sockfd)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo OPTIONS y enviar la
 *              respuesta
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_OPTIONS(RequestContent *request, char *sendBuffer, int sockfd);

/********
 * FUNCIÓN: int process_HEAD(RequestContent *request, char *sendBuffer, int sockfd)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo HEAD y enviar la
 *              respuesta
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_HEAD(RequestContent *request, char *sendBuffer, int sockfd);

/********
 * FUNCIÓN: int process_GET(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 *          FILE **toClose - Sirve para liberar los recursos en caso de salida brupta,
 *                           como por ejemplo al recibir(SIGINT)
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo GET y enviar la
 *              respuesta, que es un archivo o una respuesta creada por el script ejecutado
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_GET(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose);

/********
 * FUNCIÓN: int process_POST(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 *          FILE **toClose - Sirve para liberar los recursos en caso de salida brupta,
 *                           como por ejemplo al recibir(SIGINT)
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo POST y enviar la
 *              respuesta. Maneja argumentos tanto en la url como en el cuerpo
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_POST(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose);

/********
 * FUNCIÓN: int process_error(RequestContent *request, char *sendBuffer, int sockfd, HTTPResponseCode responseCode)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 *          HTTPResponseCode responseCode - Codigo http de respuesta a enviar
 * DESCRIPCIÓN: Función encargada enviar una respuesta dado un código de error.
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_error(RequestContent *request, char *sendBuffer, int sockfd, HTTPResponseCode responseCode);
