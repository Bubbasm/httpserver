/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                               *
 *  http_codes.h - Archivo con los códigos de respuesta de HTTP  *
 *                                                               *
 *  Autores:                                                     *
 *    - Bhavuk Sikka    (bhavuk.sikka@estudiante.uam.es)         *
 *    - Samuel de Lucas (samuel.lucas@estudiante.uam.es)         *
 *                                                               *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

/*
 * Códigos de respuesta HTTP
 */
typedef enum HTTPResponseCode {
  /* Success codes */
  OK = 200,
  NO_CONTENT = 204,

  /* Client Error codes */
  BAD_REQUEST = 400,
  FORBIDDEN = 403,
  NOT_FOUND = 404,
  IM_A_TEAPOT = 418, /* Default for unknown errors */

  /* Server error codes */
  INTERNAL_SERVER_ERROR = 500,
  NOT_IMPLEMENTED = 501,
  HTTP_VER_NOT_SUPP = 505

} HTTPResponseCode;
