/*
 * Archivo de implementación del módulo DMA del Sistema Operativo Virtual.
 * Contiene la lógica para realizar transferencias de datos entre memoria y disco
 * sin intervención de la CPU, utilizando hilos para operaciones asíncronas.
 */

/* Inclusión de cabecera propia del módulo */
#include "dma.h"

/* Inclusión de cabeceras de otros módulos del sistema */
#include "../MEMORY/memory.h"     // Para acceder a funciones de memoria
#include "../DISK/disk.h"         // Para operaciones de disco
#include "../INTERRUPTS/interrupts.h" // Para disparar interrupciones
#include "../LOGGER/logger.h"     // Para registro de eventos
#include "../types.h"            // Para tipos globales

/* Inclusión de bibliotecas estándar */
#include <stdio.h>    // Para printf y snprintf
#include <stdlib.h>   // Para funciones generales
#include <string.h>   // Para manipulación de cadenas
#include <unistd.h>   // Para usleep() en sistemas Unix

/*
 * MACROS PARA COMPATIBILIDAD MULTIPLATAFORMA
 * Define macros para manejar diferencias entre Windows y sistemas Unix-like
 * para la función de pausa/sleep durante las transferencias.
 */
#ifdef _WIN32
    #include <windows.h>          // API de Windows para Sleep()
    #define DMA_SLEEP(ms) Sleep(ms)  // Dormir en milisegundos (Windows)
#else
    #define DMA_SLEEP(ms) usleep(ms * 1000)  // Dormir en microsegundos (Unix)
#endif

/*
 * VARIABLE GLOBAL - Controlador DMA
 * Esta variable es la representación única del controlador DMA en el sistema.
 */
DMA_Controller dma;  // Instancia global del controlador DMA

/*
 * Función auxiliar: dma_disk_read
 * Parámetros:
 *   track - pista del disco
 *   cylinder - cilindro del disco
 *   sector - sector del disco
 *   buffer - buffer donde almacenar los datos leídos
 * Propósito: Leer datos simulados del disco para la transferencia DMA.
 * 
 * NOTA: Esta es una función simulada que no accede al disco real,
 *       sino que genera datos de ejemplo basados en las coordenadas.
 */
static void dma_disk_read(int track, int cylinder, int sector, char* buffer) {
    // Asegurar que las coordenadas estén dentro de límites razonables
    track = track % 100;      // Límite a 100 pistas
    cylinder = cylinder % 100; // Límite a 100 cilindros
    sector = sector % 1000;    // Límite a 1000 sectores
    
    // Crear dato simulado basado en las coordenadas
    // Formato: "T02C03S005" (Pista 2, Cilindro 3, Sector 5)
    char temp[32];  // Buffer temporal
    snprintf(temp, sizeof(temp), "T%02dC%02dS%03d", track, cylinder, sector);
    
    // Copiar solo los primeros 8 caracteres (tamaño de Word)
    strncpy(buffer, temp, 8);
    buffer[8] = '\0';  // Asegurar terminación nula
    
    // Registrar la operación para depuración
    log_event(LOG_DEBUG, "DMA: Leído disco T=%d,C=%d,S=%d -> %s", 
              track, cylinder, sector, buffer);
}

/*
 * Función auxiliar: dma_disk_write
 * Parámetros:
 *   track - pista del disco
 *   cylinder - cilindro del disco
 *   sector - sector del disco
 *   data - datos a escribir
 * Propósito: Simular escritura en disco para la transferencia DMA.
 * 
 * NOTA: Esta es una función simulada que no escribe realmente en disco,
 *       solo registra la operación para fines de demostración.
 */
static void dma_disk_write(int track, int cylinder, int sector, const char* data) {
    // Asegurar que las coordenadas estén dentro de límites razonables
    track = track % 100;
    cylinder = cylinder % 100;
    sector = sector % 1000;
    
    // Registrar la operación (en implementación real, escribiría en disco)
    log_event(LOG_DEBUG, "DMA: Escribiendo en disco T=%d,C=%d,S=%d: %s", 
              track, cylinder, sector, data);
}

