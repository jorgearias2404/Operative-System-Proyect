/*
 * Archivo de implementación del módulo de consola del Sistema Operativo Virtual
 * Contiene la lógica completa para la interfaz de línea de comandos interactiva.
 */

/* Inclusión de cabeceras propias del módulo */
#include "console.h"

/* Inclusión de cabeceras de otros módulos del sistema */
#include "../CPU/cpu.h"           // Para controlar la CPU
#include "../MEMORY/memory.h"     // Para acceder a la memoria
#include "../REGISTERS/registers.h" // Para manejar registros
#include "../DISK/disk.h"         // Para operaciones de disco
#include "../LOGGER/logger.h"     // Para registro de eventos

/* Inclusión de bibliotecas estándar */
#include <stdio.h>    // Entrada/salida estándar
#include <string.h>   // Manipulación de cadenas
#include <stdlib.h>   // Funciones generales
#include <ctype.h>    // Funciones de caracteres

/*
 * MACROS PARA COMPATIBILIDAD MULTIPLATAFORMA
 * Define macros para manejar diferencias entre Windows y sistemas Unix-like
 */
#ifdef _WIN32
    #include <windows.h>          // API de Windows para Sleep()
    #define CONSOLE_SLEEP(ms) Sleep(ms)  // Dormir en milisegundos (Windows)
#else
    #include <unistd.h>           // API POSIX para usleep()
    #define CONSOLE_SLEEP(ms) usleep(ms * 1000)  // Dormir en microsegundos (Unix)
#endif

/*
 * VARIABLES GLOBALES ESTÁTICAS
 * Solo accesibles dentro de este archivo (encapsulación)
 */
static ExecutionMode current_mode = MODE_NORMAL;  // Modo de ejecución actual
static int debug_step_count = 0;                   // Contador de pasos en modo depuración
static int program_loaded = 0;                     // Flag que indica si hay programa cargado

/*
 * PROTOTIPOS DE FUNCIONES INTERNAS
 * Funciones auxiliares no expuestas en la cabecera
 */
void debug_step();                                // Ejecutar un paso de depuración
int load_program_file(const char* filename);      // Cargar programa desde archivo
void show_detailed_registers();                   // Mostrar registros con formato detallado

// Declaración de función externa (si existe en otro módulo)
void set_memory_region(int base, int limit);

/*
 * Función: init_console
 * Propósito: Inicializa la consola mostrando el banner y ayuda inicial
 * No recibe parámetros ni retorna valores
 */
void init_console() {
    printf("========================================\n");
    printf("    SISTEMA OPERATIVO VIRTUAL - FASE I\n");
    printf("========================================\n");
    printf("Comandos disponibles:\n");
    printf("  run <archivo>      - Ejecutar programa en modo normal\n");
    printf("  debug <archivo>    - Ejecutar en modo depurador\n");
    printf("  step               - Ejecutar una instrucción (debug)\n");
    printf("  continue           - Continuar ejecución (debug)\n");
    printf("  registers          - Mostrar registros\n");
    printf("  memory [inicio] [fin] - Mostrar memoria\n");
    printf("  disk               - Información del disco\n");
    printf("  load <archivo>     - Cargar programa sin ejecutar\n");
    printf("  help               - Mostrar ayuda\n");
    printf("  exit               - Salir del sistema\n");
    printf("========================================\n\n");
}

/*
 * Función: show_prompt
 * Propósito: Muestra el prompt apropiado según el modo actual
 * El prompt cambia en modo depuración para mostrar el contador de pasos
 */
void show_prompt() {
    if (current_mode == MODE_DEBUGGER) {
        printf("DEBUG [%d] >> ", debug_step_count);  // Prompt de depuración con contador
    } else {
        printf("SYS >> ");  // Prompt normal del sistema
    }
}

/*
 * Función: parse_command
 * Parámetros: input - cadena de texto con el comando ingresado por el usuario
 * Retorna: ParsedCommand - estructura con el comando parseado y sus parámetros
 * Propósito: Analiza sintácticamente la entrada del usuario y la convierte
 *            en una estructura de comando manejable por el sistema
 */
