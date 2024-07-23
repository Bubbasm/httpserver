/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  client_process_functions.c - Funciones para procesar         *
 *                               las HTTP requests               *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "../includes/client_process_functions.h"
#include "../includes/picohttpparser.h"
#include "../includes/server.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <time.h>
#include <unistd.h>

/********
 * FUNCIÓN: static void get_response_code_string(HTTPResponseCode responseCode, char *responseString)
 * ARGS_IN: HTTPResponseCode responseCode - el codigo de respuesta
 *          char *responseString - (output) - Contiene la string de respuesta
 * DESCRIPCIÓN: Función que devuelve en responseString la cadena asociada al código HTTP dado.
 ********/
static void get_response_code_string(HTTPResponseCode responseCode, char *responseString) {
  switch (responseCode) {
  case OK:
    strcpy(responseString, "OK");
    break;
  case NO_CONTENT:
    strcpy(responseString, "No Content");
    break;
  case BAD_REQUEST:
    strcpy(responseString, "Bad Request");
    break;
  case FORBIDDEN:
    strcpy(responseString, "Forbidden");
    break;
  case NOT_FOUND:
    strcpy(responseString, "Not Found");
    break;
  case NOT_IMPLEMENTED:
    strcpy(responseString, "Not Implemented");
    break;
  case HTTP_VER_NOT_SUPP:
    strcpy(responseString, "HTTP Version Not Supported");
    break;
  case INTERNAL_SERVER_ERROR:
    strcpy(responseString, "Internal Server Error");
    break;
  default:
    strcpy(responseString, "I'm a teapot");
    break;
  }
}

/********
 * FUNCIÓN: static long get_file_size(const char *filename)
 * ARGS_IN: const char *filename - Archivo en cuestion
 * DESCRIPCIÓN: Función auxiliar para obtener el tamaño de un archivo, dado su nombre
 *              Referencia: https://www.delftstack.com/howto/c/file-size-in-c/
 * ARGS_OUT: long - longitud del archivo si existe. 0 si filename es NULL y -1 en caso de error
 ********/
static long get_file_size(const char *filename) {
  struct stat st;
  if (!filename)
    return 0;
  if (stat(filename, &st) == -1) /* failure */
    return -1;                   // when file does not exist or is not accessible
  return (long)st.st_size;
}

/********
 * FUNCIÓN: static const char *get_filename_ext(const char *filename)
 * ARGS_IN: const char *filename - Archivo en cuestion
 * DESCRIPCIÓN: Funcion auxiliar para obtener la extension de un archivo
 *              Referencia: https://stackoverflow.com/questions/5309471/getting-file-extension-in-c
 * ARGS_OUT: const char * - puntero a la extension del archivo. string vacia en caso de error
 ********/
static const char *get_filename_ext(const char *filename) {
  if (!filename)
    return "";
  const char *dot = strrchr(filename, '.');
  if (!dot)
    return "";
  return dot + 1;
}

/********
* FUNCIÓN: static int parse_url(RequestContent *request, char *url, char *filename)
* ARGS_IN: RequestContent *request - Estructura de request de donde se obtienen los datos
           char *url - (output) Rellena con la url completa solicitada
           char *filename - (output) Rellena unicamente con el archivo solicitado
* DESCRIPCIÓN: Procesa la url de la request, extrayendo el nombre del archivo contenido en esta.
*              Tambien devuelve el offset de las queries, es decir, la localizacion del caracter '?'
* ARGS_OUT: int - Offset del path de la request donde se encuentra el caracter '?'.
*                 Si no se encuentra el caracter '?', retorna la longitud completa
********/
static int parse_url(RequestContent *request, char *url, char *filename) {
  url[0] = 0;
  filename[0] = 0;
  strncat(url, request->path, request->pathLen);
  url[request->pathLen] = 0; // \0
  char *query = strchr(url, '?');
  int queryOffset = request->pathLen;
  if (query) {
    queryOffset = (int)(query - url);
  }

  strcpy(filename, configParams.rootPath);
  strncat(filename, url, queryOffset); // dont include query data

  if (strncmp(url, "/", queryOffset) == 0) {
    strcat(filename, configParams.baseFile);
  }

  return queryOffset;
}

