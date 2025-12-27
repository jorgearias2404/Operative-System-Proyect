#include "cpu.h"
#include "../MEMORY/memory.h"
#include "../REGISTERS/registers.h"
#include "../INTERRUPTS/interrupts.h"
#include "../DMA/dma.h"
#include "../LOGGER/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#define CPU_SLEEP(ms) Sleep(ms)
#else
#define CPU_SLEEP(ms) usleep(ms * 1000)
#endif

CPU_State cpu_state = CPU_HALTED;

void init_cpu() {
    reset_cpu();
    cpu_state = CPU_RUNNING;
    log_event(LOG_INFO, "CPU inicializada");
}

void reset_cpu() {
    init_registers();
    cpu_state = CPU_RUNNING;
}

Instruction fetch_instruction() {
    Instruction instr;
    
    // Fase FETCH
    cpu_registers.MAR = int_to_word(cpu_registers.PSW.PC_psw);
    int mar_value = word_to_int(cpu_registers.MAR);
    cpu_registers.MDR = read_memory(mar_value);
    cpu_registers.IR = cpu_registers.MDR;
    cpu_registers.PSW.PC_psw++;
    set_PC_int(cpu_registers.PSW.PC_psw);
    
    log_event(LOG_DEBUG, "FETCH: PC=%d, Instrucción=%s", 
              mar_value, cpu_registers.IR.data);
    
    instr = decode_instruction(cpu_registers.IR);
    return instr;
}

Instruction decode_instruction(Word instruction_word) {
    Instruction instr;
    char inst_str[9];
    strcpy(inst_str, instruction_word.data);
    
    if (strlen(inst_str) != 8) {
        instr.opcode = -1;
        return instr;
    }
    
    char opcode_str[3];
    strncpy(opcode_str, inst_str, 2);
    opcode_str[2] = '\0';
    instr.opcode = atoi(opcode_str);
    
    instr.mode = inst_str[2] - '0';
    
    char value_str[6];
    strncpy(value_str, inst_str + 3, 5);
    value_str[5] = '\0';
    instr.value = atoi(value_str);
    
    instr.effective_address = calculate_effective_address(instr.mode, instr.value);
    
    return instr;
}

int calculate_effective_address(AddressingMode mode, int value) {
    switch(mode) {
        case ADDR_DIRECT:
            return value;
        case ADDR_IMMEDIATE:
            return value;
        case ADDR_INDEXED:
            return word_to_int(cpu_registers.AC) + value;
        default:
            return -1;
    }
}

void cpu_cycle() {
    if (cpu_state != CPU_RUNNING) return;
    
    Instruction instr = fetch_instruction();
    execute_instruction(instr);
    handle_pending_interrupts();
}

void cpu_cycle_step() {
    if (cpu_state != CPU_RUNNING) {
        printf("CPU detenida. Use 'run' para iniciar ejecución.\n");
        return;
    }
    
    Instruction instr = fetch_instruction();
    printf("-> Ejecutando: %s (opcode: %02d)\n", cpu_registers.IR.data, instr.opcode);
    execute_instruction(instr);
    handle_pending_interrupts();
}

