#ifndef REGISTERS_H
#define REGISTERS_H

#include "../types.h"

// Palabra de Estado del Sistema (PSW)
typedef struct {
    unsigned int condition_code:4;   // 0-3
    unsigned int operation_mode:1;   // 0=usuario, 1=kernel
    unsigned int interrupt_enabled:1;// 0=deshab, 1=hab
    unsigned int PC_psw:10;          // PC dentro de PSW (10 bits, 0-1023)
} PSW;

// Registros de la CPU
typedef struct {
    Word AC;     // Acumulador
    Word MAR;    // Memory Address Register
    Word MDR;    // Memory Data Register
    Word IR;     // Instruction Register
    Word RB;     // Registro Base
    Word RL;     // Registro Límite
    Word RX;     // Base de pila
    Word SP;     // Stack Pointer
    Word PC;     // Program Counter real (como Word)
    PSW PSW;     // Palabra de Estado
} CPU_Registers;

// Declaraciones
void init_registers();
void dump_registers();
int word_to_int(Word w);
Word int_to_word(int value);
void update_condition_code(int result);
Word psw_to_word(PSW psw);
PSW word_to_psw(Word w);
void set_PC(Word pc_value);
void set_PC_int(int pc_int);

extern CPU_Registers cpu_registers;

#endif