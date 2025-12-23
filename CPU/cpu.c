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
#include <math.h>

CPU_State cpu_state = CPU_HALTED;

void init_cpu() {
    reset_cpu();
    cpu_state = CPU_RUNNING;
    log_event(LOG_INFO, "CPU inicializada");
}

void reset_cpu() {
    // Reiniciar registros
    init_registers();
    cpu_state = CPU_RUNNING;
}

Instruction fetch_instruction() {
    Instruction instr;
    
    // Fase FETCH
    // 1. MAR = PC
    cpu_registers.MAR = int_to_word(cpu_registers.PSW.PC);
    
    // 2. MDR = MEM[MAR]
    int mar_value = word_to_int(cpu_registers.MAR);
    cpu_registers.MDR = read_memory(mar_value);  // read_memory devuelve Word
    
    // 3. IR = MDR
    cpu_registers.IR = cpu_registers.MDR;
    
    // 4. PC++
    cpu_registers.PSW.PC++;
    
    log_event(LOG_DEBUG, "FETCH: PC=%d, Instrucción=%s", 
              mar_value, cpu_registers.IR.data);
    
    // Decodificar la instrucción
    instr = decode_instruction(cpu_registers.IR);
    
    return instr;
}
Instruction decode_instruction(Word instruction_word) {
    Instruction instr;
    char inst_str[9];
    strcpy(inst_str, instruction_word.data);
    
    if (strlen(inst_str) != 8) {
        log_event(LOG_ERROR, "Instrucción inválida (no tiene 8 dígitos): %s", inst_str);
        instr.opcode = -1;
        return instr;
    }
    
    // Extraer código de operación (primeros 2 dígitos)
    char opcode_str[3];
    strncpy(opcode_str, inst_str, 2);
    opcode_str[2] = '\0';
    instr.opcode = atoi(opcode_str);
    
    // Extraer modo de direccionamiento (tercer dígito)
    char mode_str[2];
    mode_str[0] = inst_str[2];
    mode_str[1] = '\0';
    instr.mode = atoi(mode_str);
    
    // Extraer valor (últimos 5 dígitos)
    char value_str[6];
    strncpy(value_str, inst_str + 3, 5);
    value_str[5] = '\0';
    instr.value = atoi(value_str);
    
    // Calcular dirección efectiva
    instr.effective_address = calculate_effective_address(instr.mode, instr.value);
    
    log_event(LOG_DEBUG, "DECODE: Opcode=%d, Mode=%d, Value=%d, EA=%d",
              instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    return instr;
}

int calculate_effective_address(AddressingMode mode, int value) {
    int effective_addr = 0;
    
    switch(mode) {
        case ADDR_DIRECT:
            effective_addr = value;
            break;
            
        case ADDR_IMMEDIATE:
            effective_addr = value;  // En inmediato, el valor ES el operando
            break;
            
        case ADDR_INDEXED:
            // AC + valor
            effective_addr = word_to_int(cpu_registers.AC) + value;
            break;
            
        default:
            log_event(LOG_ERROR, "Modo de direccionamiento inválido: %d", mode);
            effective_addr = -1;
            break;
    }
    
    return effective_addr;
}

void cpu_cycle() {
    if (cpu_state != CPU_RUNNING) {
        return;
    }
    
    // 1. FETCH y DECODE
    Instruction instr = fetch_instruction();
    
    // 2. EXECUTE
    execute_instruction(instr);
    
    // 3. Verificar interrupciones
    handle_pending_interrupts();
}

void cpu_cycle_step() {
    // Versión para modo debug - ejecuta solo un ciclo
    cpu_cycle();
}

void execute_instruction(Instruction instr) {
    if (instr.opcode == -1) {
        log_event(LOG_ERROR, "Instrucción inválida decodificada");
        trigger_interrupt(INT_INVALID_INSTRUCTION);
        return;
    }
    
    log_event(LOG_DEBUG, "EXECUTE: Opcode %d, Mode=%d, Value=%d, EA=%d", 
              instr.opcode, instr.mode, instr.value, instr.effective_address);
    
    // Ejecutar según código de operación
    switch(instr.opcode) {
        // Operaciones aritméticas (0-3)
        case 0:  // sum
        case 1:  // res
        case 2:  // mult
        case 3:  // divi
            handle_arithmetic_operation(instr.opcode, instr.mode, instr.value, instr.effective_address);
            break;
            
        // Operaciones de memoria (4-7)
        case 4:  // load
        case 5:  // str
        case 6:  // loadrx
        case 7:  // strrx
            handle_memory_operation(instr.opcode, instr.mode, instr.value, instr.effective_address);
            break;
            
        // Comparación y saltos (8-12, 27)
        case 8:  // comp
            handle_compare_operation(instr.mode, instr.value, instr.effective_address);
            break;
        case 9:  // jmpe
        case 10: // jmpne
        case 11: // jmplt
        case 12: // jmplgt
            handle_conditional_jump(instr.opcode, instr.effective_address);
            break;
        case 27: // j (salto incondicional)
            cpu_registers.PSW.PC = instr.effective_address;
            break;
            
        // Operaciones del sistema (13-18)
        case 13: // svc
        case 14: // retrn
        case 15: // hab
        case 16: // dhab
        case 17: // titi
        case 18: // chmod
            handle_system_operation(instr.opcode);
            break;
            
        // Operaciones con registros (19-26)
        case 19: // loadrb
        case 20: // strrb
        case 21: // loadr1
        case 22: // strr1
        case 23: // loadsp
        case 24: // strsp
        case 25: // psh
        case 26: // pop
            handle_register_operation(instr.opcode);
            break;
            
        // Operaciones de E/S (28-33)
        case 28: // sdmap
        case 29: // sdmac
        case 30: // sdmas
        case 31: // sdmaio
        case 32: // sdmann
        case 33: // sdmaon
            handle_io_operation(instr.opcode);
            break;
            
        default:
            log_event(LOG_WARNING, "Instrucción no implementada: %d", instr.opcode);
            break;
    }
}

void handle_arithmetic_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    int ac_value = word_to_int(cpu_registers.AC);
    int operand = 0;
    
    // Obtener operando según modo de direccionamiento
    switch(mode) {
        case ADDR_IMMEDIATE:
            operand = value;
            break;
        case ADDR_DIRECT:
        case ADDR_INDEXED:
            if (effective_address >= 0 && effective_address < MEMORY_SIZE) {
                Word mem_word = read_memory(effective_address);
                operand = word_to_int(mem_word);
            } else {
                log_event(LOG_ERROR, "Dirección inválida para operación aritmética: %d", effective_address);
                trigger_interrupt(INT_INVALID_ADDRESS);
                return;
            }
            break;
    }
    
    int result = 0;
    
    switch(opcode) {
        case 0:  // sum
            result = ac_value + operand;
            log_event(LOG_DEBUG, "SUM: %d + %d = %d", ac_value, operand, result);
            break;
        case 1:  // res
            result = ac_value - operand;
            log_event(LOG_DEBUG, "RES: %d - %d = %d", ac_value, operand, result);
            break;
        case 2:  // mult
            result = ac_value * operand;
            log_event(LOG_DEBUG, "MULT: %d * %d = %d", ac_value, operand, result);
            break;
        case 3:  // divi
            if (operand != 0) {
                result = ac_value / operand;
                log_event(LOG_DEBUG, "DIVI: %d / %d = %d", ac_value, operand, result);
            } else {
                log_event(LOG_ERROR, "División por cero");
                trigger_interrupt(INT_INVALID_INSTRUCTION);
                return;
            }
            break;
    }
    
    // Verificar overflow/underflow
    if (result > 9999999) {
        trigger_interrupt(INT_OVERFLOW);
        result = 9999999;
    } else if (result < -9999999) {
        trigger_interrupt(INT_UNDERFLOW);
        result = -9999999;
    }
    
    cpu_registers.AC = int_to_word(result);
    update_condition_code(result);
    
    log_event(LOG_DEBUG, "AC actualizado a: %s (valor: %d)", 
              cpu_registers.AC.data, result);
}