/********
* FUNCIÓN: static int parse_queries(char *queryString, char *queryValues)
* ARGS_IN: char *queryString - query que empieza con '?' o el cuerpo de una solicitud POST
           char *filename - (output) Rellena con los argumentos
* DESCRIPCIÓN: Recibe una string de la forma "param1=val1&param2=val2&..." en
*              el argumento queryString, y devuelve una string con los valores
*              "val1 val2 val3 ..." en el argumento de salida queryValues.
*              zeroDay solucionado: Comprueba la existencia del caracter ';'
*              que permitía ejecución de código remota
* ARGS_OUT: int - Devuelve el numero de bytes que contiene queryValues, o -1
*                 si la query se considera peligrosa
********/
static int parse_queries(char *queryString, char *queryValues) {
  int offset = 0;
  char *equalLocation = queryString - 1, *ampLocation;

  if (strchr(queryString, ';') != NULL) {
    syslog(LOG_WARNING, "SOMEONE IS TRYING TO DO REMOTE CODE EXECUTION");
    syslog(LOG_NOTICE, "Query that triggered the remote code execution alert: %s", queryString);
    return -1;
  }

  if (queryValues)
    queryValues[0] = 0; // Start from begining
  do {
    if (strlen(equalLocation) == 0)
      break;
    equalLocation = strchr(equalLocation + 1, '=');
    if (equalLocation) {
      equalLocation++;
      ampLocation = strchr(equalLocation, '&');
      if (!ampLocation)
        ampLocation = queryString + strlen(queryString);
      offset += (int)(ampLocation - equalLocation) + 1;
      if (!queryValues)
        continue;
      strncat(queryValues, equalLocation, (int)(ampLocation - equalLocation));
      strcat(queryValues, " ");
    }
  } while (equalLocation != NULL);

  return offset;
}

/********
 * FUNCIÓN: static char *obtain_executable(char *filename)
 * ARGS_IN: char *filename - Archivo en cuestion
 * DESCRIPCIÓN: Obtiene el ejecutable asociado a un cierto archivo con extension adecuada
 *              Solo soporta python y php
 * ARGS_OUT: char * - Ejecutable que hace referencia a configParams. Devuelve NULL si no soportamos ese archivo
 ********/
static char *obtain_executable(char *filename) {
  const char *ext = get_filename_ext(filename);
  if (strncmp(ext, "py", 2) == 0)
    return configParams.exe_scripts.python;
  else if (strncmp(ext, "php", 3) == 0)
    return configParams.exe_scripts.php;
  return NULL;
}

/********
 * FUNCIÓN: static int execute_script(char *filepath, char *filename, char *args, long uid)
 * ARGS_IN: char *filepath - (output) Archivo al cual redirigir la salida del script
 *          char *filename -  Archivo a ejecutar
 *          char *args - Argumentos a pasar al script
 *          long uid - UID del hilo para crear un archivo único
 * DESCRIPCIÓN: Ejecuta el archivo con el comando apropiado, pasando args
 *              como argumentos al programa. UNSECURE BY DESIGN, PART OF ASSIGNMENT
 * ARGS_OUT: int - La función retorna 0 si todo ha ido bien, o -1 en caso de error
 ********/
static int execute_script(char *filepath, char *filename, char *args, long uid) {
  char *executable = obtain_executable(filename);
  if (!executable)
    return -1;
  if (!executable)
    return -1;

  strcpy(filepath, configParams.tmpDirectory);
  sprintf(filepath + strlen(filepath), "%ld_%ld.txt", uid, time(NULL));

  char command[strlen(executable) + 1 + strlen(filename) + 1 + strlen(args) + strlen(" > ") + strlen(filepath) + 1];
  strcpy(command, executable);
  strcat(command, " ");
  strcat(command, filename);
  strcat(command, " ");
  strcat(command, args);
  strcat(command, " > ");
  strcat(command, filepath);
  syslog(LOG_INFO, "Executing: %s\n", command);
  system(command);
  return 0;
}

