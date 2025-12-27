#ifndef DMA_H
#define DMA_H

#include <pthread.h>
#include "../types.h"

// Estados del DMA
typedef enum {
    DMA_IDLE,
    DMA_READING,
    DMA_WRITING,
    DMA_ERROR
} DMA_State;

// Estructura del controlador DMA
typedef struct {
    int memory_address;      // Dirección en memoria
    int disk_track;          // Pista del disco
    int disk_cylinder;       // Cilindro del disco
    int disk_sector;         // Sector del disco
    int io_operation;        // 0=leer del disco, 1=escribir al disco
    int bytes_to_transfer;   // Cantidad de bytes a transferir
    DMA_State state;         // Estado actual
    int status;              // 0=éxito, 1=error
    pthread_t thread;        // Hilo para operaciones asíncronas
    pthread_mutex_t bus_lock; // Mutex para el bus
} DMA_Controller;

// Funciones del DMA
void init_dma();
void dma_set_memory_address(int address);
void dma_set_disk_location(int track, int cylinder, int sector);
void dma_set_io_operation(int operation);
void dma_set_transfer_size(int size);  // <-- ¡AÑADE ESTA LÍNEA FALTANTE!
void dma_start_transfer();
void dma_wait_completion();
int dma_get_status();
DMA_State dma_get_state();
void dma_bus_request();
void dma_bus_release();

extern DMA_Controller dma;

#endif