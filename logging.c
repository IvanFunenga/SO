#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

static FILE *log_file = NULL;
static int log_initialized = 0;
static sem_t *log_sem = NULL;

#define LOG_SEM_NAME "/log_mutex"

void log_init(const char *filename) {
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }

        log_sem = sem_open(LOG_SEM_NAME, O_CREAT, 0644, 1);
        if (log_sem == SEM_FAILED) {
            perror("Failed to create semaphore for logging");
            fclose(log_file);
            exit(EXIT_FAILURE);
        }

        log_initialized = 1;
    }
}

void log_message(const char *format, ...) {
    if (!log_initialized) {
        fprintf(stderr, "ERRO: log_init() não foi chamado antes de log_message()\n");
        return;
    }

    sem_wait(log_sem); // lock

    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    va_list args;
    va_start(args, format);
    printf("[%s] ", timestamp);
    vprintf(format, args);
    printf("\n");
    va_end(args);

    if (log_file) {
        va_start(args, format);
        fprintf(log_file, "[%s] ", timestamp);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
        va_end(args);
    }

    sem_post(log_sem); // unlock
}

void log_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }

    if (log_sem != NULL) {
        sem_close(log_sem);
        sem_unlink(LOG_SEM_NAME);  // Remove do sistema
        log_sem = NULL;
    }

    log_initialized = 0;
}
