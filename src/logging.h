#ifndef LOGGING_H
#define LOGGING_H

#include "common.h"

void log_init(const char *filename);
void log_message(const char *format, ...);
void log_close(void);

#endif // LOGGING_H
