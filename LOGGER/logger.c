#include "logger.h"
#include <stdarg.h>
#include <stdlib.h>

static FILE* log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_logger() {
    log_file = fopen("system.log", "w");
    if (!log_file) {
        perror("No se pudo abrir archivo de log");
        exit(1);
    }
    log_event(LOG_INFO, "Sistema iniciado");
}

char* get_timestamp() {
    static char timestamp[20];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return timestamp;
}

void log_event(LogLevel level, const char* message, ...) {
    pthread_mutex_lock(&log_mutex);
    
    const char* level_str[] = {
        "[INFO]    ",
        "[WARNING] ",
        "[ERROR]   ",
        "[INTERRUPT]",
        "[DEBUG]   "
    };
    
    va_list args;
    va_start(args, message);
    
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