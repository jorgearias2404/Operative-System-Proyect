#ifndef DISK_H
#define DISK_H

#include "../types.h"  // Incluir types.h en lugar de memory.h

#define TRACKS 10
#define CYLINDERS 10
#define SECTORS_PER_CYLINDER 100
#define SECTOR_SIZE 9  // 8 dígitos + signo

typedef struct {
    char data[TRACKS][CYLINDERS][SECTORS_PER_CYLINDER][SECTOR_SIZE + 1]; // +1 para null terminator
    int current_track;
    int current_cylinder;
    int current_sector;
} HardDisk;

// Inicializar disco
void init_disk();
// Leer sector
void read_sector(int track, int cylinder, int sector, char* buffer);
// Escribir sector
void write_sector(int track, int cylinder, int sector, const char* data);
// Mostrar información del disco
void disk_info();
// Formatear disco
void format_disk();

extern HardDisk hard_disk;

#endif