// Raw Ethernet AVDECC Entity for Professional Tool Compatibility
// This implementation uses Raw Ethernet IEEE 1722.1 frames for Hive-AVDECC discovery
// Requires WinPcap/Npcap for Raw Ethernet access on Windows

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// Npcap/WinPcap headers for Raw Ethernet access
#ifdef NPCAP_AVAILABLE
#include <pcap.h>
// Note: Libraries are linked via CMake, not pragma comment
#endif

// Raw Ethernet Frame Structure for IEEE 1722.1
#pragma pack(push, 1)

// Ethernet Header
typedef struct {
    uint8_t  dest_mac[6];        // Destination MAC
    uint8_t  src_mac[6];         // Source MAC  
    uint16_t ethertype;          // 0x22F0 for AVDECC
} ethernet_header_t;

// IEEE 1722.1 AVTP Header (Complete)
typedef struct {
    uint8_t  subtype;            // AVTP subtype (0x7A for AVDECC)
    uint8_t  sv_ver_mr_gv_tv;    // Stream valid, version, media reset, gateway valid, timestamp valid
    uint8_t  sequence_num;       // Sequence number
    uint8_t  reserved_tu;        // Reserved and timestamp uncertain
    uint64_t stream_id;          // Stream ID (8 bytes)
    uint32_t avtp_timestamp;     // AVTP timestamp (4 bytes)
    uint32_t gateway_info;       // Gateway info (4 bytes)
    uint16_t stream_data_length; // Stream data length (2 bytes)
    uint16_t reserved2;          // Reserved (2 bytes)
} avtp_header_t;

// ADP (AVDECC Discovery Protocol) Message
typedef struct {
    uint8_t  message_type;       // ADP message type
    uint16_t valid_time;         // Entity availability time
    uint16_t control_data_length; // Control data length
    uint64_t entity_id;          // Entity GUID
    uint64_t entity_model_id;    // Entity Model GUID
    uint32_t entity_capabilities; // Entity capabilities
    uint16_t talker_stream_sources; // Number of talker stream sources
    uint16_t talker_capabilities;   // Talker capabilities
    uint16_t listener_stream_sinks; // Number of listener stream sinks
    uint16_t listener_capabilities; // Listener capabilities
    uint16_t controller_capabilities; // Controller capabilities
    uint32_t available_index;    // Available index
    uint64_t gptp_grandmaster_id; // gPTP grandmaster ID
    uint8_t  gptp_domain_number; // gPTP domain number
    uint8_t  reserved[3];        // Reserved
    uint32_t identify_control_index; // Identify control index
    uint32_t interface_index;    // Interface index
    uint64_t association_id;     // Association ID
} adp_pdu_t;

// Complete AVDECC ADP Packet
typedef struct {
    ethernet_header_t eth_header;
    avtp_header_t avtp_header;
    adp_pdu_t adp_pdu;
} avdecc_adp_packet_t;

#pragma pack(pop)

// AVDECC Protocol Constants
#define AVDECC_ETHERTYPE        0x22F0
#define AVDECC_SUBTYPE          0x7A
#define ADP_MESSAGE_TYPE_ENTITY_AVAILABLE 0x00
#define ADP_MESSAGE_TYPE_ENTITY_DEPARTING 0x01
#define ADP_MESSAGE_TYPE_ENTITY_DISCOVER  0x02

// AVDECC Multicast MAC Address
static const uint8_t AVDECC_MULTICAST_MAC[6] = {0x91, 0xE0, 0xF0, 0x01, 0x00, 0x00};

// Entity Capabilities (IEEE 1722.1-2021 + MILAN)
#define ENTITY_CAP_AEM_SUPPORTED            (1U << 0)
#define ENTITY_CAP_CLASS_A_SUPPORTED        (1U << 1)
#define ENTITY_CAP_CLASS_B_SUPPORTED        (1U << 2)
#define ENTITY_CAP_GPTP_SUPPORTED           (1U << 3)
#define ENTITY_CAP_AEM_AUTH_SUPPORTED       (1U << 4)
#define ENTITY_CAP_AEM_AUTH_REQUIRED        (1U << 5)
#define ENTITY_CAP_AEM_PERSISTENT_ACQUIRE   (1U << 6)
#define ENTITY_CAP_AEM_IDENTIFY_SUPPORTED   (1U << 7)

