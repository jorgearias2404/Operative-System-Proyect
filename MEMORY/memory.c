#include "memory.h"
#include "logger.h"
#include "../REGISTERS/registers.h"  // AÑADIR ESTO
#include "../INTERRUPTS/interrupts.h"
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

// FUNCIÓN AUXILIAR: Convertir dirección lógica a física con protección
static int logical_to_physical(int logical_address) {
    int rb_value = word_to_int(cpu_registers.RB);
    int rl_value = word_to_int(cpu_registers.RL);
    
    // Si RB = 0 y RL = 0, modo kernel (sin protección)
    if (rb_value == 0 && rl_value == 0) {
        return logical_address;
    }
    
    int physical_address = logical_address + rb_value;
    
    // Verificar límites
    if (physical_address < rb_value || physical_address >= (rb_value + rl_value)) {
        log_event(LOG_ERROR, "Violación de memoria: dirección %d fuera de límites [RB=%d, RL=%d]", 
                  logical_address, rb_value, rl_value);
        trigger_interrupt(INT_INVALID_ADDRESS);
        return -1;
    }
    
    return physical_address;
}

Word read_memory(int logical_address) {
    int physical_address = logical_to_physical(logical_address);
    
    if (physical_address < 0) {
        Word error_word;
        strcpy(error_word.data, "MEM_ERR");
        return error_word;
    }
    
    if (physical_address < 0 || physical_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "Dirección física inválida: %d", physical_address);
        Word error_word;
        strcpy(error_word.data, "ADDR_ERR");
        return error_word;
    }
    
    // Si es área del SO, verificar modo kernel
    if (physical_address < OS_RESERVED && cpu_registers.PSW.operation_mode == USER_MODE) {
        log_event(LOG_ERROR, "Usuario intenta leer área del SO: %d", physical_address);
        trigger_interrupt(INT_INVALID_ADDRESS);
        Word error_word;
        strcpy(error_word.data, "PRIV_ERR");
        return error_word;
    }
    
    log_event(LOG_DEBUG, "Lectura: lógica=%d -> física=%d = %s", 
              logical_address, physical_address, memory[physical_address].data);
    return memory[physical_address];
}

void write_memory(int logical_address, Word word) {
    int physical_address = logical_to_physical(logical_address);
    
    if (physical_address < 0) {
        return;
    }
    
    if (physical_address < 0 || physical_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "Dirección física inválida para escritura: %d", physical_address);
        return;
    }
    
    // Si es área del SO, verificar modo kernel
    if (physical_address < OS_RESERVED && cpu_registers.PSW.operation_mode == USER_MODE) {
        log_event(LOG_ERROR, "Usuario intenta escribir en área del SO: %d", physical_address);
        trigger_interrupt(INT_INVALID_ADDRESS);
        return;
    }
    
    memory[physical_address] = word;
    log_event(LOG_DEBUG, "Escritura: lógica=%d -> física=%d = %s", 
              logical_address, physical_address, word.data);
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

// NUEVA FUNCIÓN: Configurar región de memoria para proceso
void set_memory_region(int base, int limit) {
    cpu_registers.RB = int_to_word(base);
    cpu_registers.RL = int_to_word(limit);
    log_event(LOG_INFO, "Región de memoria configurada: RB=%d, RL=%d", base, limit);
}