#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "common.h"  
#include "logging.h"
#include <unistd.h>
#include <fcntl.h>

void receive_block_from_miner(TransactionBlock* block); 
void listen_for_blocks(Config* config); 

#endif // VALIDATOR_H
