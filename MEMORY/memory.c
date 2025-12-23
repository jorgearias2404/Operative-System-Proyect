#include "memory.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

Word memory[MEMORY_SIZE];

void init_memory() {
    // Inicializar toda la memoria con ceros
    Word zero_word;
    strcpy(zero_word.data, "00000000");
    
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = zero_word;
    }
    
    // Marcar área del sistema operativo
    for (int i = 0; i < OS_RESERVED; i++) {
        strcpy(memory[i].data, "OS_RESERVED");
    }
    
    log_event(LOG_INFO, "Memoria inicializada: %d palabras totales, %d reservadas para SO", 
              MEMORY_SIZE, OS_RESERVED);
}

Word read_memory(int address) {
    if (address < 0 || address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "Intento de lectura en dirección inválida: %d", address);
        Word error_word;
        strcpy(error_word.data, "ERROR");
        return error_word;
    }
    
    log_event(LOG_DEBUG, "Lectura de memoria[%d] = %s", address, memory[address].data);
    return memory[address];
}

void write_memory(int address, Word word) {
    if (address < 0 || address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "Intento de escritura en dirección inválida: %d", address);
        return;
    }
    
    // Si es área del SO, solo permitir en modo kernel
    if (address < OS_RESERVED) {
        log_event(LOG_WARNING, "Intento de escribir en área del SO: %d", address);
        // Aquí deberías verificar el modo de operación
    }
    
    memory[address] = word;
    log_event(LOG_DEBUG, "Escritura en memoria[%d] = %s", address, word.data);
}

bool is_valid_address(int address, bool is_kernel_mode) {
    if (is_kernel_mode) {
        return (address >= 0 && address < MEMORY_SIZE);
    } else {
        return (address >= OS_RESERVED && address < MEMORY_SIZE);
    }
}

void dump_memory(int start, int end) {
    if (start < 0) start = 0;
    if (end >= MEMORY_SIZE) end = MEMORY_SIZE - 1;
    
    printf("\nDUMP de memoria [%d - %d]:\n", start, end);
    for (int i = start; i <= end; i++) {
        printf("%04d: %s\n", i, memory[i].data);
    }
}