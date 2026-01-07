/*
 * Archivo de implementación del módulo de memoria del Sistema Operativo Virtual.
 * Contiene la lógica completa para gestionar la memoria principal, incluyendo:
 * - Inicialización de memoria
 * - Traducción de direcciones lógicas a físicas
 * - Protección de memoria (kernel vs usuario)
 * - Validación de límites de acceso
 * - Operaciones de lectura/escritura
 */

/* Inclusión de cabecera propia del módulo */
#include "memory.h"

/* Inclusión de cabeceras de otros módulos */
#include "logger.h"                  // Para registrar eventos de memoria
#include "../REGISTERS/registers.h"  // Para acceder a registros RB y RL
#include "../INTERRUPTS/interrupts.h" // Para disparar interrupciones por violaciones

/* Inclusión de bibliotecas estándar */
#include <string.h>   // Para funciones de manipulación de cadenas (strcpy)
#include <stdlib.h>   // Para funciones generales

/*
 * VARIABLE GLOBAL - Memoria Principal
 * Esta variable representa toda la memoria física del sistema.
 * Es un array de 2000 elementos de tipo Word (8 dígitos cada uno).
 */
Word memory[MEMORY_SIZE];  // Array que representa toda la memoria física

/*
 * Función: init_memory
 * Propósito: Inicializar toda la memoria del sistema.
 * Realiza las siguientes acciones:
 * 1. Llena toda la memoria con el valor "00000000"
 * 2. Marca el área reservada para el sistema operativo
 * 3. Registra el evento de inicialización
 */
void init_memory() {
    // Crear una palabra con valor cero para inicialización
    Word zero_word;
    strcpy(zero_word.data, "00000000");  // 8 ceros
    
    /*
     * INICIALIZAR TODA LA MEMORIA CON CEROS
     * Recorre todas las 2000 posiciones de memoria y las establece a "00000000"
     */
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = zero_word;  // Copiar la palabra cero a cada posición
    }
    
    /*
     * MARCAR ÁREA DEL SISTEMA OPERATIVO
     * Las primeras 300 palabras (0-299) están reservadas para uso exclusivo
     * del sistema operativo. Se marcan con un texto especial para identificación.
     */
    for (int i = 0; i < OS_RESERVED; i++) {
        // "OS_RESERVED" indica que esta posición está reservada para el SO
        // Nota: Esto sobrescribe los ceros puestos anteriormente
        strcpy(memory[i].data, "OS_RESERVED");
    }
    
    /*
     * REGISTRAR INICIALIZACIÓN
     * Se registra en el logger que la memoria ha sido inicializada,
     * incluyendo información sobre su tamaño y áreas reservadas.
     */
    log_event(LOG_INFO, 
              "Memoria inicializada: %d palabras totales, %d reservadas para SO", 
              MEMORY_SIZE, OS_RESERVED);
}

/*
 * Función auxiliar: logical_to_physical (ESTÁTICA)
 * Parámetros: logical_address - dirección lógica proporcionada por un programa
 * Retorna: int - dirección física correspondiente, o -1 si hay violación
 * Propósito: Convertir una dirección lógica en una dirección física,
 *            aplicando protección de memoria mediante registros base y límite.
 * 
 * Concepto clave: PROTECCIÓN POR REGISTROS BASE Y LÍMITE
 * - RB (Registro Base): Dirección física donde comienza el espacio del proceso
 * - RL (Registro Límite): Tamaño máximo del espacio del proceso
 * 
 * Fórmula: Dirección Física = Dirección Lógica + RB
 * Restricción: Dirección Física debe estar en [RB, RB+RL-1]
 */
