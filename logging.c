#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static FILE *log_file = NULL;
static int log_initialized = 0; // Flag para verificar inicialização

void log_init(const char *filename) {
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }
        log_initialized = 1; // Marca como inicializado
    }
}

void log_message(const char *format, ...) {
    if (!log_initialized) {
        fprintf(stderr, "ERRO: log_init() não foi chamado antes de log_message()\n");
        return;
    }

    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Console output
    va_list args;
    va_start(args, format);
    printf("[%s] ", timestamp);
    vprintf(format, args);
    printf("\n");
    va_end(args);

    // File output
    if (log_file) {
        va_start(args, format);
        fprintf(log_file, "[%s] ", timestamp);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
        va_end(args);
    }
}

void log_close(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
        log_initialized = 0; // Reseta flag
    }
}
