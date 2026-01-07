/*
 * Archivo de cabecera del módulo de consola del Sistema Operativo Virtual
 * Define las estructuras de datos y prototipos de funciones para la interfaz
 * de línea de comandos del sistema operativo.
 */

#ifndef CONSOLE_H
#define CONSOLE_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que el contenido de este archivo solo se incluya una vez
 * durante la compilación.
 */

#include "../types.h"
/* Incluye el archivo de definiciones de tipos globales del proyecto */

/*
 * Enum: ExecutionMode
 * Propósito: Define los modos de ejecución disponibles en el sistema
 * Valores:
 *   MODE_NORMAL   - Modo de ejecución normal, sin depuración
 *   MODE_DEBUGGER - Modo de depuración paso a paso
 */
typedef enum {
    MODE_NORMAL,    // Ejecución continua sin interrupciones
    MODE_DEBUGGER   // Ejecución controlada paso a paso para depuración
} ExecutionMode;

/*
 * Enum: ConsoleCommand
 * Propósito: Define todos los comandos válidos que puede interpretar la consola
 * Valores:
 *   CMD_RUN      - Ejecutar un programa desde archivo en modo normal
 *   CMD_DEBUG    - Ejecutar un programa en modo depurador
 *   CMD_STEP     - Ejecutar una sola instrucción en modo depurador
 *   CMD_CONTINUE - Continuar ejecución normal desde modo depurador
 *   CMD_REGISTERS- Mostrar el estado de todos los registros
 *   CMD_MEMORY   - Mostrar contenido de la memoria
 *   CMD_DISK     - Mostrar información del disco
 *   CMD_HELP     - Mostrar ayuda de comandos
 *   CMD_EXIT     - Salir del sistema
 *   CMD_UNKNOWN  - Comando no reconocido (valor por defecto)
 *   CMD_LOAD     - Cargar un programa sin ejecutarlo
 */
typedef enum {
    CMD_RUN,
    CMD_DEBUG,
    CMD_STEP,
    CMD_CONTINUE,
    CMD_REGISTERS,
    CMD_MEMORY,
    CMD_DISK,
    CMD_HELP,
    CMD_EXIT,
    CMD_UNKNOWN,
    CMD_LOAD
} ConsoleCommand;

/*
 * Estructura: ParsedCommand
 * Propósito: Almacena un comando parseado con todos sus parámetros
 * Campos:
 *   cmd      - Tipo de comando (ConsoleCommand)
 *   filename - Nombre del archivo a cargar/ejecutar (si aplica)
 *   mode     - Modo de ejecución en el que se ejecutará el comando
 *   param1   - Primer parámetro numérico (ej: dirección de memoria inicial)
 *   param2   - Segundo parámetro numérico (ej: dirección de memoria final)
 */
typedef struct {
    ConsoleCommand cmd;       // Tipo de comando
    char filename[100];       // Nombre de archivo (para comandos que lo requieran)
    ExecutionMode mode;       // Modo de ejecución asociado
    int param1;               // Parámetro numérico 1
    int param2;               // Parámetro numérico 2
} ParsedCommand;

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo de consola
 */

/* 
 * Función: init_console
 * Propósito: Inicializar la consola, mostrar banner y comandos disponibles
 */
void init_console();

/* 
 * Función: run_console
 * Propósito: Bucle principal de la consola, lee y ejecuta comandos
 */
void run_console();

/* 
 * Función: parse_command
 * Parámetros: input - cadena de texto con el comando del usuario
 * Retorna: ParsedCommand - estructura con el comando parseado
 * Propósito: Analiza la entrada del usuario y la convierte en un comando estructurado
 */
ParsedCommand parse_command(const char* input);

/* 
 * Función: execute_command
 * Parámetros: cmd - comando parseado a ejecutar
 * Propósito: Ejecuta el comando especificado
 */
void execute_command(ParsedCommand cmd);

/* 
 * Función: show_help
 * Propósito: Muestra la ayuda de comandos disponibles
 */
void show_help();

/* 
 * Función: show_prompt
 * Propósito: Muestra el prompt de la consola, que varía según el modo actual
 */
void show_prompt();

/* 
 * Función: get_current_mode
 * Retorna: ExecutionMode - modo de ejecución actual
 * Propósito: Obtiene el modo de ejecución actual del sistema
 */
ExecutionMode get_current_mode();

/* 
 * Función: set_current_mode
 * Parámetros: mode - nuevo modo de ejecución
 * Propósito: Establece el modo de ejecución del sistema
 */
void set_current_mode(ExecutionMode mode);

#endif /* CONSOLE_H */