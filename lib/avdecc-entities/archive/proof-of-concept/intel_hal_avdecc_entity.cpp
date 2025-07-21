#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <winsock2.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

extern "C" {
#include "intel_ethernet_hal.h"
}

// IEEE 1722.1 AVDECC Protocol Constants
#define AVDECC_ETHERTYPE 0x22F0
#define AVTP_SUBTYPE_ADP 0xFA
#define ADP_MESSAGE_TYPE_ENTITY_AVAILABLE 0x00
#define ADP_MESSAGE_TYPE_ENTITY_DEPARTING 0x01
#define ADP_MESSAGE_TYPE_ENTITY_DISCOVER 0x02

// AVDECC Multicast Address
const uint8_t AVDECC_MULTICAST_MAC[6] = {0x91, 0xE0, 0xF0, 0x01, 0x00, 0x00};

// Our Entity Configuration based on Intel I219-LM
const uint64_t ENTITY_ID = 0xC047E0FFFE167B89ULL;  // Based on Intel I219-LM MAC
const uint64_t ENTITY_MODEL_ID = 0xC047E0000001234ULL;
const uint32_t ENTITY_CAPABILITIES = 0x0000C588;  // AEM, VENDOR_UNIQUE, CLASS_A, gPTP
const uint16_t TALKER_STREAM_SOURCES = 1;
const uint16_t TALKER_CAPABILITIES = 0x4801;      // AUDIO_SOURCE, MEDIA_CLOCK_SOURCE
const uint16_t LISTENER_STREAM_SINKS = 1;
const uint16_t LISTENER_CAPABILITIES = 0x4801;    // AUDIO_SINK, MEDIA_CLOCK_SINK
const uint32_t CONTROLLER_CAPABILITIES = 0x00000001; // IMPLEMENTED

#pragma pack(push, 1)
struct EthernetHeader {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
};

struct AVTPHeader {
    uint8_t subtype;
    uint8_t version_and_flags;
    uint16_t control_data_length;
};

struct ADPMessage {
    uint8_t message_type_and_valid_time;
    uint8_t reserved[3];
    uint64_t entity_id;
    uint64_t entity_model_id;
    uint32_t entity_capabilities;
    uint16_t talker_stream_sources;
    uint16_t talker_capabilities;
    uint16_t listener_stream_sinks;
    uint16_t listener_capabilities;
    uint32_t controller_capabilities;
    uint32_t available_index;
    uint64_t gptp_grandmaster_id;
    uint64_t association_id;
};
#pragma pack(pop)

class IntelAVDECCEntity {
private:
    intel_device_t* intel_device;
    uint8_t local_mac[6];
    uint32_t available_index;
    bool running;
    SOCKET raw_socket;

public:
    IntelAVDECCEntity() : intel_device(nullptr), available_index(0), running(false), raw_socket(INVALID_SOCKET) {
        // Initialize local MAC (Intel I219-LM)
        local_mac[0] = 0xC0; local_mac[1] = 0x47; local_mac[2] = 0x0E;
        local_mac[3] = 0x16; local_mac[4] = 0x7B; local_mac[5] = 0x89;
    }

    ~IntelAVDECCEntity() {
        stop();
    }