/********
 * FUNCIÓN: static int send_data(int sockfd, int filefd, char *sendBuffer, int sendBufferLen, char *filename)
 * ARGS_IN: int sockfd - Socket por el que enviar los datos
 *          int filefd - (opcional) Descriptor del archivo a enviar
 *          char *sendBuffer - Buffer a enviar
 *          int sendBufferLen - Longitud del buffer a enviar
 *          char *filename - (opcional) Archivo asociado filefd para obtener informacion de dicho archivo
 * DESCRIPCIÓN: Funcion para enviar la respuesta contenida en sendBuffer al cliente. 
 *              También envia un archivo (filefd, filename) en caso de ser necesario. 
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
static int send_data(int sockfd, int filefd, char *sendBuffer, int sendBufferLen, char *filename) {
  int si = 1, no = 0;
  int writeRet = 0;
  setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &si, sizeof(int));
  writeRet = write(sockfd, sendBuffer, sendBufferLen);
  if (writeRet < 0) {
    syslog(LOG_ERR, "Error sending data");
    return -1;
  }
  if (filefd > 0) {
    writeRet = sendfile(sockfd, filefd, NULL, get_file_size(filename));
    if (writeRet < 0) {
      syslog(LOG_ERR, "Error sending data");
      return -1;
    }
  }
  setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &no, sizeof(int));
  return 0;
}

/********
 * FUNCIÓN: static int response_start_line(char *sendBuffer, int minorVersion, HTTPResponseCode responseCode)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 *          int minorVersion - La version de http usada
 *          HTTPResponseCode responseCode - Código de respuesta
 * DESCRIPCIÓN: Funcion para crear la primera linea de las respuestas, 
 *              dadas la versión de HTTP y el codigo HTTP de respuesta.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int response_start_line(char *sendBuffer, int minorVersion, HTTPResponseCode responseCode) {
  char responseString[64];
  get_response_code_string(responseCode, responseString);
  return sprintf(sendBuffer, "HTTP/1.%d %d %s\r\n", minorVersion, responseCode, responseString);
}

/********
 * FUNCIÓN: static int date_header(char *sendBuffer)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 * DESCRIPCIÓN: Funcion para crear el header de fecha.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int date_header(char *sendBuffer) {
  // Date: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
  //       Wed, 21 Oct 2015 07:28:00 GMT
  time_t rawtime = time(NULL);
  struct tm currenttime;
  gmtime_r(&rawtime, &currenttime);
  return strftime(sendBuffer, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &currenttime);
}

/********
 * FUNCIÓN: static int allow_header(char *sendBuffer)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 * DESCRIPCIÓN: Funcion para crear el header Allows.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int allow_header(char *sendBuffer) { return sprintf(sendBuffer, "Allow: OPTIONS, GET, HEAD, POST\r\n"); }

/********
 * FUNCIÓN: static int server_header(char *sendBuffer)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 * DESCRIPCIÓN: Funcion para crear el header con el nombre del server.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int server_header(char *sendBuffer) {
  // Server: <product>
  return sprintf(sendBuffer, "Server: %s\r\n", configParams.serverSignature);
}

/********
 * FUNCIÓN: static int last_modified_header(char *sendBuffer, char *filename)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 *          char *filename -  Archivo del que se comprueba la ultima modificacion
 * DESCRIPCIÓN: Funcion para crear el header con la ultima modificacion de un archivo.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int last_modified_header(char *sendBuffer, char *filename) {
  // Last-Modified: <day-name>, <day> <month> <year> <hour>:<min>:<sec> GMT
  time_t rawtime;

  // https://www.roseindia.net/c-tutorials/c-file-last.shtml
  struct stat b;
  if (filename && !stat(filename, &b))
    memcpy(&rawtime, &b.st_mtime, sizeof(time_t));
  else
    rawtime = time(NULL);

  struct tm currenttime;
  gmtime_r(&rawtime, &currenttime);
  return strftime(sendBuffer, 128, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n", &currenttime);
}

/********
 * FUNCIÓN: static int response_length_header(char *sendBuffer, char *filename)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 *          char *filename -  Archivo del que se comprueba la longitud
 * DESCRIPCIÓN: Funcion para crear el header con la longitud del contenido que vamos a enviar.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien, o -1 en caso de error
 ********/