ParsedCommand parse_command(const char* input) {
    ParsedCommand cmd;
    // Valores por defecto
    cmd.cmd = CMD_UNKNOWN;   // Comando desconocido hasta que se identifique
    cmd.mode = current_mode; // Hereda el modo actual
    cmd.param1 = -1;         // -1 indica parámetro no especificado
    cmd.param2 = -1;         // -1 indica parámetro no especificado
    cmd.filename[0] = '\0';  // Cadena vacía para nombre de archivo
    
    char buffer[200];  // Buffer temporal para procesar la entrada
    strcpy(buffer, input);  // Copiar la entrada al buffer
    
    // Normalización de la entrada: convertir a minúsculas y eliminar saltos de línea
    for (int i = 0; buffer[i]; i++) {
        buffer[i] = tolower(buffer[i]);  // Convertir a minúsculas
        if (buffer[i] == '\n') buffer[i] = '\0';  // Eliminar newline
    }
    
    // Tokenización: dividir la cadena en palabras usando espacios y tabs como delimitadores
    char* token = strtok(buffer, " \t");  // Obtener primer token (el comando)
    
    if (token == NULL) {
        return cmd;  // Entrada vacía, retorna comando desconocido
    }
    
    /*
     * IDENTIFICACIÓN DE COMANDOS
     * Compara el primer token con cada comando posible
     */
    if (strcmp(token, "run") == 0) {
        cmd.cmd = CMD_RUN;
        token = strtok(NULL, " \t");  // Obtener siguiente token (nombre de archivo)
        if (token) {
            strncpy(cmd.filename, token, sizeof(cmd.filename) - 1);
            cmd.filename[sizeof(cmd.filename) - 1] = '\0';  // Asegurar terminación nula
            cmd.mode = MODE_NORMAL;  // Este comando siempre se ejecuta en modo normal
        }
    }
    else if (strcmp(token, "debug") == 0) {
        cmd.cmd = CMD_DEBUG;
        token = strtok(NULL, " \t");
        if (token) {
            strncpy(cmd.filename, token, sizeof(cmd.filename) - 1);
            cmd.filename[sizeof(cmd.filename) - 1] = '\0';
            cmd.mode = MODE_DEBUGGER;  // Este comando activa modo depurador
        }
    }
    else if (strcmp(token, "step") == 0 || strcmp(token, "s") == 0) {
        cmd.cmd = CMD_STEP;  // Comando abreviado 's' también válido
    }
    else if (strcmp(token, "continue") == 0 || strcmp(token, "c") == 0) {
        cmd.cmd = CMD_CONTINUE;  // Comando abreviado 'c' también válido
    }
    else if (strcmp(token, "registers") == 0 || strcmp(token, "reg") == 0 || strcmp(token, "r") == 0) {
        cmd.cmd = CMD_REGISTERS;  // Múltiples alias para conveniencia
    }
    else if (strcmp(token, "memory") == 0 || strcmp(token, "mem") == 0 || strcmp(token, "m") == 0) {
        cmd.cmd = CMD_MEMORY;
        // Procesar parámetros opcionales para memoria
        token = strtok(NULL, " \t");
        if (token) cmd.param1 = atoi(token);  // Convertir a entero
        token = strtok(NULL, " \t");
        if (token) cmd.param2 = atoi(token);
    }
    else if (strcmp(token, "disk") == 0 || strcmp(token, "d") == 0) {
        cmd.cmd = CMD_DISK;  // Comando abreviado 'd' también válido
    }
    else if (strcmp(token, "load") == 0) {
        cmd.cmd = CMD_LOAD;
        token = strtok(NULL, " \t");
        if (token) {
            strncpy(cmd.filename, token, sizeof(cmd.filename) - 1);
            cmd.filename[sizeof(cmd.filename) - 1] = '\0';
        }
    }
    else if (strcmp(token, "help") == 0 || strcmp(token, "?") == 0 || strcmp(token, "h") == 0) {
        cmd.cmd = CMD_HELP;  // Múltiples formas de pedir ayuda
    }
    else if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0 || strcmp(token, "q") == 0) {
        cmd.cmd = CMD_EXIT;  // Múltiples formas de salir
    }
    
    return cmd;  // Retornar comando parseado
}

/*
 * Función: load_program_file (INTERNA)
 * Parámetros: filename - nombre del archivo con el programa
 * Retorna: int - dirección de inicio del programa en memoria, o -1 si hay error
 * Propósito: Carga un programa desde archivo a memoria. Actualmente simula la carga
 *            de un programa de ejemplo. En una implementación real leería del archivo.
 */
