#ifndef CONSOLE_H
#define CONSOLE_H

#include "../types.h"

// Modos de ejecución
typedef enum {
    MODE_NORMAL,
    MODE_DEBUGGER
} ExecutionMode;

// Comandos de consola
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

// Estructura para parsear comandos
typedef struct {
    ConsoleCommand cmd;
    char filename[100];
    ExecutionMode mode;
    int param1;
    int param2;
} ParsedCommand;

// Funciones de consola
void init_console();
void run_console();
ParsedCommand parse_command(const char* input);
void execute_command(ParsedCommand cmd);
void show_help();
void show_prompt();
ExecutionMode get_current_mode();
void set_current_mode(ExecutionMode mode);

#endif