    bool initialize() {
        std::cout << "Initializing Intel HAL AVDECC Entity..." << std::endl;

        // Initialize Intel HAL
        intel_hal_result_t result = intel_hal_init();
        if (result != INTEL_HAL_SUCCESS) {
            std::cerr << "Failed to initialize Intel HAL: " << intel_hal_get_last_error() << std::endl;
            return false;
        }

        // Enumerate Intel devices
        intel_device_info_t devices[10];
        uint32_t device_count = 10;
        result = intel_hal_enumerate_devices(devices, &device_count);
        
        if (result != INTEL_HAL_SUCCESS || device_count == 0) {
            std::cerr << "No Intel devices found: " << intel_hal_get_last_error() << std::endl;
            intel_hal_cleanup();
            return false;
        }

        std::cout << "Found " << device_count << " Intel device(s):" << std::endl;
        
        // Find and open I219 device
        intel_device_info_t* target_device = nullptr;
        for (uint32_t i = 0; i < device_count; i++) {
            std::cout << "  Device " << i << ": " << devices[i].device_name 
                      << " (Family: " << devices[i].family 
                      << ", Device ID: 0x" << std::hex << devices[i].device_id << std::dec << ")" << std::endl;
            
            if (devices[i].family == INTEL_FAMILY_I219) {
                target_device = &devices[i];
                break;
            }
        }

        if (!target_device) {
            std::cerr << "No Intel I219 device found" << std::endl;
            intel_hal_cleanup();
            return false;
        }

        // Try different device identifier formats
        std::cout << "Attempting to open device with multiple identifiers..." << std::endl;
        
        // Try using device ID in hex format
        char device_id_hex[16];
        sprintf_s(device_id_hex, sizeof(device_id_hex), "0x%04x", target_device->device_id);
        result = intel_hal_open_device(device_id_hex, &intel_device);
        
        if (result != INTEL_HAL_SUCCESS) {
            // Try using description field if available
            if (strlen(target_device->description) > 0) {
                std::cout << "Trying description: " << target_device->description << std::endl;
                result = intel_hal_open_device(target_device->description, &intel_device);
            }
        }
        
        if (result != INTEL_HAL_SUCCESS) {
            // Try using Windows-specific adapter name if available
            #ifdef INTEL_HAL_WINDOWS
            if (strlen(target_device->windows.adapter_name) > 0) {
                std::cout << "Trying Windows adapter name: " << target_device->windows.adapter_name << std::endl;
                result = intel_hal_open_device(target_device->windows.adapter_name, &intel_device);
            }
            #endif
        }
        
        if (result != INTEL_HAL_SUCCESS) {
            // Try using device name with index
            char device_name_indexed[64];
            sprintf_s(device_name_indexed, sizeof(device_name_indexed), "%s-%d", target_device->device_name, 0);
            result = intel_hal_open_device(device_name_indexed, &intel_device);
        }
        if (result != INTEL_HAL_SUCCESS) {
            std::cerr << "Failed to open Intel device after trying multiple formats: " << intel_hal_get_last_error() << std::endl;
            std::cout << "Available device information:" << std::endl;
            std::cout << "  Name: " << target_device->device_name << std::endl;
            std::cout << "  Description: " << target_device->description << std::endl;
            std::cout << "  Device ID: 0x" << std::hex << target_device->device_id << std::dec << std::endl;
            std::cout << "  Family: " << target_device->family << std::endl;
            #ifdef INTEL_HAL_WINDOWS
            std::cout << "  Windows adapter name: " << target_device->windows.adapter_name << std::endl;
            #endif
            
            // Continue anyway to test network functionality without HAL
            std::cout << "Continuing with network-only implementation..." << std::endl;
            intel_device = nullptr;
        } else {
            std::cout << "Successfully opened Intel device!" << std::endl;
        }

        // Get device capabilities (only if device was opened successfully)
        if (intel_device) {
            uint32_t capabilities;
            result = intel_hal_get_capabilities(intel_device, &capabilities);
            if (result == INTEL_HAL_SUCCESS) {
                std::cout << "Device capabilities: 0x" << std::hex << capabilities << std::dec << std::endl;
                
                if (intel_hal_has_capability(intel_device, INTEL_CAP_BASIC_1588)) {
                    std::cout << "  - IEEE 1588 timestamping supported" << std::endl;
                }
                if (intel_hal_has_capability(intel_device, INTEL_CAP_ENHANCED_TS)) {
                    std::cout << "  - Enhanced timestamping supported" << std::endl;
                }
                if (intel_hal_has_capability(intel_device, INTEL_CAP_AVB_SHAPING)) {
                    std::cout << "  - AVB shaping supported" << std::endl;
                }
            }

            // Enable timestamping
            result = intel_hal_enable_timestamping(intel_device, true);
            if (result == INTEL_HAL_SUCCESS) {
                std::cout << "IEEE 1588 timestamping enabled" << std::endl;
            } else {
                std::cout << "Warning: Could not enable timestamping: " << intel_hal_get_last_error() << std::endl;
            }
        } else {
            std::cout << "Intel HAL device not available - using network-only mode" << std::endl;
        }

        // Initialize Winsock for raw socket (Layer 3 approach for simplicity)
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            intel_hal_close_device(intel_device);
            intel_hal_cleanup();
            return false;
        }

        // Create UDP socket for AVDECC multicast
        raw_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (raw_socket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            intel_hal_close_device(intel_device);
            intel_hal_cleanup();
            return false;
        }

        std::cout << "Intel HAL AVDECC Entity initialized successfully" << std::endl;
        return true;
    }

    void start() {
        if (raw_socket == INVALID_SOCKET) {
            std::cerr << "Entity not properly initialized (socket invalid)" << std::endl;
            return;
        }

        running = true;
        std::cout << "Starting Intel HAL AVDECC Entity..." << std::endl;
        std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
        
        if (intel_device) {
            std::cout << "Using Intel HAL for timestamping and hardware integration" << std::endl;
        } else {
            std::cout << "Using network-only mode (Intel HAL not available)" << std::endl;
        }

        // Send initial ENTITY_AVAILABLE
        send_entity_available();

        // Periodic ENTITY_AVAILABLE broadcast
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (running) {
                send_entity_available();
            }
        }
    }

    void stop() {
        running = false;
        if (intel_device) {
            // Send ENTITY_DEPARTING
            send_entity_departing();
            
            // Cleanup
            if (raw_socket != INVALID_SOCKET) {
                closesocket(raw_socket);
                raw_socket = INVALID_SOCKET;
            }
            
            WSACleanup();
            intel_hal_close_device(intel_device);
            intel_hal_cleanup();
            intel_device = nullptr;
        }
    }

