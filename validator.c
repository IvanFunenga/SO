#include "common.h"  
#include "logging.h"
#include <unistd.h>   
#include <fcntl.h>  

int fd = -1;

void open_fifo_for_reading() {
    fd = open(VALIDATOR_FIFO, O_RDONLY);
    if (fd == -1) {
        log_message("VALIDATOR: Failed to open FIFO for reading");
        exit(EXIT_FAILURE);  // Ou outro tipo de tratamento de erro adequado
    }
    log_message("VALIDATOR: FIFO opened successfully for reading");
}

void close_fifo() {
    if (fd != -1) {
        close(fd);
        log_message("VALIDATOR: FIFO closed successfully");
    }
}

void receive_block_from_miner() {

    if (fd == -1) {
        log_message("ERROR: FIFO is not open for reading");
        return;
    }

    // Definir o bloco que vai ser preenchido
    TransactionBlock block;

    // Tentar ler o bloco completo (tamanho da estrutura TransactionBlock)
    ssize_t bytes_read = read(fd, &block, sizeof(TransactionBlock));

    // Verificar se o número de bytes lidos é igual ao tamanho da estrutura do bloco
    if (bytes_read != sizeof(TransactionBlock)) {
        log_message("ERROR: Incomplete block received. Expected %zu bytes, got %zd", sizeof(TransactionBlock), bytes_read);
        return;  // Trate a falha como preferir
    }

    log_message("VALIDATOR: Block received from miner (ID: %s)", block.txb_id);

    // Processar o bloco (Exemplo: adicionar ao blockchain ou validar)
    process_received_block(&block);
}

