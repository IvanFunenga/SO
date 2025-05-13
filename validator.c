#include "common.h"  
#include "logging.h"
#include "validator.h"
#include <unistd.h>   
#include <fcntl.h>  
#include <stdlib.h>


int fd = -1;  // File descriptor for reading from FIFO

// Open the FIFO for reading (used by the validator)
void open_fifo_for_reading() {
    fd = open(VALIDATOR_FIFO, O_RDONLY);
    if (fd == -1) {
        log_message("VALIDATOR: Failed to open FIFO for reading");
        perror("Error opening FIFO");
        exit(EXIT_FAILURE);  // Or handle the error as appropriate
    }
    log_message("VALIDATOR: FIFO opened successfully for reading");
}

// Close the FIFO when done
void close_fifo() {
    if (fd != -1) {
        close(fd);
        log_message("VALIDATOR: FIFO closed successfully");
    }
}

// Receive a block from the miner through the FIFO
void receive_block_from_miner() {
    if (fd == -1) {
        log_message("ERROR: FIFO is not open for reading");
        return;
    }

    // Define the block that will be filled by the received data
    TransactionBlock block;

    // Try to read the block completely (size of TransactionBlock)
    ssize_t bytes_read = read(fd, &block, sizeof(TransactionBlock));

    // If the number of bytes read is not equal to the block size, it's an error
    if (bytes_read != sizeof(TransactionBlock)) {
        log_message("ERROR: Incomplete block received. Expected %zu bytes, got %zd", sizeof(TransactionBlock), bytes_read);
        return;  // Handle failure appropriately
    }

    // Successfully received the block, log its ID
    log_message("VALIDATOR: Block received from miner (ID: %s)", block.txb_id);
    log_message("VALIDATOR: Previous Block Hash: %s", block.previous_block_hash);
    log_message("VALIDATOR: Timestamp: %ld", block.timestamp);
    log_message("VALIDATOR: Nonce: %u", block.nonce);

    // Print the transactions inside the block
    log_message("VALIDATOR: Printing transactions in the block:");

    for (int i = 0; i < 50; i++) {  // Assuming the block can have up to 50 transactions
        // Check if the transaction is not empty
        if (block.transactions[i].id != 0) {  // Only print non-empty transactions
            Transaction *t = &block.transactions[i];

            // Log the transaction details
            log_message("Transaction %d: ID = %d, Reward = %d, From = %d, To = %d, Value = %d, Age = %d",
                        i + 1, t->id, t->reward, t->sender_id, t->receiver_id, t->value, t->age);
        }
    }

    // Process the received block (e.g., add to blockchain or validate)
    //process_received_block(&block);
}


// Continuously listen for blocks from the miner
void listen_for_blocks(Config* config) {
    log_message("VALIDATOR: Waiting for blocks from miner...");

    // Open FIFO for reading (open only once)
    open_fifo_for_reading();

    // Continuously receive blocks until a stop condition is met
    while (1) {
        // Log progress to confirm the validator is waiting for blocks
        log_message("VALIDATOR: Waiting for the next block...");

        // Receive a block from the miner
        receive_block_from_miner();

        // Optional: Add a small delay to avoid flooding logs or excessive CPU usage
         sleep(1);  // Uncomment if needed to reduce load
    }

    // Close the FIFO when done (in this case, the program might be killed or stopped)
    close_fifo();
}
