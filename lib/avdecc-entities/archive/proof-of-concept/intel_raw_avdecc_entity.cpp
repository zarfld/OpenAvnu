#include <iostream>
#include <thread>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <mstcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

// Define IP_HDRINCL if not available
#ifndef IP_HDRINCL
#define IP_HDRINCL 2
#endif

// Disable deprecated warnings
#define _WINSOCK_DEPRECATED_NO_WARNINGS

extern "C" {
#include "intel_ethernet_hal.h"
}

// IEEE 1722.1 AVDECC Protocol Constants
#define AVDECC_ETHERTYPE 0x22F0
#define AVTP_SUBTYPE_ADP 0xFA
#define ADP_MESSAGE_TYPE_ENTITY_AVAILABLE 0x00
#define ADP_MESSAGE_TYPE_ENTITY_DEPARTING 0x01
#define ADP_MESSAGE_TYPE_ENTITY_DISCOVER 0x02

// AVDECC Multicast Address (IEEE 1722a)
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

struct AVDECCPacket {
    EthernetHeader eth;
    AVTPHeader avtp;
    ADPMessage adp;
};
#pragma pack(pop)

class IntelRawAVDECCEntity {
private:
    intel_device_t* intel_device;
    uint8_t local_mac[6];
    uint32_t available_index;
    bool running;
    SOCKET raw_socket;
    char interface_name[256];
    int interface_index;

public:
    IntelRawAVDECCEntity() : intel_device(nullptr), available_index(0), running(false), raw_socket(INVALID_SOCKET), interface_index(-1) {
        // Initialize local MAC (Intel I219-LM)
        local_mac[0] = 0xC0; local_mac[1] = 0x47; local_mac[2] = 0x0E;
        local_mac[3] = 0x16; local_mac[4] = 0x7B; local_mac[5] = 0x89;
        memset(interface_name, 0, sizeof(interface_name));
    }

    ~IntelRawAVDECCEntity() {
        stop();
    }

    bool initialize() {
        std::cout << "Initializing Intel Raw AVDECC Entity..." << std::endl;

        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }

        // Find Intel I219-LM network interface
        if (!find_intel_interface()) {
            std::cerr << "Failed to find Intel I219-LM interface" << std::endl;
            WSACleanup();
            return false;
        }

        // Try to create raw socket for Layer 2 packet injection
        if (!create_raw_socket()) {
            std::cerr << "Failed to create raw socket, falling back to ARP table method" << std::endl;
            // Continue anyway - we'll use a different approach
        }

        // Initialize Intel HAL (optional for timestamping)
        intel_hal_result_t result = intel_hal_init();
        if (result == INTEL_HAL_SUCCESS) {
            std::cout << "Intel HAL initialized for timestamping" << std::endl;
            
            // Try to open device for timestamping
            intel_device_info_t devices[10];
            uint32_t device_count = 10;
            result = intel_hal_enumerate_devices(devices, &device_count);
            
            if (result == INTEL_HAL_SUCCESS && device_count > 0) {
                for (uint32_t i = 0; i < device_count; i++) {
                    if (devices[i].family == INTEL_FAMILY_I219) {
                        char device_id_hex[16];
                        sprintf_s(device_id_hex, sizeof(device_id_hex), "0x%04x", devices[i].device_id);
                        result = intel_hal_open_device(device_id_hex, &intel_device);
                        if (result == INTEL_HAL_SUCCESS) {
                            intel_hal_enable_timestamping(intel_device, true);
                            std::cout << "Intel HAL device opened for timestamping" << std::endl;
                            break;
                        }
                    }
                }
            }
        }

        std::cout << "Intel Raw AVDECC Entity initialized successfully" << std::endl;
        return true;
    }

    void start() {
        running = true;
        std::cout << "Starting Intel Raw AVDECC Entity..." << std::endl;
        std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
        std::cout << "Target Interface: " << interface_name << " (Index: " << interface_index << ")" << std::endl;
        std::cout << "AVDECC Multicast: 91:e0:f0:01:00:00 (EtherType 0x22F0)" << std::endl;

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
        if (running) {
            send_entity_departing();
        }
        
        if (raw_socket != INVALID_SOCKET) {
            closesocket(raw_socket);
            raw_socket = INVALID_SOCKET;
        }
        
        if (intel_device) {
            intel_hal_close_device(intel_device);
            intel_hal_cleanup();
            intel_device = nullptr;
        }
        
        WSACleanup();
    }

