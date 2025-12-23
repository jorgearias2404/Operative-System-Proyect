#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "LOGGER/logger.h"
#include "MEMORY/memory.h"
#include "REGISTERS/registers.h"
#include "INTERRUPTS/interrupts.h"
#include "DISK/disk.h"
#include "DMA/dma.h"
#include "CPU/cpu.h"
#include "CONSOLE/console.h"

int main() {
    printf("=== Inicializando Sistema Operativo Virtual ===\n");
    
    // Inicializar componentes en orden
    init_logger();
    init_memory();
    init_registers();
    init_interrupts();
    init_disk();
    init_dma();
    init_cpu();
    init_console();
    
    printf("Sistema inicializado correctamente.\n");
    
    // Ejecutar consola principal
    run_console();
    
    // Limpieza antes de salir
    close_logger();
    
    printf("=== Sistema finalizado ===\n");
    return 0;
}