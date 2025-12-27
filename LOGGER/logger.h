#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

// Niveles de log
typedef enum {
    LOG_INFO,      // Para mensajes informativos normales
    LOG_WARNING,   // Para advertencias (situaciones no críticas)
    LOG_ERROR,     // Para errores (situaciones que impiden funcionamiento normal)
    LOG_INTERRUPT, // Para eventos de interrupción del sistema
    LOG_DEBUG      // Para mensajes de depuración (debugging)
} LogLevel;

// Inicializar logger
void init_logger();
// Registrar evento
void log_event(LogLevel level,//Nivel de severidad del mensaje 
                 const char* message,//Mensaje a Registrar
                  ...);//argumentos variables como en printf
// Obtener timestamp
char* get_timestamp();//devuelve cadena con la fecha y hora actuales formateadas 
// Cerrar logger
void close_logger();//Cierra/libera recursos del sistema de logging (cierra archivos, libera memoria, etc.).

#endif