/*
 * Función: transfer_thread (función auxiliar estática)
 * Parámetros: arg - argumentos del hilo (no utilizado)
 * Retorna: void* - siempre NULL
 * Propósito: Ejecutar la transferencia DMA en un hilo separado.
 * 
 * Esta función implementa la lógica principal de transferencia:
 * 1. Solicita el bus del sistema
 * 2. Realiza la transferencia (lectura o escritura)
 * 3. Libera el bus
 * 4. Dispara interrupción de finalización
 */
static void* transfer_thread(void* arg) {
    // PASO 1: Solicitar acceso exclusivo al bus del sistema
    dma_bus_request();  // Bloquea hasta obtener el bus
    
    log_event(LOG_INFO, "DMA: Iniciando transferencia %s",
              dma.io_operation == 0 ? "lectura (disco->memoria)" : "escritura (memoria->disco)");
    
    // Configurar estado según tipo de operación
    dma.state = (dma.io_operation == 0) ? DMA_READING : DMA_WRITING;
    
    /*
     * BUCLE DE TRANSFERENCIA
     * Transfiere byte a byte (o sector a sector) según bytes_to_transfer
     */
    for (int i = 0; i < dma.bytes_to_transfer; i++) {
        if (dma.io_operation == 0) {  // LECTURA: disco → memoria
            char buffer[9];  // 8 caracteres + null terminator
            
            // Leer del disco (simulado)
            dma_disk_read(dma.disk_track, dma.disk_cylinder, 
                         dma.disk_sector + i, buffer);
            
            // Convertir a estructura Word
            Word data_word;
            strncpy(data_word.data, buffer, 8);
            data_word.data[8] = '\0';  // Asegurar terminación nula
            
            // Verificar que la dirección de memoria esté dentro de límites
            if (dma.memory_address + i < MEMORY_SIZE) {
                // Escribir en memoria
                write_memory(dma.memory_address + i, data_word);
                
                // Registrar transferencia individual para depuración
                log_event(LOG_DEBUG, "DMA: Transferido sector %d a memoria[%d] = %s",
                         i, dma.memory_address + i, data_word.data);
            } else {
                // Error: dirección fuera de límites
                log_event(LOG_ERROR, "DMA: Dirección de memoria fuera de límites");
                dma.state = DMA_ERROR;
                dma.status = 1;  // Código de error
                break;  // Salir del bucle
            }
            
        } else {  // ESCRITURA: memoria → disco
            // Verificar que la dirección de memoria esté dentro de límites
            if (dma.memory_address + i < MEMORY_SIZE) {
                // Leer de memoria
                Word data_word = read_memory(dma.memory_address + i);
                
                // Convertir a cadena
                char buffer[9];
                strncpy(buffer, data_word.data, 8);
                buffer[8] = '\0';  // Asegurar terminación nula
                
                // Escribir en disco (simulado)
                dma_disk_write(dma.disk_track, dma.disk_cylinder, 
                              dma.disk_sector + i, buffer);
                
                // Registrar transferencia individual para depuración
                log_event(LOG_DEBUG, "DMA: Transferido memoria[%d] = %s a disco sector %d",
                         dma.memory_address + i, data_word.data, i);
            } else {
                // Error: dirección fuera de límites
                log_event(LOG_ERROR, "DMA: Dirección de memoria fuera de límites");
                dma.state = DMA_ERROR;
                dma.status = 1;  // Código de error
                break;  // Salir del bucle
            }
        }
        
        // Pequeña pausa para simular tiempo real de transferencia
        // En un sistema real, esto sería el tiempo de acceso a disco/memoria
        DMA_SLEEP(1);  // 1ms por byte/sector transferido
    }
    
    // Verificar si la transferencia fue exitosa
    if (dma.state != DMA_ERROR) {
        // Transferencia exitosa
        dma.state = DMA_IDLE;  // Volver a estado inactivo
        dma.status = 0;        // Código de éxito
        log_event(LOG_INFO, "DMA: Transferencia completada exitosamente");
    } else {
        // Transferencia fallida
        log_event(LOG_ERROR, "DMA: Transferencia falló");
    }
    
    // PASO 3: Liberar el bus del sistema
    dma_bus_release();  // Libera el mutex
    
    // PASO 4: Disparar interrupción para notificar a la CPU que la transferencia terminó
    trigger_interrupt(INT_IO_COMPLETION);
    
    return NULL;  // Valor de retorno del hilo (no utilizado)
}