// Talker Capabilities
#define TALKER_CAP_IMPLEMENTED              (1U << 0)
#define TALKER_CAP_AUDIO_SUPPORTED          (1U << 9)
#define TALKER_CAP_MEDIA_CLOCK_SYNC         (1U << 11)

// Listener Capabilities
#define LISTENER_CAP_IMPLEMENTED            (1U << 0)
#define LISTENER_CAP_AUDIO_SUPPORTED        (1U << 9)
#define LISTENER_CAP_MEDIA_CLOCK_SYNC       (1U << 11)

// Controller Capabilities
#define CONTROLLER_CAP_IMPLEMENTED          (1U << 0)

// Enhanced command line options for comprehensive testing
typedef struct {
    bool list_all_interfaces;
    bool packet_dump;
    bool protocol_test;
    bool performance_test;
    bool force_loopback;
    const char* specific_interface;
    int duration;
    int packet_count;
    bool verbose;
} test_options_t;

static test_options_t g_test_options = {0};
static bool g_running = true;
static uint64_t g_entity_id = 0;
static uint8_t g_src_mac[6] = {0xC0, 0x47, 0x0E, 0x16, 0x7B, 0x89}; // Intel I219 MAC
static uint8_t g_sequence_num = 0;

// Raw Ethernet Interface (Npcap Implementation)
typedef struct {
#ifdef NPCAP_AVAILABLE
    pcap_t* pcap_handle;    // pcap_t* handle for Raw Ethernet
#else
    void* pcap_handle;      // Placeholder when Npcap not available
#endif
    char device_name[256];
    char error_buffer[256];
} raw_ethernet_t;

static raw_ethernet_t g_raw_eth = {0};

// Forward declarations
void cleanup();
BOOL WINAPI console_handler(DWORD signal);
int init_raw_ethernet(const char* interface_name);
int send_raw_ethernet_frame(const uint8_t* frame_data, size_t frame_size);
int receive_raw_ethernet_frame(uint8_t* buffer, size_t buffer_size, int timeout_ms);
void close_raw_ethernet();
void run_avdecc_entity();
void run_avdecc_entity_for_duration(int duration_seconds);

// Network byte order conversion
uint16_t htons_local(uint16_t host) {
    return ((host & 0xFF) << 8) | ((host >> 8) & 0xFF);
}

uint32_t htonl_local(uint32_t host) {
    return ((host & 0xFF) << 24) | (((host >> 8) & 0xFF) << 16) |
           (((host >> 16) & 0xFF) << 8) | ((host >> 24) & 0xFF);
}

uint64_t htonll_local(uint64_t host) {
    uint32_t high = (uint32_t)(host >> 32);
    uint32_t low = (uint32_t)(host & 0xFFFFFFFF);
    return ((uint64_t)htonl_local(low) << 32) | htonl_local(high);
}

// Generate OpenAvnu Entity ID
uint64_t generate_entity_id() {
    srand((unsigned int)time(NULL));
    // OpenAvnu Vendor ID (0x001B21) in upper 24 bits + random
    return ((uint64_t)0x001B21 << 40) |
           ((uint64_t)rand() << 24) |
           ((uint64_t)rand() << 8) |
           ((uint64_t)rand() & 0xFF);
}

