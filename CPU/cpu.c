/*
 * Archivo de implementación del módulo de CPU del Sistema Operativo Virtual.
 * Contiene la lógica completa para el ciclo de instrucción FETCH-DECODE-EXECUTE
 * y la implementación de todas las instrucciones del conjunto de instrucciones.
 */

/* Inclusión de cabecera propia del módulo */
#include "cpu.h"

/* Inclusión de cabeceras de otros módulos del sistema */
#include "../MEMORY/memory.h"     // Para acceder a la memoria
#include "../REGISTERS/registers.h" // Para manejar registros del CPU
#include "../INTERRUPTS/interrupts.h" // Para manejo de interrupciones
#include "../DMA/dma.h"           // Para operaciones de DMA
#include "../LOGGER/logger.h"     // Para registro de eventos del sistema

/* Inclusión de bibliotecas estándar */
#include <stdio.h>    // Entrada/salida estándar
#include <stdlib.h>   // Funciones generales (atoi)
#include <string.h>   // Manipulación de cadenas
#include <unistd.h>   // Para usleep() en sistemas Unix

/*
 * MACROS PARA COMPATIBILIDAD MULTIPLATAFORMA
 * Define macros para manejar diferencias entre Windows y sistemas Unix-like
 * para la función de pausa/sleep.
 */
#ifdef _WIN32
    #include <windows.h>          // API de Windows para Sleep()
    #define CPU_SLEEP(ms) Sleep(ms)  // Dormir en milisegundos (Windows)
#else
    #define CPU_SLEEP(ms) usleep(ms * 1000)  // Dormir en microsegundos (Unix)
#endif

/*
 * VARIABLE GLOBAL DEL ESTADO DE LA CPU
 * Esta variable es accesible desde otros módulos mediante extern declaration
 * Indica el estado actual de ejecución de la CPU.
 */
CPU_State cpu_state = CPU_HALTED;  // Inicialmente la CPU está detenida

/*
 * Función: init_cpu
 * Propósito: Inicializar la CPU, estableciéndola en estado de ejecución
 * y registrando el evento en el logger.
 */
void init_cpu() {
    reset_cpu();  // Reiniciar CPU a estado inicial
    cpu_state = CPU_RUNNING;  // Establecer estado a "ejecutando"
    log_event(LOG_INFO, "CPU inicializada");  // Registrar evento
}

/*
 * Función: reset_cpu
 * Propósito: Reiniciar la CPU a su estado inicial.
 * Llama a init_registers() para reiniciar todos los registros a valores por defecto.
 */
void reset_cpu() {
    init_registers();  // Reiniciar todos los registros del CPU
    cpu_state = CPU_RUNNING;  // Establecer estado a "ejecutando"
}

/*
 * Función: fetch_instruction
 * Propósito: Obtener la siguiente instrucción de la memoria (fase FETCH del ciclo).
 * Esta función implementa la primera fase del ciclo de instrucción:
 * 1. Leer la dirección del Program Counter (PC)
 * 2. Obtener la instrucción de memoria en esa dirección
 * 3. Incrementar el PC para apuntar a la siguiente instrucción
 * 4. Decodificar la instrucción
 * 
 * Retorna: Instruction - instrucción decodificada lista para ejecución
 */
Instruction fetch_instruction() {
    Instruction instr;  // Estructura para almacenar instrucción decodificada
    
    // FASE FETCH: Obtener instrucción de memoria
    // 1. Copiar PC a MAR (Memory Address Register)
    cpu_registers.MAR = int_to_word(cpu_registers.PSW.PC_psw);
    
    // 2. Leer memoria en la dirección especificada por MAR
    int mar_value = word_to_int(cpu_registers.MAR);
    cpu_registers.MDR = read_memory(mar_value);  // MDR = Memory Data Register
    
    // 3. Copiar instrucción a IR (Instruction Register)
    cpu_registers.IR = cpu_registers.MDR;
    
    // 4. Incrementar PC para siguiente instrucción
    cpu_registers.PSW.PC_psw++;
    set_PC_int(cpu_registers.PSW.PC_psw);  // Actualizar también PC como Word
    
    // Registrar evento para depuración
    log_event(LOG_DEBUG, "FETCH: PC=%d, Instrucción=%s", 
              mar_value, cpu_registers.IR.data);
    
    // Decodificar la instrucción y retornarla
    instr = decode_instruction(cpu_registers.IR);
    return instr;
}

