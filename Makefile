# Makefile que funciona seguro
CC = gcc
CFLAGS = -Wall -std=c99 -g -I.
TARGET = sistema.exe

all: sistema.exe

sistema.exe: main.o console.o cpu.o disk.o dma.o interrupts.o logger.o memory.o registers.o
	$(CC) $(CFLAGS) -o sistema.exe main.o console.o cpu.o disk.o dma.o interrupts.o logger.o memory.o registers.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

console.o: CONSOLE/console.c
	$(CC) $(CFLAGS) -c CONSOLE/console.c -o console.o

cpu.o: CPU/cpu.c
	$(CC) $(CFLAGS) -c CPU/cpu.c -o cpu.o

disk.o: DISK/disk.c
	$(CC) $(CFLAGS) -c DISK/disk.c -o disk.o

dma.o: DMA/dma.c
	$(CC) $(CFLAGS) -c DMA/dma.c -o dma.o

interrupts.o: INTERRUPTS/interrupts.c
	$(CC) $(CFLAGS) -c INTERRUPTS/interrupts.c -o interrupts.o

logger.o: LOGGER/logger.c
	$(CC) $(CFLAGS) -c LOGGER/logger.c -o logger.o

memory.o: MEMORY/memory.c
	$(CC) $(CFLAGS) -c MEMORY/memory.c -o memory.o

registers.o: REGISTERS/registers.c
	$(CC) $(CFLAGS) -c REGISTERS/registers.c -o registers.o

clean:
	@echo Limpiando...
	@if exist sistema.exe del sistema.exe
	@if exist *.o del *.o
	@echo Hecho.

run: sistema.exe
	sistema.exe

.PHONY: all clean run