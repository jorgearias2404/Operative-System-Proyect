/*
 * Archivo de cabecera del módulo de CPU (Unidad Central de Procesamiento)
 * del Sistema Operativo Virtual.
 * Define las estructuras de datos, enumeraciones y prototipos de funciones
 * para la implementación de la CPU virtual.
 */

#ifndef CPU_H
#define CPU_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

#include "../types.h"
/* Incluye definiciones de tipos globales del proyecto, especialmente Word */

/*
 * Enum: AddressingMode
 * Propósito: Define los modos de direccionamiento soportados por la CPU
 * Los modos de direccionamiento determinan cómo se calculan las direcciones
 * de memoria para las operaciones.
 * 
 * Valores:
 *   ADDR_DIRECT    = 0 - Direccionamiento directo (la dirección está en la instrucción)
 *   ADDR_IMMEDIATE = 1 - Direccionamiento inmediato (el valor está en la instrucción)
 *   ADDR_INDEXED   = 2 - Direccionamiento indexado (dirección base + desplazamiento)
 */
typedef enum {
    ADDR_DIRECT = 0,    // Ejemplo: LOAD 500 (carga contenido de dirección 500)
    ADDR_IMMEDIATE = 1, // Ejemplo: LOAD #5 (carga el valor 5)
    ADDR_INDEXED = 2    // Ejemplo: LOAD 100(AC) (carga contenido de AC+100)
} AddressingMode;

/*
 * Estructura: Instruction
 * Propósito: Representa una instrucción decodificada con todos sus componentes
 * Una instrucción en este sistema tiene el formato: OP MM VVVVV
 * donde: OP = opcode (2 dígitos), MM = modo (1 dígito), VVVVV = valor (5 dígitos)
 * 
 * Campos:
 *   opcode            - Código de operación (0-45, definiendo el tipo de operación)
 *   mode              - Modo de direccionamiento (directo, inmediato, indexado)
 *   value             - Valor inmediato o desplazamiento (5 dígitos decimales)
 *   effective_address - Dirección efectiva calculada después de aplicar el modo
 */
typedef struct {
    int opcode;             // Ejemplo: 4 = LOAD, 40 = HALT
    AddressingMode mode;    // Cómo interpretar 'value'
    int value;             // Valor numérico de la instrucción
    int effective_address; // Dirección real en memoria después de cálculo
} Instruction;

/*
 * Enum: CPU_State
 * Propósito: Define los posibles estados de la CPU durante la ejecución
 * 
 * Valores:
 *   CPU_RUNNING      - CPU ejecutando instrucciones normalmente
 *   CPU_HALTED       - CPU detenida (por instrucción HALT o error)
 *   CPU_WAITING_IO   - CPU esperando operación de E/S (no usado actualmente)
 *   CPU_ERROR        - CPU en estado de error (no usado actualmente)
 */
typedef enum {
    CPU_RUNNING,       // Ejecución activa
    CPU_HALTED,        // Detenida
    CPU_WAITING_IO,    // Esperando E/S
    CPU_ERROR          // Estado de error
} CPU_State;

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo CPU
 */

/* FUNCIONES PRINCIPALES DE CICLO DE INSTRUCCIÓN */
void init_cpu();                           // Inicializar la CPU
void cpu_cycle();                          // Ejecutar un ciclo completo de CPU
void cpu_cycle_step();                     // Ejecutar un ciclo con información de depuración
void execute_instruction(Instruction instr); // Ejecutar una instrucción específica

/* FUNCIONES DEL CICLO FETCH-DECODE-EXECUTE */
Instruction fetch_instruction();                     // Obtener siguiente instrucción
Instruction decode_instruction(Word instruction_word); // Decodificar palabra de instrucción
int calculate_effective_address(AddressingMode mode, int value); // Calcular dirección efectiva

/* HANDLERS DE OPERACIONES (para modularidad) */
void handle_arithmetic_operation(int opcode, AddressingMode mode, int value, int effective_address);
void handle_memory_operation(int opcode, AddressingMode mode, int value, int effective_address);
void handle_compare_operation(AddressingMode mode, int value, int effective_address);
void handle_conditional_jump(int opcode, int address);
void handle_jump_operation(int opcode, int address);
void handle_io_operation(int opcode);
void handle_system_operation(int opcode);
void handle_register_operation(int opcode);

/* CONTROL DE ESTADO DE LA CPU */
void set_cpu_state(CPU_State state);       // Establecer estado de la CPU
CPU_State get_cpu_state();                 // Obtener estado actual de la CPU
void reset_cpu();                          // Reiniciar CPU a estado inicial
void execute_program(int start_address);   // Ejecutar programa desde dirección específica

/* VARIABLE GLOBAL EXTERNA */
extern CPU_State cpu_state;  // Declaración externa del estado global de la CPU

#endif /* CPU_H */