static int logical_to_physical(int logical_address) {
    // Obtener valores actuales de RB y RL (convertidos de Word a int)
    int rb_value = word_to_int(cpu_registers.RB);  // Base del proceso
    int rl_value = word_to_int(cpu_registers.RL);  // Límite del proceso
    
    /*
     * MODO KERNEL ESPECIAL: RB = 0 y RL = 0
     * Cuando ambos registros son 0, se asume modo kernel con acceso directo.
     * Esto permite al sistema operativo acceder a cualquier dirección.
     */
    if (rb_value == 0 && rl_value == 0) {
        return logical_address;  // Sin traducción en modo kernel
    }
    
    /*
     * TRADUCCIÓN NORMAL: Aplicar fórmula Dirección Física = Lógica + RB
     */
    int physical_address = logical_address + rb_value;
    
    /*
     * VERIFICACIÓN DE LÍMITES
     * La dirección física resultante debe estar dentro del espacio asignado
     * al proceso: [RB, RB+RL-1]
     */
    if (physical_address < rb_value || physical_address >= (rb_value + rl_value)) {
        // VIOLACIÓN DE MEMORIA: Dirección fuera del espacio asignado
        log_event(LOG_ERROR, 
                  "Violación de memoria: dirección %d fuera de límites [RB=%d, RL=%d]", 
                  logical_address, rb_value, rl_value);
        
        // Disparar interrupción de dirección inválida
        trigger_interrupt(INT_INVALID_ADDRESS);
        
        return -1;  // Retornar error
    }
    
    // Dirección física válida, retornarla
    return physical_address;
}

/*
 * Función: read_memory
 * Parámetros: logical_address - dirección lógica a leer
 * Retorna: Word - contenido de la dirección de memoria
 * Propósito: Leer una palabra de memoria, aplicando todas las protecciones.
 * 
 * Flujo:
 * 1. Convertir dirección lógica a física (con protección RB/RL)
 * 2. Verificar que la dirección física esté dentro de MEMORY_SIZE
 * 3. Verificar permisos (usuario no puede leer área del SO)
 * 4. Retornar el valor
 */
Word read_memory(int logical_address) {
    /*
     * PASO 1: CONVERSIÓN LÓGICA A FÍSICA
     * Aplica protección por registros base y límite
     */
    int physical_address = logical_to_physical(logical_address);
    
    // Si hubo error en la conversión (violación de límites)
    if (physical_address < 0) {
        Word error_word;  // Crear palabra de error
        strcpy(error_word.data, "MEM_ERR");  // "MEM_ERR" indica error de memoria
        return error_word;  // Retornar palabra de error
    }
    
    /*
     * PASO 2: VERIFICAR LÍMITES FÍSICOS
     * Asegurar que la dirección física esté dentro de [0, MEMORY_SIZE-1]
     */
    if (physical_address < 0 || physical_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "Dirección física inválida: %d", physical_address);
        Word error_word;
        strcpy(error_word.data, "ADDR_ERR");  // "ADDR_ERR" indica error de dirección
        return error_word;
    }
    
    /*
     * PASO 3: VERIFICAR PERMISOS (PROTECCIÓN KERNEL/USUARIO)
     * Si la dirección está en el área del SO (0-299) y estamos en modo usuario,
     * se niega el acceso.
     */
    if (physical_address < OS_RESERVED && cpu_registers.PSW.operation_mode == USER_MODE) {
        log_event(LOG_ERROR, 
                  "Usuario intenta leer área del SO: %d", 
                  physical_address);
        trigger_interrupt(INT_INVALID_ADDRESS);  // Disparar interrupción
        Word error_word;
        strcpy(error_word.data, "PRIV_ERR");  // "PRIV_ERR" indica error de privilegio
        return error_word;
    }
    
    /*
     * PASO 4: LECTURA EXITOSA
     * Registrar la operación para depuración y retornar el valor
     */
    log_event(LOG_DEBUG, 
              "Lectura: lógica=%d -> física=%d = %s", 
              logical_address, physical_address, memory[physical_address].data);
    
    return memory[physical_address];  // Retornar el valor leído
}

/*
 * Función: write_memory
 * Parámetros: 
 *   logical_address - dirección lógica donde escribir
 *   word - palabra a escribir en memoria
 * Propósito: Escribir una palabra en memoria, aplicando todas las protecciones.
 * 
 * Flujo similar a read_memory, pero para escritura.
 */
