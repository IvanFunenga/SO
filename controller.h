#ifndef CONTROLLER_H
#define CONTROLLER_H

// Estrutura de configuração
typedef struct {
    int num_miners;
    int pool_size;
    int transactions_per_block;  // Nome corrigido para consistência
    int blockchain_blocks;
} Config;

typedef struct {
    int id;
    int reward;
    int sender_id;
    int receiver_id;
    int value;
    time_t timestamp;
    int aging;
} Transaction;


void load_config(const char *filename, Config *config);

#endif