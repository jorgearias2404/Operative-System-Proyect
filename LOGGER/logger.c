/*
 * Archivo de implementación del módulo Logger del Sistema Operativo Virtual.
 * Contiene la lógica completa para registrar eventos del sistema en un archivo
 * con diferentes niveles de severidad, incluyendo soporte para multihilo y
 * generación de timestamps.
 */

/* Inclusión de cabecera propia del módulo */
#include "logger.h"

/* Inclusión de bibliotecas adicionales */
#include <stdarg.h>   // Para manejo de argumentos variables (va_list, va_start, etc.)
#include <stdlib.h>   // Para función exit()
#include <stdio.h>    // Ya incluido, pero necesario para perror()

/*
 * VARIABLES GLOBALES ESTÁTICAS
 * Solo accesibles dentro de este archivo (encapsulación).
 * Esto garantiza que el estado del logger esté protegido.
 */

/* 
 * Variable: log_file
 * Tipo: FILE* (puntero a archivo)
 * Propósito: Puntero al archivo donde se escribirán los logs.
 * Es static para que solo sea accesible dentro de este archivo.
 * Se inicializa en NULL para indicar que no hay archivo abierto.
 */
static FILE* log_file = NULL;

/* 
 * Variable: log_mutex
 * Tipo: pthread_mutex_t
 * Propósito: Mutex para proteger el acceso al archivo de log en entornos multihilo.
 * Inicializado estáticamente con PTHREAD_MUTEX_INITIALIZER.
 * Esto garantiza que solo un hilo a la vez pueda escribir en el archivo,
 * previniendo corrupción de datos o mensajes entremezclados.
 */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Función: init_logger
 * Propósito: Inicializar el sistema de logging.
 * Abre el archivo "system.log" en modo escritura ("w"), que:
 * - Crea el archivo si no existe
 * - Sobrescribe el archivo si ya existe (comienza limpio cada ejecución)
 * 
 * Si falla la apertura, el programa termina porque el logging es crítico
 * para la depuración del sistema operativo virtual.
 */
void init_logger() {
    // Abrir archivo de log en modo escritura ("w" = write)
    // "system.log" es el nombre del archivo donde se guardarán todos los logs
    log_file = fopen("system.log", "w");
    
    // Verificar si la apertura fue exitosa
    if (!log_file) {
        // Mostrar error descriptivo usando perror (incluye descripción del sistema)
        perror("No se pudo abrir archivo de log");
        // Terminar programa con código de error 1
        // En un sistema real, podrías intentar abrir un archivo alternativo
        // o continuar sin logging, pero aquí es crítico para depuración
        exit(1);
    }
    
    // Registrar el primer evento: inicio del sistema
    // LOG_INFO es el nivel apropiado para mensajes informativos normales
    log_event(LOG_INFO, "Sistema iniciado");
    
    // NOTA: No es necesario cerrar el archivo aquí, se cerrará en close_logger()
}

/*
 * Función: get_timestamp
 * Propósito: Generar una cadena con la fecha y hora actuales formateadas.
 * 
 * Características:
 * - Usa buffer estático para eficiencia (evita alloc/free repetidos)
 * - Formato: "YYYY-MM-DD HH:MM:SS" (estándar ISO 8601 simplificado)
 * - Seguro para hilos porque cada llamada crea una nueva cadena
 * 
 * Retorna: char* - Puntero a cadena estática con el timestamp
 */
char* get_timestamp() {
    // Buffer estático: persiste entre llamadas pero es reescrito cada vez
    // Tamaño 20: "YYYY-MM-DD HH:MM:SS\0" = 19 + 1 para null terminator
    static char timestamp[20];
    
    // Obtener tiempo actual en segundos desde epoch (1/1/1970)
    time_t now = time(NULL);
    
    // Convertir a estructura tm (tiempo local desglosado)
    // localtime() descompone el tiempo en año, mes, día, hora, minuto, segundo
    struct tm* tm_info = localtime(&now);
    
    // Formatear la fecha/hora según el patrón especificado
    // %Y = año con 4 dígitos, %m = mes (01-12), %d = día (01-31)
    // %H = hora (00-23), %M = minuto (00-59), %S = segundo (00-59)
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Retornar el buffer estático con el timestamp formateado
    return timestamp;
}

/*
 * Función: log_event
 * Propósito: Registrar un evento/mensaje en el archivo de log.
 * 
 * Características:
 * - Incluye timestamp automático
 * - Incluye nivel de severidad formateado
 * - Soporta formato tipo printf con argumentos variables
 * - Thread-safe (protegido por mutex)
 * - Para ciertos niveles (INTERRUPT, ERROR), también muestra en consola
 * 
 * Parámetros:
 *   level   - Nivel de severidad (INFO, WARNING, ERROR, etc.)
 *   message - Cadena de formato (como en printf)
 *   ...     - Argumentos variables para el formateo
 */
