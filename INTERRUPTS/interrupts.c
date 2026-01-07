/*
 * Archivo de implementación del módulo de interrupciones del Sistema Operativo Virtual.
 * Contiene la lógica completa para manejar interrupciones, incluyendo el vector de
 * interrupciones, handlers específicos y gestión de interrupciones pendientes.
 */

/* Inclusión de cabecera propia del módulo */
#include "interrupts.h"

/* Inclusión de cabeceras de otros módulos */
#include "../LOGGER/logger.h"      // Para registro de eventos del sistema
#include "../REGISTERS/registers.h" // Para acceder a los registros de la CPU

/* Inclusión de bibliotecas estándar */
#include <stdlib.h>   // Para funciones generales

/*
 * VARIABLES GLOBALES ESTÁTICAS
 * Solo accesibles dentro de este archivo (encapsulación)
 */

/*
 * Vector de interrupciones (Interrupt Vector Table - IVT)
 * Array de punteros a funciones, donde cada posición corresponde a un código
 * de interrupción específico.
 * 
 * Índice: 0 = INT_INVALID_SYSCALL
 *         1 = INT_INVALID_INTERRUPT
 *         ... etc
 */
static InterruptHandler interrupt_vector[9];

/*
 * Array de interrupciones pendientes
 * Cada posición indica si hay una interrupción de ese tipo esperando ser procesada.
 * 0 = no pendiente, 1 = pendiente
 */
static int pending_interrupts[9] = {0};

/*
 * HANDLERS DE INTERRUPCIONES (funciones estáticas)
 * Cada función maneja un tipo específico de interrupción.
 * Estas funciones son privadas a este módulo.
 */

/*
 * Handler: invalid_syscall_handler
 * Propósito: Manejar interrupción de llamada al sistema inválida (código 0).
 * Se activa cuando un programa intenta realizar una llamada al sistema
 * con un código no válido o no implementado.
 */
static void invalid_syscall_handler() {
    // Registrar el evento en el logger con nivel LOG_INTERRUPT
    log_event(LOG_INTERRUPT, 
              "Interrupción 0: Código de llamada al sistema inválido");
    
    // En una implementación real, aquí se podría:
    // 1. Terminar el proceso que hizo la llamada inválida
    // 2. Mostrar mensaje de error al usuario
    // 3. Registrar estadísticas de errores
}

/*
 * Handler: invalid_interrupt_handler
 * Propósito: Manejar interrupción de código de interrupción inválido (código 1).
 * Se activa cuando se intenta disparar una interrupción con un código fuera de rango.
 */
static void invalid_interrupt_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 1: Código de interrupción inválido");
    
    // Esta es una interrupción "meta" - maneja errores en el sistema de interrupciones
}

/*
 * Handler: syscall_handler
 * Propósito: Manejar llamadas al sistema válidas (código 2).
 * Se activa cuando un programa realiza una llamada al sistema legítima
 * mediante la instrucción SVC (Service Call).
 */
static void syscall_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 2: Llamada al sistema");
    
    // Cambiar a modo kernel para ejecutar código privilegiado
    // En sistemas operativos reales, esto permite al sistema operativo
    // realizar operaciones que no están permitidas en modo usuario
    cpu_registers.PSW.operation_mode = 1;  // 1 = KERNEL_MODE
    
    // En una implementación completa, aquí se llamaría al dispatcher
    // del sistema operativo para manejar la llamada específica
}

/*
 * Handler: timer_handler
 * Propósito: Manejar interrupciones del reloj del sistema (código 3).
 * Se activa periódicamente para permitir al sistema operativo realizar
 * tareas como planificación de procesos y contabilidad de tiempo.
 */
static void timer_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 3: Reloj");
    
    // En una implementación real, aquí el sistema operativo podría:
    // 1. Cambiar de proceso (planificación)
    // 2. Actualizar contadores de tiempo
    // 3. Verificar timeouts
    // 4. Ejecutar tareas periódicas del sistema
}

/*
 * Handler: io_completion_handler
 * Propósito: Manejar finalización de operaciones de E/S (código 4).
 * Se activa cuando un dispositivo de entrada/salida (como el DMA o disco)
 * completa una operación solicitada.
 */
static void io_completion_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 4: Finalización de operación de E/S");
    
    // Esta interrupción es crucial para:
    // 1. Notificar a procesos que sus operaciones de E/S han terminado
    // 2. Reactivar procesos que estaban esperando E/S
    // 3. Manejar buffers de E/S
}

