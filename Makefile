# Nome dos executáveis
TARGET = controller
TXGEN = TxGen

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -Wpedantic -pthread

# Fontes do controller
SRCS = logging.c controller.c miner.c
OBJS = $(SRCS:.c=.o)

# Fontes do TxGen
TXGEN_SRCS = txgen.c logging.c
TXGEN_OBJS = $(TXGEN_SRCS:.c=.o)

# Regra padrão compila ambos
all: $(TARGET) $(TXGEN)

# Compilar controller
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilar TxGen
$(TXGEN): $(TXGEN_OBJS)
	$(CC) $(CFLAGS) -o $(TXGEN) $(TXGEN_OBJS)

# Regra geral para .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza
clean:
	rm -f $(OBJS) $(TXGEN_OBJS) $(TARGET) $(TXGEN)