static int content_length_header(char *sendBuffer, char *filename) {
  // Content-Length: <length>
  //                 The length in decimal number of octets.
  long length = get_file_size(filename);
  return sprintf(sendBuffer, "Content-Length: %ld\r\n", length);
}

/********
 * FUNCIÓN: static int content_type_header(char *sendBuffer, char *filename)
 * ARGS_IN: char *sendBuffer - (output) buffer que se rellena con la primera linea
 *          char *filename -  Archivo del que se comprueba el tipo (la extension)
 * DESCRIPCIÓN: Funcion para crear el header con el tipo de contenido que enviamos en la respuesta.
 * ARGS_OUT: int - La función retorna el numero de carácteres escritos si todo ha ido bien
 ********/
static int content_type_header(char *sendBuffer, char *filename) {
  // Content-Type: text/html; charset=UTF-8
  // Content-Type: multipart/form-data; boundary=something
  const char *extension = get_filename_ext(filename);

  strcpy(sendBuffer, "Content-Type: ");

  if (!strcmp(extension, "txt"))
    strcat(sendBuffer, "text/plain");
  else if (!strcmp(extension, "html") || !strcmp(extension, "htm"))
    strcat(sendBuffer, "text/html");
  else if (!strcmp(extension, "gif"))
    strcat(sendBuffer, "image/gif");
  else if (!strcmp(extension, "jpeg") || !strcmp(extension, "jpg"))
    strcat(sendBuffer, "image/jpeg");
  else if (!strcmp(extension, "ico"))
    strcat(sendBuffer, "image/x-icon");
  else if (!strcmp(extension, "mpeg") || !strcmp(extension, "mpg"))
    strcat(sendBuffer, "video/mpeg");
  else if (!strcmp(extension, "docx") || !strcmp(extension, "doc"))
    strcat(sendBuffer, "application/msword");
  else if (!strcmp(extension, "pdf"))
    strcat(sendBuffer, "application/pdf");
  else
    // https://stackoverflow.com/a/1176031
    strcat(sendBuffer, "application/octet-stream");

  strcat(sendBuffer, "\r\n");
  return strlen(sendBuffer);
}

