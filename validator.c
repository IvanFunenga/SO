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

int validate_block(TransactionBlock* block) {
    // 1. Verificar pow (a implementar)

    // 2. Verificar se o bloco referencia corretamente o último bloco da blockchain
    TransactionPool* pool = (TransactionPool*)tx_pool_ptr;

    // Verifica se o hash do bloco anterior é o mesmo que o ID do bloco atual na tx_pool
    char expected_previous_hash[HASH_SIZE];
    snprintf(expected_previous_hash, HASH_SIZE, "%d", pool->current_block_hash);  
    
    if (strcmp(block->previous_block_hash, expected_previous_hash) != 0) {
        log_message("ERROR: Bloco não está corretamente encadeado com o último bloco da tx_pool.");
        return -1;  // Indica que a validação falhou
    }

    // 3. Verificar se as transações ainda estão presentes na tx_pool
    for (int i = 0; i < global_config.transactions_per_block; i++) {
        if (!is_transaction_in_pool(block->transactions.id)) {
            log_message("ERROR: Transação %d não encontrada na pool.", block->transactions.id);
            return -1;  // Indica que a validação falhou
        }
    }

    // Se todas as verificações passarem, a validação foi bem-sucedida
    log_message("VALIDATOR: Bloco validado com sucesso (ID: %s)", block->txb_id);
    return 0;  // Sucesso
}

// Função para verificar se a transação está na pool
int is_transaction_in_pool(int tx_id) {
    TransactionPool* pool = (TransactionPool*)tx_pool_ptr;
    for (int i = 0; i < global_config.pool_size; i++) {
        if (!pool->transactions_pending_set[i].empty && pool->transactions_pending_set[i].id == tx_id) {
            return 1;  // Transação encontrada na pool
        }
    }
    return 0;  // Transação não encontrada na pool
}