/*
 * Handler: invalid_instruction_handler
 * Propósito: Manejar instrucciones de CPU inválidas (código 5).
 * Se activa cuando la CPU intenta ejecutar una instrucción con un
 * opcode no reconocido o en un estado no permitido.
 */
static void invalid_instruction_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 5: Instrucción inválida");
    
    // Comportamiento típico:
    // 1. Terminar el proceso que ejecutó la instrucción inválida
    // 2. Generar un core dump para depuración
    // 3. Notificar al usuario/programador
    
    // Ejemplo: si un programa intenta ejecutar datos como código
}

/*
 * Handler: invalid_address_handler
 * Propósito: Manejar accesos a direcciones de memoria inválidas (código 6).
 * Se activa cuando un programa intenta acceder a una dirección de memoria
 * fuera de su espacio de direcciones permitido.
 */
static void invalid_address_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 6: Direccionamiento inválido");
    
    // Mostrar información detallada del acceso fallido
    // MAR (Memory Address Register) contiene la dirección que causó el fallo
    int addr = word_to_int(cpu_registers.MAR);
    log_event(LOG_ERROR, 
              "Acceso inválido a dirección: %d", addr);
    
    // En sistemas con memoria virtual, esto podría ser un page fault
    // y el sistema operativo podría manejar cargando la página desde disco
}

/*
 * Handler: underflow_handler
 * Propósito: Manejar underflow aritmético (código 7).
 * Se activa cuando una operación aritmética produce un resultado
 * demasiado pequeño para ser representado (menor que el mínimo valor).
 */
static void underflow_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 7: Underflow");
    
    // Establecer condition code apropiado en la PSW
    // En este sistema, 7 representa underflow
    cpu_registers.PSW.condition_code = 7;
    
    // Underflow ocurre comúnmente en:
    // 1. Operaciones en coma flotante
    // 2. División por números muy grandes
    // 3. Operaciones con precisión limitada
}

/*
 * Handler: overflow_handler
 * Propósito: Manejar overflow aritmético (código 8).
 * Se activa cuando una operación aritmética produce un resultado
 * demasiado grande para ser representado (mayor que el máximo valor).
 */
static void overflow_handler() {
    log_event(LOG_INTERRUPT, 
              "Interrupción 8: Overflow");
    
    // Establecer condition code apropiado en la PSW
    // En este sistema, 3 representa overflow (consistente con CPU)
    cpu_registers.PSW.condition_code = 3;
    
    // Overflow ocurre comúnmente en:
    // 1. Suma de números muy grandes
    // 2. Multiplicación que excede el rango
    // 3. Conversiones de tipos
}

/*
 * Función: init_interrupts
 * Propósito: Inicializar el sistema de interrupciones.
 * Configura el vector de interrupciones y limpia el estado de interrupciones pendientes.
 */
void init_interrupts() {
    /*
     * CONFIGURAR VECTOR DE INTERRUPCIONES
     * Asigna cada handler a su posición correspondiente en el vector.
     * Esta tabla define qué función se ejecuta para cada tipo de interrupción.
     */
    interrupt_vector[0] = invalid_syscall_handler;    // INT_INVALID_SYSCALL
    interrupt_vector[1] = invalid_interrupt_handler;  // INT_INVALID_INTERRUPT
    interrupt_vector[2] = syscall_handler;            // INT_SYSCALL
    interrupt_vector[3] = timer_handler;              // INT_TIMER
    interrupt_vector[4] = io_completion_handler;      // INT_IO_COMPLETION
    interrupt_vector[5] = invalid_instruction_handler; // INT_INVALID_INSTRUCTION
    interrupt_vector[6] = invalid_address_handler;    // INT_INVALID_ADDRESS
    interrupt_vector[7] = underflow_handler;          // INT_UNDERFLOW
    interrupt_vector[8] = overflow_handler;           // INT_OVERFLOW
    
    /*
     * INICIALIZAR ARREGLO DE INTERRUPCIONES PENDIENTES
     * Establece todas las interrupciones como "no pendientes" (valor 0).
     */
    for (int i = 0; i < 9; i++) {
        pending_interrupts[i] = 0;
    }
    
    // Registrar inicialización exitosa
    log_event(LOG_INFO, "Sistema de interrupciones inicializado");
}

/*
 * Función: trigger_interrupt
 * Parámetros: code - código de la interrupción a disparar
 * Propósito: Marcar una interrupción como pendiente para ser procesada.
 * 
 * Nota: Esta función NO ejecuta el handler inmediatamente.
 * Solo marca la interrupción como pendiente. El handler se ejecutará
 * cuando se llame a handle_pending_interrupts().
 */