/*
 * Función: decode_instruction
 * Parámetros: instruction_word - palabra de 8 dígitos que representa la instrucción
 * Retorna: Instruction - estructura con la instrucción decodificada
 * Propósito: Decodificar una palabra de instrucción en sus componentes.
 * Formato de instrucción: "OOMVVVVVV" donde:
 *   OO = opcode (2 dígitos)
 *   M  = modo de direccionamiento (1 dígito)
 *   VVVVV = valor/desplazamiento (5 dígitos)
 */
Instruction decode_instruction(Word instruction_word) {
    Instruction instr;  // Estructura a retornar
    char inst_str[9];   // Buffer para cadena de instrucción
    strcpy(inst_str, instruction_word.data);  // Copiar a buffer
    
    // Verificar longitud válida (debe ser 8 dígitos + null terminator)
    if (strlen(inst_str) != 8) {
        instr.opcode = -1;  // Marcar como inválida
        return instr;
    }
    
    // EXTRAER OPCODE (primeros 2 dígitos)
    // Ejemplo: "04005000" -> opcode_str = "04"
    char opcode_str[3];
    strncpy(opcode_str, inst_str, 2);
    opcode_str[2] = '\0';  // Asegurar terminación nula
    instr.opcode = atoi(opcode_str);  // Convertir a entero
    
    // EXTRAER MODO (tercer dígito)
    // Ejemplo: "04005000" -> inst_str[2] = '0'
    instr.mode = inst_str[2] - '0';  // Convertir char a int
    
    // EXTRAER VALOR (últimos 5 dígitos)
    // Ejemplo: "04005000" -> value_str = "05000"
    char value_str[6];
    strncpy(value_str, inst_str + 3, 5);
    value_str[5] = '\0';  // Asegurar terminación nula
    instr.value = atoi(value_str);  // Convertir a entero
    
    // CALCULAR DIRECCIÓN EFECTIVA según modo de direccionamiento
    instr.effective_address = calculate_effective_address(instr.mode, instr.value);
    
    return instr;  // Retornar instrucción completamente decodificada
}

/*
 * Función: calculate_effective_address
 * Parámetros:
 *   mode - modo de direccionamiento (directo, inmediato, indexado)
 *   value - valor/desplazamiento de la instrucción
 * Retorna: int - dirección efectiva calculada
 * Propósito: Calcular la dirección real en memoria según el modo de direccionamiento.
 */
int calculate_effective_address(AddressingMode mode, int value) {
    switch(mode) {
        case ADDR_DIRECT:
            // Direccionamiento directo: el valor ES la dirección
            // Ejemplo: LOAD 500 -> carga contenido de dirección 500
            return value;
            
        case ADDR_IMMEDIATE:
            // Direccionamiento inmediato: el valor ES el dato
            // Ejemplo: LOAD #5 -> carga el valor 5 (no necesita dirección)
            return value;  // En realidad, para inmediatos no se usa como dirección
            
        case ADDR_INDEXED:
            // Direccionamiento indexado: AC + valor
            // Ejemplo: LOAD 100(AC) -> carga contenido de dirección (AC + 100)
            return word_to_int(cpu_registers.AC) + value;
            
        default:
            return -1;  // Modo no válido
    }
}

/*
 * Función: cpu_cycle
 * Propósito: Ejecutar un ciclo completo de CPU (FETCH-DECODE-EXECUTE).
 * Esta es la función principal que se llama repetidamente para ejecutar programas.
 * Implementa el ciclo básico de instrucción con manejo de interrupciones.
 */
void cpu_cycle() {
    // Verificar que la CPU esté en estado RUNNING
    if (cpu_state != CPU_RUNNING) return;
    
    // CICLO DE INSTRUCCIÓN:
    // 1. FETCH: Obtener siguiente instrucción
    Instruction instr = fetch_instruction();
    
    // 2. EXECUTE: Ejecutar la instrucción
    execute_instruction(instr);
    
    // 3. CHECK INTERRUPTS: Verificar interrupciones pendientes
    handle_pending_interrupts();
}

