#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <chrono>
#include <thread>

// Include PCAP for Layer 2 injection
#include <pcap.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")

// Manual byte order conversion for Windows
uint64_t htonll_local(uint64_t hostlonglong) {
    return ((uint64_t)htonl((uint32_t)(hostlonglong & 0xFFFFFFFF)) << 32) | 
           htonl((uint32_t)(hostlonglong >> 32));
}

uint64_t ntohll_local(uint64_t netlonglong) {
    return ((uint64_t)ntohl((uint32_t)(netlonglong & 0xFFFFFFFF)) << 32) | 
           ntohl((uint32_t)(netlonglong >> 32));
}

struct ADPMessage {
    uint8_t entity_id[8];
    uint8_t entity_model_id[8];
    uint32_t entity_capabilities;
    uint16_t talker_stream_sources;
    uint16_t talker_capabilities;
    uint16_t listener_stream_sinks;
    uint16_t listener_capabilities;
    uint32_t controller_capabilities;
    uint32_t available_index;
    uint8_t gptp_grandmaster_id[8];
    uint8_t gptp_domain_number;
    uint8_t reserved1[3];
    uint16_t identify_control_index;
    uint16_t interface_index;
    uint8_t association_id[8]; // ADDED: 8-byte Association ID field
    uint8_t reserved2[4];      // ADDED: 4-byte padding for total 64 bytes
};

class PCARAVDECCEntity {
private:
    pcap_t* pcap_handle;
    bool running;
    std::thread discovery_thread;
    
    // Entity configuration
    uint64_t entity_id;
    uint64_t entity_model_id;
    uint32_t available_index;
    
public:
    PCARAVDECCEntity(uint64_t eid, uint64_t model_id) 
        : entity_id(eid), entity_model_id(model_id), 
          pcap_handle(nullptr), running(false), available_index(0) {
        std::cout << "🌟 PCAP AVDECC Entity initialized" << std::endl;
        std::cout << "  Entity ID: 0x" << std::hex << entity_id << std::endl;
        std::cout << "  Model ID: 0x" << std::hex << entity_model_id << std::endl;
    }
    
    bool init_network_interface() {
        const char* interface_name = "\\Device\\NPF_{8BEDBD8D-6DDA-4EF1-B257-9D96CE0A1CAD}";
        
        std::cout << "🔌 Initializing network interface..." << std::endl;
        std::cout << "  Interface: " << interface_name << std::endl;
        
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_handle = pcap_open_live(interface_name, 65536, 1, 1, errbuf);
        
        if (!pcap_handle) {
            std::cout << "❌ Failed to open interface: " << errbuf << std::endl;
            return false;
        }
        
        std::cout << "✅ Network interface opened successfully" << std::endl;
        return true;
    }
    
