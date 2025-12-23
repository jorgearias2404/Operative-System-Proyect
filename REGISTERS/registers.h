// registers.h - Asegurar declaraciones
#ifndef REGISTERS_H
#define REGISTERS_H

#include "../types.h"

// Palabra de Estado del Sistema (PSW)
typedef struct {
    int condition_code;      // 0-3
    int operation_mode;      // 0=usuario, 1=kernel
    int interrupt_enabled;   // 0=deshab, 1=hab
    int PC;                  // Program Counter
} PSW;

// Registros de la CPU
typedef struct {
    Word AC;    // Acumulador
    Word MAR;   // Memory Address Register
    Word MDR;   // Memory Data Register
    Word IR;    // Instruction Register
    Word RB;    // Registro Base
    Word RL;    // Registro Límite
    Word RX;    // Base de pila
    Word SP;    // Stack Pointer
    PSW PSW;    // Palabra de Estado
} CPU_Registers;

// Declaraciones CORREGIDAS
void init_registers();
void dump_registers();
int word_to_int(Word w);      // Devuelve int
Word int_to_word(int value);  // Recibe int, devuelve Word
void update_condition_code(int result);
Word psw_to_word(PSW psw);
PSW word_to_psw(Word w);

extern CPU_Registers cpu_registers;

#endif