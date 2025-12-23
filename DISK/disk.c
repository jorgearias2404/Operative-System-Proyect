#include "disk.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>

HardDisk hard_disk;

void init_disk() {
    // Inicializar posiciones actuales
    hard_disk.current_track = 0;
    hard_disk.current_cylinder = 0;
    hard_disk.current_sector = 0;
    
    // Inicializar todos los sectores con datos vacíos
    for (int t = 0; t < TRACKS; t++) {
        for (int c = 0; c < CYLINDERS; c++) {
            for (int s = 0; s < SECTORS_PER_CYLINDER; s++) {
                strcpy(hard_disk.data[t][c][s], "00000000");
            }
        }
    }
    
    log_event(LOG_INFO, "Disco inicializado: %d pistas, %d cilindros, %d sectores por cilindro",
              TRACKS, CYLINDERS, SECTORS_PER_CYLINDER);
}

void read_sector(int track, int cylinder, int sector, char* buffer) {
    if (track < 0 || track >= TRACKS || 
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        log_event(LOG_ERROR, "Coordenadas de disco inválidas: T=%d, C=%d, S=%d", 
                  track, cylinder, sector);
        strcpy(buffer, "ERROR");
        return;
    }
    
    strcpy(buffer, hard_disk.data[track][cylinder][sector]);
    log_event(LOG_DEBUG, "Lectura de disco: T=%d, C=%d, S=%d -> %s", 
              track, cylinder, sector, buffer);
}

void write_sector(int track, int cylinder, int sector, const char* data) {
    if (track < 0 || track >= TRACKS || 
        cylinder < 0 || cylinder >= CYLINDERS ||
        sector < 0 || sector >= SECTORS_PER_CYLINDER) {
        log_event(LOG_ERROR, "Coordenadas de disco inválidas: T=%d, C=%d, S=%d", 
                  track, cylinder, sector);
        return;
    }
    
    // Verificar que los datos tengan el formato correcto
    if (strlen(data) != SECTOR_SIZE - 1) {  // -1 porque no contamos el null terminator
        log_event(LOG_WARNING, "Datos de tamaño incorrecto para sector: %s", data);
    }
    
    strcpy(hard_disk.data[track][cylinder][sector], data);
    log_event(LOG_DEBUG, "Escritura en disco: T=%d, C=%d, S=%d <- %s", 
              track, cylinder, sector, data);
}

void disk_info() {
    printf("\n=== INFORMACIÓN DEL DISCO ===\n");
    printf("Pistas: %d\n", TRACKS);
    printf("Cilindros: %d\n", CYLINDERS);
    printf("Sectores por cilindro: %d\n", SECTORS_PER_CYLINDER);
    printf("Sector size: %d caracteres\n", SECTOR_SIZE - 1);
    printf("Capacidad total: %d sectores\n", TRACKS * CYLINDERS * SECTORS_PER_CYLINDER);
    printf("Posición actual: T=%d, C=%d, S=%d\n",
           hard_disk.current_track,
           hard_disk.current_cylinder,
           hard_disk.current_sector);
}

void format_disk() {
    for (int t = 0; t < TRACKS; t++) {
        for (int c = 0; c < CYLINDERS; c++) {
            for (int s = 0; s < SECTORS_PER_CYLINDER; s++) {
                strcpy(hard_disk.data[t][c][s], "00000000");
            }
        }
    }
    log_event(LOG_INFO, "Disco formateado");
}