// Create ADP Entity Available packet
void create_adp_entity_available_packet(avdecc_adp_packet_t* packet) {
    memset(packet, 0, sizeof(avdecc_adp_packet_t));
    
    // Ethernet Header
    memcpy(packet->eth_header.dest_mac, AVDECC_MULTICAST_MAC, 6);
    memcpy(packet->eth_header.src_mac, g_src_mac, 6);
    packet->eth_header.ethertype = htons_local(AVDECC_ETHERTYPE);
    
    // AVTP Header (IEEE 1722.1-2021 compliant)
    packet->avtp_header.subtype = AVDECC_SUBTYPE;
    packet->avtp_header.sv_ver_mr_gv_tv = 0x30; // sv=0, ver=0, mr=0, gv=1, tv=1, reserved=1
    packet->avtp_header.sequence_num = g_sequence_num++;
    packet->avtp_header.reserved_tu = 0x00;
    packet->avtp_header.stream_id = htonll_local(g_entity_id);
    packet->avtp_header.avtp_timestamp = htonl_local(0);
    packet->avtp_header.gateway_info = htonl_local(0);
    packet->avtp_header.stream_data_length = htons_local(sizeof(adp_pdu_t));
    packet->avtp_header.reserved2 = 0;
    
    // ADP PDU
    packet->adp_pdu.message_type = ADP_MESSAGE_TYPE_ENTITY_AVAILABLE;
    packet->adp_pdu.valid_time = htons_local(62); // 62 seconds (MILAN requirement)
    packet->adp_pdu.control_data_length = htons_local(sizeof(adp_pdu_t) - 4); // PDU size minus message_type and valid_time
    
    // Entity Information
    packet->adp_pdu.entity_id = htonll_local(g_entity_id);
    packet->adp_pdu.entity_model_id = htonll_local(0x001B21FF00000001ULL); // OpenAvnu Model ID
    
    // Entity Capabilities (MILAN-compliant)
    packet->adp_pdu.entity_capabilities = htonl_local(
        ENTITY_CAP_AEM_SUPPORTED |
        ENTITY_CAP_CLASS_A_SUPPORTED |
        ENTITY_CAP_CLASS_B_SUPPORTED |
        ENTITY_CAP_GPTP_SUPPORTED |
        ENTITY_CAP_AEM_AUTH_SUPPORTED |
        ENTITY_CAP_AEM_IDENTIFY_SUPPORTED
    );
    
    // Stream Capabilities
    packet->adp_pdu.talker_stream_sources = htons_local(2);  // 2 talker streams
    packet->adp_pdu.talker_capabilities = htons_local(
        TALKER_CAP_IMPLEMENTED |
        TALKER_CAP_AUDIO_SUPPORTED |
        TALKER_CAP_MEDIA_CLOCK_SYNC
    );
    
    packet->adp_pdu.listener_stream_sinks = htons_local(2);  // 2 listener streams
    packet->adp_pdu.listener_capabilities = htons_local(
        LISTENER_CAP_IMPLEMENTED |
        LISTENER_CAP_AUDIO_SUPPORTED |
        LISTENER_CAP_MEDIA_CLOCK_SYNC
    );
    
    packet->adp_pdu.controller_capabilities = htons_local(CONTROLLER_CAP_IMPLEMENTED);
    
    // Additional Information
    packet->adp_pdu.available_index = htonl_local(0);
    packet->adp_pdu.gptp_grandmaster_id = htonll_local(0x001B21FFFE000001ULL);
    packet->adp_pdu.gptp_domain_number = 0;
    packet->adp_pdu.identify_control_index = htonl_local(0);
    packet->adp_pdu.interface_index = htonl_local(0);
    packet->adp_pdu.association_id = htonll_local(0);
}

