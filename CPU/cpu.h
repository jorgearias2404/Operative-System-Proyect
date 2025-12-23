#ifndef CPU_H
#define CPU_H

#include "../types.h"

// Modos de direccionamiento
typedef enum {
    ADDR_DIRECT = 0,    // Directo
    ADDR_IMMEDIATE = 1, // Inmediato
    ADDR_INDEXED = 2    // Indexado
} AddressingMode;

// Estructura de instrucción decodificada
typedef struct {
    int opcode;             // Código de operación
    AddressingMode mode;    // Modo de direccionamiento
    int value;             // Valor/desplazamiento
    int effective_address; // Dirección efectiva calculada
} Instruction;

// Estados de la CPU
typedef enum {
    CPU_RUNNING,
    CPU_HALTED,
    CPU_WAITING_IO,
    CPU_ERROR
} CPU_State;

// Funciones de la CPU
void init_cpu();
void cpu_cycle();
void cpu_cycle_step();  // Para modo debug
void execute_instruction(Instruction instr);
Instruction fetch_instruction();
Instruction decode_instruction(Word instruction_word);
int calculate_effective_address(AddressingMode mode, int value);
void handle_arithmetic_operation(int opcode, int operand);
void handle_memory_operation(int opcode, int address);
void handle_jump_operation(int opcode, int address);
void handle_io_operation(int opcode);
void handle_system_operation(int opcode);
void set_cpu_state(CPU_State state);
CPU_State get_cpu_state();
void reset_cpu();

// Función para ejecutar un programa completo
void execute_program(int start_address);

extern CPU_State cpu_state;

#endif