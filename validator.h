#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "common.h"  
#include "logging.h"
#include <unistd.h>
#include <fcntl.h>

// Define the FIFO path
#define VALIDATOR_FIFO "/tmp/validator_fifo"

// Function to open the FIFO for reading
void open_fifo_for_reading();

// Function to close the FIFO after use
void close_fifo();

// Function to receive a block from the miner via FIFO
void receive_block_from_miner();

// Function to listen for incoming blocks from the FIFO
void listen_for_blocks();

// Signal handler to stop the validator gracefully
void handle_signal(int sig);

// Function to initialize and run the validator process
void run_validator_process();

#endif // VALIDATOR_H
