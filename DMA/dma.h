/*
 * Archivo de cabecera del módulo DMA (Direct Memory Access - Acceso Directo a Memoria)
 * del Sistema Operativo Virtual.
 * Define las estructuras de datos y prototipos de funciones para implementar
 * un controlador DMA que permite transferencias de datos entre memoria y disco
 * sin intervención de la CPU, liberándola para otras tareas.
 */

#ifndef DMA_H
#define DMA_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

#include <pthread.h>       // Para soporte de hilos (threads) y mutex
#include "../types.h"      // Para tipos y constantes globales

/*
 * Enum: DMA_State
 * Propósito: Define los posibles estados del controlador DMA durante una transferencia.
 * 
 * Valores:
 *   DMA_IDLE    - DMA inactivo, listo para nueva transferencia
 *   DMA_READING - DMA leyendo datos del disco hacia memoria
 *   DMA_WRITING - DMA escribiendo datos de memoria hacia disco
 *   DMA_ERROR   - DMA en estado de error (transferencia fallida)
 */
typedef enum {
    DMA_IDLE,      // Estado de reposo, sin transferencia activa
    DMA_READING,   // Transferencia en curso: disco → memoria
    DMA_WRITING,   // Transferencia en curso: memoria → disco
    DMA_ERROR      // Error en la transferencia
} DMA_State;

/*
 * Estructura: DMA_Controller
 * Propósito: Representa el controlador DMA completo con toda su configuración
 *            y estado actual.
 * 
 * Campos:
 *   memory_address    - Dirección base en memoria para la transferencia
 *   disk_track        - Pista del disco donde comienza la transferencia
 *   disk_cylinder     - Cilindro del disco donde comienza la transferencia
 *   disk_sector       - Sector del disco donde comienza la transferencia
 *   io_operation      - Tipo de operación: 0 = lectura, 1 = escritura
 *   bytes_to_transfer - Cantidad de bytes (sectores) a transferir
 *   state             - Estado actual del DMA (DMA_State)
 *   status            - Estado de la última operación: 0 = éxito, 1 = error
 *   thread            - Identificador del hilo que ejecuta la transferencia
 *   bus_lock          - Mutex para controlar acceso exclusivo al bus del sistema
 */
typedef struct {
    int memory_address;      // Dirección base en memoria (0 a MEMORY_SIZE-1)
    int disk_track;          // Pista del disco (0 a TRACKS-1)
    int disk_cylinder;       // Cilindro del disco (0 a CYLINDERS-1)
    int disk_sector;         // Sector inicial del disco (0 a SECTORS_PER_CYLINDER-1)
    int io_operation;        // 0 = lectura (disco→memoria), 1 = escritura (memoria→disco)
    int bytes_to_transfer;   // Número de sectores/bytes a transferir
    DMA_State state;         // Estado actual del DMA (IDLE, READING, WRITING, ERROR)
    int status;              // Resultado: 0 = éxito, 1 = error
    pthread_t thread;        // Hilo para ejecución asíncrona de transferencias
    pthread_mutex_t bus_lock; // Mutex para acceso exclusivo al bus del sistema
} DMA_Controller;

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo DMA
 */

/* FUNCIONES DE INICIALIZACIÓN Y CONFIGURACIÓN */
void init_dma();                           // Inicializar controlador DMA
void dma_set_memory_address(int address);  // Configurar dirección de memoria
void dma_set_disk_location(int track, int cylinder, int sector);  // Configurar ubicación en disco
void dma_set_io_operation(int operation);  // Configurar tipo de operación (lectura/escritura)
void dma_set_transfer_size(int size);      // Configurar tamaño de transferencia

/* FUNCIONES DE CONTROL DE TRANSFERENCIA */
void dma_start_transfer();                 // Iniciar transferencia DMA (asíncrona)
void dma_wait_completion();                // Esperar a que termine la transferencia (síncrona)

/* FUNCIONES DE CONSULTA DE ESTADO */
int dma_get_status();                      // Obtener estado de la última operación (0=éxito, 1=error)
DMA_State dma_get_state();                 // Obtener estado actual del DMA

/* FUNCIONES DE CONTROL DEL BUS */
void dma_bus_request();                    // Solicitar acceso exclusivo al bus del sistema
void dma_bus_release();                    // Liberar el bus del sistema

/*
 * DECLARACIÓN DE VARIABLE GLOBAL EXTERNA
 * Permite que otros módulos accedan al controlador DMA.
 */
extern DMA_Controller dma;  // Instancia global del controlador DMA

#endif /* DMA_H */