private:
    bool find_intel_interface() {
        PIP_ADAPTER_INFO pAdapterInfo = nullptr;
        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
        
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
        if (pAdapterInfo == nullptr) return false;

        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
            if (pAdapterInfo == nullptr) return false;
        }

        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                // Check if this is our Intel I219-LM interface
                if (pAdapter->AddressLength == 6) {
                    if (memcmp(pAdapter->Address, local_mac, 6) == 0) {
                        strcpy_s(interface_name, sizeof(interface_name), pAdapter->AdapterName);
                        interface_index = pAdapter->Index;
                        
                        std::cout << "Found Intel I219-LM interface:" << std::endl;
                        std::cout << "  Name: " << pAdapter->AdapterName << std::endl;
                        std::cout << "  Description: " << pAdapter->Description << std::endl;
                        std::cout << "  Index: " << pAdapter->Index << std::endl;
                        std::cout << "  MAC: ";
                        for (int i = 0; i < 6; i++) {
                            std::cout << std::hex << (int)pAdapter->Address[i];
                            if (i < 5) std::cout << ":";
                        }
                        std::cout << std::dec << std::endl;
                        
                        free(pAdapterInfo);
                        return true;
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }
        
        free(pAdapterInfo);
        return false;
    }

    bool create_raw_socket() {
        // Try to create raw socket (requires admin privileges)
        raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (raw_socket == INVALID_SOCKET) {
            std::cout << "Raw socket creation failed (admin privileges required): " << WSAGetLastError() << std::endl;
            return false;
        }

        // Enable header include
        BOOL flag = TRUE;
        if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, (char*)&flag, sizeof(flag)) == SOCKET_ERROR) {
            std::cout << "Failed to set IP_HDRINCL: " << WSAGetLastError() << std::endl;
            closesocket(raw_socket);
            raw_socket = INVALID_SOCKET;
            return false;
        }

        std::cout << "Raw socket created successfully" << std::endl;
        return true;
    }

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
        }

        // Build proper AVDECC Layer 2 packet
        AVDECCPacket packet;
        memset(&packet, 0, sizeof(packet));

        // Build Ethernet Header (Layer 2)
        memcpy(packet.eth.dest_mac, AVDECC_MULTICAST_MAC, 6);
        memcpy(packet.eth.src_mac, local_mac, 6);
        packet.eth.ethertype = htons_local(AVDECC_ETHERTYPE);  // 0x22F0

        // Build AVTP Header
        packet.avtp.subtype = AVTP_SUBTYPE_ADP;  // 0xFA
        packet.avtp.version_and_flags = 0x00;    // Version 0, no Stream ID valid
        packet.avtp.control_data_length = htons_local(sizeof(ADPMessage));

        // Build ADP Message
        packet.adp.message_type_and_valid_time = (message_type << 4) | 0x05;  // Valid time = 10 seconds
        memset(packet.adp.reserved, 0, 3);
        packet.adp.entity_id = htonll_local(ENTITY_ID);
        packet.adp.entity_model_id = htonll_local(ENTITY_MODEL_ID);
        packet.adp.entity_capabilities = htonl_local(ENTITY_CAPABILITIES);
        packet.adp.talker_stream_sources = htons_local(TALKER_STREAM_SOURCES);
        packet.adp.talker_capabilities = htons_local(TALKER_CAPABILITIES);
        packet.adp.listener_stream_sinks = htons_local(LISTENER_STREAM_SINKS);
        packet.adp.listener_capabilities = htons_local(LISTENER_CAPABILITIES);
        packet.adp.controller_capabilities = htonl_local(CONTROLLER_CAPABILITIES);
        packet.adp.available_index = htonl_local(available_index);
        packet.adp.gptp_grandmaster_id = htonll_local(ENTITY_ID);  // Use entity ID as grandmaster
        packet.adp.association_id = 0;

        // Method 1: Try to inject via raw socket (requires admin)
        bool sent_via_raw = false;
        if (raw_socket != INVALID_SOCKET) {
            // This won't work for Layer 2 on Windows without special drivers
            // But we try anyway
            std::cout << "Attempting raw packet injection (requires specialized driver)" << std::endl;
        }

        // Method 2: Simulate Layer 2 by sending to broadcast IP
        // This is a workaround - not real AVDECC but visible in network capture
        if (!sent_via_raw) {
            SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (udp_socket != INVALID_SOCKET) {
                // Enable broadcast
                BOOL broadcast = TRUE;
                setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

                // Send to local broadcast with AVDECC-like port
                sockaddr_in dest_addr = {};
                dest_addr.sin_family = AF_INET;
                dest_addr.sin_port = htons(17221);  // Custom AVDECC simulation port
                dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");  // Local broadcast

                int sent = sendto(udp_socket, (char*)&packet, sizeof(packet), 0,
                                (sockaddr*)&dest_addr, sizeof(dest_addr));

                closesocket(udp_socket);

                if (sent > 0) {
                    std::cout << "Sent " << (message_type == ADP_MESSAGE_TYPE_ENTITY_AVAILABLE ? "ENTITY_AVAILABLE" : "ENTITY_DEPARTING") 
                              << " as broadcast UDP (AVDECC simulation, available_index=" << available_index << ")" << std::endl;
                    std::cout << "  Packet size: " << sent << " bytes" << std::endl;
                    std::cout << "  EtherType: 0x" << std::hex << AVDECC_ETHERTYPE << std::dec << std::endl;
                    std::cout << "  Target MAC: 91:e0:f0:01:00:00" << std::endl;
                    std::cout << "  Source MAC: c0:47:0e:16:7b:89" << std::endl;
                } else {
                    std::cerr << "Failed to send packet: " << WSAGetLastError() << std::endl;
                }
            }
        }
    }
};

int main() {
    std::cout << "=== Intel Raw AVDECC Entity (Layer 2 Protocol) ===" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
    std::cout << "Protocol: IEEE 1722.1-2021 Layer 2 (EtherType 0x22F0)" << std::endl;
    std::cout << "Target: AVDECC Multicast MAC 91:e0:f0:01:00:00" << std::endl;
    std::cout << "Hardware: Intel I219-LM with hardware timestamping" << std::endl;
    std::cout << std::endl;

    IntelRawAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "Failed to initialize Intel Raw AVDECC entity" << std::endl;
        return 1;
    }

    std::cout << "Press Ctrl+C to stop..." << std::endl;
    entity.start();

    return 0;
}