int load_program_file(const char* filename) {
    printf("Cargando programa: %s\n", filename);
    
    /*
     * SIMULACIÓN DE CARGA DE PROGRAMA
     * En una implementación real, se leería el archivo y se cargarían
     * las instrucciones reales. Aquí se simula con un programa de ejemplo.
     */
    
    // Programa de ejemplo: suma 5 + 3 y almacena el resultado
    // Cada instrucción es una Word de 8 dígitos
    
    // 00050000 = LOAD inmediato 5 (carga el valor 5 en el acumulador)
    write_memory(300, (Word){"00050000"});
    
    // 01030000 = ADD inmediato 3 (suma 3 al acumulador)
    write_memory(301, (Word){"01030000"});
    
    // 05001200 = STORE en dirección 312 (guarda resultado en memoria)
    write_memory(302, (Word){"05001200"});
    
    // 45000000 = HALT (detiene la ejecución)
    write_memory(303, (Word){"45000000"});
    
    /*
     * CONFIGURACIÓN DE LA REGIÓN DE MEMORIA PARA EL PROCESO
     * Define el espacio de memoria que el proceso puede usar
     */
    // Configurar registro base (RB) a 300
    cpu_registers.RB = int_to_word(300);
    // Configurar registro límite (RL) a 100 (tamaño de la región)
    cpu_registers.RL = int_to_word(100);
    
    program_loaded = 1;  // Marcar que hay un programa cargado
    return 300;  // Retornar dirección de inicio del programa
}

/*
 * Función: execute_command
 * Parámetros: cmd - comando parseado a ejecutar
 * Propósito: Ejecuta el comando especificado, llamando a las funciones
 *            apropiadas de otros módulos del sistema
 */
