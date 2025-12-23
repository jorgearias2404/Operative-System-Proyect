#include "interrupts.h"
#include "logger.h"
#include "registers.h"
#include <stdlib.h>

static InterruptHandler interrupt_vector[9];
static int pending_interrupts[9] = {0};

// Handlers de interrupciones
static void invalid_syscall_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 0: Código de llamada al sistema inválido");
}

static void invalid_interrupt_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 1: Código de interrupción inválido");
}

static void syscall_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 2: Llamada al sistema");
    // Cambiar a modo kernel
    cpu_registers.PSW.operation_mode = 1;
}

static void timer_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 3: Reloj");
}

static void io_completion_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 4: Finalización de operación de E/S");
}

static void invalid_instruction_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 5: Instrucción inválida");
    // Aquí podrías terminar el proceso
}

static void invalid_address_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 6: Direccionamiento inválido");
    // Mostrar información del acceso fallido
    int addr = word_to_int(cpu_registers.MAR);
    log_event(LOG_ERROR, "Acceso inválido a dirección: %d", addr);
}

static void underflow_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 7: Underflow");
    cpu_registers.PSW.condition_code = 7;
}

static void overflow_handler() {
    log_event(LOG_INTERRUPT, "Interrupción 8: Overflow");
    cpu_registers.PSW.condition_code = 3;
}

void init_interrupts() {
    // Configurar vector de interrupciones
    interrupt_vector[0] = invalid_syscall_handler;
    interrupt_vector[1] = invalid_interrupt_handler;
    interrupt_vector[2] = syscall_handler;
    interrupt_vector[3] = timer_handler;
    interrupt_vector[4] = io_completion_handler;
    interrupt_vector[5] = invalid_instruction_handler;
    interrupt_vector[6] = invalid_address_handler;
    interrupt_vector[7] = underflow_handler;
    interrupt_vector[8] = overflow_handler;
    
    // Inicializar arreglo de interrupciones pendientes
    for (int i = 0; i < 9; i++) {
        pending_interrupts[i] = 0;
    }
    
    log_event(LOG_INFO, "Sistema de interrupciones inicializado");
}

void trigger_interrupt(InterruptCode code) {
    if (code < 0 || code > 8) {
        log_event(LOG_ERROR, "Código de interrupción inválido: %d", code);
        trigger_interrupt(INT_INVALID_INTERRUPT);
        return;
    }
    
    // Marcar como pendiente si las interrupciones están habilitadas
    if (cpu_registers.PSW.interrupt_enabled) {
        pending_interrupts[code] = 1;
        log_event(LOG_DEBUG, "Interrupción %d marcada como pendiente", code);
    } else {
        log_event(LOG_DEBUG, "Interrupción %d ignorada (interrupciones deshabilitadas)", code);
    }
}

void handle_pending_interrupts() {
    // Verificar si hay interrupciones pendientes
    for (int i = 0; i < 9; i++) {
        if (pending_interrupts[i]) {
            log_event(LOG_DEBUG, "Manejando interrupción pendiente: %d", i);
            
            // Guardar contexto antes de manejar la interrupción
            save_context();
            
            // Cambiar a modo kernel
            cpu_registers.PSW.operation_mode = 1;
            
            // Ejecutar handler
            interrupt_vector[i]();
            
            // Limpiar interrupción pendiente
            pending_interrupts[i] = 0;
            
            // Restaurar contexto
            restore_context();
        }
    }
}

void save_context() {
    // Aquí deberías guardar el contexto actual en la pila
    // Por simplicidad, solo logueamos
    log_event(LOG_DEBUG, "Contexto guardado (simulado)");
}

void restore_context() {
    // Aquí deberías restaurar el contexto desde la pila
    log_event(LOG_DEBUG, "Contexto restaurado (simulado)");
}