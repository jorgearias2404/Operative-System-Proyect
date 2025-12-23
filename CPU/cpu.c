#include "cpu.h"
#include "../MEMORY/memory.h"
#include "../REGISTERS/registers.h"
#include "../INTERRUPTS/interrupts.h"
#include "../DMA/dma.h"
#include "../LOGGER/logger.h"  // Asegúrate de que esté
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    cpu_registers.MAR = int_to_word(cpu_registers.PSW.PC);
    int mar_value = word_to_int(cpu_registers.MAR);
    cpu_registers.MDR = read_memory(mar_value);
    cpu_registers.IR = cpu_registers.MDR;
    cpu_registers.PSW.PC++;
    
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
    cpu_cycle();
}

void execute_instruction(Instruction instr) {
    if (instr.opcode == -1) {
        trigger_interrupt(INT_INVALID_INSTRUCTION);
        return;
    }
    
    log_event(LOG_DEBUG, "EXECUTE: Opcode %d", instr.opcode);
    
    // Versión SIMPLIFICADA
    switch(instr.opcode) {
        case 0:  // sum
        case 1:  // res
        case 2:  // mult
        case 3:  // divi
            {
                int ac_value = word_to_int(cpu_registers.AC);
                int operand = instr.value;
                int result = 0;
                
                switch(instr.opcode) {
                    case 0: result = ac_value + operand; break;
                    case 1: result = ac_value - operand; break;
                    case 2: result = ac_value * operand; break;
                    case 3: result = (operand != 0) ? ac_value / operand : 0; break;
                }
                
                cpu_registers.AC = int_to_word(result);
                update_condition_code(result);
            }
            break;
            
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
            
        case 27: // j (salto incondicional)
            cpu_registers.PSW.PC = instr.effective_address;
            break;
            
        case 13: // svc
            trigger_interrupt(INT_SYSCALL);
            break;
            
        default:
            log_event(LOG_DEBUG, "Instrucción %d ejecutada (sin efecto)", instr.opcode);
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
    cpu_registers.PSW.PC = start_address;
    cpu_state = CPU_RUNNING;
    
    while (cpu_state == CPU_RUNNING) {
        cpu_cycle();
        usleep(10000);
    }
}

// Implementaciones simples de funciones faltantes
void handle_arithmetic_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    execute_instruction((Instruction){opcode, mode, value, effective_address});
}

void handle_memory_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    execute_instruction((Instruction){opcode, mode, value, effective_address});
}

void handle_compare_operation(AddressingMode mode, int value, int effective_address) {
    // Implementación simple
    log_event(LOG_DEBUG, "COMPARE ejecutada");
}

void handle_conditional_jump(int opcode, int address) {
    // Implementación simple
    if (opcode >= 9 && opcode <= 12) {
        cpu_registers.PSW.PC = address;
    }
}

void handle_system_operation(int opcode) {
    execute_instruction((Instruction){opcode, ADDR_DIRECT, 0, 0});
}

void handle_register_operation(int opcode) {
    log_event(LOG_DEBUG, "Register op: %d", opcode);
}

void handle_jump_operation(int opcode, int address) {
    execute_instruction((Instruction){opcode, ADDR_DIRECT, address, address});
}

void handle_io_operation(int opcode) {
    log_event(LOG_DEBUG, "I/O op: %d", opcode);
}