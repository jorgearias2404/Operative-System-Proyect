#include "dma.h"
#include "../MEMORY/memory.h"
#include "../DISK/disk.h"
#include "../INTERRUPTS/interrupts.h"
#include "../LOGGER/logger.h"
#include "../types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#define DMA_SLEEP(ms) Sleep(ms)
#else
#define DMA_SLEEP(ms) usleep(ms * 1000)
#endif

DMA_Controller dma;

// Función auxiliar para leer del disco usando DMA
static void dma_disk_read(int track, int cylinder, int sector, char* buffer) {
    // Asegurar límites
    track = track % 100;
    cylinder = cylinder % 100;
    sector = sector % 1000;
    
    char temp[32];
    snprintf(temp, sizeof(temp), "T%02dC%02dS%03d", track, cylinder, sector);
    
    // Copiar solo 8 caracteres (tamaño de Word)
    strncpy(buffer, temp, 8);
    buffer[8] = '\0';
    
    log_event(LOG_DEBUG, "DMA: Leído disco T=%d,C=%d,S=%d -> %s", 
              track, cylinder, sector, buffer);
}

// Función auxiliar para escribir en disco usando DMA
static void dma_disk_write(int track, int cylinder, int sector, const char* data) {
    // Asegurar límites
    track = track % 100;
    cylinder = cylinder % 100;
    sector = sector % 1000;
    
    log_event(LOG_DEBUG, "DMA: Escribiendo en disco T=%d,C=%d,S=%d: %s", 
              track, cylinder, sector, data);
}

// Función para simular transferencia (se ejecuta en hilo separado)
static void* transfer_thread(void* arg) {
    dma_bus_request();  // Solicitar bus
    
    log_event(LOG_INFO, "DMA: Iniciando transferencia %s",
              dma.io_operation == 0 ? "lectura (disco->memoria)" : "escritura (memoria->disco)");
    
    dma.state = (dma.io_operation == 0) ? DMA_READING : DMA_WRITING;
    
    // Simular tiempo de transferencia
    for (int i = 0; i < dma.bytes_to_transfer; i++) {
        if (dma.io_operation == 0) {  // Lectura: disco -> memoria
            char buffer[9];  // 8 + null
            dma_disk_read(dma.disk_track, dma.disk_cylinder, 
                         dma.disk_sector + i, buffer);
            
            // Convertir a Word y escribir en memoria
            Word data_word;
            strncpy(data_word.data, buffer, 8);
            data_word.data[8] = '\0';
            
            // Verificar límites de memoria
            if (dma.memory_address + i < MEMORY_SIZE) {
                write_memory(dma.memory_address + i, data_word);
                log_event(LOG_DEBUG, "DMA: Transferido sector %d a memoria[%d] = %s",
                         i, dma.memory_address + i, data_word.data);
            } else {
                log_event(LOG_ERROR, "DMA: Dirección de memoria fuera de límites");
                dma.state = DMA_ERROR;
                dma.status = 1;
                break;
            }
            
        } else {  // Escritura: memoria -> disco
            if (dma.memory_address + i < MEMORY_SIZE) {
                Word data_word = read_memory(dma.memory_address + i);
                
                char buffer[9];
                strncpy(buffer, data_word.data, 8);
                buffer[8] = '\0';
                
                dma_disk_write(dma.disk_track, dma.disk_cylinder, 
                              dma.disk_sector + i, buffer);
                
                log_event(LOG_DEBUG, "DMA: Transferido memoria[%d] = %s a disco sector %d",
                         dma.memory_address + i, data_word.data, i);
            } else {
                log_event(LOG_ERROR, "DMA: Dirección de memoria fuera de límites");
                dma.state = DMA_ERROR;
                dma.status = 1;
                break;
            }
        }
        
        // Pequeña pausa para simular tiempo real
        DMA_SLEEP(1);  // 1ms por byte/sector
    }
    
    if (dma.state != DMA_ERROR) {
        dma.state = DMA_IDLE;
        dma.status = 0;  // Éxito
        log_event(LOG_INFO, "DMA: Transferencia completada exitosamente");
    } else {
        log_event(LOG_ERROR, "DMA: Transferencia falló");
    }
    
    dma_bus_release();  // Liberar bus
    
    // Disparar interrupción de finalización de E/S
    trigger_interrupt(INT_IO_COMPLETION);
    
    return NULL;
}

