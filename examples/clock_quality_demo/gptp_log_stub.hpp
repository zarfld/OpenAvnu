/**
 * @file gptp_log_stub.hpp
 * @brief Simple logging stub for clock quality demo
 */

#ifndef GPTP_LOG_STUB_HPP
#define GPTP_LOG_STUB_HPP

#include <iostream>
#include <cstdio>
#include <cstdarg>

// Simple logging macros for demo purposes
#define GPTP_LOG_INFO(fmt, ...) \
    do { \
        std::printf("[INFO] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stdout); \
    } while(0)

#define GPTP_LOG_WARNING(fmt, ...) \
    do { \
        std::printf("[WARN] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stdout); \
    } while(0)

#define GPTP_LOG_ERROR(fmt, ...) \
    do { \
        std::printf("[ERROR] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stderr); \
    } while(0)

#define GPTP_LOG_DEBUG(fmt, ...) \
    do { \
        std::printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stdout); \
    } while(0)

// Stub for gptpLog function with correct signature
typedef enum {
    GPTP_LOG_LVL_CRITICAL,
    GPTP_LOG_LVL_ERROR,
    GPTP_LOG_LVL_EXCEPTION,
    GPTP_LOG_LVL_WARNING,
    GPTP_LOG_LVL_INFO,
    GPTP_LOG_LVL_STATUS,
    GPTP_LOG_LVL_DEBUG,
    GPTP_LOG_LVL_VERBOSE,
} GPTP_LOG_LEVEL;

// Function declarations to match gptp_log.hpp
void gptplogRegister(void);
void gptplogUnregister(void);
void gptpLog(GPTP_LOG_LEVEL level, const char *tag, const char *path, int line, const char *fmt, ...);

#endif // GPTP_LOG_STUB_HPP
