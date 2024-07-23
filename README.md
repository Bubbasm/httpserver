# Servidor HTTP

Como parte de un proyecto en el Doble Grado en Ingeniería Informática y Matemáticas en la Universidad Autónoma de Madrid, hemos desarrollado un servidor HTTP en C capaz de ejecutar algunos scripts.
Este servidor se ha implementado con un pool de hilos dinámicos, creando hilos en forma de batches para optimizar los tiempos de creación de estos.

## Plan de trabajo

1. [x] Implementar una librería para la gestión de sockets.
2. [x] Informarse y elegir el esquema de servidor concurrente a desarrollar (con procesos o hilos, con o sin pool).
3. [x] Implementar la gestión de peticiones HTTP:
    - [x] Recepción.
    - [x] Parseo (p.ej: usando [PicoHTTPParser](https://github.com/h2o/picohttpparser)).
    - [x] Procesamiento
    - [x] Respuesta
4. [x] Implementar uso de archivo de configuración (p.ej: [libconfuse](https://github.com/libconfuse/libconfuse)).
5. [x] Implementar ejecución de scripts.
6. [x] Implementar la demonización.
7. [x] Implementar la escritura en el syslog de las operaciones que va realizando el servidor.

## Ejecución

Basta con compilar el código haciendo uso del archivo CMakeLists.txt. Como nota adicional, es importante posicionar el archivo server.conf en el `working directory`.
