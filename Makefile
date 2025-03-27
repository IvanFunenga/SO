# Nome do executável
TARGET = controller

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -Wpedantic

# Arquivos fonte (agora inclui miner.c)
SRCS = logging.c controller.c miner.c
OBJS = $(SRCS:.c=.o)

# Regra padrão para compilar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos compilados
clean:
	rm -f $(OBJS) $(TARGET)
