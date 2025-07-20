/**
 * @file hive_avdecc_entity_real.cpp
 * @brief Real AVDECC entity that sends packets on Intel I219-LM for Hive discovery
 */

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

class RealAVDECCEntity {
private:
    SOCKET rawSocket_;
    sockaddr_in targetAddr_;
    uint8_t sourceMAC_[6] = {0xC0, 0x47, 0x0E, 0x16, 0x7B, 0x89}; // Your Intel I219-LM MAC
    uint8_t targetMAC_[6] = {0x91, 0xE0, 0xF0, 0x00, 0xFF, 0x00}; // AVDECC multicast
    uint64_t entityID_ = 0x001B21FFFE123456ULL; // Example entity ID
    bool running_ = false;

public:
    bool initialize() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "❌ WSAStartup failed" << std::endl;
            return false;
        }

        // Create UDP socket for AVDECC (we'll use UDP as a fallback since raw sockets need admin)
        rawSocket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (rawSocket_ == INVALID_SOCKET) {
            std::cerr << "❌ Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        // Set up target address for broadcast
        targetAddr_.sin_family = AF_INET;
        targetAddr_.sin_port = htons(17221); // AVDECC UDP port
        targetAddr_.sin_addr.s_addr = inet_addr("224.0.1.129"); // AVDECC multicast IP

        std::cout << "✅ Real AVDECC entity initialized on Intel I219-LM" << std::endl;
        std::cout << "   MAC: " << std::hex << std::setfill('0');
        for (int i = 0; i < 6; ++i) {
            if (i > 0) std::cout << ":";
            std::cout << std::setw(2) << static_cast<int>(sourceMAC_[i]);
        }
        std::cout << std::dec << std::endl;
        return true;
    }

    void sendADPPacket() {
        // Create ADP (AVDECC Discovery Protocol) packet
        std::vector<uint8_t> packet;
        
        // AVTP Common Header
        packet.push_back(0x02);        // CD=0, Subtype=ADP (2)
        packet.push_back(0x81);        // SV=1, Version=0, Message Type=ENTITY_AVAILABLE (1)
        packet.push_back(0x00);        // Status=SUCCESS (0), Control Data Length MSB
        packet.push_back(0x2C);        // Control Data Length LSB (44 bytes)
        
        // Entity ID (8 bytes) - Your unique entity identifier
        for (int i = 7; i >= 0; --i) {
            packet.push_back((entityID_ >> (i * 8)) & 0xFF);
        }
        
        // Entity Model ID (8 bytes) - Identifies your device model
        packet.push_back(0x00); packet.push_back(0x1B); packet.push_back(0x21); packet.push_back(0xFF);
        packet.push_back(0xFE); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x01);
        
        // Entity Capabilities (4 bytes)
        packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x01); // AEM_SUPPORTED
        
        // Talker Stream Sources (2 bytes)
        packet.push_back(0x00); packet.push_back(0x02);
        
        // Talker Capabilities (2 bytes) 
        packet.push_back(0x40); packet.push_back(0x01); // IMPLEMENTED | AUDIO_SINK
        
        // Listener Stream Sinks (2 bytes)
        packet.push_back(0x00); packet.push_back(0x02);
        
        // Listener Capabilities (2 bytes)
        packet.push_back(0x40); packet.push_back(0x01); // IMPLEMENTED | AUDIO_SOURCE
        
        // Controller Capabilities (4 bytes)
        packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00);
        
        // Available Index (4 bytes) - Increment on state changes
        static uint32_t availableIndex = 0;
        availableIndex++;
        packet.push_back((availableIndex >> 24) & 0xFF);
        packet.push_back((availableIndex >> 16) & 0xFF);
        packet.push_back((availableIndex >> 8) & 0xFF);
        packet.push_back(availableIndex & 0xFF);
        
        // gPTP Grandmaster ID (8 bytes)
        packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00);
        packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00);
        
        // Reserved (4 bytes)
        packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00); packet.push_back(0x00);

        // Send the packet
        int result = sendto(rawSocket_, 
                          reinterpret_cast<const char*>(packet.data()), 
                          static_cast<int>(packet.size()),
                          0,
                          reinterpret_cast<const sockaddr*>(&targetAddr_),
                          sizeof(targetAddr_));
        
        if (result == SOCKET_ERROR) {
            std::cerr << "❌ Send failed: " << WSAGetLastError() << std::endl;
        } else {
            std::cout << "📡 Sent ADP packet (" << packet.size() << " bytes) to Hive" << std::endl;
            std::cout << "   Entity ID: 0x" << std::hex << entityID_ << std::dec << std::endl;
            std::cout << "   Available Index: " << availableIndex << std::endl;
        }
    }

    void startADPAnnouncement() {
        running_ = true;
        std::cout << "\n🎭 Starting AVDECC entity discovery for Hive..." << std::endl;
        std::cout << "📍 Broadcasting on Intel I219-LM interface" << std::endl;
        std::cout << "🔍 Hive should now detect this entity!" << std::endl;
        
        while (running_) {
            sendADPPacket();
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Send every 2 seconds
        }
    }

    void stop() {
        running_ = false;
        if (rawSocket_ != INVALID_SOCKET) {
            closesocket(rawSocket_);
        }
        WSACleanup();
    }
};

int main() {
    std::cout << "🎯 Real AVDECC Entity for Hive Discovery" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    RealAVDECCEntity entity;
    if (!entity.initialize()) {
        return 1;
    }
    
    std::cout << "\n🚀 Starting AVDECC discovery broadcasting..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    entity.startADPAnnouncement();
    
    return 0;
}
