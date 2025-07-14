/*
Copyright (c) 2025, Intel Corporation
All rights reserved.

ASIO Audio Listener Example for OpenAvnu
AVB/TSN Audio Streaming with ASIO Professional Audio Interface

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

// ASIO SDK includes
#ifdef ASIO_STUB
#include "asio_stub.h"
#else
#include "asio.h"
#include "asiodrivers.h"
#endif

// Network includes
#ifdef PCAP_STUB
#include "pcap_stub.h"
#else
#include <pcap.h>
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#define VERSION_STR "1.0"

// AVB/1722 Protocol definitions
#define ETHERNET_HEADER_SIZE (18)
#define SEVENTEEN22_HEADER_PART1_SIZE (4)
#define STREAM_ID_SIZE (8)
#define SEVENTEEN22_HEADER_PART2_SIZE (10)
#define SIX1883_HEADER_SIZE (10)
#define HEADER_SIZE (ETHERNET_HEADER_SIZE + SEVENTEEN22_HEADER_PART1_SIZE + \
                     STREAM_ID_SIZE + SEVENTEEN22_HEADER_PART2_SIZE + SIX1883_HEADER_SIZE)

// Audio configuration
#define MAX_CHANNELS 8
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_BUFFER_SIZE 256

// ASIO callback and driver management
typedef struct {
    ASIODriverInfo driverInfo;
    ASIOChannelInfo channelInfos[MAX_CHANNELS];
    ASIOBufferInfo bufferInfos[MAX_CHANNELS];
    ASIOCallbacks callbacks;
    long inputChannels;
    long outputChannels;
    long bufferSize;
    double sampleRate;
    bool postOutput;
    bool canTimeInfo;
    bool canTimeCode;
    ASIOTime timeInfo;
    bool active;
} asio_context_t;

// Global variables
static asio_context_t g_asio_ctx = {0};
static volatile bool g_running = true;
static pcap_t *g_pcap_handle = NULL;
static char g_interface_name[256] = {0};
static char g_stream_id[17] = "0123456789ABCDEF"; // Example stream ID

// Function prototypes
static bool init_asio_driver(const char* driver_name);
static void cleanup_asio_driver(void);
static bool init_network_capture(const char* interface_name);
static void cleanup_network_capture(void);
static void process_avb_packet(const u_char* packet, int packet_len);
static void signal_handler(int sig);

// ASIO Callback functions
static void buffer_switch(long doubleBufferIndex, ASIOBool directProcess);
static ASIOTime* buffer_switch_time_info(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
static void sample_rate_changed(ASIOSampleRate sRate);
static long asio_messages(long selector, long value, void* message, double* opt);

// Buffer management
static void process_audio_buffers(long bufferIndex);

int main(int argc, char* argv[])
{
    printf("ASIO AVB Audio Listener v%s\\n", VERSION_STR);
    printf("Professional Audio Streaming for OpenAvnu\\n");
    printf("========================================\\n\\n");

    // Parse command line arguments
    if (argc < 2) {
        printf("Usage: %s <network_interface> [asio_driver_name]\\n", argv[0]);
        printf("Example: %s \"Intel I219\" \"ASIO4ALL v2\"\\n", argv[0]);
        return 1;
    }

    strncpy(g_interface_name, argv[1], sizeof(g_interface_name) - 1);
    const char* asio_driver = (argc > 2) ? argv[2] : NULL;

    // Install signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("ERROR: WSAStartup failed\\n");
        return 1;
    }

    // Initialize network capture
    if (!init_network_capture(g_interface_name)) {
        printf("ERROR: Failed to initialize network capture\\n");
        WSACleanup();
        return 1;
    }

    // Initialize ASIO driver
    if (!init_asio_driver(asio_driver)) {
        printf("ERROR: Failed to initialize ASIO driver\\n");
        cleanup_network_capture();
        WSACleanup();
        return 1;
    }

    printf("\\nASIO Audio Listener initialized successfully\\n");
    printf("Listening for AVB stream ID: %s\\n", g_stream_id);
    printf("ASIO Driver: %s\\n", g_asio_ctx.driverInfo.name);
    printf("Sample Rate: %.0f Hz\\n", g_asio_ctx.sampleRate);
    printf("Buffer Size: %ld samples\\n", g_asio_ctx.bufferSize);
    printf("Input Channels: %ld\\n", g_asio_ctx.inputChannels);
    printf("Output Channels: %ld\\n", g_asio_ctx.outputChannels);
    printf("\\nPress Ctrl+C to stop...\\n\\n");

    // Start ASIO
    if (ASIOStart() == ASE_OK) {
        g_asio_ctx.active = true;
        
        // Main packet capture loop
        struct pcap_pkthdr* header;
        const u_char* packet;
        int result;

        while (g_running) {
            result = pcap_next_ex(g_pcap_handle, &header, &packet);
            
            if (result == 1) {
                // Process AVB packet
                process_avb_packet(packet, header->len);
            } else if (result == 0) {
                // Timeout, continue
                Sleep(1);
            } else if (result == -1) {
                printf("ERROR: pcap_next_ex failed: %s\\n", pcap_geterr(g_pcap_handle));
                break;
            } else if (result == -2) {
                printf("No more packets (EOF)\\n");
                break;
            }
        }

        ASIOStop();
        g_asio_ctx.active = false;
    } else {
        printf("ERROR: Failed to start ASIO\\n");
    }

    // Cleanup
    cleanup_asio_driver();
    cleanup_network_capture();
    WSACleanup();

    printf("\\nASIO AVB Audio Listener stopped\\n");
    return 0;
}

static bool init_asio_driver(const char* driver_name)
{
    // Load ASIO driver
    if (!loadAsioDriver((char*)driver_name)) {
        // Try to find any available ASIO driver
        char** driverNames = NULL;
        long numDrivers = 0;
        
        getDriverNames(&driverNames, &numDrivers);
        if (numDrivers > 0) {
            printf("Available ASIO drivers:\\n");
            for (long i = 0; i < numDrivers; i++) {
                printf("  %ld: %s\\n", i, driverNames[i]);
            }
            
            if (!loadAsioDriver(driverNames[0])) {
                printf("ERROR: Failed to load ASIO driver: %s\\n", driverNames[0]);
                return false;
            }
            printf("Using ASIO driver: %s\\n", driverNames[0]);
        } else {
            printf("ERROR: No ASIO drivers found\\n");
            return false;
        }
    }

    // Initialize ASIO driver
    if (ASIOInit(&g_asio_ctx.driverInfo) != ASE_OK) {
        printf("ERROR: ASIOInit failed\\n");
        return false;
    }

    // Get channel counts
    if (ASIOGetChannels(&g_asio_ctx.inputChannels, &g_asio_ctx.outputChannels) != ASE_OK) {
        printf("ERROR: ASIOGetChannels failed\\n");
        return false;
    }

    // Get buffer size
    long minSize, maxSize, preferredSize, granularity;
    if (ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity) != ASE_OK) {
        printf("ERROR: ASIOGetBufferSize failed\\n");
        return false;
    }
    g_asio_ctx.bufferSize = preferredSize;

    // Get sample rate
    if (ASIOGetSampleRate(&g_asio_ctx.sampleRate) != ASE_OK) {
        printf("ERROR: ASIOGetSampleRate failed\\n");
        return false;
    }

    // Set up callbacks
    g_asio_ctx.callbacks.bufferSwitch = &buffer_switch;
    g_asio_ctx.callbacks.sampleRateDidChange = &sample_rate_changed;
    g_asio_ctx.callbacks.asioMessage = &asio_messages;
    g_asio_ctx.callbacks.bufferSwitchTimeInfo = &buffer_switch_time_info;

    // Create buffers (for output only in this listener example)
    long numChannels = min(g_asio_ctx.outputChannels, MAX_CHANNELS);
    for (long i = 0; i < numChannels; i++) {
        g_asio_ctx.bufferInfos[i].isInput = ASIOFalse;
        g_asio_ctx.bufferInfos[i].channelNum = i;
        g_asio_ctx.bufferInfos[i].buffers[0] = g_asio_ctx.bufferInfos[i].buffers[1] = NULL;
    }

    if (ASIOCreateBuffers(g_asio_ctx.bufferInfos, numChannels, g_asio_ctx.bufferSize, &g_asio_ctx.callbacks) != ASE_OK) {
        printf("ERROR: ASIOCreateBuffers failed\\n");
        return false;
    }

    // Get channel info
    for (long i = 0; i < numChannels; i++) {
        g_asio_ctx.channelInfos[i].channel = g_asio_ctx.bufferInfos[i].channelNum;
        g_asio_ctx.channelInfos[i].isInput = g_asio_ctx.bufferInfos[i].isInput;
        if (ASIOGetChannelInfo(&g_asio_ctx.channelInfos[i]) != ASE_OK) {
            printf("ERROR: ASIOGetChannelInfo failed for channel %ld\\n", i);
            return false;
        }
    }

    return true;
}

static void cleanup_asio_driver(void)
{
    if (g_asio_ctx.active) {
        ASIOStop();
        g_asio_ctx.active = false;
    }
    
    ASIODisposeBuffers();
    ASIOExit();
    removeCurrentDriver();
}

static bool init_network_capture(const char* interface_name)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs, *device;
    
    // Find all network devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        printf("ERROR: pcap_findalldevs failed: %s\\n", errbuf);
        return false;
    }

    // Find the specified interface
    device = alldevs;
    while (device != NULL) {
        if (strstr(device->description, interface_name) != NULL) {
            break;
        }
        device = device->next;
    }

    if (device == NULL) {
        printf("ERROR: Interface '%s' not found\\n", interface_name);
        printf("Available interfaces:\\n");
        device = alldevs;
        while (device != NULL) {
            printf("  %s: %s\\n", device->name, device->description ? device->description : "No description");
            device = device->next;
        }
        pcap_freealldevs(alldevs);
        return false;
    }

    // Open the device
    g_pcap_handle = pcap_open_live(device->name, 65536, 1, 1000, errbuf);
    pcap_freealldevs(alldevs);

    if (g_pcap_handle == NULL) {
        printf("ERROR: pcap_open_live failed: %s\\n", errbuf);
        return false;
    }

    // Set filter for AVB traffic (Ethertype 0x22F0 for IEEE 1722)
    struct bpf_program filter;
    if (pcap_compile(g_pcap_handle, &filter, "ether proto 0x22F0", 0, PCAP_NETMASK_UNKNOWN) == -1) {
        printf("ERROR: pcap_compile failed: %s\\n", pcap_geterr(g_pcap_handle));
        return false;
    }

    if (pcap_setfilter(g_pcap_handle, &filter) == -1) {
        printf("ERROR: pcap_setfilter failed: %s\\n", pcap_geterr(g_pcap_handle));
        pcap_freecode(&filter);
        return false;
    }

    pcap_freecode(&filter);
    return true;
}

static void cleanup_network_capture(void)
{
    if (g_pcap_handle) {
        pcap_close(g_pcap_handle);
        g_pcap_handle = NULL;
    }
}

static void process_avb_packet(const u_char* packet, int packet_len)
{
    // Basic AVB packet parsing
    if (packet_len < HEADER_SIZE) {
        return; // Packet too small
    }

    // Extract stream ID from packet (simplified)
    const u_char* stream_id_ptr = packet + ETHERNET_HEADER_SIZE + SEVENTEEN22_HEADER_PART1_SIZE;
    
    // Check if this is our target stream (simplified check)
    // In a real implementation, you'd want proper stream ID matching
    
    // Extract audio data (simplified)
    const u_char* audio_data = packet + HEADER_SIZE;
    int audio_data_len = packet_len - HEADER_SIZE;
    
    if (audio_data_len > 0) {
        // TODO: Queue audio data for ASIO playback
        // This would typically involve:
        // 1. Converting network byte order to host byte order
        // 2. Handling different audio formats
        // 3. Managing timing and synchronization
        // 4. Buffering for ASIO callback
        
        printf("Received AVB audio packet: %d bytes of audio data\\n", audio_data_len);
    }
}

static void signal_handler(int sig)
{
    printf("\\nReceived signal %d, stopping...\\n", sig);
    g_running = false;
}

// ASIO Callback implementations
static void buffer_switch(long doubleBufferIndex, ASIOBool directProcess)
{
    process_audio_buffers(doubleBufferIndex);
}

static ASIOTime* buffer_switch_time_info(ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess)
{
    process_audio_buffers(doubleBufferIndex);
    return params;
}

static void sample_rate_changed(ASIOSampleRate sRate)
{
    printf("ASIO sample rate changed to: %.0f Hz\\n", sRate);
    g_asio_ctx.sampleRate = sRate;
}

static long asio_messages(long selector, long value, void* message, double* opt)
{
    long ret = 0;
    
    switch(selector) {
        case kAsioSelectorSupported:
            if (value == kAsioResetRequest ||
                value == kAsioEngineVersion ||
                value == kAsioResyncRequest ||
                value == kAsioLatenciesChanged ||
                value == kAsioSupportsTimeInfo ||
                value == kAsioSupportsTimeCode ||
                value == kAsioSupportsInputMonitor) {
                ret = 1L;
            }
            break;
        case kAsioResetRequest:
            ret = 1L;
            break;
        case kAsioResyncRequest:
            ret = 1L;
            break;
        case kAsioLatenciesChanged:
            ret = 1L;
            break;
        case kAsioEngineVersion:
            ret = 2L;
            break;
        case kAsioSupportsTimeInfo:
            ret = 1L;
            break;
        case kAsioSupportsTimeCode:
            ret = 0L;
            break;
    }
    return ret;
}

static void process_audio_buffers(long bufferIndex)
{
    // Process audio buffers for playback
    // In a real implementation, this would:
    // 1. Take audio data from the received AVB stream queue
    // 2. Apply any necessary format conversions
    // 3. Handle synchronization and timing
    // 4. Write to ASIO output buffers
    
    long numChannels = min(g_asio_ctx.outputChannels, MAX_CHANNELS);
    
    for (long i = 0; i < numChannels; i++) {
        if (g_asio_ctx.channelInfos[i].type == ASIOSTInt16LSB) {
            // 16-bit integer samples
            short* buffer = (short*)g_asio_ctx.bufferInfos[i].buffers[bufferIndex];
            for (long j = 0; j < g_asio_ctx.bufferSize; j++) {
                buffer[j] = 0; // Silence for now
            }
        } else if (g_asio_ctx.channelInfos[i].type == ASIOSTInt24LSB) {
            // 24-bit integer samples
            char* buffer = (char*)g_asio_ctx.bufferInfos[i].buffers[bufferIndex];
            memset(buffer, 0, g_asio_ctx.bufferSize * 3);
        } else if (g_asio_ctx.channelInfos[i].type == ASIOSTFloat32LSB) {
            // 32-bit float samples
            float* buffer = (float*)g_asio_ctx.bufferInfos[i].buffers[bufferIndex];
            for (long j = 0; j < g_asio_ctx.bufferSize; j++) {
                buffer[j] = 0.0f; // Silence for now
            }
        }
    }
    
    // Signal that output is ready
    ASIOOutputReady();
}