void write_memory(int logical_address, Word word) {
    /*
     * PASO 1: CONVERSIÓN LÓGICA A FÍSICA
     */
    int physical_address = logical_to_physical(logical_address);
    
    // Si hubo error en la conversión, terminar sin escribir
    if (physical_address < 0) {
        return;
    }
    
    /*
     * PASO 2: VERIFICAR LÍMITES FÍSICOS
     */
    if (physical_address < 0 || physical_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, 
                  "Dirección física inválida para escritura: %d", 
                  physical_address);
        return;  // No escribir en dirección inválida
    }
    
    /*
     * PASO 3: VERIFICAR PERMISOS (PROTECCIÓN KERNEL/USUARIO)
     */
    if (physical_address < OS_RESERVED && cpu_registers.PSW.operation_mode == USER_MODE) {
        log_event(LOG_ERROR, 
                  "Usuario intenta escribir en área del SO: %d", 
                  physical_address);
        trigger_interrupt(INT_INVALID_ADDRESS);  // Disparar interrupción
        return;  // No permitir escritura en área protegida
    }
    
    /*
     * PASO 4: ESCRITURA EXITOSA
     */
    memory[physical_address] = word;  // Escribir la palabra en memoria
    
    // Registrar la operación para depuración
    log_event(LOG_DEBUG, 
              "Escritura: lógica=%d -> física=%d = %s", 
              logical_address, physical_address, word.data);
}

/*
 * Función: is_valid_address
 * Parámetros:
 *   address - dirección a validar
 *   is_kernel_mode - indica si estamos en modo kernel (true) o usuario (false)
 * Retorna: bool - true si la dirección es válida, false si no
 * Propósito: Verificar si una dirección es válida para acceso según el modo actual.
 * 
 * Esta función es una verificación simple sin aplicar traducción RB/RL.
 */
bool is_valid_address(int address, bool is_kernel_mode) {
    if (is_kernel_mode) {
        // En modo kernel: acceso a toda la memoria [0, MEMORY_SIZE-1]
        return (address >= 0 && address < MEMORY_SIZE);
    } else {
        // En modo usuario: solo acceso a área de usuario [OS_RESERVED, MEMORY_SIZE-1]
        return (address >= OS_RESERVED && address < MEMORY_SIZE);
    }
}

/*
 * Función: dump_memory
 * Parámetros:
 *   start - dirección inicial del rango a mostrar
 *   end - dirección final del rango a mostrar
 * Propósito: Mostrar el contenido de memoria en un rango específico.
 * Útil para depuración y verificación del estado de la memoria.
 */
void dump_memory(int start, int end) {
    // Ajustar límites para evitar accesos fuera de rango
    if (start < 0) start = 0;  // No menor que 0
    if (end >= MEMORY_SIZE) end = MEMORY_SIZE - 1;  // No mayor que tamaño-1
    
    // Encabezado del dump
    printf("\nDUMP de memoria [%d - %d]:\n", start, end);
    
    // Recorrer y mostrar cada posición en el rango
    for (int i = start; i <= end; i++) {
        // Formato: "0000: 00000000"
        printf("%04d: %s\n", i, memory[i].data);
    }
}

/*
 * Función: set_memory_region
 * Parámetros:
 *   base - dirección base para el proceso (va a RB)
 *   limit - tamaño del espacio para el proceso (va a RL)
 * Propósito: Configurar la región de memoria para un proceso.
 * Establece los registros RB (Base) y RL (Límite) que definen el espacio
 * de direcciones válido para el proceso actual.
 */
void set_memory_region(int base, int limit) {
    // Configurar registro base (RB) - dónde comienza el proceso en memoria física
    cpu_registers.RB = int_to_word(base);
    
    // Configurar registro límite (RL) - cuánto espacio tiene el proceso
    cpu_registers.RL = int_to_word(limit);
    
    // Registrar la configuración
    log_event(LOG_INFO, 
              "Región de memoria configurada: RB=%d, RL=%d", 
              base, limit);
}