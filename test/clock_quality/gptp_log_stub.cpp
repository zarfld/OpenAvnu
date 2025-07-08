// Logging stub for unit tests
#include "gptp_log.hpp"
#include <iostream>
#include <cstdarg>

#ifndef GPTP_LOG_STUB_CPP
#define GPTP_LOG_STUB_CPP

void gptpLog(GPTP_LOG_LEVEL level, const char *tag, const char *path, int line, const char *fmt, ...) {
    // Simple console output for testing
    va_list args;
    va_start(args, fmt);
    std::cout << "[TEST LOG " << (tag ? tag : "UNKNOWN") << "] ";
    vprintf(fmt, args);
    std::cout << std::endl;
    va_end(args);
}

void gptplogRegister(void) {
    // Stub implementation
}

void gptplogUnregister(void) {
    // Stub implementation
}

#endif // GPTP_LOG_STUB_CPP
