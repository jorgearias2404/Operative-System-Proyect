#include "logger.h"
#include <stdarg.h>
#include <stdlib.h>

static FILE* log_file = NULL;/* Puntero al archivo donde se escribirán los logs. 
Es static para que solo sea accesible dentro de este archivo.
c*/
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;/*
Protege el acceso al archivo de log en entornos multihilo. Se inicializa estáticamente.
*/

void init_logger() {
    log_file = fopen("system.log", "w");  // Abre archivo en modo escritura
    if (!log_file) {                      // Si falla la apertura
        perror("No se pudo abrir archivo de log");  // Muestra error
        exit(1);                          // Termina el programa
    }
    log_event(LOG_INFO, "Sistema iniciado");  // Registra inicio
}

char* get_timestamp() {
    static char timestamp[20];                    // Buffer estático para el timestamp
    time_t now = time(NULL);                      // Obtiene tiempo actual
    struct tm* tm_info = localtime(&now);         // Convierte a estructura local
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);  // Formatea la fecha
    return timestamp;                             // Retorna la cadena formateada
}

void log_event(LogLevel level, const char* message, ...) {
    pthread_mutex_lock(&log_mutex);  // Bloquea para acceso exclusivo
    
    const char* level_str[] = {
        "[INFO]    ",    // LOG_INFO
        "[WARNING] ",    // LOG_WARNING
        "[ERROR]   ",    // LOG_ERROR
        "[INTERRUPT]",   // LOG_INTERRUPT
        "[DEBUG]   "     // LOG_DEBUG
    };
    va_list args;
    va_start(args, message);  // Inicializa lista de argumentos variables
    // Escribir en archivo
    fprintf(log_file, "%s %s ", get_timestamp(), level_str[level]);
    vfprintf(log_file, message, args);
    fprintf(log_file, "\n");
    fflush(log_file);
    
    // Para interrupciones, también imprimir en stdout
    if (level == LOG_INTERRUPT || level == LOG_ERROR) {
        printf("%s %s ", get_timestamp(), level_str[level]);
        vprintf(message, args);
        printf("\n");
    }
    
    va_end(args);
    pthread_mutex_unlock(&log_mutex);
}

void close_logger() {
    if (log_file) {
        log_event(LOG_INFO, "Sistema finalizado");
        fclose(log_file);
    }
}