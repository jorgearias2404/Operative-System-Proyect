// types.h en la raíz
#ifndef TYPES_H
#define TYPES_H

// Definición de Word (8 dígitos decimales)
typedef struct {
    char data[9];  // 8 dígitos + null terminator
} Word;

// Constantes globales
#define MEMORY_SIZE 2000
#define OS_RESERVED 300
#define TRACKS 10
#define CYLINDERS 10
#define SECTORS_PER_CYLINDER 100
#define SECTOR_SIZE 9  // 8 dígitos + null terminator

// Modos de operación
#define USER_MODE 0
#define KERNEL_MODE 1

// Estados de interrupciones
#define INTERRUPTS_DISABLED 0
#define INTERRUPTS_ENABLED 1

#endif