/********
 * FUNCIÓN: int process_OPTIONS(RequestContent *request, char *sendBuffer, int sockfd)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo OPTIONS y enviar la
 *              respuesta
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_OPTIONS(RequestContent *request, char *sendBuffer, int sockfd) {
  int retValue = 0, sendBufferLen = 0;
  sendBufferLen = response_start_line(sendBuffer, request->minorVersion, NO_CONTENT);
  sendBufferLen += allow_header(sendBuffer + sendBufferLen);
  sendBufferLen += date_header(sendBuffer + sendBufferLen);
  sendBufferLen += server_header(sendBuffer + sendBufferLen);
  sendBufferLen += sprintf(sendBuffer + sendBufferLen, "\r\n");

  int writeRet = send_data(sockfd, -1, sendBuffer, sendBufferLen, NULL);
  if (writeRet < 0) {
    retValue = -1;
  }

  return retValue;
}

/********
 * FUNCIÓN: int process_HEAD(RequestContent *request, char *sendBuffer, int sockfd)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 * DESCRIPCIÓN: Función encargada de procesar una request de tipo HEAD y enviar la
 *              respuesta
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_HEAD(RequestContent *request, char *sendBuffer, int sockfd) {
  char url[request->pathLen + 1]; // picohttpparser no crea una nueva memoria
  char filename[request->pathLen + 1 + strlen(configParams.baseFile) + strlen(configParams.rootPath)];
  int retValue = 0;
  int sendBufferLen = 0;

  parse_url(request, url, filename);

  if (access(filename, F_OK) != 0) {
    syslog(LOG_ERR, "Error: file not found");
    return process_error(request, sendBuffer, sockfd, NOT_FOUND);
  }

  sendBufferLen = response_start_line(sendBuffer, request->minorVersion, OK);
  sendBufferLen += date_header(sendBuffer + sendBufferLen);
  sendBufferLen += server_header(sendBuffer + sendBufferLen);
  sendBufferLen += last_modified_header(sendBuffer + sendBufferLen, filename);
  sendBufferLen += content_length_header(sendBuffer + sendBufferLen, filename);
  sendBufferLen += content_type_header(sendBuffer + sendBufferLen, filename);
  sendBufferLen += sprintf(sendBuffer + sendBufferLen, "\r\n");

  int writeRet = send_data(sockfd, -1, sendBuffer, sendBufferLen, NULL);
  if (writeRet < 0)
    retValue = -1;

  return retValue;
}

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
int process_GET(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose) {
  char url[request->pathLen + 1]; // picohttpparser no crea nueva memoria
  char filename[request->pathLen + 1 + strlen(configParams.baseFile) + strlen(configParams.rootPath)];
  char outputFile[strlen(configParams.tmpDirectory) + 1024];
  FILE *pf = NULL; // closed with fclose
  int retValue = 0;
  int queryOffset = parse_url(request, url, filename);
  char *queryString = url + queryOffset;
  char queryValues[strlen(queryString)];

  if (access(filename, F_OK) != 0) {
    syslog(LOG_ERR, "Error: file not found");
    return process_error(request, sendBuffer, sockfd, NOT_FOUND);
  }

  char *file = filename;
  int filefd = 0;
  if (queryOffset != (int)request->pathLen) {
    // Parse queries if there are any
    if (parse_queries(queryString, queryValues) == -1) {
      return process_error(request, sendBuffer, sockfd, FORBIDDEN);
    }
    int err = execute_script(outputFile, filename, queryValues, sockfd);
    if (err == -1) {
      syslog(LOG_ERR, "Error executing the script");
      return process_error(request, sendBuffer, sockfd, INTERNAL_SERVER_ERROR);
    }
    file = outputFile;
  } // if there are no queries, send back the requested file
  if (!(pf = fopen(file, "rb"))) {
    syslog(LOG_ERR, "Error opening the script's output file");
    return process_error(request, sendBuffer, sockfd, INTERNAL_SERVER_ERROR);
  }
  *toClose = pf;
  filefd = pf->_fileno;

  int sendBufferLen = 0;
  sendBufferLen = response_start_line(sendBuffer, request->minorVersion, OK);
  sendBufferLen += date_header(sendBuffer + sendBufferLen);
  sendBufferLen += server_header(sendBuffer + sendBufferLen);
  sendBufferLen += last_modified_header(sendBuffer + sendBufferLen, filename);
  sendBufferLen += content_length_header(sendBuffer + sendBufferLen, file);
  sendBufferLen += content_type_header(sendBuffer + sendBufferLen, file);
  sendBufferLen += sprintf(sendBuffer + sendBufferLen, "\r\n");

  int writeRet = send_data(sockfd, filefd, sendBuffer, sendBufferLen, file);
  if (writeRet < 0)
    retValue = -1;

  fclose(pf);
  *toClose = NULL;

  return retValue;
}

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
int process_POST(RequestContent *request, char *sendBuffer, int sockfd, FILE **toClose) {
  char url[request->pathLen + 1]; // picohttpparser no crea nueva memoria
  char filename[request->pathLen + 1 + strlen(configParams.baseFile) + strlen(configParams.rootPath)];
  char outputFile[strlen(configParams.tmpDirectory) + 1024];
  int retValue = 0;
  FILE *pf = NULL; // closed with fclose

  int queryOffset = parse_url(request, url, filename);
  int requestBodyLen = request->totalLen - request->requestLen;
  char *queryString = url + queryOffset;
  char queryValues[strlen(queryString) + requestBodyLen];

  if (access(filename, F_OK) != 0) {
    syslog(LOG_ERR, "Error: file not found");
    return process_error(request, sendBuffer, sockfd, NOT_FOUND);
  }

  char *file = filename;
  int filefd = 0;
  // Parse queries de url si existen
  int offset = parse_queries(queryString, queryValues);
  // Parse queries del cuerpo si existen
  parse_queries(request->completeRequest + request->requestLen, queryValues + offset);
  int err = execute_script(outputFile, filename, queryValues, sockfd);
  if (err == -1) {
    syslog(LOG_ERR, "Error executing the script");
    return process_error(request, sendBuffer, sockfd, INTERNAL_SERVER_ERROR);
  }
  file = outputFile;
  // if there are no queries, send back the requested file
  if (!(pf = fopen(file, "rb"))) {
    syslog(LOG_ERR, "Error opening the script's output file");
    return process_error(request, sendBuffer, sockfd, INTERNAL_SERVER_ERROR);
  }
  *toClose = pf;
  filefd = pf->_fileno;

  int sendBufferLen = 0;

  sendBufferLen = response_start_line(sendBuffer, request->minorVersion, OK);
  sendBufferLen += date_header(sendBuffer + sendBufferLen);
  sendBufferLen += server_header(sendBuffer + sendBufferLen);
  sendBufferLen += last_modified_header(sendBuffer + sendBufferLen, filename);
  sendBufferLen += content_length_header(sendBuffer + sendBufferLen, file);
  sendBufferLen += content_type_header(sendBuffer + sendBufferLen, file);
  sendBufferLen += sprintf(sendBuffer + sendBufferLen, "\r\n");

  int writeRet = send_data(sockfd, filefd, sendBuffer, sendBufferLen, file);
  if (writeRet < 0)
    retValue = -1;

  fclose(pf);
  *toClose = NULL;
  return retValue;
}

/********
 * FUNCIÓN: int process_error(RequestContent *request, char *sendBuffer, int sockfd, HTTPResponseCode responseCode)
 * ARGS_IN: RequestContent *request - Request con la inforacion a usar para responder
 *          int sockfd - Socket por el que enviar los datos
 *          char *sendBuffer - Buffer con memoria reservada a completar y enviar
 *          HTTPResponseCode responseCode - Codigo http de respuesta a enviar
 * DESCRIPCIÓN: Función encargada enviar una respuesta dado un código de error.
 * ARGS_OUT: int - Devuelve -1 en caso de error, 0 en el resto de casos
 ********/
