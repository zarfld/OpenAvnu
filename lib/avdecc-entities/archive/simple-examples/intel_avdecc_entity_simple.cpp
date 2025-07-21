#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

// IEEE 1722.1 AVDECC Protocol Constants
#define AVDECC_MULTICAST_IP "224.0.1.129"  // Alternative: use IPv4 multicast for discovery
#define AVDECC_PORT 17221

// Our Entity Configuration (Intel I219-LM based)
const uint64_t ENTITY_ID = 0xC047E0FFFE167B89ULL;  // Based on Intel I219-LM MAC
const uint64_t ENTITY_MODEL_ID = 0xC047E0000001234ULL;
const char* ENTITY_NAME = "Intel I219-LM AVDECC Entity";

struct SimpleAVDECCPacket {
    uint32_t magic;           // 0x22F0FA00 (EtherType + Subtype + Message Type)
    uint64_t entity_id;
    uint64_t entity_model_id;
    uint32_t capabilities;
    uint32_t available_index;
    char entity_name[64];
} __attribute__((packed));

class SimpleAVDECCEntity {
private:
    SOCKET udp_socket;
    sockaddr_in multicast_addr;
    uint32_t available_index;
    bool running;

public:
    SimpleAVDECCEntity() : udp_socket(INVALID_SOCKET), available_index(0), running(false) {}

    ~SimpleAVDECCEntity() {
        stop();
    }

    bool initialize() {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }

        // Create UDP socket
        udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udp_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Enable broadcast
        BOOL broadcast = TRUE;
        if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast)) == SOCKET_ERROR) {
            std::cerr << "Failed to enable broadcast: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Setup multicast address
        memset(&multicast_addr, 0, sizeof(multicast_addr));
        multicast_addr.sin_family = AF_INET;
        multicast_addr.sin_port = htons(AVDECC_PORT);
        multicast_addr.sin_addr.s_addr = inet_addr(AVDECC_MULTICAST_IP);

        // Get local IP address bound to Intel I219-LM
        std::string local_ip = get_intel_interface_ip();
        if (!local_ip.empty()) {
            std::cout << "Found Intel I219-LM interface IP: " << local_ip << std::endl;
            
            // Bind to specific interface
            sockaddr_in bind_addr = {};
            bind_addr.sin_family = AF_INET;
            bind_addr.sin_addr.s_addr = inet_addr(local_ip.c_str());
            bind_addr.sin_port = 0;  // Let system choose port
            
            if (bind(udp_socket, (sockaddr*)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR) {
                std::cout << "Warning: Could not bind to Intel interface, using default" << std::endl;
            }
        }

        std::cout << "Simple AVDECC Entity initialized (UDP simulation)" << std::endl;
        std::cout << "Target multicast: " << AVDECC_MULTICAST_IP << ":" << AVDECC_PORT << std::endl;
        return true;
    }

    void start() {
        if (udp_socket == INVALID_SOCKET) {
            std::cerr << "Entity not initialized" << std::endl;
            return;
        }

        running = true;
        std::cout << "Starting AVDECC Entity simulation..." << std::endl;
        std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
        std::cout << "This simulates Layer 2 AVDECC using UDP multicast" << std::endl;
        std::cout << "Real implementation would use EtherType 0x22F0" << std::endl;
        std::cout << std::endl;

        // Send initial announcement
        send_entity_available();

        // Periodic announcements
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (running) {
                send_entity_available();
            }
        }
    }

    void stop() {
        running = false;
        if (udp_socket != INVALID_SOCKET) {
            send_entity_departing();
            closesocket(udp_socket);
            udp_socket = INVALID_SOCKET;
        }
        WSACleanup();
    }