/*
 * Función: init_dma
 * Propósito: Inicializar el controlador DMA con valores por defecto.
 * Configura todos los campos de la estructura DMA_Controller y
 * inicializa el mutex para control de acceso al bus.
 */
void init_dma() {
    // Configurar valores por defecto
    dma.memory_address = 0;        // Dirección de memoria inicial
    dma.disk_track = 0;            // Pista inicial
    dma.disk_cylinder = 0;         // Cilindro inicial
    dma.disk_sector = 0;           // Sector inicial
    dma.io_operation = 0;          // Operación por defecto: lectura
    dma.bytes_to_transfer = 1;     // Transferir 1 sector por defecto
    dma.state = DMA_IDLE;          // Estado inicial: inactivo
    dma.status = 0;                // Estado inicial: éxito
    
    // Inicializar el mutex para control de acceso al bus
    pthread_mutex_init(&dma.bus_lock, NULL);
    
    // Registrar inicialización
    log_event(LOG_INFO, "DMA inicializado");
}

/*
 * Función: dma_set_memory_address
 * Parámetros: address - dirección de memoria para la transferencia
 * Propósito: Configurar la dirección base en memoria para la transferencia DMA.
 */
void dma_set_memory_address(int address) {
    // Validar que la dirección esté dentro de los límites de memoria
    if (address < 0 || address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "DMA: Dirección de memoria inválida: %d", address);
        return;  // No configurar dirección inválida
    }
    
    // Configurar dirección de memoria
    dma.memory_address = address;
    
    // Registrar la configuración para depuración
    log_event(LOG_DEBUG, "DMA: Dirección de memoria configurada a %d", address);
}

/*
 * Función: dma_set_disk_location
 * Parámetros:
 *   track - número de pista del disco
 *   cylinder - número de cilindro del disco
 *   sector - número de sector del disco
 * Propósito: Configurar la ubicación en disco para la transferencia DMA.
 */
void dma_set_disk_location(int track, int cylinder, int sector) {
    // Validar coordenadas del disco
    if (track < 0 || track >= TRACKS ||
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        log_event(LOG_ERROR, "DMA: Coordenadas de disco inválidas: T=%d, C=%d, S=%d", 
                  track, cylinder, sector);
        return;  // No configurar coordenadas inválidas
    }
    
    // Configurar ubicación en disco
    dma.disk_track = track;
    dma.disk_cylinder = cylinder;
    dma.disk_sector = sector;
    
    // Registrar la configuración para depuración
    log_event(LOG_DEBUG, "DMA: Disco configurado a T=%d, C=%d, S=%d", 
              track, cylinder, sector);
}

/*
 * Función: dma_set_io_operation
 * Parámetros: operation - tipo de operación (0 = lectura, 1 = escritura)
 * Propósito: Configurar el tipo de operación DMA.
 */
void dma_set_io_operation(int operation) {
    // Validar que la operación sea 0 o 1
    if (operation != 0 && operation != 1) {
        log_event(LOG_ERROR, "DMA: Operación I/O inválida: %d", operation);
        return;  // No configurar operación inválida
    }
    
    // Configurar operación
    dma.io_operation = operation;
    
    // Registrar la configuración
    log_event(LOG_DEBUG, "DMA: Operación configurada a %s", 
              operation == 0 ? "LECTURA" : "ESCRITURA");
}

/*
 * Función: dma_set_transfer_size
 * Parámetros: size - tamaño de la transferencia en bytes/sectores
 * Propósito: Configurar la cantidad de datos a transferir.
 */