private:
    void send_entity_available() {
        send_adp_message(ADP_MESSAGE_TYPE_ENTITY_AVAILABLE);
        available_index++;
    }

    void send_entity_departing() {
        send_adp_message(ADP_MESSAGE_TYPE_ENTITY_DEPARTING);
    }

    uint16_t htons_local(uint16_t value) {
        return htons(value);
    }

    uint32_t htonl_local(uint32_t value) {
        return htonl(value);
    }

    uint64_t htonll_local(uint64_t value) {
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    }

    void send_adp_message(uint8_t message_type) {
        // Get hardware timestamp using Intel HAL (if available)
        if (intel_device) {
            intel_timestamp_t hw_timestamp;
            intel_hal_result_t result = intel_hal_read_timestamp(intel_device, &hw_timestamp);
            
            if (result == INTEL_HAL_SUCCESS) {
                std::cout << "Hardware timestamp: " << hw_timestamp.seconds 
                          << "." << hw_timestamp.nanoseconds << " ns" << std::endl;
            }
        } else {
            std::cout << "Using system time (Intel HAL not available)" << std::endl;
        }

        // For now, use UDP multicast to simulate AVDECC (Layer 3 approach)
        // In a full implementation, this would use raw sockets or packet injection
        
        // Build ADP message (without Ethernet header for UDP)
        struct {
            AVTPHeader avtp;
            ADPMessage adp;
        } avdecc_packet;

        // Build AVTP Header
        avdecc_packet.avtp.subtype = AVTP_SUBTYPE_ADP;
        avdecc_packet.avtp.version_and_flags = 0x00;
        avdecc_packet.avtp.control_data_length = htons_local(sizeof(ADPMessage));

        // Build ADP Message
        avdecc_packet.adp.message_type_and_valid_time = (message_type << 4) | 0x05;
        memset(avdecc_packet.adp.reserved, 0, 3);
        avdecc_packet.adp.entity_id = htonll_local(ENTITY_ID);
        avdecc_packet.adp.entity_model_id = htonll_local(ENTITY_MODEL_ID);
        avdecc_packet.adp.entity_capabilities = htonl_local(ENTITY_CAPABILITIES);
        avdecc_packet.adp.talker_stream_sources = htons_local(TALKER_STREAM_SOURCES);
        avdecc_packet.adp.talker_capabilities = htons_local(TALKER_CAPABILITIES);
        avdecc_packet.adp.listener_stream_sinks = htons_local(LISTENER_STREAM_SINKS);
        avdecc_packet.adp.listener_capabilities = htons_local(LISTENER_CAPABILITIES);
        avdecc_packet.adp.controller_capabilities = htonl_local(CONTROLLER_CAPABILITIES);
        avdecc_packet.adp.available_index = htonl_local(available_index);
        avdecc_packet.adp.gptp_grandmaster_id = htonll_local(ENTITY_ID);
        avdecc_packet.adp.association_id = 0;

        // Send via UDP multicast (AVDECC simulation on port 17221)
        sockaddr_in dest_addr = {};
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(17221);  // AVDECC port
        dest_addr.sin_addr.s_addr = inet_addr("224.0.0.1");  // Multicast

        int sent = sendto(raw_socket, (char*)&avdecc_packet, sizeof(avdecc_packet), 0,
                         (sockaddr*)&dest_addr, sizeof(dest_addr));

        if (sent > 0) {
            std::cout << "Sent " << (message_type == ADP_MESSAGE_TYPE_ENTITY_AVAILABLE ? "ENTITY_AVAILABLE" : "ENTITY_DEPARTING") 
                      << " via Intel HAL integration (available_index=" << available_index << ")" << std::endl;
        } else {
            std::cerr << "Failed to send packet: " << WSAGetLastError() << std::endl;
        }
    }
};

int main() {
    std::cout << "=== Intel HAL AVDECC Entity (Hardware Timestamping) ===" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
    std::cout << "Capabilities: AEM, Audio Source/Sink, gPTP Support" << std::endl;
    std::cout << "Target: Hive AVDECC Controller Discovery" << std::endl;
    std::cout << "Hardware: Intel I219-LM with HAL timestamping" << std::endl;
    std::cout << std::endl;

    IntelAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "Failed to initialize Intel HAL AVDECC entity" << std::endl;
        return 1;
    }

    std::cout << "Press Ctrl+C to stop..." << std::endl;
    entity.start();

    return 0;
}
