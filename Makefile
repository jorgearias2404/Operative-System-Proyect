# Makefile para Windows con MinGW
CC = gcc
CFLAGS = -Wall -std=c99 -g -pthread -I. -ICONSOLE -ICPU -IDISK -IDMA -IINTERRUPTS -ILOGGER -IMEMORY -IREGISTERS
TARGET = sistema.exe

# Todos los archivos .c
SRCS = main.c \
       CONSOLE/console.c \
       CPU/cpu.c \
       DISK/disk.c \
       DMA/dma.c \
       INTERRUPTS/interrupts.c \
       LOGGER/logger.c \
       MEMORY/memory.c \
       REGISTERS/registers.c

# Todos los archivos .o
OBJS = $(SRCS:.c=.o)

# Regla principal
all: $(TARGET)

# Compilar ejecutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Regla genérica para .c -> .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar
clean:
	del /Q $(TARGET) $(OBJS) system.log 2>nul

# Ayuda
help:
	@echo Comandos:
	@echo   make      - Compilar
	@echo   make clean - Limpiar
	@echo   sistema.exe - Ejecutar

.PHONY: all clean help