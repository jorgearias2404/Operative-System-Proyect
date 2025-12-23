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

static ExecutionMode current_mode = MODE_NORMAL;
static int debug_step_count = 0;

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
            strcpy(cmd.filename, token);
            cmd.mode = MODE_NORMAL;
        }
    }
    else if (strcmp(token, "debug") == 0) {
        cmd.cmd = CMD_DEBUG;
        token = strtok(NULL, " \t");
        if (token) {
            strcpy(cmd.filename, token);
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
    else if (strcmp(token, "help") == 0 || strcmp(token, "?") == 0 || strcmp(token, "h") == 0) {
        cmd.cmd = CMD_HELP;
    }
    else if (strcmp(token, "exit") == 0 || strcmp(token, "quit") == 0 || strcmp(token, "q") == 0) {
        cmd.cmd = CMD_EXIT;
    }
    
    return cmd;
}

void execute_command(ParsedCommand cmd) {
    switch(cmd.cmd) {
        case CMD_RUN:
            printf("Ejecutando %s en modo normal...\n", cmd.filename);
            current_mode = MODE_NORMAL;
            // TODO: Implementar carga y ejecución del programa
            break;
            
        case CMD_DEBUG:
            printf("Ejecutando %s en modo depurador...\n", cmd.filename);
            current_mode = MODE_DEBUGGER;
            debug_step_count = 0;
            // TODO: Implementar carga para depuración
            break;
            
        case CMD_STEP:
            if (current_mode == MODE_DEBUGGER) {
                printf("--- Paso %d ---\n", ++debug_step_count);
                // Ejecutar un ciclo de instrucción
                // TODO: Implementar cpu_cycle_step()
                dump_registers();
            } else {
                printf("Comando 'step' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_CONTINUE:
            if (current_mode == MODE_DEBUGGER) {
                printf("Continuando ejecución...\n");
                current_mode = MODE_NORMAL;
            } else {
                printf("Comando 'continue' solo disponible en modo depurador\n");
            }
            break;
            
        case CMD_REGISTERS:
            dump_registers();
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
    init_console();  // Reutilizamos la función de inicialización
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