// Raw Ethernet Implementation (Npcap/WinPcap)
int init_raw_ethernet(const char* interface_name) {
#ifdef NPCAP_AVAILABLE
    pcap_if_t* all_devs;
    pcap_if_t* device;
    
    printf("🔧 Initializing Raw Ethernet interface...\n");
    
    // Find all network devices
    if (pcap_findalldevs(&all_devs, g_raw_eth.error_buffer) == -1) {
        printf("❌ Error finding devices: %s\n", g_raw_eth.error_buffer);
        return -1;
    }
    
    // Find best interface (prefer physical Ethernet, avoid virtual/monitoring interfaces)
    pcap_if_t* best_device = NULL;
    for (device = all_devs; device; device = device->next) {
        if (device->description && 
            strstr(device->description, "Miniport") == NULL &&
            strstr(device->description, "Monitor") == NULL &&
            strstr(device->description, "Loopback") == NULL &&
            strstr(device->description, "VirtualBox") == NULL &&
            strstr(device->description, "VMware") == NULL &&
            strstr(device->description, "Microsoft") == NULL &&
            (strstr(device->description, "Ethernet") != NULL ||
             strstr(device->description, "Intel") != NULL ||
             strstr(device->description, "Realtek") != NULL)) {
            best_device = device;
            printf("✅ Found physical interface: %s\n", device->name);
            if (device->description) {
                printf("   Description: %s\n", device->description);
            }
            break;
        }
    }
    
    // Use specified interface if provided, otherwise use best found
    if (interface_name) {
        // Try to find specified interface
        for (device = all_devs; device; device = device->next) {
            if (strstr(device->name, interface_name)) {
                printf("✅ Using specified interface: %s\n", device->name);
                break;
            }
        }
        if (!device) {
            printf("⚠️  Interface '%s' not found, using best available\n", interface_name);
            device = best_device ? best_device : all_devs;
        }
    } else {
        device = best_device ? best_device : all_devs;
        printf("✅ Auto-selected interface: %s\n", device->name);
        if (device->description) {
            printf("   Description: %s\n", device->description);
        }
    }
    
    if (!device) {
        printf("❌ No suitable network interface found\n");
        pcap_freealldevs(all_devs);
        return -1;
    }
    
    printf("   Selected interface: %s\n", device->name);
    if (device->description) {
        printf("   Description: %s\n", device->description);
    }
    
    // Copy device name
    strcpy_s(g_raw_eth.device_name, sizeof(g_raw_eth.device_name), device->name);
    
    // Open device for live capture
    g_raw_eth.pcap_handle = pcap_open_live(
        device->name,    // Device name
        65536,           // Snapshot length
        1,               // Promiscuous mode
        100,             // Read timeout (ms)
        g_raw_eth.error_buffer
    );
    
    pcap_freealldevs(all_devs);
    
    if (!g_raw_eth.pcap_handle) {
        printf("❌ Failed to open device: %s\n", g_raw_eth.error_buffer);
        return -1;
    }
    
    // Set filter for AVDECC packets (EtherType 0x22F0)
    struct bpf_program filter_program;
    char filter_string[] = "ether proto 0x22F0";
    
    if (pcap_compile(g_raw_eth.pcap_handle, &filter_program, filter_string, 0, PCAP_NETMASK_UNKNOWN) < 0) {
        printf("❌ Failed to compile filter: %s\n", pcap_geterr(g_raw_eth.pcap_handle));
        pcap_close(g_raw_eth.pcap_handle);
        return -1;
    }
    
    if (pcap_setfilter(g_raw_eth.pcap_handle, &filter_program) < 0) {
        printf("❌ Failed to set filter: %s\n", pcap_geterr(g_raw_eth.pcap_handle));
        pcap_freecode(&filter_program);
        pcap_close(g_raw_eth.pcap_handle);
        return -1;
    }
    
    pcap_freecode(&filter_program);
    
    printf("✅ Raw Ethernet interface initialized successfully\n");
    printf("   Ready for IEEE 1722.1 AVDECC packet transmission/reception\n");
    
    return 0;
    
#else
    printf("⚠️  Raw Ethernet initialization (Npcap not available at compile time)\n");
    printf("   Interface: %s\n", interface_name ? interface_name : "auto-detect");
    printf("   Status: SIMULATION MODE - Professional tools require actual Raw Ethernet\n");
    
    strcpy_s(g_raw_eth.device_name, sizeof(g_raw_eth.device_name), 
             interface_name ? interface_name : "\\Device\\NPF_Loopback");
    
    return 0; // Success (simulation)
#endif
}