private:
    void send_entity_available() {
        SimpleAVDECCPacket packet = {};
        packet.magic = 0x22F0FA00;  // EtherType 0x22F0 + ADP Subtype 0xFA + ENTITY_AVAILABLE 0x00
        packet.entity_id = htonll(ENTITY_ID);
        packet.entity_model_id = htonll(ENTITY_MODEL_ID);
        packet.capabilities = htonl(0x0000C588);  // AEM, VENDOR_UNIQUE, CLASS_A, gPTP
        packet.available_index = htonl(available_index);
        strncpy(packet.entity_name, ENTITY_NAME, sizeof(packet.entity_name) - 1);

        // Send to multicast group
        int result = sendto(udp_socket, (char*)&packet, sizeof(packet), 0, 
                           (sockaddr*)&multicast_addr, sizeof(multicast_addr));
        
        if (result == SOCKET_ERROR) {
            std::cerr << "Failed to send packet: " << WSAGetLastError() << std::endl;
        } else {
            std::cout << "Sent ENTITY_AVAILABLE simulation (index=" << available_index 
                      << ", " << result << " bytes)" << std::endl;
        }
        
        available_index++;
    }

    void send_entity_departing() {
        SimpleAVDECCPacket packet = {};
        packet.magic = 0x22F0FA01;  // EtherType 0x22F0 + ADP Subtype 0xFA + ENTITY_DEPARTING 0x01
        packet.entity_id = htonll(ENTITY_ID);
        packet.entity_model_id = htonll(ENTITY_MODEL_ID);
        packet.available_index = htonl(available_index);
        strncpy(packet.entity_name, ENTITY_NAME, sizeof(packet.entity_name) - 1);

        sendto(udp_socket, (char*)&packet, sizeof(packet), 0, 
               (sockaddr*)&multicast_addr, sizeof(multicast_addr));
        
        std::cout << "Sent ENTITY_DEPARTING simulation" << std::endl;
    }

    std::string get_intel_interface_ip() {
        // Get adapter information to find Intel I219-LM
        ULONG buflen = 15000;
        PIP_ADAPTER_INFO adapter_info = (IP_ADAPTER_INFO*)malloc(buflen);
        
        if (GetAdaptersInfo(adapter_info, &buflen) == ERROR_BUFFER_OVERFLOW) {
            free(adapter_info);
            adapter_info = (IP_ADAPTER_INFO*)malloc(buflen);
        }

        if (GetAdaptersInfo(adapter_info, &buflen) == NO_ERROR) {
            PIP_ADAPTER_INFO adapter = adapter_info;
            while (adapter) {
                // Check if this is Intel I219-LM (MAC: C0:47:0E:16:7B:89)
                if (adapter->AddressLength == 6) {
                    if (adapter->Address[0] == 0xC0 && adapter->Address[1] == 0x47 && 
                        adapter->Address[2] == 0x0E && adapter->Address[3] == 0x16 &&
                        adapter->Address[4] == 0x7B && adapter->Address[5] == 0x89) {
                        
                        std::string ip = adapter->IpAddressList.IpAddress.String;
                        free(adapter_info);
                        return ip;
                    }
                }
                adapter = adapter->Next;
            }
        }
        
        if (adapter_info) free(adapter_info);
        return "";
    }

    uint64_t htonll(uint64_t value) {
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    }
};

int main() {
    std::cout << "=== Intel I219-LM AVDECC Entity (Simple UDP Simulation) ===" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << ENTITY_ID << std::dec << std::endl;
    std::cout << "Target: Demonstrate AVDECC concepts" << std::endl;
    std::cout << "Note: This is a UDP simulation - real AVDECC uses Layer 2" << std::endl;
    std::cout << "MAC Address: C0:47:0E:16:7B:89 (Intel I219-LM)" << std::endl;
    std::cout << std::endl;

    SimpleAVDECCEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "Failed to initialize AVDECC entity" << std::endl;
        return 1;
    }

    std::cout << "Analysis of your Wireshark capture:" << std::endl;
    std::cout << "- RME MADIface (48:0b:b2:d9:6a:d2) sends real AVDECC packets" << std::endl;
    std::cout << "- Intel I219-LM (c0:47:0e:16:7b:89) sends IPv4 multicast (SOAP?)" << std::endl;
    std::cout << "- Hive is likely listening on the RME interface" << std::endl;
    std::cout << std::endl;
    std::cout << "This simulation runs on Intel interface to test UDP concept..." << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    entity.start();

    return 0;
}
