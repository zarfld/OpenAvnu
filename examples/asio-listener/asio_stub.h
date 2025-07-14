/*
ASIO Stub Implementation for Development
This file provides stub definitions when the full ASIO SDK is not available
*/

#ifndef ASIO_STUB_H
#define ASIO_STUB_H

#ifdef ASIO_STUB

#include <windows.h>

// ASIO types and constants (simplified)
typedef long ASIOError;
typedef double ASIOSampleRate;
typedef long ASIOSampleType;
typedef long ASIOBool;
typedef struct ASIOTime ASIOTime;
typedef struct ASIOTimeInfo ASIOTimeInfo;
typedef struct ASIODriverInfo ASIODriverInfo;
typedef struct ASIOChannelInfo ASIOChannelInfo;
typedef struct ASIOBufferInfo ASIOBufferInfo;
typedef struct ASIOCallbacks ASIOCallbacks;

// ASIO constants
#define ASE_OK                0
#define ASE_SUCCESS          ASE_OK
#define ASE_NotPresent       -1000
#define ASE_HWMalfunction    -999
#define ASE_InvalidParameter -998
#define ASE_InvalidMode      -997
#define ASE_SPNotAdvancing   -996
#define ASE_NoClock          -995
#define ASE_NoMemory         -994

#define ASIOFalse 0
#define ASIOTrue  1

// Sample types
#define ASIOSTInt16LSB   16
#define ASIOSTInt24LSB   17  
#define ASIOSTInt32LSB   18
#define ASIOSTFloat32LSB 23
#define ASIOSTFloat64LSB 24

// Message selectors
#define kAsioSelectorSupported     1
#define kAsioEngineVersion         2
#define kAsioResetRequest          3
#define kAsioBufferSizeChange      4
#define kAsioResyncRequest         5
#define kAsioLatenciesChanged      6
#define kAsioSupportsTimeInfo      7
#define kAsioSupportsTimeCode      8
#define kAsioSupportsInputMonitor  9

// Structures
struct ASIODriverInfo {
    long asioVersion;
    long driverVersion;
    char name[32];
    char errorMessage[124];
    void* sysRef;
};

struct ASIOChannelInfo {
    long channel;
    ASIOBool isInput;
    ASIOBool isActive;
    long channelGroup;
    ASIOSampleType type;
    char name[32];
};

struct ASIOBufferInfo {
    ASIOBool isInput;
    long channelNum;
    void* buffers[2];
};

struct ASIOCallbacks {
    void (*bufferSwitch)(long doubleBufferIndex, ASIOBool directProcess);
    void (*sampleRateDidChange)(ASIOSampleRate sRate);
    long (*asioMessage)(long selector, long value, void* message, double* opt);
    ASIOTime* (*bufferSwitchTimeInfo)(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
};

struct ASIOTimeInfo {
    double speed;
    ASIOSampleRate sampleRate;
    long flags;
};

struct ASIOTime {
    long reserved[4];
    ASIOTimeInfo timeInfo;
    long reserved2[8];
};

// ASIO function prototypes (stubs)
ASIOError ASIOInit(ASIODriverInfo* info);
ASIOError ASIOExit(void);
ASIOError ASIOStart(void);
ASIOError ASIOStop(void);
ASIOError ASIOGetChannels(long* numInputChannels, long* numOutputChannels);
ASIOError ASIOGetBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity);
ASIOError ASIOGetSampleRate(ASIOSampleRate* currentRate);
ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate);
ASIOError ASIOGetChannelInfo(ASIOChannelInfo* info);
ASIOError ASIOCreateBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks);
ASIOError ASIODisposeBuffers(void);
ASIOError ASIOControlPanel(void);
ASIOError ASIOFuture(long selector, void* params);
ASIOError ASIOOutputReady(void);

// Driver loading functions (stubs)
ASIOBool loadAsioDriver(char* name);
void removeCurrentDriver(void);
void getDriverNames(char*** names, long* numDrivers);

// Stub implementations
static char* stub_driver_names[] = {"ASIO Stub Driver", "Windows Audio", NULL};
static char stub_driver_name[32] = "ASIO Stub Driver";
static long stub_num_drivers = 2;

ASIOError ASIOInit(ASIODriverInfo* info) {
    if (!info) return ASE_InvalidParameter;
    info->asioVersion = 2;
    info->driverVersion = 1;
    strcpy(info->name, stub_driver_name);
    strcpy(info->errorMessage, "ASIO Stub - Development Only");
    printf("ASIO STUB: ASIOInit() called\n");
    return ASE_OK;
}

