// memory.h - Asegurar que read_memory devuelve Word
#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include "../types.h"  // Incluir types.h para Word

#define MEMORY_SIZE 2000
#define OS_RESERVED 300

// Declaraciones CORREGIDAS
void init_memory();
Word read_memory(int address);  // Devuelve Word, no int
void write_memory(int address, Word word);
bool is_valid_address(int address, bool is_kernel_mode);
void dump_memory(int start, int end);

extern Word memory[MEMORY_SIZE];

#endif