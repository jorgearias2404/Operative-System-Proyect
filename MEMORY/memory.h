/*
 * Archivo de cabecera del módulo de memoria del Sistema Operativo Virtual.
 * Define las constantes, tipos y prototipos de funciones para gestionar
 * la memoria principal del sistema, incluyendo protección, asignación
 * y acceso controlado a direcciones de memoria.
 */

#ifndef MEMORY_H
#define MEMORY_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

#include <stdbool.h>    // Para tipo bool (verdadero/falso)
#include "../types.h"   // Para tipo Word y constantes globales

/*
 * CONSTANTES DE CONFIGURACIÓN DE MEMORIA
 * Definen el tamaño total de memoria y la porción reservada para el sistema operativo.
 * 
 * MEMORY_SIZE  - Tamaño total de memoria en palabras (Word)
 * OS_RESERVED  - Cantidad de palabras reservadas para el sistema operativo
 *                (0 a OS_RESERVED-1 son para el SO, OS_RESERVED a MEMORY_SIZE-1 para usuario)
 */
#define MEMORY_SIZE 2000   // 2000 palabras de 8 dígitos cada una
#define OS_RESERVED 300    // 300 palabras reservadas para el sistema operativo

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo de memoria
 */

/* FUNCIONES DE INICIALIZACIÓN Y CONFIGURACIÓN */
void init_memory();                   // Inicializar toda la memoria con valores por defecto
void set_memory_region(int base, int limit); // Configurar región de memoria para un proceso

/* FUNCIONES DE ACCESO A MEMORIA */
Word read_memory(int address);        // Leer una palabra de memoria (retorna Word)
void write_memory(int address, Word word); // Escribir una palabra en memoria

/* FUNCIONES DE VERIFICACIÓN Y VISUALIZACIÓN */
bool is_valid_address(int address, bool is_kernel_mode); // Validar dirección de memoria
void dump_memory(int start, int end); // Mostrar contenido de memoria en rango específico

/*
 * DECLARACIÓN DE VARIABLE GLOBAL EXTERNA
 * Permite que otros módulos accedan al array de memoria.
 */
extern Word memory[MEMORY_SIZE];  // Array global que representa toda la memoria principal

#endif /* MEMORY_H */