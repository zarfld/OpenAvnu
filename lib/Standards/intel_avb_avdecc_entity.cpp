#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

// Windows-specific headers for network functionality
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

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
const uint64_t ENTITY_MODEL_ID = 0x0017FFFE00000001ULL;  // Match working PCAP version
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
    device_t intel_device;
    uint8_t local_mac[6];
    uint32_t available_index;
    bool running;
    bool entity_state_changed;  // Track state changes for available_index
    struct intel_dma_alloc tx_buffer;
    struct intel_packet tx_packet;

    // Persistence functions for available_index
    uint32_t load_available_index() {
        std::ifstream file("available_index_intel.dat", std::ios::binary);
        if (file.is_open()) {
            uint32_t saved_index;
            file.read(reinterpret_cast<char*>(&saved_index), sizeof(saved_index));
            file.close();
            std::cout << "📂 Intel AVB: Loaded available_index from file: " << saved_index << std::endl;
            return saved_index;
        } else {
            // First run - start from a high professional value (offset from PCAP entity)
            std::cout << "📂 Intel AVB: First run - starting available_index: 2000" << std::endl;
            return 2000;
        }
    }
    
    void save_available_index() {
        std::ofstream file("available_index_intel.dat", std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(&available_index), sizeof(available_index));
            file.close();
            std::cout << "💾 Intel AVB: Saved available_index: " << available_index << std::endl;
        }
    }

public:
    IntelAVDECCEntity() : available_index(0), running(false), entity_state_changed(true) {
        memset(&intel_device, 0, sizeof(intel_device));
        memset(&tx_buffer, 0, sizeof(tx_buffer));
        memset(&tx_packet, 0, sizeof(tx_packet));
        
        // Initialize local MAC (Intel I219-LM)
        local_mac[0] = 0xC0; local_mac[1] = 0x47; local_mac[2] = 0x0E;
        local_mac[3] = 0x16; local_mac[4] = 0x7B; local_mac[5] = 0x89;
        
        // CRITICAL FIX: Increment available_index on EVERY entity startup
        // This ensures Hive sees a different value after each restart
        available_index = load_available_index();
        available_index++;
        save_available_index();
        std::cout << "🔄 Intel AVB Entity startup: available_index incremented to " << available_index << std::endl;
    }

    ~IntelAVDECCEntity() {
        stop();
    }

    bool initialize() {
        std::cout << "Initializing Intel AVB AVDECC Entity..." << std::endl;

        // Probe for Intel devices
        if (intel_probe(&intel_device) != 0) {
            std::cerr << "Failed to probe Intel device" << std::endl;
            return false;
        }

        std::cout << "Found Intel device: " << intel_get_device_name(&intel_device) << std::endl;
        
        // Display capabilities
        uint32_t caps = intel_get_capabilities(&intel_device);
        std::cout << "Device capabilities: 0x" << std::hex << caps << std::dec << std::endl;
        
        if (intel_has_capability(&intel_device, INTEL_CAP_BASIC_1588)) {
            std::cout << "  - IEEE 1588 timestamping supported" << std::endl;
        }
        if (intel_has_capability(&intel_device, INTEL_CAP_ENHANCED_TS)) {
            std::cout << "  - Enhanced timestamping supported" << std::endl;
        }

        // Initialize device
        if (intel_init(&intel_device) != 0) {
            std::cerr << "Failed to initialize Intel device" << std::endl;
            return false;
        }

        // Attach for TX operations
        if (intel_attach_tx(&intel_device) != 0) {
            std::cerr << "Failed to attach TX interface" << std::endl;
            intel_detach(&intel_device);
            return false;
        }

        // Allocate DMA buffer for packet transmission
        if (intel_dma_malloc_page(&intel_device, &tx_buffer) != 0) {
            std::cerr << "Failed to allocate DMA buffer" << std::endl;
            intel_detach(&intel_device);
            return false;
        }

        // Setup packet structure
        tx_packet.vaddr = tx_buffer.dma_vaddr;
        tx_packet.map.paddr = tx_buffer.dma_paddr;
        tx_packet.map.mmap_size = tx_buffer.mmap_size;
        tx_packet.offset = 0;
        tx_packet.flags = INTEL_PACKET_LAUNCHTIME;
        tx_packet.next = nullptr;

        std::cout << "Intel AVB AVDECC Entity initialized successfully" << std::endl;
        return true;
    }

    void start() {
        if (!tx_buffer.dma_vaddr) {
            std::cerr << "Entity not initialized" << std::endl;
            return;
        }

        running = true;
        entity_state_changed = true;  // Starting is a state change
        std::cout << "Starting Intel AVB AVDECC Entity..." << std::endl;
        std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
        std::cout << "Using Intel hardware timestamping and packet injection" << std::endl;

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
        if (tx_buffer.dma_vaddr) {
            // Send ENTITY_DEPARTING
            send_entity_departing();
            
            // Cleanup
            intel_dma_free_page(&intel_device, &tx_buffer);
            intel_detach(&intel_device);
            
            memset(&tx_buffer, 0, sizeof(tx_buffer));
        }
    }