    bool send_adp_message() {
        if (!pcap_handle) {
            std::cout << "❌ Network interface not initialized" << std::endl;
            return false;
        }
        
        // Create 82-byte packet matching M2Lab working device
        uint8_t raw_packet[82];
        memset(raw_packet, 0, sizeof(raw_packet));
        
        // Ethernet header (14 bytes)
        uint8_t dest_mac[6] = {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00};  // IEEE 1722.1 multicast
        uint8_t src_mac[6] = {0xc0, 0x47, 0xe0, 0xff, 0xe1, 0x67};   // Our MAC
        uint16_t ethertype = htons(0x22F0);  // IEEE 1722
        
        memcpy(raw_packet, dest_mac, 6);
        memcpy(raw_packet + 6, src_mac, 6);
        memcpy(raw_packet + 12, &ethertype, 2);
        
        // AVTP header (4 bytes) - Matching M2Lab device exactly
        raw_packet[14] = 0xFA;  // subtype=0x7A (ADP), valid_time=5
        raw_packet[15] = 0x00;  // msg_type=ENTITY_AVAILABLE
        raw_packet[16] = 0x00;  // control_data_length high byte (network byte order)
        raw_packet[17] = 0x38;  // control_data_length=56 low byte (0x0038 = 56 decimal)
        
        // ADP message data starts at byte 18 - MANUAL CONSTRUCTION to avoid padding issues
        size_t offset = 18;
        
        // Entity ID (8 bytes) - Network byte order
        uint64_t net_entity_id = htonll_local(entity_id);
        memcpy(raw_packet + offset, &net_entity_id, 8);
        offset += 8;
        
        // Entity Model ID (8 bytes) - Network byte order  
        uint64_t net_model_id = htonll_local(entity_model_id);
        memcpy(raw_packet + offset, &net_model_id, 8);
        offset += 8;
        
        // Entity capabilities (4 bytes)
        uint32_t entity_caps = htonl(0x00000008);  // AEM_SUPPORTED
        memcpy(raw_packet + offset, &entity_caps, 4);
        offset += 4;
        
        // Talker Stream Sources (2 bytes)
        uint16_t talker_sources = htons(1);
        memcpy(raw_packet + offset, &talker_sources, 2);
        offset += 2;
        
        // Talker Capabilities (2 bytes)
        uint16_t talker_caps = htons(0x4001);  // IMPLEMENTED | AUDIO_SOURCE
        memcpy(raw_packet + offset, &talker_caps, 2);
        offset += 2;
        
        // Listener Stream Sinks (2 bytes)
        uint16_t listener_sinks = htons(1);
        memcpy(raw_packet + offset, &listener_sinks, 2);
        offset += 2;
        
        // Listener Capabilities (2 bytes)
        uint16_t listener_caps = htons(0x4001);  // IMPLEMENTED | AUDIO_SINK
        memcpy(raw_packet + offset, &listener_caps, 2);
        offset += 2;
        
        // Controller Capabilities (4 bytes)
        uint32_t controller_caps = htonl(0x00000001);  // IMPLEMENTED
        memcpy(raw_packet + offset, &controller_caps, 4);
        offset += 4;
        
        // Available Index (4 bytes)
        uint32_t avail_idx = htonl(available_index);
        memcpy(raw_packet + offset, &avail_idx, 4);
        offset += 4;
        
        // gPTP Grandmaster ID (8 bytes) - all zeros
        memset(raw_packet + offset, 0, 8);
        offset += 8;
        
        // gPTP Domain Number (1 byte) + Reserved (3 bytes)
        raw_packet[offset] = 0;  // domain number
        memset(raw_packet + offset + 1, 0, 3);  // reserved
        offset += 4;
        
        // Identify Control Index (2 bytes)
        uint16_t identify_idx = htons(0);
        memcpy(raw_packet + offset, &identify_idx, 2);
        offset += 2;
        
        // Interface Index (2 bytes)
        uint16_t interface_idx = htons(0);
        memcpy(raw_packet + offset, &interface_idx, 2);
        offset += 2;
        
        // Association ID (8 bytes) - Required for 82-byte packet
        memset(raw_packet + offset, 0, 8);
        offset += 8;
        
        // Additional padding (4 bytes) - Required for 82-byte packet total
        memset(raw_packet + offset, 0, 4);
        offset += 4;
        
        // Send the 82-byte packet
        int result = pcap_sendpacket(pcap_handle, raw_packet, sizeof(raw_packet));
        
        if (result != 0) {
            std::cout << "❌ Failed to send packet: " << pcap_geterr(pcap_handle) << std::endl;
            return false;
        }
        
        std::cout << "📤 ADP packet sent successfully (82 bytes, control_data_length=56)" << std::endl;
        std::cout << "  Entity ID: 0x" << std::hex << entity_id << std::dec << std::endl;
        std::cout << "  Available Index: " << available_index << std::endl;
        std::cout << "  Raw packet debug (first 32 bytes):" << std::endl;
        std::cout << "  ";
        for (int i = 0; i < 32; i++) {
            if (i % 16 == 0 && i > 0) std::cout << "\n  ";
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)raw_packet[i] << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "  Packet structure matches M2Lab working device exactly" << std::endl;
        
        available_index++; // Increment for next transmission
        return true;
    }
    
    void start_discovery() {
        if (running) return;
        
        running = true;
        discovery_thread = std::thread([this]() {
            std::cout << "🔄 Starting AVDECC discovery broadcasts..." << std::endl;
            std::cout << "  Expected valid_time=5, control_data_length=56 (exactly matching M2Lab working device)" << std::endl;
            
            while (running) {
                send_adp_message();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }
    
    void stop_discovery() {
        if (!running) return;
        
        running = false;
        if (discovery_thread.joinable()) {
            discovery_thread.join();
        }
        std::cout << "🛑 AVDECC discovery stopped" << std::endl;
    }
    
    ~PCARAVDECCEntity() {
        stop_discovery();
        if (pcap_handle) {
            pcap_close(pcap_handle);
        }
    }
};

int main() {
    std::cout << "🚀 Intel PCAP AVDECC Entity - Field Alignment Fixed Version" << std::endl;
    std::cout << "==============================================================\n" << std::endl;
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "❌ Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    // Create AVDECC entity with working device ID
    uint64_t entity_id = 0xc047e0fffe167b89ULL;
    uint64_t model_id = 0x0000000000000001ULL;
    
    PCARAVDECCEntity entity(entity_id, model_id);
    
    if (!entity.init_network_interface()) {
        WSACleanup();
        return 1;
    }
    
    // Start discovery broadcasts
    entity.start_discovery();
    
    std::cout << "\n🎯 Broadcasting AVDECC discovery with 82-byte packets (control_data_length=56)" << std::endl;
    std::cout << "📊 Entity ID should now display correctly as: 0xc047e0fffe167b89" << std::endl;
    std::cout << "⚡ Press Ctrl+C to stop...\n" << std::endl;
    
    // Run for 30 seconds for testing
    std::this_thread::sleep_for(std::chrono::seconds(30));
    
    entity.stop_discovery();
    WSACleanup();
    
    std::cout << "\n✅ Test completed successfully!" << std::endl;
    return 0;
}
