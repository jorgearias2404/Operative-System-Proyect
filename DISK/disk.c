/*
 * Archivo de implementación del módulo de disco del Sistema Operativo Virtual.
 * Contiene la lógica para simular un disco duro con geometría tridimensional
 * (pistas, cilindros, sectores) y operaciones de lectura/escritura.
 */

/* Inclusión de cabecera propia del módulo */
#include "disk.h"

/* Inclusión de cabeceras de otros módulos */
#include "../LOGGER/logger.h"  // Para registro de eventos del sistema

/* Inclusión de bibliotecas estándar */
#include <string.h>   // Para funciones de manipulación de cadenas (strcpy, strlen)
#include <stdlib.h>   // Para funciones generales

/*
 * VARIABLE GLOBAL - Instancia del disco duro
 * Esta variable es la representación única del disco en el sistema.
 * Se declara como global para que sea accesible desde otros módulos.
 */
HardDisk hard_disk;  // Instancia global del disco duro virtual

/*
 * Función: init_disk
 * Propósito: Inicializar el disco duro virtual.
 * Realiza las siguientes acciones:
 * 1. Establece la posición inicial del cabezal en (0, 0, 0)
 * 2. Llena todos los sectores del disco con el valor "00000000"
 * 3. Registra el evento de inicialización en el logger
 */
void init_disk() {
    // Inicializar posición actual del cabezal de lectura/escritura
    // Se establece en la posición inicial (pista 0, cilindro 0, sector 0)
    hard_disk.current_track = 0;      // Primera pista
    hard_disk.current_cylinder = 0;   // Primer cilindro
    hard_disk.current_sector = 0;     // Primer sector
    
    /*
     * INICIALIZACIÓN DE TODOS LOS SECTORES DEL DISCO
     * Se utiliza un triple bucle anidado para recorrer toda la geometría del disco:
     * - Bucle externo: pistas (tracks)
     * - Bucle intermedio: cilindros (cylinders)
     * - Bucle interno: sectores (sectors)
     * 
     * Cada sector se inicializa con "00000000" (8 ceros), que representa datos vacíos.
     */
    for (int t = 0; t < TRACKS; t++) {                    // Recorrer todas las pistas
        for (int c = 0; c < CYLINDERS; c++) {             // Recorrer todos los cilindros
            for (int s = 0; s < SECTORS_PER_CYLINDER; s++) {  // Recorrer todos los sectores
                // Inicializar este sector específico con datos vacíos
                // strcpy copia la cadena "00000000" en el sector
                strcpy(hard_disk.data[t][c][s], "00000000");
            }
        }
    }
    
    /*
     * REGISTRO DE EVENTO
     * Se registra en el logger que el disco ha sido inicializado exitosamente,
     * incluyendo información sobre su geometría.
     */
    log_event(LOG_INFO, 
              "Disco inicializado: %d pistas, %d cilindros, %d sectores por cilindro",
              TRACKS, CYLINDERS, SECTORS_PER_CYLINDER);
}

/*
 * Función: read_sector
 * Parámetros:
 *   track - número de pista (0 a TRACKS-1)
 *   cylinder - número de cilindro (0 a CYLINDERS-1)
 *   sector - número de sector (0 a SECTORS_PER_CYLINDER-1)
 *   buffer - buffer de destino donde se copiarán los datos leídos
 * Propósito: Leer el contenido de un sector específico del disco.
 * 
 * El proceso incluye:
 * 1. Validación de las coordenadas del sector
 * 2. Copia de datos del disco al buffer
 * 3. Registro de la operación en el logger
 */
void read_sector(int track, int cylinder, int sector, char* buffer) {
    /*
     * VALIDACIÓN DE COORDENADAS
     * Verifica que las coordenadas proporcionadas estén dentro de los límites válidos.
     * Si las coordenadas son inválidas, se registra un error y se devuelve "ERROR".
     */
    if (track < 0 || track >= TRACKS || 
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        // Coordenadas inválidas, registrar error
        log_event(LOG_ERROR, 
                  "Coordenadas de disco inválidas: T=%d, C=%d, S=%d", 
                  track, cylinder, sector);
        
        // Copiar mensaje de error al buffer
        strcpy(buffer, "ERROR");
        return;  // Terminar función
    }
    
    /*
     * LECTURA DE DATOS
     * Copia el contenido del sector solicitado desde el disco al buffer proporcionado.
     * strcpy copia la cadena completa incluyendo el terminador nulo.
     */
    strcpy(buffer, hard_disk.data[track][cylinder][sector]);
    
    /*
     * REGISTRO DE OPERACIÓN
     * Registra en modo DEBUG la operación de lectura con las coordenadas
     * y los datos leídos.
     */
    log_event(LOG_DEBUG, 
              "Lectura de disco: T=%d, C=%d, S=%d -> %s", 
              track, cylinder, sector, buffer);
}