ASIOError ASIOExit(void) {
    printf("ASIO STUB: ASIOExit() called\n");
    return ASE_OK;
}

ASIOError ASIOStart(void) {
    printf("ASIO STUB: ASIOStart() called\n");
    return ASE_OK;
}

ASIOError ASIOStop(void) {
    printf("ASIO STUB: ASIOStop() called\n");
    return ASE_OK;
}

ASIOError ASIOGetChannels(long* numInputChannels, long* numOutputChannels) {
    if (!numInputChannels || !numOutputChannels) return ASE_InvalidParameter;
    *numInputChannels = 2;
    *numOutputChannels = 2;
    printf("ASIO STUB: ASIOGetChannels() - 2 in, 2 out\n");
    return ASE_OK;
}

ASIOError ASIOGetBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity) {
    if (!minSize || !maxSize || !preferredSize || !granularity) return ASE_InvalidParameter;
    *minSize = 64;
    *maxSize = 2048;
    *preferredSize = 256;
    *granularity = 64;
    printf("ASIO STUB: ASIOGetBufferSize() - preferred: 256\n");
    return ASE_OK;
}

ASIOError ASIOGetSampleRate(ASIOSampleRate* currentRate) {
    if (!currentRate) return ASE_InvalidParameter;
    *currentRate = 48000.0;
    printf("ASIO STUB: ASIOGetSampleRate() - 48000 Hz\n");
    return ASE_OK;
}

ASIOError ASIOSetSampleRate(ASIOSampleRate sampleRate) {
    printf("ASIO STUB: ASIOSetSampleRate() - %.0f Hz\n", sampleRate);
    return ASE_OK;
}

ASIOError ASIOCanSampleRate(ASIOSampleRate sampleRate) {
    printf("ASIO STUB: ASIOCanSampleRate() - %.0f Hz\n", sampleRate);
    return ASE_OK;
}

ASIOError ASIOGetChannelInfo(ASIOChannelInfo* info) {
    if (!info) return ASE_InvalidParameter;
    info->isActive = ASIOTrue;
    info->channelGroup = 1;
    info->type = ASIOSTFloat32LSB;
    sprintf(info->name, "Stub %s %ld", info->isInput ? "In" : "Out", info->channel);
    printf("ASIO STUB: ASIOGetChannelInfo() - %s\n", info->name);
    return ASE_OK;
}

ASIOError ASIOCreateBuffers(ASIOBufferInfo* bufferInfos, long numChannels, long bufferSize, ASIOCallbacks* callbacks) {
    if (!bufferInfos || !callbacks) return ASE_InvalidParameter;
    
    printf("ASIO STUB: ASIOCreateBuffers() - %ld channels, %ld samples\n", numChannels, bufferSize);
    
    // Allocate dummy buffers
    for (long i = 0; i < numChannels; i++) {
        bufferInfos[i].buffers[0] = malloc(bufferSize * sizeof(float));
        bufferInfos[i].buffers[1] = malloc(bufferSize * sizeof(float));
        if (!bufferInfos[i].buffers[0] || !bufferInfos[i].buffers[1]) {
            return ASE_NoMemory;
        }
        memset(bufferInfos[i].buffers[0], 0, bufferSize * sizeof(float));
        memset(bufferInfos[i].buffers[1], 0, bufferSize * sizeof(float));
    }
    
    return ASE_OK;
}

ASIOError ASIODisposeBuffers(void) {
    printf("ASIO STUB: ASIODisposeBuffers() called\n");
    return ASE_OK;
}

ASIOError ASIOControlPanel(void) {
    printf("ASIO STUB: ASIOControlPanel() called\n");
    return ASE_OK;
}

ASIOError ASIOFuture(long selector, void* params) {
    printf("ASIO STUB: ASIOFuture() called\n");
    return ASE_OK;
}

ASIOError ASIOOutputReady(void) {
    return ASE_OK;
}

ASIOBool loadAsioDriver(char* name) {
    printf("ASIO STUB: loadAsioDriver(%s)\n", name ? name : "NULL");
    if (name) {
        strncpy(stub_driver_name, name, sizeof(stub_driver_name) - 1);
    }
    return ASIOTrue;
}

void removeCurrentDriver(void) {
    printf("ASIO STUB: removeCurrentDriver()\n");
}

void getDriverNames(char*** names, long* numDrivers) {
    printf("ASIO STUB: getDriverNames()\n");
    if (names) *names = stub_driver_names;
    if (numDrivers) *numDrivers = stub_num_drivers;
}

#endif // ASIO_STUB

#endif // ASIO_STUB_H
