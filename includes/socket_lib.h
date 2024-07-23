/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  socketLib.h - Archivo .h para socketLib.c                    *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#define LISTENMAXCONNECTIONS 128

/********
 * FUNCIÓN: int initiate_server()
 * DESCRIPCIÓN: Devuelve el descriptor del socket del servidor (o EXIT_FAILURE en caso de error)
 * ARGS_OUT: int - Devuelve el descriptor del servidor en caso de éxito y -1 en caso contrario
 ********/
int initiate_server();

/********
 * FUNCIÓN: int establece_manejador(int signal, void (*handler))
 * ARGS_IN: int sockval - Socket del servidor por el que se acepta una conexión
 * DESCRIPCIÓN: Aceptar una nueva conexión de un cliente.
 * ARGS_OUT: int - Devuelve el descriptor del socket del cliente, -1 en caso de error
 ********/
int accept_connection(int);
