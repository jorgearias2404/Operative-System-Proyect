#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

// Niveles de log
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_INTERRUPT,
    LOG_DEBUG
} LogLevel;

// Inicializar logger
void init_logger();
// Registrar evento
void log_event(LogLevel level, const char* message, ...);
// Obtener timestamp
char* get_timestamp();
// Cerrar logger
void close_logger();

#endif