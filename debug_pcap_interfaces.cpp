#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <pcap.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")

int main() {
    std::cout << "🔍 PCAP Interface Debug Tool" << std::endl;
    std::cout << "=============================" << std::endl;
    
    // Check if running as administrator
    BOOL isAdmin = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    
    std::cout << "🔑 Administrator Rights: " << (isAdmin ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* alldevs;
    
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        std::cerr << "❌ Error finding devices: " << errbuf << std::endl;
        return 1;
    }
    
    std::cout << "📡 Available Network Interfaces:" << std::endl;
    int count = 0;
    for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
        count++;
        std::cout << "  " << count << ". " << d->name << std::endl;
        if (d->description) {
            std::cout << "     Description: " << d->description << std::endl;
        }
        
        // Show addresses if available
        for (pcap_addr_t* addr = d->addresses; addr != nullptr; addr = addr->next) {
            if (addr->addr && addr->addr->sa_family == AF_INET) {
                struct sockaddr_in* addr_in = (struct sockaddr_in*)addr->addr;
                std::cout << "     IP: " << inet_ntoa(addr_in->sin_addr) << std::endl;
            }
        }
        
        // Try to open this interface
        pcap_t* handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
        if (handle) {
            std::cout << "     ✅ Can open interface" << std::endl;
            
            // Test sending a simple packet
            uint8_t test_packet[60] = {0}; // Minimum Ethernet frame
            // Broadcast destination
            test_packet[0] = 0xff; test_packet[1] = 0xff; test_packet[2] = 0xff; 
            test_packet[3] = 0xff; test_packet[4] = 0xff; test_packet[5] = 0xff;
            // Test source MAC
            test_packet[6] = 0xaa; test_packet[7] = 0xbb; test_packet[8] = 0xcc; 
            test_packet[9] = 0xdd; test_packet[10] = 0xee; test_packet[11] = 0xff;
            // Test EtherType
            test_packet[12] = 0x08; test_packet[13] = 0x00; // IP
            
            int result = pcap_sendpacket(handle, test_packet, sizeof(test_packet));
            if (result == 0) {
                std::cout << "     ✅ Can send packets" << std::endl;
            } else {
                std::cout << "     ❌ Cannot send packets: " << pcap_geterr(handle) << std::endl;
            }
            
            pcap_close(handle);
        } else {
            std::cout << "     ❌ Cannot open: " << errbuf << std::endl;
        }
        std::cout << std::endl;
    }
    
    pcap_freealldevs(alldevs);
    
    if (!isAdmin) {
        std::cout << "⚠️  RECOMMENDATION: Run as Administrator for full packet injection capabilities" << std::endl;
    }
    
    return 0;
}