void dma_set_transfer_size(int size) {
    // Validar tamaño positivo
    if (size <= 0) {
        log_event(LOG_ERROR, "DMA: Tamaño de transferencia inválido: %d", size);
        return;  // No configurar tamaño inválido
    }
    
    // Configurar tamaño de transferencia
    dma.bytes_to_transfer = size;
    
    // Registrar la configuración
    log_event(LOG_DEBUG, "DMA: Tamaño de transferencia configurado a %d", size);
}

/*
 * Función: dma_start_transfer
 * Propósito: Iniciar una transferencia DMA de forma asíncrona.
 * Crea un hilo separado que ejecuta la transferencia mientras
 * la CPU puede continuar con otras tareas.
 */
void dma_start_transfer() {
    // Verificar que el DMA no esté ya ocupado
    if (dma.state != DMA_IDLE) {
        log_event(LOG_WARNING, "DMA: Ya hay una transferencia en curso (estado: %d)", dma.state);
        return;  // No iniciar nueva transferencia si ya hay una activa
    }
    
    // Validar dirección de memoria
    if (dma.memory_address < 0 || dma.memory_address >= MEMORY_SIZE) {
        log_event(LOG_ERROR, "DMA: Dirección de memoria inválida para transferencia");
        dma.status = 1;      // Establecer estado de error
        dma.state = DMA_ERROR; // Cambiar a estado de error
        return;  // No iniciar transferencia con parámetros inválidos
    }
    
    // Crear hilo para ejecutar la transferencia
    if (pthread_create(&dma.thread, NULL, transfer_thread, NULL) != 0) {
        log_event(LOG_ERROR, "DMA: No se pudo crear hilo de transferencia");
        dma.status = 1;      // Establecer estado de error
        dma.state = DMA_ERROR; // Cambiar a estado de error
        return;  // Falló la creación del hilo
    }
    
    /*
     * Desacoplar (detach) el hilo para que se limpie automáticamente
     * cuando termine. Esto permite que la transferencia sea realmente asíncrona.
     */
    pthread_detach(dma.thread);
    
    // Registrar inicio de transferencia
    log_event(LOG_INFO, "DMA: Transferencia iniciada (asíncrono)");
}

/*
 * Función: dma_wait_completion
 * Propósito: Esperar a que termine la transferencia DMA actual.
 * Esta función bloquea hasta que el hilo de transferencia termine,
 * haciendo la operación síncrona.
 */
void dma_wait_completion() {
    // Si el DMA está inactivo o en error, no hay nada que esperar
    if (dma.state == DMA_IDLE || dma.state == DMA_ERROR) {
        return;
    }
    
    // Esperar (bloquear) hasta que el hilo termine
    pthread_join(dma.thread, NULL);
    
    // Registrar finalización de espera
    log_event(LOG_DEBUG, "DMA: Transferencia finalizada (síncrona)");
}

/*
 * Función: dma_get_status
 * Retorna: int - estado de la última operación (0 = éxito, 1 = error)
 * Propósito: Obtener el estado de la última transferencia DMA.
 */
int dma_get_status() {
    return dma.status;
}

/*
 * Función: dma_get_state
 * Retorna: DMA_State - estado actual del controlador DMA
 * Propósito: Obtener el estado actual del DMA (IDLE, READING, WRITING, ERROR).
 */
DMA_State dma_get_state() {
    return dma.state;
}

/*
 * Función: dma_bus_request
 * Propósito: Solicitar acceso exclusivo al bus del sistema.
 * Esta función bloquea hasta que el bus esté disponible.
 */
void dma_bus_request() {
    // Bloquear (lock) el mutex del bus
    // Si otro dispositivo está usando el bus, esta llamada se bloqueará
    pthread_mutex_lock(&dma.bus_lock);
    
    // Registrar adquisición del bus
    log_event(LOG_DEBUG, "DMA: Bus adquirido");
}

/*
 * Función: dma_bus_release
 * Propósito: Liberar el bus del sistema para que otros dispositivos lo usen.
 */
void dma_bus_release() {
    // Liberar (unlock) el mutex del bus
    pthread_mutex_unlock(&dma.bus_lock);
    
    // Registrar liberación del bus
    log_event(LOG_DEBUG, "DMA: Bus liberado");
}