void execute_instruction(Instruction instr) {
    if (instr.opcode == -1) {
        trigger_interrupt(INT_INVALID_INSTRUCTION);
        return;
    }
    
    log_event(LOG_DEBUG, "EXECUTE: Opcode %d, modo=%d, valor=%d, EA=%d", 
              instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    switch(instr.opcode) {
        // ========== ARITMÉTICAS (00-03) ==========
        case 0:  // sum
        case 1:  // res
        case 2:  // mult
        case 3:  // divi
            {
                int ac_value = word_to_int(cpu_registers.AC);
                int operand = (instr.mode == ADDR_IMMEDIATE) ? instr.value : 
                              word_to_int(read_memory(instr.effective_address));
                int result = 0;
                
                switch(instr.opcode) {
                    case 0: result = ac_value + operand; break;
                    case 1: result = ac_value - operand; break;
                    case 2: result = ac_value * operand; break;
                    case 3: result = (operand != 0) ? ac_value / operand : 0; break;
                }
                
                cpu_registers.AC = int_to_word(result);
                update_condition_code(result);
                
                // Verificar overflow
                if ((instr.opcode == 0 && result < ac_value && operand > 0) ||
                    (instr.opcode == 1 && result > ac_value && operand < 0) ||
                    (instr.opcode == 2 && ac_value != 0 && result / ac_value != operand)) {
                    cpu_registers.PSW.condition_code = 3;  // Overflow
                    trigger_interrupt(INT_OVERFLOW);  // Puedes añadir 
                }
            }
            break;
            
        // ========== MEMORIA (04-05) ==========
        case 4:  // load
            if (instr.mode == ADDR_IMMEDIATE) {
                cpu_registers.AC = int_to_word(instr.value);
            } else {
                cpu_registers.AC = read_memory(instr.effective_address);
            }
            break;
            
        case 5:  // str
            write_memory(instr.effective_address, cpu_registers.AC);
            break;
            
        // ========== COMPARACIÓN (06-08) ==========
        case 6:  // cmp (comparar)
        case 7:  // tst (test)
        case 8:  // mov (mover)
            {
                int ac_value = word_to_int(cpu_registers.AC);
                int operand = (instr.mode == ADDR_IMMEDIATE) ? instr.value : 
                              word_to_int(read_memory(instr.effective_address));
                
                if (instr.opcode == 6) {  // CMP
                    update_condition_code(ac_value - operand);
                } 
                else if (instr.opcode == 7) {  // TST
                    update_condition_code(ac_value & operand);
                }
                else if (instr.opcode == 8) {  // MOV
                    cpu_registers.AC = int_to_word(operand);
                }
            }
            break;
            
        // ========== SALTOS CONDICIONALES (09-12) ==========
        case 9:  // jeq (jump if equal)
        case 10: // jgt (jump if greater)
        case 11: // jlt (jump if less)
        case 12: // jov (jump if overflow)
            {
                int should_jump = 0;
                int condition = cpu_registers.PSW.condition_code;
                
                switch(instr.opcode) {
                    case 9:  should_jump = (condition == 0); break;  // EQ
                    case 10: should_jump = (condition == 2); break;  // GT
                    case 11: should_jump = (condition == 1); break;  // LT
                    case 12: should_jump = (condition == 3); break;  // OV
                }
                
                if (should_jump) {
                    cpu_registers.PSW.PC_psw = instr.effective_address;
                    set_PC_int(instr.effective_address);
                }
            }
            break;
            
        // ========== LLAMADAS (13-14) ==========
        case 13: // svc (service call)
            trigger_interrupt(INT_SYSCALL);
            break;
            
        case 14: // call
            // Guardar dirección de retorno en pila
            {
                int sp_value = word_to_int(cpu_registers.SP);
                Word return_addr = int_to_word(cpu_registers.PSW.PC_psw);
                write_memory(sp_value, return_addr);
                cpu_registers.SP = int_to_word(sp_value - 1);
                
                // Saltar a la subrutina
                cpu_registers.PSW.PC_psw = instr.effective_address;
                set_PC_int(instr.effective_address);
            }
            break;
            
        // ========== RETORNO (15) ==========
        case 15: // ret
            {
                int sp_value = word_to_int(cpu_registers.SP) + 1;
                cpu_registers.SP = int_to_word(sp_value);
                
                Word return_addr = read_memory(sp_value);
                int return_value = word_to_int(return_addr);
                
                cpu_registers.PSW.PC_psw = return_value;
                set_PC_int(return_value);
            }
            break;
            
        // ========== REGISTROS (16-24) ==========
        case 16: // ldr (load register)
            cpu_registers.AC = cpu_registers.RB;
            break;
        case 17: // strr (store register)
            cpu_registers.RB = cpu_registers.AC;
            break;
        case 18: // ldrl
            cpu_registers.AC = cpu_registers.RL;
            break;
        case 19: // strl
            cpu_registers.RL = cpu_registers.AC;
            break;
            
        // ========== PILA (25-26) ==========
        case 25: // push
            {
                int sp_value = word_to_int(cpu_registers.SP);
                write_memory(sp_value, cpu_registers.AC);
                cpu_registers.SP = int_to_word(sp_value - 1);
            }
            break;
            
        case 26: // pop
            {
                int sp_value = word_to_int(cpu_registers.SP) + 1;
                cpu_registers.SP = int_to_word(sp_value);
                cpu_registers.AC = read_memory(sp_value);
            }
            break;
            
        // ========== SALTOS (27) ==========
        case 27: // j (salto incondicional)
            cpu_registers.PSW.PC_psw = instr.effective_address;
            set_PC_int(instr.effective_address);
            break;
            
        // ========== DMA (28-33) ==========
        case 28: // dma_read
            dma_set_memory_address(instr.value);
            dma_set_io_operation(0);  // Lectura
            dma_start_transfer();
            break;
            
        case 29: // dma_write
            dma_set_memory_address(instr.value);
            dma_set_io_operation(1);  // Escritura
            dma_start_transfer();
            break;
            
        case 30: // dma_wait
            dma_wait_completion();
            break;
            
        case 31: // dma_status
            cpu_registers.AC = int_to_word(dma_get_status());
            break;
            
        case 32: // dma_config
            dma_set_disk_location(instr.value / 10000, 
                                 (instr.value % 10000) / 100, 
                                 instr.value % 100);
            break;
            
        case 33: // dma_size
            dma_set_transfer_size(instr.value);
            break;
            
        // ========== I/O (34-39) ==========
        case 34: // in
        case 35: // out
        case 36: // io_status
            log_event(LOG_INFO, "Operación I/O %d solicitada", instr.opcode);
            trigger_interrupt(INT_IO_COMPLETION);
            break;
            
        // ========== SISTEMA (40-45) ==========
        case 40: // halt
            cpu_state = CPU_HALTED;
            log_event(LOG_INFO, "CPU detenida por instrucción HALT");
            printf("CPU HALTED\n");
            break;
            
        case 41: // nop
            // No operation
            break;
            
        case 42: // ei (enable interrupts)
            cpu_registers.PSW.interrupt_enabled = 1;
            break;
            
        case 43: // di (disable interrupts)
            cpu_registers.PSW.interrupt_enabled = 0;
            break;
            
        case 44: // switch_user
            cpu_registers.PSW.operation_mode = USER_MODE;
            break;
            
        case 45: // switch_kernel
            cpu_registers.PSW.operation_mode = KERNEL_MODE;
            break;
            
        default:
            log_event(LOG_WARNING, "Instrucción no implementada: %d", instr.opcode);
            trigger_interrupt(INT_INVALID_INSTRUCTION);
            break;
    }
}

void set_cpu_state(CPU_State state) {
    cpu_state = state;
}

CPU_State get_cpu_state() {
    return cpu_state;
}

void execute_program(int start_address) {
    cpu_registers.PSW.PC_psw = start_address;
    set_PC_int(start_address);
    cpu_state = CPU_RUNNING;
    
    printf("Iniciando ejecución en dirección %d...\n", start_address);
    
    while (cpu_state == CPU_RUNNING) {
        cpu_cycle();
        CPU_SLEEP(10);
    }
    
    printf("Ejecución finalizada.\n");
}

// Función para debug paso a paso mejorada
void debug_step() {
    printf("\n=== DEBUGGER PASO A PASO ===\n");
    
    // Mostrar estado actual antes de ejecutar
    printf("PC actual: %d\n", cpu_registers.PSW.PC_psw);
    
    // Mostrar instrucción que se va a ejecutar
    Word current_instruction = read_memory(cpu_registers.PSW.PC_psw);
    printf("Instrucción: %s\n", current_instruction.data);
    
    // Decodificar y mostrar detalles
    Instruction instr = decode_instruction(current_instruction);
    printf("Opcode: %02d, Modo: %d, Valor: %d, EA: %d\n", 
           instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    // Mostrar AC antes
    printf("AC antes: %s (int: %d)\n", 
           cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    
    // Ejecutar un ciclo
    cpu_cycle_step();
    
    // Mostrar resultado
    printf("AC después: %s (int: %d)\n", 
           cpu_registers.AC.data, word_to_int(cpu_registers.AC));
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

// Implementaciones simples de funciones faltantes (compatibilidad)
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