void log_event(LogLevel level, const char* message, ...) {
    /*
     * PASO 1: BLOQUEAR MUTEX PARA THREAD-SAFETY
     * Garantiza que solo un hilo escriba en el archivo a la vez.
     * Si otro hilo está escribiendo, este hilo se bloquea hasta que se libere.
     */
    pthread_mutex_lock(&log_mutex);
    
    /*
     * Tabla de cadenas para los niveles de log.
     * Cada nivel tiene un formato consistente de 10 caracteres:
     * [INFO]    - 4 caracteres + 6 espacios
     * [WARNING] - 7 caracteres + 3 espacios
     * [ERROR]   - 5 caracteres + 5 espacios
     * [INTERRUPT]- 9 caracteres + 1 espacio
     * [DEBUG]   - 5 caracteres + 5 espacios
     * 
     * El formato consistente hace que los logs sean más legibles.
     */
    const char* level_str[] = {
        "[INFO]    ",    // Índice 0: LOG_INFO
        "[WARNING] ",    // Índice 1: LOG_WARNING  
        "[ERROR]   ",    // Índice 2: LOG_ERROR
        "[INTERRUPT]",   // Índice 3: LOG_INTERRUPT
        "[DEBUG]   "     // Índice 4: LOG_DEBUG
    };
    
    /*
     * PASO 2: PREPARAR ARGUMENTOS VARIABLES
     * Las funciones con formato tipo printf usan va_list para manejar
     * un número variable de argumentos.
     */
    va_list args;           // Variable para almacenar la lista de argumentos
    va_start(args, message); // Inicializar args con el último parámetro fijo (message)
    
    /*
     * PASO 3: ESCRIBIR EN ARCHIVO DE LOG
     * Formato de cada línea de log:
     * TIMESTAMP NIVEL MENSAJE\n
     * Ejemplo: "2024-01-07 14:30:45 [INFO]     Sistema iniciado"
     */
    
    // Escribir timestamp y nivel
    fprintf(log_file, "%s %s ", get_timestamp(), level_str[level]);
    
    // Escribir mensaje formateado (usa vfprintf para argumentos variables)
    vfprintf(log_file, message, args);
    
    // Nueva línea al final
    fprintf(log_file, "\n");
    
    /*
     * Flushear el buffer para asegurar que los datos se escriban inmediatamente.
     * Esto es importante para:
     * - No perder logs si el programa crashea
     * - Ver logs en tiempo real durante depuración
     * - Garantizar orden de escritura en sistemas con buffering
     */
    fflush(log_file);
    
    /*
     * PASO 4: MOSTRAR EN CONSOLA PARA NIVELES IMPORTANTES
     * Para interrupciones y errores, también se muestran en stdout (consola)
     * para que el usuario/desarrollador los vea inmediatamente.
     */
    if (level == LOG_INTERRUPT || level == LOG_ERROR) {
        // Mismo formato que en archivo, pero en consola
        printf("%s %s ", get_timestamp(), level_str[level]);
        vprintf(message, args);  // vprintf para argumentos variables en consola
        printf("\n");
    }
    
    /*
     * PASO 5: LIMPIAR ARGUMENTOS VARIABLES
     * Siempre se debe llamar va_end después de va_start.
     */
    va_end(args);
    
    /*
     * PASO 6: LIBERAR MUTEX
     * Permite que otros hilos puedan escribir en el log.
     */
    pthread_mutex_unlock(&log_mutex);
}

/*
 * Función: close_logger
 * Propósito: Cerrar el sistema de logging de manera ordenada.
 * Garantiza que:
 * 1. Se registre el evento de finalización del sistema
 * 2. Se cierre el archivo de log correctamente
 * 3. Todos los buffers se vacíen al disco
 * 
 * Esta función debe llamarse antes de terminar el programa.
 */
void close_logger() {
    // Verificar que el archivo esté abierto
    if (log_file) {
        // Registrar evento de finalización
        // LOG_INFO es apropiado para mensaje informativo de cierre
        log_event(LOG_INFO, "Sistema finalizado");
        
        // Cerrar el archivo
        // fclose() también hace flush automático de buffers
        fclose(log_file);
        
        // Opcional: establecer a NULL para evitar uso accidental
        log_file = NULL;
    }
    // Si log_file es NULL, el logger ya estaba cerrado o no se inicializó
}