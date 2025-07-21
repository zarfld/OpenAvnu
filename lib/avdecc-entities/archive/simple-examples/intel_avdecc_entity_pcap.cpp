#include <winsock2.h>
#include <windows.h>
#include <pcap.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "ws2_32.lib")

// IEEE 1722.1 AVDECC Protocol Constants
#define AVDECC_ETHERTYPE 0x22F0
#define AVTP_SUBTYPE_ADP 0xFA
#define ADP_MESSAGE_TYPE_ENTITY_AVAILABLE 0x00
#define ADP_MESSAGE_TYPE_ENTITY_DEPARTING 0x01
#define ADP_MESSAGE_TYPE_ENTITY_DISCOVER 0x02

// AVDECC Multicast Address
const uint8_t AVDECC_MULTICAST_MAC[6] = {0x91, 0xE0, 0xF0, 0x01, 0x00, 0x00};

// Our Entity Configuration
const uint64_t ENTITY_ID = 0xC047E0FFFE167B89ULL;  // Based on Intel I219-LM MAC
const uint64_t ENTITY_MODEL_ID = 0xC047E0000001234ULL;
const uint32_t ENTITY_CAPABILITIES = 0x0000C588;  // AEM, VENDOR_UNIQUE, CLASS_A, gPTP
const uint16_t TALKER_STREAM_SOURCES = 1;
const uint16_t TALKER_CAPABILITIES = 0x4801;      // AUDIO_SOURCE, MEDIA_CLOCK_SOURCE
const uint16_t LISTENER_STREAM_SINKS = 1;
const uint16_t LISTENER_CAPABILITIES = 0x4801;    // AUDIO_SINK, MEDIA_CLOCK_SINK
const uint32_t CONTROLLER_CAPABILITIES = 0x00000001; // IMPLEMENTED

struct EthernetHeader {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed));

struct AVTPHeader {
    uint8_t subtype;
    uint8_t version_and_flags;
    uint16_t control_data_length;
} __attribute__((packed));

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
} __attribute__((packed));

class PCAPAVDECCEntity {
private:
    pcap_t* pcap_handle;
    uint8_t local_mac[6];
    uint32_t available_index;
    bool running;

public:
    PCAPAVDECCEntity() : pcap_handle(nullptr), available_index(0), running(false) {
        // Initialize local MAC (Intel I219-LM)
        local_mac[0] = 0xC0; local_mac[1] = 0x47; local_mac[2] = 0x0E;
        local_mac[3] = 0x16; local_mac[4] = 0x7B; local_mac[5] = 0x89;
    }

    ~PCAPAVDECCEntity() {
        stop();
    }

    bool initialize() {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_if_t* devices = nullptr;
        
        // Find network interfaces
        if (pcap_findalldevs(&devices, errbuf) == -1) {
            std::cerr << "Error finding devices: " << errbuf << std::endl;
            return false;
        }

        // Look for Intel I219-LM interface
        pcap_if_t* intel_device = nullptr;
        for (pcap_if_t* device = devices; device != nullptr; device = device->next) {
            std::cout << "Found interface: " << device->name << std::endl;
            if (device->description) {
                std::cout << "  Description: " << device->description << std::endl;
            }
            
            // Try to match Intel interface (you may need to adjust this)
            if (strstr(device->description, "Intel") || strstr(device->name, "Intel")) {
                intel_device = device;
                break;
            }
        }

        if (!intel_device) {
            std::cerr << "Intel network interface not found" << std::endl;
            pcap_freealldevs(devices);
            return false;
        }

        std::cout << "Using Intel interface: " << intel_device->name << std::endl;

        // Open the interface for packet injection
        pcap_handle = pcap_open_live(intel_device->name, 65536, 1, 1000, errbuf);
        pcap_freealldevs(devices);

        if (!pcap_handle) {
            std::cerr << "Error opening interface: " << errbuf << std::endl;
            return false;
        }

        std::cout << "PCAP AVDECC Entity initialized successfully" << std::endl;
        return true;
    }

