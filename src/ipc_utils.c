#include "ipc_utils.h"
#include "logging.h"        // For log_message
#include <sys/ipc.h>        // For semget, msgget
#include <sys/sem.h>        // For semctl
#include <sys/msg.h>        // For msgctl
#include <sys/stat.h>       // For mkfifo
#include <fcntl.h>          // For open, close
#include <unistd.h>         // For unlink
#include <stdio.h>          // For perror
#include <stdlib.h>         // For exit
#include <errno.h>          // For errno and EEXIST

// Global variables for IPC resources
int sem_id;              // Semaphore ID
int msg_queue_id;        // Message queue ID
const char* named_pipe = "/tmp/validator_input"; // Named pipe path

// Initialize IPC mechanisms
void initialize_ipc() {
    log_message("INFO: Initializing IPC mechanisms.");

    // Create a semaphore
    sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the semaphore value to 1 (unlocked)
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("semctl failed");
        exit(EXIT_FAILURE);
    }

    // Create a message queue
    msg_queue_id = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    // Create a named pipe (FIFO)
    if (mkfifo(named_pipe, 0666) == -1) {
        if (errno != EEXIST) { // Ignore error if the pipe already exists
            perror("mkfifo failed");
            exit(EXIT_FAILURE);
        }
    }

    log_message("INFO: IPC mechanisms initialized successfully.");
}

// Clean up IPC resources
void cleanup_ipc() {
    log_message("INFO: Cleaning up IPC resources.");

    // Remove the semaphore
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl (IPC_RMID) failed");
    }

    // Remove the message queue
    if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
        perror("msgctl (IPC_RMID) failed");
    }

    // Remove the named pipe
    if (unlink(named_pipe) == -1) {
        perror("unlink (named pipe) failed");
    }

    log_message("INFO: IPC resources cleaned up successfully.");
}