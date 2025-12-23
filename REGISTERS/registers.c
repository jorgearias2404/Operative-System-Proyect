#include "registers.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

CPU_Registers cpu_registers;

void init_registers() {
    // Inicializar acumulador
    strcpy(cpu_registers.AC.data, "00000000");
    
    // Inicializar otros registros de palabra
    strcpy(cpu_registers.MAR.data, "00000000");
    strcpy(cpu_registers.MDR.data, "00000000");
    strcpy(cpu_registers.IR.data, "00000000");
    strcpy(cpu_registers.RB.data, "00000000");
    strcpy(cpu_registers.RL.data, "00000000");
    strcpy(cpu_registers.RX.data, "00000000");
    strcpy(cpu_registers.SP.data, "00000000");
    
    // Inicializar PSW
    cpu_registers.PSW.condition_code = 0;
    cpu_registers.PSW.operation_mode = 1;  // Empieza en modo kernel
    cpu_registers.PSW.interrupt_enabled = 0;  // Interrupciones deshabilitadas inicialmente
    cpu_registers.PSW.PC = OS_RESERVED;    // Comienza después del área del SO
    
    log_event(LOG_INFO, "Registros inicializados");
}

void dump_registers() {
    printf("\n=== REGISTROS DE LA CPU ===\n");
    printf("AC:  %s\n", cpu_registers.AC.data);
    printf("MAR: %s\n", cpu_registers.MAR.data);
    printf("MDR: %s\n", cpu_registers.MDR.data);
    printf("IR:  %s\n", cpu_registers.IR.data);
    printf("RB:  %s\n", cpu_registers.RB.data);
    printf("RL:  %s\n", cpu_registers.RL.data);
    printf("RX:  %s\n", cpu_registers.RX.data);
    printf("SP:  %s\n", cpu_registers.SP.data);
    printf("\n=== PSW ===\n");
    printf("Condition Code:    %d\n", cpu_registers.PSW.condition_code);
    printf("Operation Mode:    %s\n", cpu_registers.PSW.operation_mode ? "KERNEL" : "USER");
    printf("Interrupt Enabled: %s\n", cpu_registers.PSW.interrupt_enabled ? "SI" : "NO");
    printf("PC:                %d\n", cpu_registers.PSW.PC);
    printf("=====================\n");
}

int word_to_int(Word w) {
    char* data = w.data;
    
    // Verificar que tenga 8 dígitos
    if (strlen(data) != 8) {
        log_event(LOG_ERROR, "Palabra no tiene 8 dígitos: %s", data);
        return 0;
    }
    
    // Primer dígito es el signo
    char sign = data[0];
    int is_negative = (sign == '1');  // 1 = negativo, 0 = positivo
    
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
    
    // Verificar que no exceda 7 dígitos
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
    result.data[8] = '\0';
    
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
    // Nota: el código 3 (overflow) se maneja en las operaciones aritméticas
}