    void start() {
        if (!pcap_handle) {
            std::cerr << "Entity not initialized" << std::endl;
            return;
        }

        running = true;
        std::cout << "Starting AVDECC Entity on Intel I219-LM..." << std::endl;

        // Send initial ENTITY_AVAILABLE
        send_entity_available();

        // Start discovery listening thread
        std::thread discovery_thread(&PCAPAVDECCEntity::discovery_loop, this);
        
        // Periodic ENTITY_AVAILABLE broadcast
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (running) {
                send_entity_available();
            }
        }

        discovery_thread.join();
    }

    void stop() {
        running = false;
        if (pcap_handle) {
            // Send ENTITY_DEPARTING
            send_entity_departing();
            pcap_close(pcap_handle);
            pcap_handle = nullptr;
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

    void send_adp_message(uint8_t message_type) {
        std::vector<uint8_t> packet(sizeof(EthernetHeader) + sizeof(AVTPHeader) + sizeof(ADPMessage));
        
        // Ethernet Header
        EthernetHeader* eth = reinterpret_cast<EthernetHeader*>(packet.data());
        memcpy(eth->dest_mac, AVDECC_MULTICAST_MAC, 6);
        memcpy(eth->src_mac, local_mac, 6);
        eth->ethertype = htons(AVDECC_ETHERTYPE);

        // AVTP Header
        AVTPHeader* avtp = reinterpret_cast<AVTPHeader*>(packet.data() + sizeof(EthernetHeader));
        avtp->subtype = AVTP_SUBTYPE_ADP;
        avtp->version_and_flags = 0x00;  // Version 0, no Stream ID valid
        avtp->control_data_length = htons(sizeof(ADPMessage));

        // ADP Message
        ADPMessage* adp = reinterpret_cast<ADPMessage*>(packet.data() + sizeof(EthernetHeader) + sizeof(AVTPHeader));
        adp->message_type_and_valid_time = (message_type << 4) | 0x05;  // Valid time = 10 seconds
        memset(adp->reserved, 0, 3);
        adp->entity_id = htonll(ENTITY_ID);
        adp->entity_model_id = htonll(ENTITY_MODEL_ID);
        adp->entity_capabilities = htonl(ENTITY_CAPABILITIES);
        adp->talker_stream_sources = htons(TALKER_STREAM_SOURCES);
        adp->talker_capabilities = htons(TALKER_CAPABILITIES);
        adp->listener_stream_sinks = htons(LISTENER_STREAM_SINKS);
        adp->listener_capabilities = htons(LISTENER_CAPABILITIES);
        adp->controller_capabilities = htonl(CONTROLLER_CAPABILITIES);
        adp->available_index = htonl(available_index);
        adp->gptp_grandmaster_id = htonll(ENTITY_ID);  // Use entity ID as grandmaster
        adp->association_id = 0;

        // Send packet
        if (pcap_sendpacket(pcap_handle, packet.data(), packet.size()) != 0) {
            std::cerr << "Error sending packet: " << pcap_geterr(pcap_handle) << std::endl;
        } else {
            std::cout << "Sent " << (message_type == ADP_MESSAGE_TYPE_ENTITY_AVAILABLE ? "ENTITY_AVAILABLE" : "ENTITY_DEPARTING") 
                      << " (available_index=" << available_index << ")" << std::endl;
        }
    }

    void discovery_loop() {
        // TODO: Listen for ENTITY_DISCOVER messages and respond
        std::cout << "Discovery loop started (listening for ENTITY_DISCOVER)" << std::endl;
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    uint64_t htonll(uint64_t value) {
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    }
};

int main() {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    std::cout << "=== Intel I219-LM AVDECC Entity (PCAP Implementation) ===" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
    std::cout << "Capabilities: AEM, Audio Source/Sink, gPTP Support" << std::endl;
    std::cout << "Target: Hive AVDECC Controller Discovery" << std::endl;
    std::cout << "Protocol: IEEE 1722.1-2021 Layer 2 (EtherType 0x22F0)" << std::endl;
    std::cout << std::endl;

    PCAPAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "Failed to initialize AVDECC entity" << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Press Ctrl+C to stop..." << std::endl;
    entity.start();

    WSACleanup();
    return 0;
}
