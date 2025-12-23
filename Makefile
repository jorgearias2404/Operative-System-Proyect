# Makefile para Sistema Operativo Virtual
CC = gcc
CFLAGS = -Wall -std=c99 -g -pthread
TARGET = sistema

# Todos los archivos .c
SRC = main.c \
      CONSOLE/console.c \
      CPU/cpu.c \
      DISK/disk.c \
      DMA/dma.c \
      INTERRUPTS/interrupts.c \
      LOGGER/logger.c \
      MEMORY/memory.c \
      REGISTERS/registers.c

# Todos los archivos .o
OBJ = $(SRC:.c=.o)

# Regla principal
all: $(TARGET)

# Compilar ejecutable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

# Compilar cada .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpiar archivos compilados
clean:
	rm -f $(TARGET) $(OBJ) system.log

# Para recompilar todo
rebuild: clean all

# Ayuda
help:
	@echo "Comandos disponibles:"
	@echo "  make        - Compilar el sistema"
	@echo "  make clean  - Eliminar archivos compilados"
	@echo "  make rebuild- Recompilar desde cero"
	@echo "  make help   - Mostrar esta ayuda"

# Phony targets
.PHONY: all clean rebuild help