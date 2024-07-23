#pragma once

/* Numero de threads en cada lote */
#define THREADBATCHCOUNT 10

/********
 * FUNCIÓN: int initialize_pool(void *(*client_fun)(void *), void (*cleanup_fun)(void *))
 * ARGS_IN: void *(*client_fun)(void *) - Funcion que recibe un parametro void *.
 *                                        Esta es la funcion que se llamará al añadir un trabajo
 *          void *(*cleanup_fun)(void *) - Funcion que recibe un parametro void *.
 *                                         Esta es la funcion que se llamará al destruir un hilo
 * DESCRIPCIÓN: Inicializa la pool de hilos, estableciendo a su vez las funciones del cliente y limpieza
 * ARGS_OUT: int - Devuelve 0 en caso de exito, -1 en caso de error
 ********/
int initialize_pool(void *(*client_fun)(void *), void (*cleanup_fun)(void *));

/********
 * FUNCIÓN: int add_job(void *info)
 * ARGS_IN: void *info - Puntero a la informacion que se quiere delegar al trabajo
 *                       asignado en initialize_pool
 * DESCRIPCIÓN: Inicia un trabajo
 * ARGS_OUT: int - Devuelve el id trabajo (valor no negativo), -1 en caso de error
 ********/
int add_job(void *info);

/********
 * FUNCIÓN: void terminate_pool()
 * DESCRIPCIÓN: Destruye todos los trabajos activos y libera la memoria
 ********/
void terminate_pool();
