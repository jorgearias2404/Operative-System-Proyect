/*
 * Archivo de cabecera del módulo de disco del Sistema Operativo Virtual.
 * Define las estructuras de datos y prototipos de funciones para simular
 * un disco duro con geometría de pistas, cilindros y sectores.
 */

#ifndef DISK_H
#define DISK_H
/* 
 * Directivas de preprocesador para evitar inclusión múltiple.
 * Garantiza que este archivo solo se incluya una vez durante la compilación.
 */

#include "../types.h"  // Incluir types.h para constantes globales y tipos

/*
 * CONSTANTES DE CONFIGURACIÓN DEL DISCO
 * Definen la geometría del disco duro simulado.
 * 
 * TRACKS: Número de pistas (cabezales de lectura/escritura)
 * CYLINDERS: Número de cilindros (conjuntos de pistas alineadas verticalmente)
 * SECTORS_PER_CYLINDER: Número de sectores por cilindro
 * SECTOR_SIZE: Tamaño de cada sector en bytes (8 dígitos + 1 para terminador nulo)
 */
#define TRACKS 10                   // 10 pistas
#define CYLINDERS 10                // 10 cilindros
#define SECTORS_PER_CYLINDER 100    // 100 sectores por cilindro
#define SECTOR_SIZE 9               // 8 dígitos de datos + 1 para null terminator

/*
 * Estructura: HardDisk
 * Propósito: Representa el disco duro virtual completo con su geometría y datos.
 * 
 * La estructura simula un disco con organización tridimensional:
 * - Pistas (tracks): Posicionamiento radial (como anillos concéntricos)
 * - Cilindros (cylinders): Conjunto de pistas alineadas verticalmente
 * - Sectores (sectors): Divisiones angulares dentro de cada pista
 * 
 * Campos:
 *   data - Array 4D que almacena todos los sectores del disco
 *          [pista][cilindro][sector][dato] donde dato es cadena de 9 caracteres
 *   current_track - Pista actual donde está posicionado el cabezal
 *   current_cylinder - Cilindro actual
 *   current_sector - Sector actual
 * 
 * Ejemplo: Para acceder al sector 5 del cilindro 3 en pista 2:
 *   hard_disk.data[2][3][5]
 */
typedef struct {
    // Almacenamiento de datos del disco
    // Dimensiones: [TRACKS][CYLINDERS][SECTORS_PER_CYLINDER][SECTOR_SIZE + 1]
    // El +1 es para el carácter nulo terminador de cadena
    char data[TRACKS][CYLINDERS][SECTORS_PER_CYLINDER][SECTOR_SIZE + 1];
    
    // Posición actual del cabezal de lectura/escritura
    int current_track;      // Pista actual (0 a TRACKS-1)
    int current_cylinder;   // Cilindro actual (0 a CYLINDERS-1)
    int current_sector;     // Sector actual (0 a SECTORS_PER_CYLINDER-1)
} HardDisk;

/*
 * PROTOTIPOS DE FUNCIONES - Interfaz pública del módulo de disco
 */

/* 
 * Función: init_disk
 * Propósito: Inicializar el disco, llenando todos los sectores con ceros
 *            y estableciendo la posición inicial del cabezal.
 */
void init_disk();

/* 
 * Función: read_sector
 * Parámetros:
 *   track - número de pista (0 a TRACKS-1)
 *   cylinder - número de cilindro (0 a CYLINDERS-1)
 *   sector - número de sector (0 a SECTORS_PER_CYLINDER-1)
 *   buffer - buffer donde se copiarán los datos leídos (debe tener al menos SECTOR_SIZE bytes)
 * Propósito: Leer un sector específico del disco y copiar su contenido al buffer.
 */
void read_sector(int track, int cylinder, int sector, char* buffer);

/* 
 * Función: write_sector
 * Parámetros:
 *   track - número de pista (0 a TRACKS-1)
 *   cylinder - número de cilindro (0 a CYLINDERS-1)
 *   sector - número de sector (0 a SECTORS_PER_CYLINDER-1)
 *   data - cadena de datos a escribir (debe tener 8 dígitos + null terminator)
 * Propósito: Escribir datos en un sector específico del disco.
 */
void write_sector(int track, int cylinder, int sector, const char* data);

/* 
 * Función: disk_info
 * Propósito: Mostrar información sobre la configuración y estado del disco
 *            en la consola. Incluye geometría, capacidad y posición actual.
 */
void disk_info();

/* 
 * Función: format_disk
 * Propósito: Formatear el disco, sobrescribiendo todos los sectores con ceros.
 *            Equivalente a un formateo de bajo nivel.
 */
void format_disk();

/*
 * DECLARACIÓN DE VARIABLE GLOBAL EXTERNA
 * Permite que otros módulos accedan al objeto del disco.
 */
extern HardDisk hard_disk;  // Instancia global del disco

#endif /* DISK_H */