/*
 * Función: write_sector
 * Parámetros:
 *   track - número de pista (0 a TRACKS-1)
 *   cylinder - número de cilindro (0 a CYLINDERS-1)
 *   sector - número de sector (0 a SECTORS_PER_CYLINDER-1)
 *   data - cadena de datos a escribir en el sector
 * Propósito: Escribir datos en un sector específico del disco.
 * 
 * El proceso incluye:
 * 1. Validación de las coordenadas del sector
 * 2. Verificación del tamaño de los datos
 * 3. Copia de datos del buffer al disco
 * 4. Registro de la operación en el logger
 */
void write_sector(int track, int cylinder, int sector, const char* data) {
    /*
     * VALIDACIÓN DE COORDENADAS
     * Verifica que las coordenadas estén dentro de los límites válidos.
     */
    if (track < 0 || track >= TRACKS || 
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        // Coordenadas inválidas, registrar error
        log_event(LOG_ERROR, 
                  "Coordenadas de disco inválidas: T=%d, C=%d, S=%d", 
                  track, cylinder, sector);
        return;  // Terminar función sin escribir
    }
    
    /*
     * VERIFICACIÓN DEL TAMAÑO DE DATOS
     * Cada sector debe contener exactamente 8 dígitos.
     * strlen(data) debe ser 8, pero SECTOR_SIZE es 9 (incluye null terminator).
     * Por eso se compara con SECTOR_SIZE - 1.
     */
    if (strlen(data) != SECTOR_SIZE - 1) {  // -1 porque no contamos el null terminator
        // Datos de tamaño incorrecto, registrar advertencia
        log_event(LOG_WARNING, 
                  "Datos de tamaño incorrecto para sector: %s", 
                  data);
        // NOTA: Aún así se procede con la escritura
    }
    
    /*
     * ESCRITURA DE DATOS
     * Copia los datos proporcionados al sector especificado del disco.
     * strcpy copia la cadena completa incluyendo el terminador nulo.
     */
    strcpy(hard_disk.data[track][cylinder][sector], data);
    
    /*
     * REGISTRO DE OPERACIÓN
     * Registra en modo DEBUG la operación de escritura con las coordenadas
     * y los datos escritos.
     */
    log_event(LOG_DEBUG, 
              "Escritura en disco: T=%d, C=%d, S=%d <- %s", 
              track, cylinder, sector, data);
}

/*
 * Función: disk_info
 * Propósito: Mostrar información detallada sobre el disco en la consola.
 * Proporciona un resumen de la configuración, capacidad y estado actual del disco.
 */
void disk_info() {
    printf("\n=== INFORMACIÓN DEL DISCO ===\n");
    
    // Mostrar geometría del disco
    printf("Pistas: %d\n", TRACKS);
    printf("Cilindros: %d\n", CYLINDERS);
    printf("Sectores por cilindro: %d\n", SECTORS_PER_CYLINDER);
    
    // Mostrar tamaño de sector (excluyendo el terminador nulo)
    printf("Sector size: %d caracteres\n", SECTOR_SIZE - 1);
    
    // Calcular y mostrar capacidad total del disco
    int total_sectors = TRACKS * CYLINDERS * SECTORS_PER_CYLINDER;
    printf("Capacidad total: %d sectores\n", total_sectors);
    
    // Mostrar posición actual del cabezal
    printf("Posición actual: T=%d, C=%d, S=%d\n",
           hard_disk.current_track,
           hard_disk.current_cylinder,
           hard_disk.current_sector);
}

/*
 * Función: format_disk
 * Propósito: Formatear completamente el disco, sobrescribiendo todos los sectores
 *            con el valor "00000000" (datos vacíos).
 * 
 * Esta función es similar a init_disk, pero se puede llamar en cualquier momento
 * para limpiar todo el contenido del disco.
 */
void format_disk() {
    /*
     * RECORRER TODOS LOS SECTORES
     * Utiliza el mismo triple bucle anidado que init_disk para recorrer
     * toda la geometría del disco y sobrescribir cada sector.
     */
    for (int t = 0; t < TRACKS; t++) {                    // Recorrer todas las pistas
        for (int c = 0; c < CYLINDERS; c++) {             // Recorrer todos los cilindros
            for (int s = 0; s < SECTORS_PER_CYLINDER; s++) {  // Recorrer todos los sectores
                // Sobrescribir este sector con datos vacíos
                strcpy(hard_disk.data[t][c][s], "00000000");
            }
        }
    }
    
    // Registrar evento de formateo
    log_event(LOG_INFO, "Disco formateado");
}