private:
    void send_entity_available() {
        // Only increment available_index on state changes, not every broadcast
        if (entity_state_changed) {
            available_index++;
            entity_state_changed = false;
            std::cout << "Entity state changed - incremented available_index to " << available_index << std::endl;
        }
        send_adp_message(ADP_MESSAGE_TYPE_ENTITY_AVAILABLE);
    }

    void send_entity_departing() {
        entity_state_changed = true;  // Departing is a state change
        send_adp_message(ADP_MESSAGE_TYPE_ENTITY_DEPARTING);
    }

    uint16_t htons_local(uint16_t value) {
        return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
    }

    uint32_t htonl_local(uint32_t value) {
        return ((value & 0xFF) << 24) | 
               (((value >> 8) & 0xFF) << 16) | 
               (((value >> 16) & 0xFF) << 8) | 
               ((value >> 24) & 0xFF);
    }

    uint64_t htonll_local(uint64_t value) {
        return ((uint64_t)htonl_local(value & 0xFFFFFFFF) << 32) | 
               htonl_local(value >> 32);
    }

    void send_adp_message(uint8_t message_type) {
        // Build AVDECC packet in DMA buffer
        uint8_t *packet_data = static_cast<uint8_t*>(tx_buffer.dma_vaddr);
        
        // Calculate packet size
        size_t packet_size = sizeof(EthernetHeader) + sizeof(AVTPHeader) + sizeof(ADPMessage);
        
        if (packet_size > tx_buffer.mmap_size) {
            std::cerr << "Packet too large for DMA buffer" << std::endl;
            return;
        }

        // Build Ethernet Header
        EthernetHeader *eth = reinterpret_cast<EthernetHeader*>(packet_data);
        memcpy(eth->dest_mac, AVDECC_MULTICAST_MAC, 6);
        memcpy(eth->src_mac, local_mac, 6);
        eth->ethertype = htons_local(AVDECC_ETHERTYPE);

        // Build AVTP Header
        AVTPHeader *avtp = reinterpret_cast<AVTPHeader*>(packet_data + sizeof(EthernetHeader));
        avtp->subtype = AVTP_SUBTYPE_ADP;
        avtp->version_and_flags = 0x00;  // Version 0, no Stream ID valid
        avtp->control_data_length = htons_local(sizeof(ADPMessage));

        // Build ADP Message
        ADPMessage *adp = reinterpret_cast<ADPMessage*>(packet_data + sizeof(EthernetHeader) + sizeof(AVTPHeader));
        adp->message_type_and_valid_time = (message_type << 4) | 0x0A;  // Valid time = 10 seconds (was 0x05)
        memset(adp->reserved, 0, 3);
        adp->entity_id = htonll_local(ENTITY_ID);
        adp->entity_model_id = htonll_local(ENTITY_MODEL_ID);
        adp->entity_capabilities = htonl_local(ENTITY_CAPABILITIES);
        adp->talker_stream_sources = htons_local(TALKER_STREAM_SOURCES);
        adp->talker_capabilities = htons_local(TALKER_CAPABILITIES);
        adp->listener_stream_sinks = htons_local(LISTENER_STREAM_SINKS);
        adp->listener_capabilities = htons_local(LISTENER_CAPABILITIES);
        adp->controller_capabilities = htonl_local(CONTROLLER_CAPABILITIES);
        adp->available_index = htonl_local(available_index);
        adp->gptp_grandmaster_id = htonll_local(ENTITY_ID);  // Use entity ID as grandmaster
        adp->association_id = htonll_local(ENTITY_ID);  // Set association_id to entity_id (was 0)

        // Get current hardware timestamp for precise transmission
        uint64_t current_time = 0;
        if (intel_get_systime(&intel_device, &current_time) == 0) {
            // Schedule transmission slightly in the future for precise timing
            tx_packet.attime = current_time + 1000000ULL; // 1ms in the future
        } else {
            tx_packet.attime = 0; // Send immediately if timestamp fails
        }

        // Set packet parameters
        tx_packet.len = static_cast<uint32_t>(packet_size);

        // Transmit packet using Intel hardware
        if (intel_xmit(&intel_device, 0, &tx_packet) == 0) {
            std::cout << "Sent " << (message_type == ADP_MESSAGE_TYPE_ENTITY_AVAILABLE ? "ENTITY_AVAILABLE" : "ENTITY_DEPARTING") 
                      << " via Intel AVB HAL (available_index=" << available_index 
                      << ", timestamp=" << tx_packet.attime << ")" << std::endl;
        } else {
            std::cerr << "Failed to send packet via Intel AVB HAL" << std::endl;
        }

        // Clean up any completed transmissions
        struct intel_packet *cleaned_packets = nullptr;
        intel_clean(&intel_device, &cleaned_packets);
    }
};

int main() {
    std::cout << "=== Intel AVB AVDECC Entity (Native Intel HAL) ===" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
    std::cout << "Capabilities: AEM, Audio Source/Sink, gPTP Support" << std::endl;
    std::cout << "Target: Hive AVDECC Controller Discovery" << std::endl;
    std::cout << "Protocol: IEEE 1722.1-2021 Layer 2 (EtherType 0x22F0)" << std::endl;
    std::cout << "Hardware: Intel I219-LM with AVB HAL integration" << std::endl;
    std::cout << std::endl;

    IntelAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "Failed to initialize Intel AVB AVDECC entity" << std::endl;
        return 1;
    }

    std::cout << "Press Ctrl+C to stop..." << std::endl;
    entity.start();

    return 0;
}
