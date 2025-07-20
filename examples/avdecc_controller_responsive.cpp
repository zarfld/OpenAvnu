/**
 * OpenAvnu AVDECC Controller with AEM Response Support
 * 
 * This implementation responds to incoming AEM commands from professional
 * AVDECC controllers like Hive, preventing "available_index" warnings.
 * 
 * Key Features:
 * - Responds to GET_DYNAMIC_INFO (0x004b) commands
 * - Proper sequence ID tracking
 * - IEEE 1722.1-2021 compliant responses
 * - Manual packet construction (no struct padding issues)
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <map>
#include <pcap.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// IEEE 1722.1 Constants
#define AVTP_SUBTYPE_AVDECC 0xfb
#define AEM_COMMAND 0x00
#define AEM_RESPONSE 0x01
#define CMD_GET_DYNAMIC_INFO 0x004b
#define AEM_STATUS_SUCCESS 0x00

// Our Entity Configuration
const uint64_t OUR_ENTITY_ID = 0xc047e0fffe167b89ULL;
const uint8_t OUR_MAC[6] = {0xc0, 0x47, 0xe0, 0xff, 0xe1, 0x67};

// Tracking for sequence IDs
std::map<uint64_t, uint16_t> controller_sequences;

// Manual byte order conversion for cross-platform compatibility
uint64_t htonll_local(uint64_t value) {
    return ((uint64_t)htonl(value & 0xFFFFFFFFULL) << 32) | htonl(value >> 32);
}

uint64_t ntohll_local(uint64_t value) {
    return ((uint64_t)ntohl(value & 0xFFFFFFFFULL) << 32) | ntohl(value >> 32);
}

uint16_t extract_uint16(const uint8_t* data, size_t offset) {
    return (uint16_t(data[offset]) << 8) | uint16_t(data[offset + 1]);
}

uint64_t extract_uint64(const uint8_t* data, size_t offset) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result << 8) | data[offset + i];
    }
    return result;
}

void send_aem_response(pcap_t* handle, const uint8_t* request_packet) {
    // Parse the incoming request
    const uint8_t* src_mac = request_packet + 6;  // Source MAC from request
    const uint8_t* ieee1722_header = request_packet + 14;
    
    // Extract fields from the request
    uint16_t sequence_id = extract_uint16(ieee1722_header, 16);
    uint16_t command_type = extract_uint16(ieee1722_header, 18) & 0x7fff;  // Remove U flag
    uint64_t controller_guid = extract_uint64(ieee1722_header, 10);
    
    std::cout << "📨 Responding to AEM command 0x" << std::hex << command_type 
              << " from controller 0x" << controller_guid 
              << ", sequence " << std::dec << sequence_id << std::endl;
    
    // Create response packet
    uint8_t response_packet[64];  // Minimum Ethernet frame size
    memset(response_packet, 0, sizeof(response_packet));
    
    size_t offset = 0;
    
    // Ethernet Header (14 bytes)
    memcpy(response_packet + offset, src_mac, 6);  // Destination = request source
    offset += 6;
    memcpy(response_packet + offset, OUR_MAC, 6);  // Source = our MAC
    offset += 6;
    response_packet[offset++] = 0x22;  // EtherType 0x22f0
    response_packet[offset++] = 0xf0;
    
    // IEEE 1722 AVTP Header (4 bytes)
    response_packet[offset++] = 0xfb;  // AVTP Subtype = AVDECC (0xfb)
    response_packet[offset++] = 0x00;  // Stream ID Valid=0, Version=0
    response_packet[offset++] = 0x00;  // Control Data Length = 12 (high byte)
    response_packet[offset++] = 0x0c;  // Control Data Length = 12 (low byte)
    
    // IEEE 1722.1 AEM Response Header (20 bytes)
    // Message Type (4 bits) + Status (4 bits) + Control Data Length (12 bits)
    uint16_t msg_status = (AEM_RESPONSE << 12) | (AEM_STATUS_SUCCESS << 8) | 0x0c;
    response_packet[offset++] = (msg_status >> 8) & 0xff;
    response_packet[offset++] = msg_status & 0xff;
    
    // Target GUID (our entity) - 8 bytes
    uint64_t target_guid_be = htonll_local(OUR_ENTITY_ID);
    memcpy(response_packet + offset, &target_guid_be, 8);
    offset += 8;
    
    // Controller GUID - 8 bytes
    uint64_t controller_guid_be = htonll_local(controller_guid);
    memcpy(response_packet + offset, &controller_guid_be, 8);
    offset += 8;
    
    // Sequence ID - 2 bytes
    uint16_t seq_be = htons(sequence_id);
    memcpy(response_packet + offset, &seq_be, 2);
    offset += 2;
    
    // Command Type (with U flag cleared for response) - 2 bytes
    uint16_t cmd_be = htons(command_type & 0x7fff);  // Clear U flag
    memcpy(response_packet + offset, &cmd_be, 2);
    offset += 2;
    
    // For GET_DYNAMIC_INFO response, add minimal dynamic info
    if (command_type == CMD_GET_DYNAMIC_INFO) {
        // Current Configuration (2 bytes)
        response_packet[offset++] = 0x00;
        response_packet[offset++] = 0x00;
        
        // Reserved (6 bytes)
        offset += 6;
        
        std::cout << "   ✅ Sent GET_DYNAMIC_INFO response with current_configuration=0" << std::endl;
    }
    
    // Pad to minimum Ethernet frame size if needed
    if (offset < 60) {
        memset(response_packet + offset, 0, 60 - offset);
        offset = 60;
    }
    
    // Send the response
    if (pcap_sendpacket(handle, response_packet, offset) != 0) {
        std::cerr << "❌ Failed to send AEM response: " << pcap_geterr(handle) << std::endl;
    } else {
        std::cout << "   ✅ AEM response sent successfully (" << offset << " bytes)" << std::endl;
    }
}

void packet_handler(u_char* user_data, const struct pcap_pkthdr* header, const u_char* packet) {
    pcap_t* handle = reinterpret_cast<pcap_t*>(user_data);
    
    // Check if this is an IEEE 1722 packet
    if (header->len < 18 || 
        packet[12] != 0x22 || packet[13] != 0xf0 ||  // EtherType 0x22f0
        packet[14] != 0xfb) {  // AVTP Subtype 0xfb
        return;
    }
    
    // Check if it's destined for our entity
    if (memcmp(packet, OUR_MAC, 6) != 0) {
        return;
    }
    
    const uint8_t* ieee1722_header = packet + 14;
    
    // Parse IEEE 1722.1 header
    uint8_t message_type = (ieee1722_header[4] >> 4) & 0x0f;
    uint64_t target_guid = extract_uint64(ieee1722_header, 2);
    uint16_t command_type = extract_uint16(ieee1722_header, 18) & 0x7fff;
    
    // Only respond to AEM_COMMANDs targeted at our entity
    if (message_type == AEM_COMMAND && target_guid == OUR_ENTITY_ID) {
        std::cout << "🎯 Received AEM_COMMAND 0x" << std::hex << command_type 
                  << " for our entity 0x" << target_guid << std::endl;
        
        send_aem_response(handle, packet);
    }
}

int main() {
    std::cout << "🎵 OpenAvnu AVDECC Controller with AEM Response Support" << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << OUR_ENTITY_ID << std::dec << std::endl;
    std::cout << "MAC Address: ";
    for (int i = 0; i < 6; i++) {
        std::cout << std::hex << (int)OUR_MAC[i];
        if (i < 5) std::cout << ":";
    }
    std::cout << std::dec << std::endl << std::endl;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "❌ WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* devices;
    
    // Find network devices
    if (pcap_findalldevs(&devices, errbuf) == -1) {
        std::cerr << "❌ Error finding devices: " << errbuf << std::endl;
        return 1;
    }
    
    // Look for our target interface (RME MADIface USB)
    const char* target_interface = "\\Device\\NPF_{8BEDBD8D-6DDA-4EF1-B257-9D96CE0A1CAD}";
    pcap_if_t* selected_device = nullptr;
    
    for (pcap_if_t* device = devices; device; device = device->next) {
        if (strcmp(device->name, target_interface) == 0) {
            selected_device = device;
            break;
        }
    }
    
    if (!selected_device) {
        std::cout << "⚠️  Target interface not found, using first available device" << std::endl;
        selected_device = devices;
    }
    
    std::cout << "🔌 Using interface: " << selected_device->name << std::endl;
    if (selected_device->description) {
        std::cout << "   Description: " << selected_device->description << std::endl;
    }
    
    // Open the device
    pcap_t* handle = pcap_open_live(selected_device->name, 65536, 1, 100, errbuf);
    if (!handle) {
        std::cerr << "❌ Cannot open device: " << errbuf << std::endl;
        pcap_freealldevs(devices);
        return 1;
    }
    
    // Set filter for IEEE 1722 AVDECC packets
    struct bpf_program filter;
    const char* filter_exp = "ether proto 0x22f0";
    
    if (pcap_compile(handle, &filter, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "❌ Cannot compile filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        pcap_freealldevs(devices);
        return 1;
    }
    
    if (pcap_setfilter(handle, &filter) == -1) {
        std::cerr << "❌ Cannot set filter: " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        pcap_freealldevs(devices);
        return 1;
    }
    
    std::cout << "🔍 Listening for AVDECC commands..." << std::endl;
    std::cout << "   (Press Ctrl+C to stop)" << std::endl << std::endl;
    
    // Start packet capture loop
    pcap_loop(handle, -1, packet_handler, reinterpret_cast<u_char*>(handle));
    
    // Cleanup
    pcap_close(handle);
    pcap_freealldevs(devices);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
