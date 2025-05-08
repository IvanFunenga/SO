CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread

# Ficheiros de origem
SRC_COMMON = logging.c miner.c
HDR_COMMON = logging.h miner.h

# Programa 1: controller
CONTROLLER_SRC = controller.c $(SRC_COMMON)
CONTROLLER_OBJ = $(CONTROLLER_SRC:.c=.o)
CONTROLLER_BIN = controller

# Programa 2: txgen
TXGEN_SRC = txgen.c logging.c
TXGEN_OBJ = $(TXGEN_SRC:.c=.o)
TXGEN_BIN = txgen

# Target principal
all: $(CONTROLLER_BIN) $(TXGEN_BIN)

# Compilação do controller (com -lrt)
$(CONTROLLER_BIN): $(CONTROLLER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lrt

# Compilação do txgen (com -lrt)
$(TXGEN_BIN): $(TXGEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lrt

# Limpar os ficheiros compilados
clean:
	rm -f *.o $(CONTROLLER_BIN) $(TXGEN_BIN)

# Evita que targets com nome de ficheiro sejam interpretados como ficheiros
.PHONY: all clean
