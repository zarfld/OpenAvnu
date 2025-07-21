#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <fstream>

// Include Intel AVB Hardware Abstraction Layer instead of PCAP
extern "C" {
#include "../intel_avb/lib/intel.h"
}

#pragma comment(lib, "ws2_32.lib")

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
    uint8_t association_id[8];
    uint8_t reserved2[4];
};

class IntelAvbAVDECCEntity {
private:
    device_t intel_device;
    struct intel_dma_alloc tx_buffer;
    struct intel_dma_alloc rx_buffer;
    bool running;
    uint32_t available_index;
    uint16_t sequence_id;
    
    // Persistent available_index management
    uint32_t load_available_index() {
        std::ifstream file("available_index.dat", std::ios::binary);
        if (file.is_open()) {
            uint32_t saved_index;
            file.read(reinterpret_cast<char*>(&saved_index), sizeof(saved_index));
            file.close();
            std::cout << "📂 Loaded available_index from file: " << saved_index << std::endl;
            return saved_index;
        } else {
            std::cout << "📂 First run - starting available_index: 1000" << std::endl;
            return 1000;
        }
    }
    
    void save_available_index() {
        std::ofstream file("available_index.dat", std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(&available_index), sizeof(available_index));
            file.close();
            std::cout << "💾 Saved available_index: " << available_index << std::endl;
        }
    }
    
