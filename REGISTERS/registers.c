#include "registers.h"
#include "../LOGGER/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CPU_Registers cpu_registers;

void init_registers() {
    // Inicializar acumulador y otros registros como Words
    cpu_registers.AC = int_to_word(0);
    cpu_registers.MAR = int_to_word(0);
    cpu_registers.MDR = int_to_word(0);
    cpu_registers.IR = int_to_word(0);
    cpu_registers.RB = int_to_word(0);
    cpu_registers.RL = int_to_word(1024);  // Ejemplo: límite de 1024 palabras
    cpu_registers.RX = int_to_word(0);
    cpu_registers.SP = int_to_word(1023);  // Stack al final de memoria
    
    // Inicializar PC como Word
    cpu_registers.PC = int_to_word(0);
    
    // Inicializar PSW
    cpu_registers.PSW.condition_code = 0;
    cpu_registers.PSW.operation_mode = 1;      // Empieza en modo kernel
    cpu_registers.PSW.interrupt_enabled = 0;   // Interrupciones deshabilitadas
    cpu_registers.PSW.PC_psw = 0;              // PC en PSW inicializado a 0
    
    log_event(LOG_INFO, "Registros inicializados");
}

void dump_registers() {
    printf("\n=== REGISTROS DE LA CPU ===\n");
    printf("AC:  %s (int: %d)\n", cpu_registers.AC.data, word_to_int(cpu_registers.AC));
    printf("MAR: %s\n", cpu_registers.MAR.data);
    printf("MDR: %s\n", cpu_registers.MDR.data);
    printf("IR:  %s\n", cpu_registers.IR.data);
    printf("RB:  %s\n", cpu_registers.RB.data);
    printf("RL:  %s\n", cpu_registers.RL.data);
    printf("RX:  %s\n", cpu_registers.RX.data);
    printf("SP:  %s\n", cpu_registers.SP.data);
    printf("PC:  %s (int: %d)\n", cpu_registers.PC.data, word_to_int(cpu_registers.PC));
    
    printf("\n=== PSW ===\n");
    printf("Condition Code:    %d\n", cpu_registers.PSW.condition_code);
    printf("Operation Mode:    %s\n", cpu_registers.PSW.operation_mode ? "KERNEL" : "USER");
    printf("Interrupt Enabled: %s\n", cpu_registers.PSW.interrupt_enabled ? "SI" : "NO");
    printf("PC (en PSW):       %d\n", cpu_registers.PSW.PC_psw);
    
    Word psw_word = psw_to_word(cpu_registers.PSW);
    printf("PSW como Word:     %s (int: %d)\n", psw_word.data, word_to_int(psw_word));
    printf("=====================\n");
}

// FUNCIONES DE CONVERSIÓN PSW <-> WORD
Word psw_to_word(PSW psw) {
    // Construir valor entero de 16 bits
    unsigned int value = 0;
    value |= (psw.condition_code & 0xF);        // bits 0-3
    value |= (psw.operation_mode & 0x1) << 4;   // bit 4
    value |= (psw.interrupt_enabled & 0x1) << 5; // bit 5
    value |= (psw.PC_psw & 0x3FF) << 6;         // bits 6-15 (10 bits para PC)
    
    // Asegurar que no exceda 16 bits
    value &= 0xFFFF;
    
    return int_to_word((int)value);
}

PSW word_to_psw(Word w) {
    PSW psw;
    int value = word_to_int(w);
    
    // Asegurar que sea positivo para extracción de bits
    if (value < 0) value = 0;
    
    // Extraer bits según la estructura
    psw.condition_code = value & 0xF;                // bits 0-3
    psw.operation_mode = (value >> 4) & 0x1;        // bit 4
    psw.interrupt_enabled = (value >> 5) & 0x1;     // bit 5
    psw.PC_psw = (value >> 6) & 0x3FF;              // bits 6-15 (10 bits)
    
    return psw;
}

// FUNCIÓN PARA SINCRONIZAR PC (desde Word)
void set_PC(Word pc_value) {
    // Convertir Word a int
    int pc_int = word_to_int(pc_value);
    
    // Limitar a 10 bits (0-1023) para PSW (porque PC_psw tiene 10 bits)
    if (pc_int > 1023) pc_int = 1023;
    if (pc_int < 0) pc_int = 0;
    
    // Actualizar ambos
    cpu_registers.PC = pc_value;  // Guardar como Word
    cpu_registers.PSW.PC_psw = pc_int;  // Guardar como int en PSW
    
    log_event(LOG_DEBUG, "PC actualizado a %d", pc_int);
}

// FUNCIÓN PARA SINCRONIZAR PC (desde int) - NUEVA
void set_PC_int(int pc_int) {
    // Limitar a 10 bits (0-1023) para PSW
    if (pc_int > 1023) pc_int = 1023;
    if (pc_int < 0) pc_int = 0;
    
    // Actualizar ambos
    cpu_registers.PC = int_to_word(pc_int);  // Convertir a Word y guardar
    cpu_registers.PSW.PC_psw = pc_int;       // Guardar como int en PSW
    
    log_event(LOG_DEBUG, "PC actualizado a %d", pc_int);
}

// Funciones de conversión Word <-> int (PARA 8 DÍGITOS)
int word_to_int(Word w) {
    char* data = w.data;
    
    // Verificar que tenga 8 dígitos
    if (strlen(data) != 8) {
        log_event(LOG_ERROR, "Palabra no tiene 8 dígitos: %s", data);
        return 0;
    }
    
    // Primer dígito es el signo
    char sign = data[0];
    int is_negative = (sign == '1');
    
    // Convertir los 7 dígitos de magnitud
    char magnitude[8];
    strncpy(magnitude, data + 1, 7);
    magnitude[7] = '\0';
    
    int value = atoi(magnitude);
    if (is_negative) {
        value = -value;
    }
    
    return value;
}

Word int_to_word(int value) {
    Word result;
    
    // Determinar signo
    char sign = (value < 0) ? '1' : '0';
    int abs_value = abs(value);
    
    // Verificar que no exceda 7 dígitos (para 8 dígitos con signo)
    if (abs_value > 9999999) {
        log_event(LOG_ERROR, "Overflow: valor %d excede 7 dígitos", value);
        strcpy(result.data, "OVERFLOW");
        return result;
    }
    
    // Formatear con 7 dígitos
    char magnitude[8];
    sprintf(magnitude, "%07d", abs_value);
    
    // Combinar signo y magnitud
    result.data[0] = sign;
    strncpy(result.data + 1, magnitude, 7);
    result.data[8] = '\0';  // Null terminator
    
    return result;
}

void update_condition_code(int result) {
    if (result == 0) {
        cpu_registers.PSW.condition_code = 0;  // X = Y
    } else if (result < 0) {
        cpu_registers.PSW.condition_code = 1;  // X < Y
    } else {
        cpu_registers.PSW.condition_code = 2;  // X > Y
    }
    // Código 3 (overflow) se maneja en operaciones aritméticas
}