/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  server.h - Archivo .h para server.c                          *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <semaphore.h>

/* path de los ejecutables de python y php para los scripts */
typedef struct ExecutableScripts {
  char *python;
  char *php;
} ExecutableScripts;

typedef struct ConfigParameters {
  /* opciones obligatorias que aparecen en el enunciado */
  char *rootPath;
  long int maxClients;
  long int port;
  char *serverSignature;

  /* opciones nuevas que hemos añadido */
  char *baseFile;         // archivo base del servidor
  long int recvBufferLen; // tamaño del buffer de llegada
  long int timeout;   // timeout para las conexiones con el cliente en segundos
  char *tmpDirectory; // path temporal para el output de los scripts
  ExecutableScripts exe_scripts; // path de los ejecutables de python y php
} ConfigParameters;

/* Global variable containing information from the config file
 * Relevant to most files */
extern ConfigParameters configParams;

/* Semáforo para limitar el numero máximo de clientes */
extern sem_t numConnections;
