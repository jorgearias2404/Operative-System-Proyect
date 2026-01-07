/*
 * Archivo de cabecera del módulo Logger (Sistema de Registro de Eventos)
 * del Sistema Operativo Virtual.
 * Define los tipos de datos y prototipos de funciones para un sistema de logging
 * que registra eventos del sistema en un archivo con diferentes niveles de severidad.
 * Este módulo es crucial para depuración y monitoreo del sistema.
 */

#ifndef LOGGER_H
#define LOGGER_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

/* Inclusión de bibliotecas necesarias */
#include <stdio.h>      // Para operaciones de archivo (fprintf, fclose)
#include <time.h>       // Para manejo de tiempo (time, localtime, strftime)
#include <string.h>     // Para manipulación de cadenas
#include <pthread.h>    // Para soporte de hilos y mutex (seguridad en entorno multihilo)

/*
 * Enum: LogLevel
 * Propósito: Define los niveles de severidad/importancia de los mensajes de log.
 * Los niveles permiten filtrar mensajes según su importancia durante la depuración
 * o el análisis del sistema.
 * 
 * Valores (ordenados de menor a mayor severidad):
 *   LOG_INFO      - Información general del funcionamiento normal del sistema
 *   LOG_WARNING   - Situaciones anómalas pero no críticas que podrían indicar problemas
 *   LOG_ERROR     - Errores que afectan el funcionamiento pero no detienen el sistema
 *   LOG_INTERRUPT - Eventos de interrupción del sistema (especial para este proyecto)
 *   LOG_DEBUG     - Información detallada para depuración, normalmente desactivada en producción
 */
typedef enum {
    LOG_INFO,      // Mensajes informativos: "CPU inicializada", "Disco formateado"
    LOG_WARNING,   // Advertencias: "Tamaño de datos incorrecto", "Configuración inusual"
    LOG_ERROR,     // Errores: "Memoria fuera de límites", "Archivo no encontrado"
    LOG_INTERRUPT, // Interrupciones: "Interrupción 3: Reloj", "Llamada al sistema"
    LOG_DEBUG      // Depuración: "FETCH: PC=300, Instrucción=00050000"
} LogLevel;

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo Logger
 */

/* 
 * Función: init_logger
 * Propósito: Inicializar el sistema de logging.
 * Abre el archivo de log en modo escritura y registra el inicio del sistema.
 * Esta función debe llamarse al inicio del programa.
 */
void init_logger();

/* 
 * Función: log_event
 * Parámetros:
 *   level   - Nivel de severidad del mensaje (LogLevel)
 *   message - Cadena de formato (similar a printf) con el mensaje a registrar
 *   ...     - Argumentos variables para formatear el mensaje (como en printf)
 * Propósito: Registrar un evento en el archivo de log con timestamp y nivel de severidad.
 * Esta es la función principal del módulo, usada por todos los otros módulos
 * para registrar sus actividades y errores.
 */
void log_event(LogLevel level, const char* message, ...);

/* 
 * Función: get_timestamp
 * Retorna: char* - Cadena con la fecha y hora actuales formateadas
 * Propósito: Generar un timestamp en formato legible para incluir en los logs.
 * Formato: "YYYY-MM-DD HH:MM:SS" (ejemplo: "2024-01-07 14:30:45")
 */
char* get_timestamp();

/* 
 * Función: close_logger
 * Propósito: Cerrar el sistema de logging de manera ordenada.
 * Registra el cierre del sistema y cierra el archivo de log.
 * Esta función debe llamarse al final del programa.
 */
void close_logger();

#endif /* LOGGER_H */