void trigger_interrupt(InterruptCode code) {
    // VALIDAR CÓDIGO DE INTERRUPCIÓN
    if (code < 0 || code > 8) {
        // Código inválido, registrar error
        log_event(LOG_ERROR, 
                  "Código de interrupción inválido: %d", code);
        
        // Disparar interrupción de error por interrupción inválida
        // Esto previene que errores en el código de interrupción causen
        // comportamientos indefinidos
        trigger_interrupt(INT_INVALID_INTERRUPT);
        return;
    }
    
    /*
     * VERIFICAR SI LAS INTERRUPCIONES ESTÁN HABILITADAS
     * Las interrupciones pueden estar deshabilitadas temporalmente
     * durante operaciones críticas del sistema.
     */
    if (cpu_registers.PSW.interrupt_enabled) {
        // Interrupciones habilitadas: marcar como pendiente
        pending_interrupts[code] = 1;
        
        // Registrar para depuración
        log_event(LOG_DEBUG, 
                  "Interrupción %d marcada como pendiente", code);
    } else {
        // Interrupciones deshabilitadas: ignorar (por ahora)
        log_event(LOG_DEBUG, 
                  "Interrupción %d ignorada (interrupciones deshabilitadas)", code);
        
        // NOTA: En algunos sistemas, las interrupciones deshabilitadas
        // se mantienen pendientes hasta que se habiliten nuevamente.
        // Esta implementación las descarta.
    }
}

/*
 * Función: handle_pending_interrupts
 * Propósito: Procesar todas las interrupciones pendientes.
 * Esta función debe ser llamada periódicamente (normalmente al final de
 * cada ciclo de instrucción de la CPU) para verificar y manejar interrupciones.
 */
void handle_pending_interrupts() {
    // Recorrer todas las posibles interrupciones
    for (int i = 0; i < 9; i++) {
        // Verificar si esta interrupción está pendiente
        if (pending_interrupts[i]) {
            // Registrar que se va a manejar esta interrupción
            log_event(LOG_DEBUG, 
                      "Manejando interrupción pendiente: %d", i);
            
            /*
             * PASO 1: GUARDAR CONTEXTO
             * Antes de manejar la interrupción, se debe guardar el estado
             * actual de la CPU para poder restaurarlo después.
             * Esto incluye registros, flags, etc.
             */
            save_context();
            
            /*
             * PASO 2: CAMBIAR A MODO KERNEL
             * Las interrupciones se manejan en modo kernel (privilegiado)
             * para que el sistema operativo tenga control completo.
             */
            cpu_registers.PSW.operation_mode = 1;  // KERNEL_MODE
            
            /*
             * PASO 3: EJECUTAR HANDLER
             * Llama a la función correspondiente en el vector de interrupciones.
             * interrupt_vector[i]() ejecuta el handler para la interrupción i.
             */
            interrupt_vector[i]();
            
            /*
             * PASO 4: LIMPIAR INTERRUPCIÓN PENDIENTE
             * Una vez manejada, se marca como no pendiente.
             */
            pending_interrupts[i] = 0;
            
            /*
             * PASO 5: RESTAURAR CONTEXTO
             * Restaura el estado de la CPU a como estaba antes de la interrupción.
             * Esto permite que el programa interrumpido continúe normalmente.
             */
            restore_context();
        }
    }
}

/*
 * Función: save_context
 * Propósito: Guardar el contexto actual de la CPU.
 * 
 * En un sistema real, esto implicaría:
 * 1. Guardar todos los registros en la pila del kernel
 * 2. Guardar la PSW (Palabra de Estado)
 * 3. Guardar información del proceso actual
 * 
 * En esta implementación simplificada, solo se registra el evento.
 */
void save_context() {
    // En una implementación real:
    // 1. push_registers_to_stack();
    // 2. save_psw_to_kernel_stack();
    // 3. update_process_table();
    
    log_event(LOG_DEBUG, 
              "Contexto guardado (simulado)");
}

/*
 * Función: restore_context
 * Propósito: Restaurar el contexto anterior de la CPU.
 * 
 * En un sistema real, esto implicaría:
 * 1. Restaurar registros desde la pila del kernel
 * 2. Restaurar la PSW
 * 3. Preparar la CPU para continuar ejecución
 * 
 * En esta implementación simplificada, solo se registra el evento.
 */
void restore_context() {
    // En una implementación real:
    // 1. pop_registers_from_stack();
    // 2. restore_psw_from_kernel_stack();
    // 3. resume_execution();
    
    log_event(LOG_DEBUG, 
              "Contexto restaurado (simulado)");
}