// ============================================================================
// Simple ASIO Audio Listener for OpenAvnu Testing
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define MULTICAST_GROUP "91.E0.F0.01"  // AVB multicast address
#define PORT 17220                      // AVB port
#define BUFFER_SIZE 1500

int main(int argc, char *argv[]) {
    printf("\n");
    printf("🎵 OpenAvnu ASIO Audio Listener (Test Version) 🎵\n");
    printf("=================================================\n");
    printf("\n");
    
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int packet_count = 0;
    DWORD reuse = 1;
    
    // Initialize Winsock
    printf("🔧 Initializing Windows Socket API...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    printf("✅ Winsock initialized\n");
    
    // Create socket
    printf("🔗 Creating UDP socket...\n");
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("❌ Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("✅ Socket created\n");
    
    // Set socket options for reuse
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
        printf("⚠️  SO_REUSEADDR failed, continuing anyway...\n");
    }
    
    // Bind socket
    printf("🌐 Binding to port %d...\n", PORT);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("❌ Bind failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("✅ Socket bound to port %d\n", PORT);
    
    // Join multicast group
    printf("📡 Joining AVB multicast group %s...\n", MULTICAST_GROUP);
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
        printf("⚠️  Multicast join failed: %d (continuing in unicast mode)\n", WSAGetLastError());
    } else {
        printf("✅ Joined multicast group\n");
    }
    
    printf("\n");
    printf("🎧 ASIO Audio Listener Ready!\n");
    printf("────────────────────────────────\n");
    printf("Listening for AVB audio streams on:\n");
    printf("  Port: %d\n", PORT);
    printf("  Multicast: %s\n", MULTICAST_GROUP);
    printf("  Press Ctrl+C to stop\n");
    printf("\n");
    
    // Main listening loop
    while (1) {
        bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        
        if (bytes_received == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                Sleep(1);
                continue;
            }
            printf("❌ Receive error: %d\n", error);
            break;
        }
        
        if (bytes_received > 0) {
            packet_count++;
            
            // Print packet info every 100 packets
            if (packet_count % 100 == 0) {
                printf("📦 Received %d packets (latest: %d bytes)\n", packet_count, bytes_received);
            }
            
            // Simple audio data detection
            if (bytes_received > 20) {
                // Check if this looks like AVB/AVTP data
                unsigned char* data = (unsigned char*)buffer;
                if (data[12] == 0x22 && data[13] == 0xF0) {  // AVTP subtype
                    printf("🎵 AVB Audio packet detected! Size: %d bytes\n", bytes_received);
                }
            }
        }
    }
    
    // Cleanup
    printf("\n🧹 Cleaning up...\n");
    closesocket(sock);
    WSACleanup();
    printf("✅ ASIO Listener stopped\n");
    
    return 0;
}
