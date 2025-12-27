#include "console.h"
#include "../CPU/cpu.h"
#include "../MEMORY/memory.h"
#include "../REGISTERS/registers.h"
#include "../DISK/disk.h"
#include "../LOGGER/logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#define CONSOLE_SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define CONSOLE_SLEEP(ms) usleep(ms * 1000)
#endif

static ExecutionMode current_mode = MODE_NORMAL;
static int debug_step_count = 0;
static int program_loaded = 0;

// Prototipos de funciones internas
void debug_step();
int load_program_file(const char* filename);
void show_detailed_registers();

// Declaración de set_memory_region (si no está en memory.h)
void set_memory_region(int base, int limit);

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

void show_prompt() {
    if (current_mode == MODE_DEBUGGER) {
        printf("DEBUG [%d] >> ", debug_step_count);
    } else {
        printf("SYS >> ");
    }
}

ParsedCommand parse_command(const char* input) {
    ParsedCommand cmd;
    cmd.cmd = CMD_UNKNOWN;
    cmd.mode = current_mode;
    cmd.param1 = -1;
    cmd.param2 = -1;
    cmd.filename[0] = '\0';  // Inicializar filename
    
    char buffer[200];
    strcpy(buffer, input);
    
    // Convertir a minúsculas y eliminar newline
    for (int i = 0; buffer[i]; i++) {
        buffer[i] = tolower(buffer[i]);
        if (buffer[i] == '\n') buffer[i] = '\0';
    }
    
    // Tokenizar
    char* token = strtok(buffer, " \t");
    
    if (token == NULL) {
        return cmd;
    }
    
    // Identificar comando
    if (strcmp(token, "run") == 0) {
        cmd.cmd = CMD_RUN;
        token = strtok(NULL, " \t");
        if (token) {
            strncpy(cmd.filename, token, sizeof(cmd.filename) - 1);
            cmd.filename[sizeof(cmd.filename) - 1] = '\0';
            cmd.mode = MODE_NORMAL;
        }
    }
    else if (strcmp(token, "debug") == 0) {
        cmd.cmd = CMD_DEBUG;
        token = strtok(NULL, " \t");
        if (token) {
            strncpy(cmd.filename, token, sizeof(cmd.filename) - 1);
            cmd.filename[sizeof(cmd.filename) - 1] = '\0';
            cmd.mode = MODE_DEBUGGER;
        }
    }
    else if (strcmp(token, "step") == 0 || strcmp(token, "s") == 0) {
        cmd.cmd = CMD_STEP;
    }
    else if (strcmp(token, "continue") == 0 || strcmp(token, "c") == 0) {
        cmd.cmd = CMD_CONTINUE;
    }
    else if (strcmp(token, "registers") == 0 || strcmp(token, "reg") == 0 || strcmp(token, "r") == 0) {
        cmd.cmd = CMD_REGISTERS;
    }
    else if (strcmp(token, "memory") == 0 || strcmp(token, "mem") == 0 || strcmp(token, "m") == 0) {
        cmd.cmd = CMD_MEMORY;
        token = strtok(NULL, " \t");
        if (token) cmd.param1 = atoi(token);
        token = strtok(NULL, " \t");
        if (token) cmd.param2 = atoi(token);
    }
    else if (strcmp(token, "disk") == 0 || strcmp(token, "d") == 0) {
        cmd.cmd = CMD_DISK;
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
        cmd.cmd = CMD_HELP;
    }
    else if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0 || strcmp(token, "q") == 0) {
        cmd.cmd = CMD_EXIT;
    }
    
    return cmd;
}

// Función para cargar programa desde archivo
int load_program_file(const char* filename) {
    printf("Cargando programa: %s\n", filename);
    
    // Simulación: crear un programa simple en memoria
    // En una implementación real, leerías del archivo
    
    // Ejemplo de programa: suma 5 + 3
    // Formato: 00050000 = 04 0 00500 (LOAD inmediato 5)
    write_memory(300, (Word){"00050000"});
    // 01030000 = 00 0 00300 (ADD inmediato 3)
    write_memory(301, (Word){"01030000"});
    // 05001200 = 05 0 01200 (STORE en dirección 312)
    write_memory(302, (Word){"05001200"});
    // 45000000 = 40 0 00000 (HALT)
    write_memory(303, (Word){"45000000"});
    
    // Configurar región de memoria para el proceso
    // Si set_memory_region no existe, configurar manualmente:
    cpu_registers.RB = int_to_word(300);
    cpu_registers.RL = int_to_word(100);
    
    program_loaded = 1;
    return 300;  // Dirección de inicio
}