public:
    IntelAvbAVDECCEntity() : running(false), sequence_id(0) {
        memset(&intel_device, 0, sizeof(intel_device));
        memset(&tx_buffer, 0, sizeof(tx_buffer));
        memset(&rx_buffer, 0, sizeof(rx_buffer));
        available_index = load_available_index();
    }
    
    ~IntelAvbAVDECCEntity() {
        save_available_index();
        cleanup();
    }
    
    bool initialize() {
        std::cout << "🚀 Initializing Intel AVB Hardware Interface..." << std::endl;
        
        // Initialize device structure with known Intel hardware
        memset(&intel_device, 0, sizeof(intel_device));
        intel_device.pci_vendor_id = INTEL_VENDOR_ID;
        
        // Try common Intel device IDs (including your I219-LM)
        uint16_t intel_device_ids[] = {
            0x0dc7, // I219-LM (22) - Your actual hardware!
            0x15b7, // I219-LM
            0x15b8, // I219-V  
            0x15f2, // I225-LM
            0x15f3, // I225-V
            0x125b, // I226-LM
            0x125c, // I226-V
            0x1533, // I210
        };
        
        bool device_found = false;
        int probe_result = -1;
        
        for (int i = 0; i < sizeof(intel_device_ids)/sizeof(intel_device_ids[0]); i++) {
            intel_device.pci_device_id = intel_device_ids[i];
            
            // For Windows, use attach directly instead of probe+init
            char device_path[64];
            snprintf(device_path, sizeof(device_path), "intel:%04x", intel_device_ids[i]);
            
            std::cout << "🔌 Attempting to attach to Intel device: " << device_path << std::endl;
            int attach_result = intel_attach(device_path, &intel_device);
            if (attach_result == 0) {
                device_found = true;
                std::cout << "✅ Found and attached Intel device: " << intel_get_device_name(&intel_device) 
                          << " (ID: 0x" << std::hex << intel_device_ids[i] << ")" << std::endl;
                std::cout << "✅ Device type: " << std::dec << intel_device.device_type << std::endl;
                std::cout << "✅ Capabilities: 0x" << std::hex << intel_device.capabilities << std::endl;
                break;
            } else {
                std::cout << "❌ Failed to attach to device ID 0x" << std::hex << intel_device_ids[i] 
                          << " (error: " << std::dec << attach_result << ")" << std::endl;
            }
        }
        
        if (!device_found) {
            std::cerr << "❌ No supported Intel devices found or attached" << std::endl;
            return false;
        }

        // Continue with TX/RX setup
        int result;
        
        // Attach TX and RX
        result = intel_attach_tx(&intel_device);
        if (result != 0) {
            std::cerr << "❌ Failed to attach TX: " << result << std::endl;
            return false;
        }
        
        result = intel_attach_rx(&intel_device);
        if (result != 0) {
            std::cerr << "❌ Failed to attach RX: " << result << std::endl;
            return false;
        }
        
        // Allocate DMA buffers for TX and RX
        result = intel_dma_malloc_page(&intel_device, &tx_buffer);
        if (result != 0) {
            std::cerr << "❌ Failed to allocate TX buffer: " << result << std::endl;
            return false;
        }
        
        result = intel_dma_malloc_page(&intel_device, &rx_buffer);
        if (result != 0) {
            std::cerr << "❌ Failed to allocate RX buffer: " << result << std::endl;
            intel_dma_free_page(&intel_device, &tx_buffer);
            return false;
        }
        
        std::cout << "✅ Intel AVB interface initialized successfully" << std::endl;
        std::cout << "   TX Buffer: " << tx_buffer.dma_vaddr << " (size: " << tx_buffer.mmap_size << ")" << std::endl;
        std::cout << "   RX Buffer: " << rx_buffer.dma_vaddr << " (size: " << rx_buffer.mmap_size << ")" << std::endl;
        
        return true;
    }
    
    void cleanup() {
        if (tx_buffer.dma_vaddr) {
            intel_dma_free_page(&intel_device, &tx_buffer);
            memset(&tx_buffer, 0, sizeof(tx_buffer));
        }
        
        if (rx_buffer.dma_vaddr) {
            intel_dma_free_page(&intel_device, &rx_buffer);
            memset(&rx_buffer, 0, sizeof(rx_buffer));
        }
        
        intel_detach(&intel_device);
    }
    
    bool send_adp_message() {
        if (!tx_buffer.dma_vaddr) {
            std::cerr << "❌ TX buffer not allocated" << std::endl;
            return false;
        }
        
        // Create 82-byte packet: 14 (Ethernet) + 4 (AVTP) + 64 (ADP)
        uint8_t* raw_packet = static_cast<uint8_t*>(tx_buffer.dma_vaddr);
        size_t offset = 0;
        
        // Ethernet Header (14 bytes)
        // Destination: 91:E0:F0:01:00:00 (AVDECC multicast)
        raw_packet[offset++] = 0x91; raw_packet[offset++] = 0xE0; raw_packet[offset++] = 0xF0;
        raw_packet[offset++] = 0x01; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00;
        
        // Source: c0:47:e0:16:7b:89 (MAC derived from Entity ID)
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0;
        raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // EtherType: 0x22f0 (IEEE 1722)
        raw_packet[offset++] = 0x22; raw_packet[offset++] = 0xf0;
        
        // AVTP Header (4 bytes) - ADP
        raw_packet[offset++] = 0xfa; // subtype=0xfa (ADP), sv=0, version=0
        raw_packet[offset++] = 0x50; // message_type=0 (ENTITY_AVAILABLE), valid_time=10 (0x50)
        raw_packet[offset++] = 0x00; // control_data_length high byte
        raw_packet[offset++] = 0x38; // control_data_length = 56 (0x38)
        
        // ADP Message (64 bytes) - Manual construction
        // Entity ID: c0:47:e0:ff:fe:16:7b:89
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0; raw_packet[offset++] = 0xff;
        raw_packet[offset++] = 0xfe; raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // Entity Model ID: 00:17:ff:fe:00:00:00:01 (IEEE registered)
        raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x17; raw_packet[offset++] = 0xff; raw_packet[offset++] = 0xfe;
        raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x01;
        
        // Entity Capabilities (4 bytes) - AEM_SUPPORTED + CLASS_A_SUPPORTED
        uint32_t capabilities = htonl(0x00000001 | 0x00000008);
        memcpy(&raw_packet[offset], &capabilities, 4); offset += 4;
        
        // Talker Stream Sources (2 bytes)
        uint16_t talker_sources = htons(1);
        memcpy(&raw_packet[offset], &talker_sources, 2); offset += 2;
        
        // Talker Capabilities (2 bytes)
        uint16_t talker_caps = htons(0x4001); // IMPLEMENTED + OTHER_SOURCE
        memcpy(&raw_packet[offset], &talker_caps, 2); offset += 2;
        
        // Listener Stream Sinks (2 bytes)
        uint16_t listener_sinks = htons(1);
        memcpy(&raw_packet[offset], &listener_sinks, 2); offset += 2;
        
        // Listener Capabilities (2 bytes)
        uint16_t listener_caps = htons(0x4001); // IMPLEMENTED + OTHER_SINK
        memcpy(&raw_packet[offset], &listener_caps, 2); offset += 2;
        
        // Controller Capabilities (4 bytes) - IMPLEMENTED
        uint32_t controller_caps = htonl(0x00000001);
        memcpy(&raw_packet[offset], &controller_caps, 4); offset += 4;
        
        // Available Index (4 bytes) - Keep stable until ACTUAL state change
        // IEEE 1722.1: Only increment when entity state actually changes (not on periodic ADP)
        uint32_t avail_idx = htonl(available_index);
        memcpy(&raw_packet[offset], &avail_idx, 4); offset += 4;
        
        // gPTP Grandmaster ID (8 bytes)
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0; raw_packet[offset++] = 0xff;
        raw_packet[offset++] = 0xfe; raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // gPTP Domain Number + Reserved (4 bytes)
        raw_packet[offset++] = 0x00; // domain 0
        raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; // reserved
        
        // Identify Control Index (2 bytes)
        uint16_t identify_idx = htons(0);
        memcpy(&raw_packet[offset], &identify_idx, 2); offset += 2;
        
        // Interface Index (2 bytes)
        uint16_t interface_idx = htons(0);
        memcpy(&raw_packet[offset], &interface_idx, 2); offset += 2;
        
        // Association ID (8 bytes) - same as Entity ID
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0; raw_packet[offset++] = 0xff;
        raw_packet[offset++] = 0xfe; raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // Reserved (4 bytes)
        raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00;
        
        // Create Intel packet structure
        struct intel_packet tx_packet;
        memset(&tx_packet, 0, sizeof(tx_packet));
        
        tx_packet.map.paddr = tx_buffer.dma_paddr;
        tx_packet.map.mmap_size = tx_buffer.mmap_size;
        tx_packet.vaddr = tx_buffer.dma_vaddr;
        tx_packet.len = 82; // Total packet size
        tx_packet.offset = 0;
        tx_packet.flags = 0; // No special timing for ADP
        
        // Get hardware timestamp for transmission time
        uint64_t current_time;
        int result = intel_get_systime(&intel_device, &current_time);
        if (result == 0) {
            tx_packet.attime = current_time + 1000; // Send in 1 microsecond
            tx_packet.flags |= INTEL_PACKET_LAUNCHTIME;
        }
        
        // Transmit packet using Intel AVB hardware
        result = intel_xmit(&intel_device, 0, &tx_packet); // Queue 0
        if (result != 0) {
            std::cout << "❌ Failed to transmit ADP packet via Intel AVB: " << result << std::endl;
            return false;
        }
        
        std::cout << "📤 ADP ENTITY_AVAILABLE sent via Intel AVB (available_index=" << available_index 
                  << " - stable until state change)" << std::endl;
        return true;
    }
    
    bool send_aem_response(const uint8_t* source_mac, uint16_t command_type, uint16_t seq_id, uint8_t status = 0) {
        if (!tx_buffer.dma_vaddr) {
            std::cerr << "❌ TX buffer not allocated" << std::endl;
            return false;
        }
        
        // Create AEM response packet
        uint8_t* raw_packet = static_cast<uint8_t*>(tx_buffer.dma_vaddr);
        size_t offset = 0;
        
        // Ethernet Header (14 bytes)
        // Destination: source MAC from incoming command
        memcpy(&raw_packet[offset], source_mac, 6); offset += 6;
        
        // Source: our MAC (consistent with Entity ID)
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0;
        raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // EtherType: 0x22f0 (IEEE 1722)
        raw_packet[offset++] = 0x22; raw_packet[offset++] = 0xf0;
        
        // AVTP Header (4 bytes)
        raw_packet[offset++] = 0xfb; // subtype=0xfb (AECP), sv=0, version=0
        raw_packet[offset++] = 0x00; // status + control_data_length high
        raw_packet[offset++] = 0x00; // control_data_length low  
        raw_packet[offset++] = 0x2c; // control_data_length = 44 for basic response
        
        // AECP AEM Response (minimum required fields)
        // Target GUID (8 bytes) - controller that sent the command  
        raw_packet[offset++] = 0x48; raw_packet[offset++] = 0x0b; raw_packet[offset++] = 0xb2; raw_packet[offset++] = 0xd9;
        raw_packet[offset++] = 0x6a; raw_packet[offset++] = 0xd3; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x82;
        
        // Controller GUID (8 bytes) - our entity
        raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0; raw_packet[offset++] = 0xff;
        raw_packet[offset++] = 0xfe; raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
        
        // Sequence ID (2 bytes) - echo back the sequence from command
        uint16_t seq = htons(seq_id);
        memcpy(&raw_packet[offset], &seq, 2); offset += 2;
        
        // Command Type (2 bytes) - with response bit set
        uint16_t cmd = htons(command_type | 0x8000); // Set response bit
        memcpy(&raw_packet[offset], &cmd, 2); offset += 2;
        
        // Status (1 byte)
        raw_packet[offset++] = status; // 0 = SUCCESS
        
        // For READ_DESCRIPTOR commands, add minimal descriptor data
        if (command_type == 0x0002 && status == 0) {
            // Descriptor Type (2 bytes) - ENTITY_DESCRIPTOR = 0x0000
            raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00;
            // Descriptor Index (2 bytes) - 0
            raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00;
            // Entity ID (8 bytes)
            raw_packet[offset++] = 0xc0; raw_packet[offset++] = 0x47; raw_packet[offset++] = 0xe0; raw_packet[offset++] = 0xff;
            raw_packet[offset++] = 0xfe; raw_packet[offset++] = 0x16; raw_packet[offset++] = 0x7b; raw_packet[offset++] = 0x89;
            // Entity Model ID (8 bytes)
            raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x17; raw_packet[offset++] = 0xff; raw_packet[offset++] = 0xfe;
            raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x00; raw_packet[offset++] = 0x01;
            // Entity Capabilities (4 bytes)
            uint32_t caps = htonl(0x00000001 | 0x00000008); // AEM_SUPPORTED + CLASS_A_SUPPORTED
            memcpy(&raw_packet[offset], &caps, 4); offset += 4;
            
            // Update control_data_length for descriptor response
            raw_packet[17] = 0x30; // 48 bytes of control data
        }
        
        // Pad to minimum frame size
        while (offset < 60) {
            raw_packet[offset++] = 0x00;
        }
        
        // Create Intel packet structure
        struct intel_packet tx_packet;
        memset(&tx_packet, 0, sizeof(tx_packet));
        
        tx_packet.map.paddr = tx_buffer.dma_paddr;
        tx_packet.map.mmap_size = tx_buffer.mmap_size;
        tx_packet.vaddr = tx_buffer.dma_vaddr;
        tx_packet.len = offset;
        tx_packet.offset = 0;
        tx_packet.flags = 0; // No special timing for responses
        
        // Transmit response using Intel AVB hardware
        int result = intel_xmit(&intel_device, 0, &tx_packet); // Queue 0
        if (result != 0) {
            std::cout << "❌ Failed to send AEM response via Intel AVB: " << result << std::endl;
            return false;
        }
        
        std::cout << "📤 AEM Response sent via Intel AVB: cmd=0x" << std::hex << (command_type | 0x8000) 
                  << " seq=" << std::dec << seq_id << " status=" << (int)status << std::endl;
        return true;
    }
    
    void check_received_packets() {
        struct intel_packet *received_packets = nullptr;
        uint32_t count = 0;
        
        // Check for received packets using Intel AVB
        int result = intel_receive(&intel_device, 0, &received_packets, &count);
        if (result != 0 || count == 0) {
            return; // No packets or error
        }
        
        // Process received packets
        for (uint32_t i = 0; i < count; i++) {
            struct intel_packet *packet = &received_packets[i];
            uint8_t *frame_data = static_cast<uint8_t*>(packet->vaddr);
            
            if (packet->len < 60) continue; // Too short for AVDECC
            
            // Check if it's IEEE 1722 (EtherType 0x22f0)
            if (frame_data[12] != 0x22 || frame_data[13] != 0xf0) continue;
            
            // Check if it's AECP (subtype 0xfb)
            if (frame_data[14] != 0xfb) continue;
            
            // Check if target GUID matches our entity ID
            if (memcmp(&frame_data[18], "\xc0\x47\xe0\xff\xfe\x16\x7b\x89", 8) != 0) continue;
            
            // Extract command details
            uint16_t command_type = ntohs(*(uint16_t*)&frame_data[34]);
            uint16_t seq_id = ntohs(*(uint16_t*)&frame_data[32]);
            
            // Check if it's a command (not response)
            if (command_type & 0x8000) continue; // Already a response
            
            std::cout << "📥 Received AEM command via Intel AVB: type=0x" << std::hex << command_type 
                      << " seq=" << std::dec << seq_id << std::endl;
            
            // Respond to specific commands
            switch (command_type) {
                case 0x004b: // GET_DYNAMIC_INFO (Milan extension)
                    std::cout << "🎯 Responding to GET_DYNAMIC_INFO command" << std::endl;
                    send_aem_response(&frame_data[6], command_type, seq_id, 0); // SUCCESS
                    break;
                    
                case 0x0000: // ACQUIRE_ENTITY
                    std::cout << "🎯 Responding to ACQUIRE_ENTITY command" << std::endl;
                    send_aem_response(&frame_data[6], command_type, seq_id, 0); // SUCCESS
                    available_index++;
                    save_available_index();
                    std::cout << "🔄 Entity state changed - available_index now " << available_index << std::endl;
                    break;
                    
                case 0x0001: // LOCK_ENTITY / RELEASE_ENTITY  
                    std::cout << "🎯 Responding to LOCK/RELEASE_ENTITY command" << std::endl;
                    send_aem_response(&frame_data[6], command_type, seq_id, 0); // SUCCESS
                    available_index++;
                    save_available_index();
                    std::cout << "🔄 Entity state changed - available_index now " << available_index << std::endl;
                    break;
                    
                case 0x0002: // READ_DESCRIPTOR
                    std::cout << "🎯 Responding to READ_DESCRIPTOR command" << std::endl;
                    send_aem_response(&frame_data[6], command_type, seq_id, 0); // SUCCESS with data
                    break;
                    
                default:
                    std::cout << "❓ Unknown command, responding with NOT_IMPLEMENTED" << std::endl;
                    send_aem_response(&frame_data[6], command_type, seq_id, 9); // NOT_IMPLEMENTED
                    break;
            }
        }
        
        // Clean up processed packets
        struct intel_packet *cleaned_packets = nullptr;
        intel_clean(&intel_device, &cleaned_packets);
    }
    
    void run() {
        running = true;
        std::cout << "🚀 Starting Intel AVB AVDECC entity..." << std::endl;
        std::cout << "   Entity ID: 0xc047e0fffe167b89" << std::endl;
        std::cout << "   MAC: c0:47:e0:16:7b:89 (consistent with Entity ID)" << std::endl;
        std::cout << "   Intel Device: " << intel_get_device_name(&intel_device) << std::endl;
        std::cout << "   Hardware Features: Direct Intel AVB transmission" << std::endl;
        std::cout << std::endl;
        
        // Send initial ADP message with current available_index (no increment needed)
        send_adp_message();
        
        auto last_adp = std::chrono::steady_clock::now();
        
        while (running) {
            // Check for incoming packets
            check_received_packets();
            
            // Send periodic ADP messages every 2 seconds with SAME available_index
            // IEEE 1722.1: available_index only increments on ACTUAL state changes
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_adp).count() >= 2) {
                send_adp_message(); // Send with current available_index (no increment!)
                last_adp = now;
            }
            
            // Small sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        std::cout << "🛑 Intel AVB AVDECC entity stopped" << std::endl;
    }
    
    void stop() {
        running = false;
    }
};

int main() {
    std::cout << "🎯 Intel AVB Hardware AVDECC Entity" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "✅ Entity ID: 0xc047e0fffe167b89 (fixed corruption)" << std::endl;
    std::cout << "✅ Hardware: Direct Intel AVB transmission (no PCAP)" << std::endl;
    std::cout << "✅ Features: Hardware timestamping, DMA buffers, Intel optimizations" << std::endl;
    std::cout << "✅ ADP Discovery: Periodic ENTITY_AVAILABLE messages" << std::endl;
    std::cout << "✅ AEM Responses: Responds to Hive controller commands" << std::endl;
    std::cout << "✅ Available Index: Properly incremented for state changes" << std::endl;
    std::cout << std::endl;
    
    IntelAvbAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "❌ Failed to initialize Intel AVB AVDECC entity" << std::endl;
        return 1;
    }
    
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;
    
    try {
        entity.run();
    } catch (const std::exception& e) {
        std::cerr << "💥 Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
