#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../types.h"  // Incluir types.h en lugar de memory.h

// Códigos de interrupción
typedef enum {
    INT_INVALID_SYSCALL = 0,
    INT_INVALID_INTERRUPT = 1,
    INT_SYSCALL = 2,
    INT_TIMER = 3,
    INT_IO_COMPLETION = 4,
    INT_INVALID_INSTRUCTION = 5,
    INT_INVALID_ADDRESS = 6,
    INT_UNDERFLOW = 7,
    INT_OVERFLOW = 8
} InterruptCode;

// Vector de interrupciones (9 entradas)
typedef void (*InterruptHandler)(void);

// Inicializar sistema de interrupciones
void init_interrupts();
// Disparar una interrupción
void trigger_interrupt(InterruptCode code);
// Manejar interrupciones pendientes
void handle_pending_interrupts();
// Guardar contexto
void save_context();
// Restaurar contexto
void restore_context();

#endif