void handle_memory_operation(int opcode, AddressingMode mode, int value, int effective_address) {
    switch(opcode) {
        case 4:  // load
            if (mode == ADDR_IMMEDIATE) {
                cpu_registers.AC = int_to_word(value);
            } else if (effective_address >= 0 && effective_address < MEMORY_SIZE) {
                cpu_registers.AC = read_memory(effective_address);
            }
            log_event(LOG_DEBUG, "LOAD: AC = %s", cpu_registers.AC.data);
            break;
            
        case 5:  // str
            if (effective_address >= 0 && effective_address < MEMORY_SIZE) {
                write_memory(effective_address, cpu_registers.AC);
                log_event(LOG_DEBUG, "STR: memoria[%d] = %s", 
                         effective_address, cpu_registers.AC.data);
            }
            break;
            
        case 6:  // loadrx
            cpu_registers.AC = cpu_registers.RX;
            log_event(LOG_DEBUG, "LOADRX: AC = RX = %s", cpu_registers.AC.data);
            break;
            
        case 7:  // strrx
            cpu_registers.RX = cpu_registers.AC;
            log_event(LOG_DEBUG, "STRRX: RX = AC = %s", cpu_registers.RX.data);
            break;
    }
}

void handle_compare_operation(AddressingMode mode, int value, int effective_address) {
    int ac_value = word_to_int(cpu_registers.AC);
    int operand = 0;
    
    switch(mode) {
        case ADDR_IMMEDIATE:
            operand = value;
            break;
        case ADDR_DIRECT:
        case ADDR_INDEXED:
            if (effective_address >= 0 && effective_address < MEMORY_SIZE) {
                Word mem_word = read_memory(effective_address);
                operand = word_to_int(mem_word);
            }
            break;
    }
    
    update_condition_code(ac_value - operand);
    log_event(LOG_DEBUG, "COMP: %d vs %d, condition_code=%d", 
              ac_value, operand, cpu_registers.PSW.condition_code);
}