int process_error(RequestContent *request, char *sendBuffer, int sockfd, HTTPResponseCode responseCode) {
  char url[request->pathLen + 1]; // picohttpparser no crea nueva memoria
  char filename[request->pathLen + 1 + strlen(configParams.baseFile) + strlen(configParams.rootPath)];
  int sendBufferLen = 0, minorVersion = request->minorVersion;
  int retValue = 0;

  syslog(LOG_INFO, "Error in the request: %d", responseCode);

  parse_url(request, url, filename);
  if (responseCode == HTTP_VER_NOT_SUPP)
    minorVersion = 1; /* Nuestra versión por defecto es 1.1 */

  sendBufferLen = response_start_line(sendBuffer, minorVersion, responseCode);
  sendBufferLen += date_header(sendBuffer + sendBufferLen);
  sendBufferLen += server_header(sendBuffer + sendBufferLen);
  sendBufferLen += content_length_header(sendBuffer + sendBufferLen, NULL);

  /* El tipo por defecto en estos errores parece ser text/html */
  sendBufferLen += content_type_header(sendBuffer + sendBufferLen, ".html");
  sendBufferLen += sprintf(sendBuffer + sendBufferLen, "\r\n");

  int writeRet = send_data(sockfd, -1, sendBuffer, sendBufferLen, NULL);
  if (writeRet < 0)
    retValue = -1;

  return retValue;
}
