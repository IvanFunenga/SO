#include "logging.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

void log_message(const char* format, ...) {
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    va_list args;
    va_start(args, format);
    
    // Print to console
    printf("[%s] ", timestamp);
    vprintf(format, args);
    printf("\n");
    
    // Print to log file
    FILE* log_file = fopen("DEIChain_log.txt", "a");
    if (log_file) {
        fprintf(log_file, "[%s] ", timestamp);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fclose(log_file);
    }
    
    va_end(args);
}