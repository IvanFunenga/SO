CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread

# Ficheiros de origem
SRC_COMMON = logging.c miner.c common.c
HDR_COMMON = logging.h miner.h common.h

# Add validator.c to the source files
SRC_VALIDATOR = validator.c  # Add validator.c to the list of source files

# Programa 1: controller
CONTROLLER_SRC = controller.c $(SRC_COMMON) $(SRC_VALIDATOR)  # Include validator.c here
CONTROLLER_OBJ = $(CONTROLLER_SRC:.c=.o)
CONTROLLER_BIN = controller

# Programa 2: txgen
TXGEN_SRC = txgen.c logging.c common.c
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
