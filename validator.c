#include "common.h"  
#include <sys/mman.h>
#include "logging.h"
#include <unistd.h>   
#include <fcntl.h>  
#include <signal.h>

int fd = -1;
static volatile sig_atomic_t running_validator = 1;

void handle_sigint_validator(int sig) {
    (void)sig;
    running_validator = 0;  // Mudar a variável de controle apenas para o validator
    log_message("INFO: SIGINT received by validator process, stopping validation...");
}

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

void receive_block_from_miner(TransactionBlock* block) {

    // Tentar ler o bloco completo (tamanho da estrutura TransactionBlock)
    ssize_t bytes_read = read(fd, block, sizeof(TransactionBlock));  // Preenche o bloco passado

    // Verificar se o número de bytes lidos é igual ao tamanho da estrutura do bloco
    if (bytes_read != sizeof(TransactionBlock)) {
        log_message("ERROR: Incomplete block received. Expected %zu bytes, got %zd", sizeof(TransactionBlock), bytes_read);
        return;  // Se a leitura falhar, tenta ler o próximo bloco
    }

    log_message("VALIDATOR: Block received from miner (ID: %s)", block->txb_id);
    log_message("VALIDATOR: Previous Block Hash: %s", block->previous_block_hash);
    log_message("VALIDATOR: Timestamp: %ld", block->timestamp);
    log_message("VALIDATOR: Nonce: %u", block->nonce);

    log_message("VALIDATOR: Printing transactions in the block:");
    for (int i = 0; i < 10; i++) {
        if (block->transactions[i].id != 0) {
            Transaction* t = &block->transactions[i];
            log_message("Transaction %d: ID = %d, Reward = %d, From = %d, To = %d, Value = %d, Age = %d",
                        i + 1, t->id, t->reward, t->sender_id, t->receiver_id, t->value, t->age);
        }
    }
}

int validate_block(TransactionBlock* block) {
    // 1. Verificar pow (a implementar)

    // 2. Verificar se o bloco referencia corretamente o último bloco da blockchain
    TransactionPool* pool = (TransactionPool*)tx_pool_ptr;

    // Verifica se o hash do bloco anterior é o mesmo que o ID do bloco atual na tx_pool
    char expected_previous_hash[HASH_SIZE];
    snprintf(expected_previous_hash, HASH_SIZE, "%s", pool->current_block_hash);  

    if (strcmp(block->previous_block_hash, expected_previous_hash) != 0) {
        log_message("ERROR: Bloco não está corretamente encadeado com o último bloco da tx_pool.");
        return -1;  // Indica que a validação falhou
    }

    // 3. Verificar se as transações ainda estão presentes na tx_pool
    for (int i = 0; i < global_config.transactions_per_block; i++) {
        if (!is_transaction_in_pool(block->transactions[i].id)) {  // Acesse com o índice do array
            log_message("ERROR: Transação %d não encontrada na pool.", block->transactions[i].id);
            return -1;  // Indica que a validação falhou
        }
    }

    // Se todas as verificações passarem, a validação foi bem-sucedida
    log_message("VALIDATOR: Bloco validado com sucesso (ID: %s)", block->txb_id);
    return 0;  // Sucesso
}

void cleanup_validator_resources() {
    // Detach from shared memory
    if (tx_pool_ptr != NULL) {
        munmap(tx_pool_ptr, sizeof(TransactionPool) + sizeof(Transaction) * global_config.pool_size);
        tx_pool_ptr = NULL;
    }

    // Close other resources (e.g., semaphores) if the validator opened them
}
// Continuously listen for blocks from the miner

void listen_for_blocks(Config* config) {
    (void)config;
    log_message("VALIDATOR: Waiting for blocks from miner...");

    // Abrir FIFO para leitura (só abrir uma vez)
    open_fifo_for_reading();
    signal(SIGINT, handle_sigint_validator);

    // Continuamente receber blocos até que uma condição de parada seja atendida
    while (running_validator) {
        // Log de progresso para confirmar que o validador está aguardando por blocos
        log_message("VALIDATOR: Waiting for the next block...");

        // Receber um bloco do minerador
        TransactionBlock block;
        receive_block_from_miner(&block);

        // Verificar se o bloco foi validado
        if (validate_block(&block) == 0) {
            // Lógica do que fazer caso o bloco seja validado
            log_message("VALIDATOR: Block validated successfully (ID: %s)", block.txb_id);
            // Exemplo: Adicionar o bloco à blockchain ou processar mais
            // add_block_to_blockchain(&block);
        } else {
            // Lógica para se o bloco não for validado
            log_message("VALIDATOR: Block validation failed (ID: %s)", block.txb_id);
            // Exemplo: Rejeitar o bloco ou pedir para o minerador corrigir
            // reject_block(&block);
        }

        // Opcional: Adicionar um pequeno atraso para evitar sobrecarga de logs ou uso excessivo de CPU
        sleep(1);  // Descomente se necessário para reduzir a carga
    }

    // Fechar o FIFO quando terminar (neste caso, o programa pode ser finalizado ou parado)
    close_fifo();
    cleanup_validator_resources();
}
