#ifndef CONTROLLER_H
#define CONTROLLER_H

// Estrutura de configuração
typedef struct {
    int num_miners;
    int pool_size;
    int transactions_per_block;  // Nome corrigido para consistência
    int blockchain_blocks;
} Config;

void load_config(const char *filename, Config *config);

#endif