void execute_command(ParsedCommand cmd) {
    switch(cmd.cmd) {
        case CMD_RUN:
            printf("Ejecutando %s en modo normal...\n", cmd.filename);
            current_mode = MODE_NORMAL;
            {
                int start_addr = load_program_file(cmd.filename);
                if (start_addr != -1) {
                    execute_program(start_addr);
                }
            }
            break;
            
        case CMD_DEBUG:
            printf("Ejecutando %s en modo depurador...\n", cmd.filename);
            current_mode = MODE_DEBUGGER;
            debug_step_count = 0;
            {
                int start_addr = load_program_file(cmd.filename);
                if (start_addr != -1) {
                    cpu_registers.PSW.PC_psw = start_addr;
                    set_PC_int(start_addr);
                    set_cpu_state(CPU_RUNNING);
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
                debug_step();
                debug_step_count++;
            } else {
                printf("Comando 'step' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_CONTINUE:
            if (current_mode == MODE_DEBUGGER) {
                printf("Continuando ejecución automática...\n");
                current_mode = MODE_NORMAL;
                // Ejecutar hasta HALT
                while (get_cpu_state() == CPU_RUNNING) {
                    cpu_cycle();
                    CONSOLE_SLEEP(100);  // Usar CONSOLE_SLEEP en lugar de CPU_SLEEP
                }
                printf("Ejecución completada.\n");
                dump_registers();
            } else {
                printf("Comando 'continue' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_REGISTERS:
            show_detailed_registers();
            break;
            
        case CMD_MEMORY:
            if (cmd.param1 == -1) {
                dump_memory(OS_RESERVED, OS_RESERVED + 20);
            } else if (cmd.param2 == -1) {
                dump_memory(cmd.param1, cmd.param1 + 20);
            } else {
                dump_memory(cmd.param1, cmd.param2);
            }
            break;
            
        case CMD_DISK:
            disk_info();
            break;
            
        case CMD_LOAD:
            printf("Cargando programa: %s\n", cmd.filename);
            load_program_file(cmd.filename);
            printf("Programa cargado. Use 'run' o 'debug' para ejecutar.\n");
            break;
            
        case CMD_HELP:
            show_help();
            break;
            
        case CMD_EXIT:
            printf("Saliendo del sistema...\n");
            break;
            
        case CMD_UNKNOWN:
            printf("Comando desconocido. Escribe 'help' para ver comandos disponibles.\n");
            break;
    }
}

void show_help() {
    init_console();
}

// Función mejorada para mostrar registros con más detalles
void show_detailed_registers() {
    printf("\n=== REGISTROS DETALLADOS ===\n");
    printf("AC:  %s (int: %d)\n", cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    printf("PC:  %d (Word: %s)\n", cpu_registers.PSW.PC_psw, cpu_registers.PC.data);
    printf("IR:  %s\n", cpu_registers.IR.data);
    printf("MAR: %s\n", cpu_registers.MAR.data);
    printf("MDR: %s\n", cpu_registers.MDR.data);
    printf("RB:  %s (int: %d) - Registro Base\n", cpu_registers.RB.data, word_to_int(cpu_registers.RB));
    printf("RL:  %s (int: %d) - Registro Límite\n", cpu_registers.RL.data, word_to_int(cpu_registers.RL));
    printf("SP:  %s (int: %d) - Stack Pointer\n", cpu_registers.SP.data, word_to_int(cpu_registers.SP));
    printf("RX:  %s - Base de pila\n", cpu_registers.RX.data);
    
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
    
    printf("\n=== ESTADO CPU ===\n");
    printf("Estado: %s\n", (get_cpu_state() == CPU_RUNNING) ? "RUNNING" : "HALTED");
    printf("Modo consola: %s\n", (current_mode == MODE_DEBUGGER) ? "DEBUGGER" : "NORMAL");
    printf("===================================\n");
}

void run_console() {
    char input[200];
    
    while (1) {
        show_prompt();
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        ParsedCommand cmd = parse_command(input);
        
        if (cmd.cmd == CMD_EXIT) {
            execute_command(cmd);
            break;
        }
        
        execute_command(cmd);
    }
}

ExecutionMode get_current_mode() {
    return current_mode;
}

void set_current_mode(ExecutionMode mode) {
    current_mode = mode;
    if (mode == MODE_DEBUGGER) {
        debug_step_count = 0;
    }
}