/*
 * Función: cpu_cycle_step
 * Propósito: Versión de depuración de cpu_cycle que muestra información detallada.
 * Usada en modo depurador para ejecutar instrucciones paso a paso.
 */
void cpu_cycle_step() {
    // Verificar estado de CPU
    if (cpu_state != CPU_RUNNING) {
        printf("CPU detenida. Use 'run' para iniciar ejecución.\n");
        return;
    }
    
    // Obtener y mostrar información de la instrucción
    Instruction instr = fetch_instruction();
    printf("-> Ejecutando: %s (opcode: %02d)\n", cpu_registers.IR.data, instr.opcode);
    
    // Ejecutar instrucción
    execute_instruction(instr);
    
    // Verificar interrupciones
    handle_pending_interrupts();
}

/*
 * Función: execute_instruction
 * Parámetros: instr - instrucción decodificada a ejecutar
 * Propósito: Ejecutar la instrucción especificada.
 * Esta función contiene un switch gigante que implementa todas las instrucciones
 * del conjunto de instrucciones de la CPU virtual.
 * Las instrucciones están organizadas por categorías.
 */
void execute_instruction(Instruction instr) {
    // Verificar si la instrucción es inválida
    if (instr.opcode == -1) {
        trigger_interrupt(INT_INVALID_INSTRUCTION);  // Disparar interrupción
        return;
    }
    
    // Registrar evento de depuración
    log_event(LOG_DEBUG, "EXECUTE: Opcode %d, modo=%d, valor=%d, EA=%d", 
              instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    // SWITCH PRINCIPAL DE INSTRUCCIONES
    // Organizado por categorías de operaciones
    switch(instr.opcode) {
        // ========== CATEGORÍA: ARITMÉTICAS (opcodes 00-03) ==========
        case 0:  // sum (suma)
        case 1:  // res (resta)
        case 2:  // mult (multiplicación)
        case 3:  // divi (división)
            {
                // Obtener valor actual del acumulador (AC)
                int ac_value = word_to_int(cpu_registers.AC);
                
                // Obtener operando según modo de direccionamiento
                int operand;
                if (instr.mode == ADDR_IMMEDIATE) {
                    // Modo inmediato: el valor está en la instrucción
                    operand = instr.value;
                } else {
                    // Modo directo o indexado: leer de memoria
                    operand = word_to_int(read_memory(instr.effective_address));
                }
                
                int result = 0;
                
                // Realizar operación aritmética según opcode
                switch(instr.opcode) {
                    case 0: result = ac_value + operand; break;  // SUMA
                    case 1: result = ac_value - operand; break;  // RESTA
                    case 2: result = ac_value * operand; break;  // MULTIPLICACIÓN
                    case 3: result = (operand != 0) ? ac_value / operand : 0; break;  // DIVISIÓN
                }
                
                // Almacenar resultado en AC
                cpu_registers.AC = int_to_word(result);
                
                // Actualizar condition code según resultado
                update_condition_code(result);
                
                // Verificar posibles overflows
                if ((instr.opcode == 0 && result < ac_value && operand > 0) ||  // Overflow en suma
                    (instr.opcode == 1 && result > ac_value && operand < 0) ||  // Overflow en resta
                    (instr.opcode == 2 && ac_value != 0 && result / ac_value != operand)) {  // Overflow en multiplicación
                    cpu_registers.PSW.condition_code = 3;  // Establecer condition code a overflow
                    trigger_interrupt(INT_OVERFLOW);  // Disparar interrupción de overflow
                }
            }
            break;
            
        // ========== CATEGORÍA: MEMORIA (opcodes 04-05) ==========
        case 4:  // load (cargar de memoria a AC)
            if (instr.mode == ADDR_IMMEDIATE) {
                // Carga inmediata: cargar valor directamente en AC
                cpu_registers.AC = int_to_word(instr.value);
            } else {
                // Carga desde memoria: leer de dirección efectiva
                cpu_registers.AC = read_memory(instr.effective_address);
            }
            break;
            
        case 5:  // str (store - guardar AC en memoria)
            // Escribir contenido de AC en dirección efectiva
            write_memory(instr.effective_address, cpu_registers.AC);
            break;
            
        // ========== CATEGORÍA: COMPARACIÓN (opcodes 06-08) ==========
        case 6:  // cmp (comparar)
        case 7:  // tst (test - operación AND bit a bit)
        case 8:  // mov (mover valor a AC)
            {
                int ac_value = word_to_int(cpu_registers.AC);
                int operand;
                
                if (instr.mode == ADDR_IMMEDIATE) {
                    operand = instr.value;
                } else {
                    operand = word_to_int(read_memory(instr.effective_address));
                }
                
                if (instr.opcode == 6) {  // CMP (comparar)
                    // Actualizar condition code según comparación AC - operando
                    update_condition_code(ac_value - operand);
                } 
                else if (instr.opcode == 7) {  // TST (test - operación AND)
                    // Realizar AND bit a bit y actualizar condition code
                    update_condition_code(ac_value & operand);
                }
                else if (instr.opcode == 8) {  // MOV (mover)
                    // Copiar operando a AC
                    cpu_registers.AC = int_to_word(operand);
                }
            }
            break;
            
        // ========== CATEGORÍA: SALTOS CONDICIONALES (opcodes 09-12) ==========
        case 9:  // jeq (jump if equal - saltar si igual)
        case 10: // jgt (jump if greater - saltar si mayor)
        case 11: // jlt (jump if less - saltar si menor)
        case 12: // jov (jump if overflow - saltar si overflow)
            {
                int should_jump = 0;  // Flag para determinar si saltar
                int condition = cpu_registers.PSW.condition_code;  // Condition code actual
                
                // Evaluar condición según opcode
                switch(instr.opcode) {
                    case 9:  should_jump = (condition == 0); break;  // Saltar si ZERO/EQ
                    case 10: should_jump = (condition == 2); break;  // Saltar si GREATER
                    case 11: should_jump = (condition == 1); break;  // Saltar si LESS
                    case 12: should_jump = (condition == 3); break;  // Saltar si OVERFLOW
                }
                
                // Si condición se cumple, realizar salto
                if (should_jump) {
                    cpu_registers.PSW.PC_psw = instr.effective_address;  // Cambiar PC
                    set_PC_int(instr.effective_address);  // Actualizar también PC como Word
                }
            }
            break;
            
        // ========== CATEGORÍA: LLAMADAS (opcodes 13-14) ==========
        case 13: // svc (service call - llamada al sistema)
            // Disparar interrupción de llamada al sistema
            trigger_interrupt(INT_SYSCALL);
            break;
            
        case 14: // call (llamada a subrutina)
            {
                // Guardar dirección de retorno en la pila
                int sp_value = word_to_int(cpu_registers.SP);  // Stack pointer actual
                Word return_addr = int_to_word(cpu_registers.PSW.PC_psw);  // Dirección de retorno
                
                // Escribir dirección de retorno en la pila
                write_memory(sp_value, return_addr);
                
                // Decrementar stack pointer
                cpu_registers.SP = int_to_word(sp_value - 1);
                
                // Saltar a la dirección de la subrutina
                cpu_registers.PSW.PC_psw = instr.effective_address;
                set_PC_int(instr.effective_address);
            }
            break;
            
        // ========== CATEGORÍA: RETORNO (opcode 15) ==========
        case 15: // ret (retorno de subrutina)
            {
                // Incrementar stack pointer (pila crece hacia abajo)
                int sp_value = word_to_int(cpu_registers.SP) + 1;
                cpu_registers.SP = int_to_word(sp_value);
                
                // Leer dirección de retorno de la pila
                Word return_addr = read_memory(sp_value);
                int return_value = word_to_int(return_addr);
                
                // Establecer PC a dirección de retorno
                cpu_registers.PSW.PC_psw = return_value;
                set_PC_int(return_value);
            }
            break;
            
        // ========== CATEGORÍA: REGISTROS (opcodes 16-24) ==========
        case 16: // ldr (load register - cargar RB a AC)
            cpu_registers.AC = cpu_registers.RB;  // Copiar RB a AC
            break;
        case 17: // strr (store register - guardar AC en RB)
            cpu_registers.RB = cpu_registers.AC;  // Copiar AC a RB
            break;
        case 18: // ldrl (load RL a AC)
            cpu_registers.AC = cpu_registers.RL;  // Copiar RL a AC
            break;
        case 19: // strl (store AC en RL)
            cpu_registers.RL = cpu_registers.AC;  // Copiar AC a RL
            break;
            
        // ========== CATEGORÍA: PILA (opcodes 25-26) ==========
        case 25: // push (empujar AC a la pila)
            {
                int sp_value = word_to_int(cpu_registers.SP);
                // Escribir AC en la pila
                write_memory(sp_value, cpu_registers.AC);
                // Decrementar stack pointer
                cpu_registers.SP = int_to_word(sp_value - 1);
            }
            break;
            
        case 26: // pop (sacar de pila a AC)
            {
                // Incrementar stack pointer
                int sp_value = word_to_int(cpu_registers.SP) + 1;
                cpu_registers.SP = int_to_word(sp_value);
                // Leer valor de la pila a AC
                cpu_registers.AC = read_memory(sp_value);
            }
            break;
            
        // ========== CATEGORÍA: SALTOS (opcode 27) ==========
        case 27: // j (salto incondicional)
            // Cambiar PC a dirección efectiva
            cpu_registers.PSW.PC_psw = instr.effective_address;
            set_PC_int(instr.effective_address);
            break;
            
        // ========== CATEGORÍA: DMA (opcodes 28-33) ==========
        case 28: // dma_read (iniciar lectura DMA)
            dma_set_memory_address(instr.value);  // Establecer dirección de memoria
            dma_set_io_operation(0);  // 0 = operación de lectura
            dma_start_transfer();  // Iniciar transferencia
            break;
            
        case 29: // dma_write (iniciar escritura DMA)
            dma_set_memory_address(instr.value);
            dma_set_io_operation(1);  // 1 = operación de escritura
            dma_start_transfer();
            break;
            
        case 30: // dma_wait (esperar completar DMA)
            dma_wait_completion();
            break;
            
        case 31: // dma_status (obtener estado DMA)
            cpu_registers.AC = int_to_word(dma_get_status());  // Almacenar estado en AC
            break;
            
        case 32: // dma_config (configurar DMA)
            // El valor instrucción contiene: cilindro(2) + pista(2) + sector(2)
            dma_set_disk_location(instr.value / 10000,        // Cilindro
                                 (instr.value % 10000) / 100, // Pista
                                 instr.value % 100);          // Sector
            break;
            
        case 33: // dma_size (establecer tamaño transferencia)
            dma_set_transfer_size(instr.value);
            break;
            
        // ========== CATEGORÍA: I/O (opcodes 34-39) ==========
        case 34: // in (entrada desde dispositivo)
        case 35: // out (salida a dispositivo)
        case 36: // io_status (estado E/S)
            // Actualmente solo loguea la operación
            log_event(LOG_INFO, "Operación I/O %d solicitada", instr.opcode);
            trigger_interrupt(INT_IO_COMPLETION);  // Disparar interrupción de E/S
            break;
            
        // ========== CATEGORÍA: SISTEMA (opcodes 40-45) ==========
        case 40: // halt (detener CPU)
            cpu_state = CPU_HALTED;  // Cambiar estado a HALTED
            log_event(LOG_INFO, "CPU detenida por instrucción HALT");
            printf("CPU HALTED\n");  // Mensaje a consola
            break;
            
        case 41: // nop (no operation - no hace nada)
            // Instrucción vacía, solo consume un ciclo
            break;
            
        case 42: // ei (enable interrupts - habilitar interrupciones)
            cpu_registers.PSW.interrupt_enabled = 1;
            break;
            
        case 43: // di (disable interrupts - deshabilitar interrupciones)
            cpu_registers.PSW.interrupt_enabled = 0;
            break;
            
        case 44: // switch_user (cambiar a modo usuario)
            cpu_registers.PSW.operation_mode = USER_MODE;
            break;
            
        case 45: // switch_kernel (cambiar a modo kernel)
            cpu_registers.PSW.operation_mode = KERNEL_MODE;
            break;
            
        // ========== INSTRUCCIÓN NO IMPLEMENTADA ==========
        default:
            log_event(LOG_WARNING, "Instrucción no implementada: %d", instr.opcode);
            trigger_interrupt(INT_INVALID_INSTRUCTION);  // Disparar interrupción
            break;
    }
}

/*
 * Función: set_cpu_state
 * Parámetros: state - nuevo estado de la CPU
 * Propósito: Cambiar el estado global de la CPU.
 */
void set_cpu_state(CPU_State state) {
    cpu_state = state;
}

/*
 * Función: get_cpu_state
 * Retorna: CPU_State - estado actual de la CPU
 * Propósito: Obtener el estado actual de la CPU.
 */
CPU_State get_cpu_state() {
    return cpu_state;
}

/*
 * Función: execute_program
 * Parámetros: start_address - dirección de memoria donde comienza el programa
 * Propósito: Ejecutar un programa completo desde una dirección específica.
 * Esta función ejecuta ciclos de CPU continuamente hasta que la CPU se detenga.
 */
void execute_program(int start_address) {
    // Configurar PC para comenzar en la dirección especificada
    cpu_registers.PSW.PC_psw = start_address;
    set_PC_int(start_address);
    
    // Establecer CPU en estado RUNNING
    cpu_state = CPU_RUNNING;
    
    printf("Iniciando ejecución en dirección %d...\n", start_address);
    
    // Bucle principal de ejecución
    while (cpu_state == CPU_RUNNING) {
        cpu_cycle();  // Ejecutar un ciclo de CPU
        CPU_SLEEP(10);  // Pequeña pausa para controlar velocidad
    }
    
    printf("Ejecución finalizada.\n");
}

/*
 * Función: debug_step
 * Propósito: Versión mejorada para depuración paso a paso.
 * Muestra información detallada antes y después de ejecutar una instrucción.
 */
void debug_step() {
    printf("\n=== DEBUGGER PASO A PASO ===\n");
    
    // Mostrar estado actual antes de ejecutar
    printf("PC actual: %d\n", cpu_registers.PSW.PC_psw);
    
    // Mostrar instrucción que se va a ejecutar
    Word current_instruction = read_memory(cpu_registers.PSW.PC_psw);
    printf("Instrucción: %s\n", current_instruction.data);
    
    // Decodificar y mostrar detalles de la instrucción
    Instruction instr = decode_instruction(current_instruction);
    printf("Opcode: %02d, Modo: %d, Valor: %d, EA: %d\n", 
           instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    // Mostrar valor de AC antes de ejecutar
    printf("AC antes: %s (int: %d)\n", 
           cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    
    // Ejecutar un ciclo (con información de depuración)
    cpu_cycle_step();
    
    // Mostrar resultado después de ejecutar
    printf("AC después: %s (int: %d)\n", 
           cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    
    // Mostrar condition code con descripción textual
    printf("Condition Code: %d (", cpu_registers.PSW.condition_code);
    switch(cpu_registers.PSW.condition_code) {
        case 0: printf("ZERO/Equal"); break;
        case 1: printf("Less Than"); break;
        case 2: printf("Greater Than"); break;
        case 3: printf("Overflow"); break;
    }
    printf(")\n");
    printf("=============================\n");
}

/*
 * FUNCIONES DE COMPATIBILIDAD (WRAPPERS)
 * Estas funciones son wrappers alrededor de execute_instruction
 * para mantener compatibilidad con código que espera handlers específicos.
 */

void handle_arithmetic_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    execute_instruction((Instruction){opcode, mode, value, effective_address});
}

void handle_memory_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    execute_instruction((Instruction){opcode, mode, value, effective_address});
}

void handle_compare_operation(AddressingMode mode, int value, int effective_address) {
    execute_instruction((Instruction){6, mode, value, effective_address});  // CMP opcode=6
}

void handle_conditional_jump(int opcode, int address) {
    Instruction instr;
    instr.opcode = opcode;
    instr.mode = ADDR_DIRECT;
    instr.value = address;
    instr.effective_address = address;
    execute_instruction(instr);
}

void handle_system_operation(int opcode) {
    execute_instruction((Instruction){opcode, ADDR_DIRECT, 0, 0});
}

void handle_register_operation(int opcode) {
    execute_instruction((Instruction){opcode, ADDR_DIRECT, 0, 0});
}

void handle_jump_operation(int opcode, int address) {
    Instruction instr;
    instr.opcode = opcode;
    instr.mode = ADDR_DIRECT;
    instr.value = address;
    instr.effective_address = address;
    execute_instruction(instr);
}

void handle_io_operation(int opcode) {
    execute_instruction((Instruction){opcode, ADDR_DIRECT, 0, 0});
}