void handle_conditional_jump(int opcode, int address) {
    int should_jump = 0;
    
    switch(opcode) {
        case 9:  // jmpe - salta si AC = M[SP]
            should_jump = (cpu_registers.PSW.condition_code == 0);
            break;
        case 10: // jmpne - salta si AC != M[SP]
            should_jump = (cpu_registers.PSW.condition_code != 0);
            break;
        case 11: // jmplt - salta si AC < M[SP]
            should_jump = (cpu_registers.PSW.condition_code == 1);
            break;
        case 12: // jmplgt - salta si AC > M[SP]
            should_jump = (cpu_registers.PSW.condition_code == 2);
            break;
    }
    
    if (should_jump) {
        cpu_registers.PSW.PC = address;
        log_event(LOG_DEBUG, "SALTO a dirección %d", address);
    } else {
        log_event(LOG_DEBUG, "Salto condicional no tomado");
    }
}

void handle_system_operation(int opcode) {
    switch(opcode) {
        case 13:  // svc
            trigger_interrupt(INT_SYSCALL);
            log_event(LOG_INFO, "Llamada al sistema (SVC)");
            break;
        case 14:  // retrn
            // Implementar retorno de subrutina
            log_event(LOG_DEBUG, "RETRN: Retorno de subrutina");
            break;
        case 15:  // hab
            cpu_registers.PSW.interrupt_enabled = INTERRUPTS_ENABLED;
            log_event(LOG_DEBUG, "HAB: Interrupciones habilitadas");
            break;
        case 16:  // dhab
            cpu_registers.PSW.interrupt_enabled = INTERRUPTS_DISABLED;
            log_event(LOG_DEBUG, "DHAB: Interrupciones deshabilitadas");
            break;
        case 17:  // titi
            // Establecer tiempo del reloj
            log_event(LOG_DEBUG, "TITI: Configurar reloj");
            break;
        case 18:  // chmod
            cpu_registers.PSW.operation_mode = !cpu_registers.PSW.operation_mode;
            log_event(LOG_DEBUG, "CHMOD: Modo cambiado a %s", 
                      cpu_registers.PSW.operation_mode ? "KERNEL" : "USER");
            break;
    }
}

void handle_register_operation(int opcode) {
    switch(opcode) {
        case 19:  // loadrb
            cpu_registers.AC = cpu_registers.RB;
            break;
        case 20:  // strrb
            cpu_registers.RB = cpu_registers.AC;
            break;
        case 21:  // loadr1 (RL)
            cpu_registers.AC = cpu_registers.RL;
            break;
        case 22:  // strr1 (RL)
            cpu_registers.RL = cpu_registers.AC;
            break;
        case 23:  // loadsp
            cpu_registers.AC = cpu_registers.SP;
            break;
        case 24:  // strsp
            cpu_registers.SP = cpu_registers.AC;
            break;
        case 25:  // psh
            // Implementar push en pila
            log_event(LOG_DEBUG, "PSH: Push en pila");
            break;
        case 26:  // pop
            // Implementar pop de pila
            log_event(LOG_DEBUG, "POP: Pop de pila");
            break;
    }
}

void handle_io_operation(int opcode) {
    int ac_value = word_to_int(cpu_registers.AC);
    
    switch(opcode) {
        case 28:  // sdmap - establecer pista
            dma_set_disk_location(ac_value, dma.disk_cylinder, dma.disk_sector);
            break;
        case 29:  // sdmac - establecer cilindro
            dma_set_disk_location(dma.disk_track, ac_value, dma.disk_sector);
            break;
        case 30:  // sdmas - establecer sector
            dma_set_disk_location(dma.disk_track, dma.disk_cylinder, ac_value);
            break;
        case 31:  // sdmaio - establecer operación I/O
            dma_set_io_operation(ac_value);
            break;
        case 32:  // sdmann - establecer dirección de memoria
            dma_set_memory_address(ac_value);
            break;
        case 33:  // sdmaon - iniciar DMA
            dma_start_transfer();
            break;
    }
    
    log_event(LOG_DEBUG, "E/S: Instrucción %d ejecutada, AC=%d", opcode, ac_value);
}

void set_cpu_state(CPU_State state) {
    cpu_state = state;
    log_event(LOG_INFO, "Estado de CPU cambiado a %d", state);
}

CPU_State get_cpu_state() {
    return cpu_state;
}

void execute_program(int start_address) {
    cpu_registers.PSW.PC = start_address;
    cpu_state = CPU_RUNNING;
    
    while (cpu_state == CPU_RUNNING) {
        cpu_cycle();
        usleep(10000);  // Pequeña pausa para no saturar
    }
}