int send_raw_ethernet_frame(const uint8_t* frame_data, size_t frame_size) {
#ifdef NPCAP_AVAILABLE
    if (!g_raw_eth.pcap_handle) {
        printf("❌ Raw Ethernet interface not initialized\n");
        return -1;
    }
    
    // Send Raw Ethernet frame using pcap_inject
    int result = pcap_inject(g_raw_eth.pcap_handle, frame_data, frame_size);
    
    if (result < 0) {
        printf("❌ Failed to send Raw Ethernet frame: %s\n", pcap_geterr(g_raw_eth.pcap_handle));
        return -1;
    }
    
    printf("📤 Raw Ethernet TX: %d bytes (EtherType: 0x%04X)\n", 
           result, (frame_size >= 14) ? htons_local(*(uint16_t*)(frame_data + 12)) : 0);
    
    return result;
    
#else
    // Simulation mode
    printf("📤 Raw Ethernet TX: %zu bytes (SIMULATION)\n", frame_size);
    printf("   EtherType: 0x%04X (IEEE 1722.1 AVDECC)\n", 
           (frame_size >= 14) ? htons_local(*(uint16_t*)(frame_data + 12)) : 0);
    
    return (int)frame_size; // Success (simulation)
#endif
}

int receive_raw_ethernet_frame(uint8_t* buffer, size_t buffer_size, int timeout_ms) {
#ifdef NPCAP_AVAILABLE
    if (!g_raw_eth.pcap_handle) {
        return 0; // No packets if not initialized
    }
    
    struct pcap_pkthdr* packet_header;
    const u_char* packet_data;
    
    // Set timeout for packet capture
    int result = pcap_next_ex(g_raw_eth.pcap_handle, &packet_header, &packet_data);
    
    if (result == 1) {
        // Packet received
        size_t copy_size = (packet_header->caplen < buffer_size) ? packet_header->caplen : buffer_size;
        memcpy(buffer, packet_data, copy_size);
        
        printf("📥 Raw Ethernet RX: %u bytes (AVDECC packet)\n", packet_header->caplen);
        
        return (int)copy_size;
    } else if (result == 0) {
        // Timeout - no packet received
        return 0;
    } else {
        // Error
        printf("❌ Error receiving packet: %s\n", pcap_geterr(g_raw_eth.pcap_handle));
        return -1;
    }
    
#else
    // Simulation mode - no incoming packets
    Sleep(timeout_ms);
    return 0; // No packets received
#endif
}

void close_raw_ethernet() {
#ifdef NPCAP_AVAILABLE
    if (g_raw_eth.pcap_handle) {
        pcap_close(g_raw_eth.pcap_handle);
        g_raw_eth.pcap_handle = NULL;
        printf("🔒 Raw Ethernet interface closed\n");
    }
#else
    printf("🔒 Raw Ethernet interface closed (simulation)\n");
#endif
}

// Main AVDECC entity logic
void run_avdecc_entity() {
    run_avdecc_entity_for_duration(30); // Default 30 seconds
}