void init_dma() {
    dma.memory_address = 0;
    dma.disk_track = 0;
    dma.disk_cylinder = 0;
    dma.disk_sector = 0;
    dma.io_operation = 0;
    dma.bytes_to_transfer = 1;  // Por defecto 1 sector
    dma.state = DMA_IDLE;
    dma.status = 0;
    
    pthread_mutex_init(&dma.bus_lock, NULL);
    
    log_event(LOG_INFO, "DMA inicializado");
}

void dma_set_memory_address(int address) {
    if (address < 0 || address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "DMA: Dirección de memoria inválida: %d", address);
        return;
    }
    dma.memory_address = address;
    log_event(LOG_DEBUG, "DMA: Dirección de memoria configurada a %d", address);
}

void dma_set_disk_location(int track, int cylinder, int sector) {
    if (track < 0 || track >= TRACKS ||
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        log_event(LOG_ERROR, "DMA: Coordenadas de disco inválidas: T=%d, C=%d, S=%d", track, cylinder, sector);
        return;
    }
    
    dma.disk_track = track;
    dma.disk_cylinder = cylinder;
    dma.disk_sector = sector;
    
    log_event(LOG_DEBUG, "DMA: Disco configurado a T=%d, C=%d, S=%d", track, cylinder, sector);
}

void dma_set_io_operation(int operation) {
    if (operation != 0 && operation != 1) {
        log_event(LOG_ERROR, "DMA: Operación I/O inválida: %d", operation);
        return;
    }
    dma.io_operation = operation;
    log_event(LOG_DEBUG, "DMA: Operación configurada a %s", operation == 0 ? "LECTURA" : "ESCRITURA");
}

void dma_set_transfer_size(int size) {
    if (size <= 0) {
        log_event(LOG_ERROR, "DMA: Tamaño de transferencia inválido: %d", size);
        return;
    }
    dma.bytes_to_transfer = size;
    log_event(LOG_DEBUG, "DMA: Tamaño de transferencia configurado a %d", size);
}

void dma_start_transfer() {
    if (dma.state != DMA_IDLE) {
        log_event(LOG_WARNING, "DMA: Ya hay una transferencia en curso (estado: %d)", dma.state);
        return;
    }
    
    // Verificar parámetros
    if (dma.memory_address < 0 || dma.memory_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "DMA: Dirección de memoria inválida para transferencia");
        dma.status = 1;
        dma.state = DMA_ERROR;
        return;
    }
    
    // Crear hilo para la transferencia
    if (pthread_create(&dma.thread, NULL, transfer_thread, NULL) != 0) {
        log_event(LOG_ERROR, "DMA: No se pudo crear hilo de transferencia");
        dma.status = 1;
        dma.state = DMA_ERROR;
        return;
    }
    
    // No hacemos join aquí para que sea asíncrono
    pthread_detach(dma.thread);
    
    log_event(LOG_INFO, "DMA: Transferencia iniciada (asíncrono)");
}

void dma_wait_completion() {
    if (dma.state == DMA_IDLE || dma.state == DMA_ERROR) {
        return;
    }
    
    // Esperar a que termine el hilo
    pthread_join(dma.thread, NULL);
    log_event(LOG_DEBUG, "DMA: Transferencia finalizada (síncrona)");
}

int dma_get_status() {
    return dma.status;
}

DMA_State dma_get_state() {
    return dma.state;
}

void dma_bus_request() {
    pthread_mutex_lock(&dma.bus_lock);
    log_event(LOG_DEBUG, "DMA: Bus adquirido");
}

void dma_bus_release() {
    pthread_mutex_unlock(&dma.bus_lock);
    log_event(LOG_DEBUG, "DMA: Bus liberado");
}