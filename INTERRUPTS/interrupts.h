/*
 * Archivo de cabecera del módulo de interrupciones del Sistema Operativo Virtual.
 * Define los tipos de datos y prototipos de funciones para el sistema de manejo
 * de interrupciones, que permite a dispositivos y programas notificar eventos
 * importantes a la CPU de manera asíncrona.
 */

#ifndef INTERRUPTS_H
#define INTERRUPTS_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

#include "../types.h"  // Para tipos y constantes globales

/*
 * Enum: InterruptCode
 * Propósito: Define todos los tipos de interrupciones soportados por el sistema.
 * Cada código corresponde a un evento específico que requiere atención inmediata.
 * 
 * Valores:
 *   INT_INVALID_SYSCALL      = 0 - Llamada al sistema inválida o no implementada
 *   INT_INVALID_INTERRUPT    = 1 - Código de interrupción inválido
 *   INT_SYSCALL              = 2 - Llamada al sistema válida
 *   INT_TIMER                = 3 - Interrupción del reloj del sistema
 *   INT_IO_COMPLETION        = 4 - Finalización de operación de Entrada/Salida
 *   INT_INVALID_INSTRUCTION  = 5 - Instrucción de CPU no válida
 *   INT_INVALID_ADDRESS      = 6 - Acceso a dirección de memoria inválida
 *   INT_UNDERFLOW            = 7 - Underflow aritmético (resultado muy pequeño)
 *   INT_OVERFLOW             = 8 - Overflow aritmético (resultado muy grande)
 */
typedef enum {
    INT_INVALID_SYSCALL = 0,     // Código de syscall inválido
    INT_INVALID_INTERRUPT = 1,   // Código de interrupción inválido
    INT_SYSCALL = 2,             // Llamada al sistema
    INT_TIMER = 3,               // Reloj del sistema
    INT_IO_COMPLETION = 4,       // Operación de E/S completada
    INT_INVALID_INSTRUCTION = 5, // Instrucción ilegal
    INT_INVALID_ADDRESS = 6,     // Dirección inválida
    INT_UNDERFLOW = 7,           // Underflow matemático
    INT_OVERFLOW = 8             // Overflow matemático
} InterruptCode;

/*
 * Tipo: InterruptHandler
 * Propósito: Define el tipo de función que maneja una interrupción.
 * Es un puntero a función que no recibe parámetros ni retorna valores.
 * Cada interrupción tiene su propio handler específico.
 */
typedef void (*InterruptHandler)(void);

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo de interrupciones
 */

/* FUNCIONES DE INICIALIZACIÓN Y MANEJO */
void init_interrupts();                   // Inicializar sistema de interrupciones
void trigger_interrupt(InterruptCode code); // Disparar una interrupción específica
void handle_pending_interrupts();         // Procesar todas las interrupciones pendientes

/* FUNCIONES DE MANEJO DE CONTEXTO */
void save_context();                      // Guardar estado actual de la CPU (contexto)
void restore_context();                   // Restaurar estado anterior de la CPU (contexto)

#endif /* INTERRUPTS_H */