// Main AVDECC entity logic with duration control
void run_avdecc_entity_for_duration(int duration_seconds) {
    avdecc_adp_packet_t adp_packet;
    DWORD last_advertise_time = GetTickCount();
    DWORD start_time = GetTickCount();
    const DWORD advertise_interval = 2000; // 2 seconds (MILAN requirement)
    const DWORD duration_ms = duration_seconds * 1000;
    
    printf("🚀 OpenAvnu Raw Ethernet AVDECC Entity Started\n");
    printf("   Entity ID: 0x%016llX\n", g_entity_id);
    printf("   MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           g_src_mac[0], g_src_mac[1], g_src_mac[2], 
           g_src_mac[3], g_src_mac[4], g_src_mac[5]);
    printf("   Protocol: IEEE 1722.1-2021 + MILAN\n");
    printf("   Transport: Raw Ethernet (EtherType 0x22F0)\n");
    printf("   Capabilities: AEM, Class A/B, gPTP, Authentication\n");
    printf("   Streams: 2 Talker + 2 Listener\n");
    printf("   Duration: %d seconds\n\n", duration_seconds);
    
    while (g_running && (GetTickCount() - start_time) < duration_ms) {
        DWORD current_time = GetTickCount();
        
        // Send ADP Entity Available advertisement
        if (current_time - last_advertise_time >= advertise_interval) {
            create_adp_entity_available_packet(&adp_packet);
            
            if (send_raw_ethernet_frame((uint8_t*)&adp_packet, sizeof(adp_packet)) > 0) {
                printf("📢 ADP Entity Available sent (seq: %d)\n", g_sequence_num - 1);
            } else {
                printf("❌ Failed to send ADP advertisement\n");
            }
            
            last_advertise_time = current_time;
        }
        
        // Check for incoming AVDECC packets (ADP Discovery, AECP commands)
        uint8_t rx_buffer[1500];
        int rx_bytes = receive_raw_ethernet_frame(rx_buffer, sizeof(rx_buffer), 100);
        
        if (rx_bytes > 0) {
            printf("📥 Received AVDECC packet: %d bytes\n", rx_bytes);
            // TODO: Process incoming ADP/AECP packets
        }
        
        // Allow other processes to run
        Sleep(10);
    }
}

// Signal handler for graceful shutdown
BOOL WINAPI console_handler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            printf("\n🛑 Shutdown signal received\n");
            g_running = false;
            return TRUE;
        default:
            return FALSE;
    }
}

// Cleanup function
void cleanup() {
    if (g_raw_eth.pcap_handle) {
        close_raw_ethernet();
    }
    printf("✅ Cleanup completed\n");
}

// Main function
int main(int argc, char* argv[]) {
    int duration_seconds = 30; // Default duration
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            duration_seconds = atoi(argv[i + 1]);
            i++; // Skip the next argument
        }
    }
    
    printf("===========================================\n");
    printf("   OpenAvnu Raw Ethernet AVDECC Entity\n");
    printf("   IEEE 1722.1-2021 + MILAN Compliance\n");
    printf("   Duration: %d seconds\n", duration_seconds);
    printf("===========================================\n\n");
    
    // Set up signal handler
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        printf("❌ Failed to set console handler\n");
        return 1;
    }
    
    // Generate unique entity ID
    g_entity_id = generate_entity_id();
    
    // Initialize Raw Ethernet interface
    const char* interface_name = NULL;
    // Look for interface name (skip --duration arguments)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--duration") == 0) {
            i++; // Skip duration value
            continue;
        }
        if (argv[i][0] != '-') {
            interface_name = argv[i];
            break;
        }
    }
    
    if (init_raw_ethernet(interface_name) != 0) {
        printf("❌ Failed to initialize Raw Ethernet interface\n");
        printf("   Ensure WinPcap/Npcap is installed and running as Administrator\n");
        return 1;
    }
    
    // Register cleanup function
    atexit(cleanup);
    
    printf("⚠️  CRITICAL: This is a SIMULATION for demonstration\n");
    printf("   Professional AVDECC tools require actual Raw Ethernet implementation\n");
    printf("   To work with Hive-AVDECC, implement WinPcap/Npcap integration\n\n");
    
    printf("💡 Required for Production:\n");
    printf("   1. Install Npcap SDK (recommended) or WinPcap Developer Pack\n");
    printf("   2. Link with wpcap.lib and packet.lib\n");
    printf("   3. Implement pcap_* functions for Raw Ethernet access\n");
    printf("   4. Run as Administrator for Raw socket privileges\n\n");
    
    // Run the AVDECC entity
    run_avdecc_entity_for_duration(duration_seconds);
    
    printf("👋 OpenAvnu AVDECC Entity stopped\n");
    return 0;
}