void execute_command(ParsedCommand cmd) {
    switch(cmd.cmd) {
        case CMD_RUN:
            printf("Ejecutando %s en modo normal...\n", cmd.filename);
            current_mode = MODE_NORMAL;  // Establecer modo normal
            {
                int start_addr = load_program_file(cmd.filename);
                if (start_addr != -1) {
                    execute_program(start_addr);  // Ejecutar programa continuamente
                }
            }
            break;
            
        case CMD_DEBUG:
            printf("Ejecutando %s en modo depurador...\n", cmd.filename);
            current_mode = MODE_DEBUGGER;  // Establecer modo depuración
            debug_step_count = 0;  // Reiniciar contador de pasos
            {
                int start_addr = load_program_file(cmd.filename);
                if (start_addr != -1) {
                    // Configurar CPU para depuración
                    cpu_registers.PSW.PC_psw = start_addr;  // Poner PC en inicio
                    set_PC_int(start_addr);  // Configurar contador de programa
                    set_cpu_state(CPU_RUNNING);  // Poner CPU en estado de ejecución
                    printf("Programa cargado en dirección %d. Use 'step' para ejecutar paso a paso.\n", start_addr);
                }
            }
            break;
            
        case CMD_STEP:
            if (current_mode == MODE_DEBUGGER) {
                if (get_cpu_state() != CPU_RUNNING) {
                    printf("CPU no está en ejecución. Use 'debug <archivo>' primero.\n");
                    break;
                }
                debug_step();  // Ejecutar un solo ciclo de CPU
                debug_step_count++;  // Incrementar contador de pasos
            } else {
                printf("Comando 'step' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_CONTINUE:
            if (current_mode == MODE_DEBUGGER) {
                printf("Continuando ejecución automática...\n");
                current_mode = MODE_NORMAL;  // Cambiar a modo normal
                // Ejecutar ciclos de CPU hasta que la CPU se detenga (HALT)
                while (get_cpu_state() == CPU_RUNNING) {
                    cpu_cycle();  // Ejecutar un ciclo de CPU
                    CONSOLE_SLEEP(100);  // Pequeña pausa para visibilidad
                }
                printf("Ejecución completada.\n");
                dump_registers();  // Mostrar estado final de registros
            } else {
                printf("Comando 'continue' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_REGISTERS:
            show_detailed_registers();  // Mostrar registros con formato detallado
            break;
            
        case CMD_MEMORY:
            // Mostrar memoria según parámetros proporcionados
            if (cmd.param1 == -1) {
                // Sin parámetros: mostrar zona del sistema operativo
                dump_memory(OS_RESERVED, OS_RESERVED + 20);
            } else if (cmd.param2 == -1) {
                // Un parámetro: mostrar 20 palabras desde esa dirección
                dump_memory(cmd.param1, cmd.param1 + 20);
            } else {
                // Dos parámetros: mostrar rango específico
                dump_memory(cmd.param1, cmd.param2);
            }
            break;
            
        case CMD_DISK:
            disk_info();  // Mostrar información del disco
            break;
            
        case CMD_LOAD:
            printf("Cargando programa: %s\n", cmd.filename);
            load_program_file(cmd.filename);  // Solo cargar, no ejecutar
            printf("Programa cargado. Use 'run' o 'debug' para ejecutar.\n");
            break;
            
        case CMD_HELP:
            show_help();  // Mostrar ayuda
            break;
            
        case CMD_EXIT:
            printf("Saliendo del sistema...\n");
            break;
            
        case CMD_UNKNOWN:
            printf("Comando desconocido. Escribe 'help' para ver comandos disponibles.\n");
            break;
    }
}

/*
 * Función: show_help
 * Propósito: Muestra la ayuda de comandos. Reutiliza init_console que ya la muestra.
 */
void show_help() {
    init_console();  // Reutilizar función de inicialización que ya muestra ayuda
}

/*
 * Función: show_detailed_registers (INTERNA)
 * Propósito: Muestra el estado de todos los registros con formato detallado
 *            y descripciones comprensibles
 */
void show_detailed_registers() {
    printf("\n=== REGISTROS DETALLADOS ===\n");
    
    // Mostrar registros principales con su valor decimal
    printf("AC:  %s (int: %d)\n", cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    printf("PC:  %d (Word: %s)\n", cpu_registers.PSW.PC_psw, cpu_registers.PC.data);
    printf("IR:  %s\n", cpu_registers.IR.data);
    printf("MAR: %s\n", cpu_registers.MAR.data);
    printf("MDR: %s\n", cpu_registers.MDR.data);
    printf("RB:  %s (int: %d) - Registro Base\n", cpu_registers.RB.data, word_to_int(cpu_registers.RB));
    printf("RL:  %s (int: %d) - Registro Límite\n", cpu_registers.RL.data, word_to_int(cpu_registers.RL));
    printf("SP:  %s (int: %d) - Stack Pointer\n", cpu_registers.SP.data, word_to_int(cpu_registers.SP));
    printf("RX:  %s - Base de pila\n", cpu_registers.RX.data);
    
    // Mostrar Palabra de Estado (PSW) con descripciones
    printf("\n=== PALABRA DE ESTADO (PSW) ===\n");
    printf("Condition Code:    %d (", cpu_registers.PSW.condition_code);
    switch(cpu_registers.PSW.condition_code) {
        case 0: printf("ZERO/Equal"); break;
        case 1: printf("Less Than"); break;
        case 2: printf("Greater Than"); break;
        case 3: printf("Overflow"); break;
        default: printf("Unknown"); break;
    }
    printf(")\n");
    printf("Operation Mode:    %s\n", cpu_registers.PSW.operation_mode ? "KERNEL" : "USER");
    printf("Interrupt Enabled: %s\n", cpu_registers.PSW.interrupt_enabled ? "SI" : "NO");
    printf("PC en PSW:         %d\n", cpu_registers.PSW.PC_psw);
    printf("PSW como Word:     %s\n", psw_to_word(cpu_registers.PSW).data);
    
    // Mostrar estado general del sistema
    printf("\n=== ESTADO CPU ===\n");
    printf("Estado: %s\n", (get_cpu_state() == CPU_RUNNING) ? "RUNNING" : "HALTED");
    printf("Modo consola: %s\n", (current_mode == MODE_DEBUGGER) ? "DEBUGGER" : "NORMAL");
    printf("===================================\n");
}

/*
 * Función: run_console
 * Propósito: Bucle principal de la consola. Lee comandos del usuario,
 *            los parsea y ejecuta hasta que se recibe el comando de salida.
 */
void run_console() {
    char input[200];  // Buffer para entrada del usuario
    
    while (1) {  // Bucle infinito hasta comando exit
        show_prompt();  // Mostrar prompt apropiado
        
        // Leer entrada del usuario
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;  // Salir si hay error de lectura (EOF)
        }
        
        // Parsear comando
        ParsedCommand cmd = parse_command(input);
        
        // Si es comando exit, ejecutarlo y salir del bucle
        if (cmd.cmd == CMD_EXIT) {
            execute_command(cmd);
            break;
        }
        
        // Ejecutar comando (excepto exit que ya se manejó)
        execute_command(cmd);
    }
}

/*
 * Función: get_current_mode
 * Retorna: ExecutionMode - modo de ejecución actual
 * Propósito: Obtiene el modo actual para que otros módulos puedan consultarlo
 */
ExecutionMode get_current_mode() {
    return current_mode;
}

/*
 * Función: set_current_mode
 * Parámetros: mode - nuevo modo de ejecución
 * Propósito: Establece el modo de ejecución, con lógica adicional para modo depuración
 */
void set_current_mode(ExecutionMode mode) {
    current_mode = mode;  // Actualizar modo actual
    if (mode == MODE_DEBUGGER) {
        debug_step_count = 0;  // Reiniciar contador de pasos al entrar en depuración
    }
}