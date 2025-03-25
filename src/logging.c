#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static FILE *log_file = NULL;

void log_init(const char *filename) {
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            perror("Failed to open log file");
            exit(EXIT_FAILURE);
        }
    }
}

void log_message(const char *format, ...) {
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

    // File output (if enabled)
    if (log_file) {
        va_start(args, format);  // Reinicia os argumentos para o arquivo
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
    }
}
