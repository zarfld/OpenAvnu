/**
 * @file gptp_log_stub.cpp
 * @brief Simple logging stub implementation for clock quality demo
 */

#include "gptp_log_stub.hpp"
#include <cstdarg>

// Stub implementations for gPTP logging functions
void gptplogRegister(void) {
    // Stub - do nothing for demo
}

void gptplogUnregister(void) {
    // Stub - do nothing for demo
}

void gptpLog(GPTP_LOG_LEVEL level, const char *tag, const char *path, int line, const char *fmt, ...) {
    const char* level_str = "INFO";
    switch(level) {
        case GPTP_LOG_LVL_CRITICAL: level_str = "CRITICAL"; break;
        case GPTP_LOG_LVL_ERROR: level_str = "ERROR"; break;
        case GPTP_LOG_LVL_EXCEPTION: level_str = "EXCEPTION"; break;
        case GPTP_LOG_LVL_WARNING: level_str = "WARNING"; break;
        case GPTP_LOG_LVL_INFO: level_str = "INFO"; break;
        case GPTP_LOG_LVL_STATUS: level_str = "STATUS"; break;
        case GPTP_LOG_LVL_DEBUG: level_str = "DEBUG"; break;
        case GPTP_LOG_LVL_VERBOSE: level_str = "VERBOSE"; break;
    }
    
    if (tag) {
        std::printf("[%s] %s: ", level_str, tag);
    } else {
        std::printf("[%s] ", level_str);
    }
    
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);
    
    std::printf("\n");
    std::fflush(stdout);
}
