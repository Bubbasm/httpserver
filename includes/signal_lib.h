/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  signal_lib.h - Archivo .h para signal_lib.c                    *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

/********
 * FUNCIÓN: int establece_manejador(int signal, void (*handler))
 * ARGS_IN: int signal - Señal a manejar
 *          void (*handler) - Funcion que se asigna a la señal
 * DESCRIPCIÓN: Establece el manejador de la señal proporcionada.
 *              En caso de que handler sea NULL, se bloque la señal indicada
 *              si tiene valor positivo y se desbloquea si tiene valor negativo
 * ARGS_OUT: int - devuelve 0 en caso de éxito y -1 en caso contrario
 ********/
int establece_manejador(int signal, void(*handler));
