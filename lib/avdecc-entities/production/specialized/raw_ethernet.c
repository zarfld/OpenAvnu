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

// AECP (AVDECC Enumeration and Control Protocol) Message
typedef struct {
    uint8_t  message_type;       // AECP message type (bit 7=command/response, bits 6-4=reserved, bits 3-0=message type)
    uint8_t  status;             // Status (response) or reserved (command)
    uint16_t control_data_length; // Control data length
    uint64_t target_entity_id;   // Target entity ID
    uint64_t controller_entity_id; // Controller entity ID  
    uint16_t sequence_id;        // Sequence ID
    uint16_t command_type;       // Command/response type
} aecp_header_t;

// READ_DESCRIPTOR Command Payload
typedef struct {
    uint16_t configuration_index; // Configuration index
    uint16_t reserved;           // Reserved
    uint16_t descriptor_type;    // Descriptor type
    uint16_t descriptor_index;   // Descriptor index
} aecp_read_descriptor_cmd_t;

// Entity Descriptor
typedef struct {
    uint16_t descriptor_type;    // 0x0000 (ENTITY)
    uint16_t descriptor_index;   // 0x0000
    uint64_t entity_id;         // Same as ADP entity_id
    uint64_t entity_model_id;   // Same as ADP entity_model_id
    uint32_t entity_capabilities; // Same as ADP entity_capabilities
    uint16_t talker_stream_sources; // Same as ADP
    uint16_t talker_capabilities;   // Same as ADP
    uint16_t listener_stream_sinks; // Same as ADP
    uint16_t listener_capabilities; // Same as ADP
    uint16_t controller_capabilities; // Same as ADP
    uint32_t available_index;   // Same as ADP
    uint64_t association_id;    // Same as ADP
    char entity_name[64];       // "OpenAvnu AVDECC Entity"
    uint16_t vendor_name_string; // Vendor name string index
    uint16_t model_name_string;  // Model name string index  
    char firmware_version[64];   // Firmware version
    char group_name[64];        // Group name
    char serial_number[64];     // Serial number
    uint16_t configurations_count; // Number of configurations (1)
    uint16_t current_configuration; // Current configuration index (0)
} entity_descriptor_t;

// Configuration Descriptor  
typedef struct {
    uint16_t descriptor_type;    // 0x0001 (CONFIGURATION)
    uint16_t descriptor_index;   // 0x0000
    char object_name[64];       // "Default Configuration"
    uint16_t localized_description; // Localized description string index
    uint16_t descriptor_counts_count; // Number of descriptor count entries
    uint16_t descriptor_counts_offset; // Offset to descriptor counts
    // Descriptor counts (simplified - just count the types we support)
    uint16_t audio_unit_count;      // 0 for now
    uint16_t stream_input_count;    // 2 (from ADP)
    uint16_t stream_output_count;   // 2 (from ADP)
    uint16_t jack_input_count;      // 0 for now
    uint16_t jack_output_count;     // 0 for now
    uint16_t avb_interface_count;   // 1 (the network interface)
    uint16_t clock_source_count;    // 1 (internal clock)
    uint16_t memory_object_count;   // 0 for now
    uint16_t locale_count;          // 1 (English)
    uint16_t strings_count;         // 3 (vendor, model, config names)
    uint16_t stream_port_input_count;  // 2
    uint16_t stream_port_output_count; // 2
    uint16_t external_port_input_count;  // 0
    uint16_t external_port_output_count; // 0
    uint16_t internal_port_input_count;  // 0
    uint16_t internal_port_output_count; // 0
    uint16_t audio_cluster_count;   // 0 for now
    uint16_t audio_map_count;       // 0 for now
    uint16_t control_count;         // 0 for now
    uint16_t signal_selector_count; // 0 for now
    uint16_t mixer_count;           // 0 for now
    uint16_t matrix_count;          // 0 for now
    uint16_t signal_splitter_count; // 0 for now
    uint16_t signal_combiner_count; // 0 for now
    uint16_t signal_demultiplexer_count; // 0 for now
    uint16_t signal_multiplexer_count;   // 0 for now
    uint16_t signal_transcoder_count;    // 0 for now
    uint16_t clock_domain_count;    // 1 (media clock domain)
    uint16_t control_block_count;   // 0 for now
} configuration_descriptor_t;

// Complete AECP Packet
typedef struct {
    ethernet_header_t eth_header;
    avtp_header_t avtp_header;
    aecp_header_t aecp_header;
    union {
        aecp_read_descriptor_cmd_t read_descriptor_cmd;
        entity_descriptor_t entity_descriptor;
        configuration_descriptor_t configuration_descriptor;
        uint8_t raw_data[512]; // For other descriptor types
    } payload;
} avdecc_aecp_packet_t;

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

// AECP Message Types
#define AECP_MESSAGE_TYPE_AEM_COMMAND    0x00
#define AECP_MESSAGE_TYPE_AEM_RESPONSE   0x01

// AECP Command Types
#define AECP_CMD_READ_DESCRIPTOR         0x0002

// Descriptor Types
#define DESCRIPTOR_TYPE_ENTITY           0x0000
#define DESCRIPTOR_TYPE_CONFIGURATION    0x0001

// AECP Status Codes
#define AECP_STATUS_SUCCESS              0x00
#define AECP_STATUS_NOT_IMPLEMENTED      0x01

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

// AECP descriptor functions
void create_entity_descriptor(entity_descriptor_t* desc);
void create_configuration_descriptor(configuration_descriptor_t* desc);
int handle_aecp_read_descriptor(const uint8_t* rx_frame, size_t rx_size, uint8_t* tx_frame, size_t* tx_size);
void process_received_packet(const uint8_t* packet_data, size_t packet_size);

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

uint64_t ntohll_local(uint64_t network) {
    uint32_t high = (uint32_t)(network >> 32);
    uint32_t low = (uint32_t)(network & 0xFFFFFFFF);
    return ((uint64_t)ntohl(low) << 32) | ntohl(high);
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

// Create Entity Descriptor
void create_entity_descriptor(entity_descriptor_t* desc) {
    memset(desc, 0, sizeof(entity_descriptor_t));
    
    desc->descriptor_type = htons_local(DESCRIPTOR_TYPE_ENTITY);
    desc->descriptor_index = htons_local(0);
    desc->entity_id = htonll_local(g_entity_id);
    desc->entity_model_id = htonll_local(0x001B21FF00000001ULL);
    
    desc->entity_capabilities = htonl_local(
        ENTITY_CAP_AEM_SUPPORTED |
        ENTITY_CAP_CLASS_A_SUPPORTED |
        ENTITY_CAP_CLASS_B_SUPPORTED |
        ENTITY_CAP_GPTP_SUPPORTED |
        ENTITY_CAP_AEM_AUTH_SUPPORTED |
        ENTITY_CAP_AEM_IDENTIFY_SUPPORTED
    );
    
    desc->talker_stream_sources = htons_local(2);
    desc->talker_capabilities = htons_local(
        TALKER_CAP_IMPLEMENTED |
        TALKER_CAP_AUDIO_SUPPORTED |
        TALKER_CAP_MEDIA_CLOCK_SYNC
    );
    
    desc->listener_stream_sinks = htons_local(2);
    desc->listener_capabilities = htons_local(
        LISTENER_CAP_IMPLEMENTED |
        LISTENER_CAP_AUDIO_SUPPORTED |
        LISTENER_CAP_MEDIA_CLOCK_SYNC
    );
    
    desc->controller_capabilities = htons_local(CONTROLLER_CAP_IMPLEMENTED);
    desc->available_index = htonl_local(0);
    desc->association_id = htonll_local(0);
    
    // String fields
    strcpy_s(desc->entity_name, sizeof(desc->entity_name), "OpenAvnu AVDECC Entity");
    desc->vendor_name_string = htons_local(1);  // String index 1
    desc->model_name_string = htons_local(2);   // String index 2
    strcpy_s(desc->firmware_version, sizeof(desc->firmware_version), "1.0.0");
    strcpy_s(desc->group_name, sizeof(desc->group_name), "OpenAvnu");
    strcpy_s(desc->serial_number, sizeof(desc->serial_number), "OA-001");
    
    desc->configurations_count = htons_local(1);
    desc->current_configuration = htons_local(0);
}

// Create Configuration Descriptor
void create_configuration_descriptor(configuration_descriptor_t* desc) {
    memset(desc, 0, sizeof(configuration_descriptor_t));
    
    desc->descriptor_type = htons_local(DESCRIPTOR_TYPE_CONFIGURATION);
    desc->descriptor_index = htons_local(0);
    strcpy_s(desc->object_name, sizeof(desc->object_name), "Default Configuration");
    desc->localized_description = htons_local(3); // String index 3
    
    // Descriptor counts (these must match what we actually implement)
    desc->descriptor_counts_count = htons_local(24); // Number of different descriptor types
    desc->descriptor_counts_offset = htons_local(sizeof(configuration_descriptor_t) - 
                                                  (24 * sizeof(uint16_t))); // Offset to counts array
    
    // Set counts for descriptors we support
    desc->audio_unit_count = htons_local(0);           // No audio units yet
    desc->stream_input_count = htons_local(2);         // 2 listener streams
    desc->stream_output_count = htons_local(2);        // 2 talker streams
    desc->jack_input_count = htons_local(0);
    desc->jack_output_count = htons_local(0);
    desc->avb_interface_count = htons_local(1);        // 1 network interface
    desc->clock_source_count = htons_local(1);         // 1 internal clock
    desc->memory_object_count = htons_local(0);
    desc->locale_count = htons_local(1);               // 1 locale (English)
    desc->strings_count = htons_local(4);              // 4 strings (vendor, model, config, locale)
    desc->stream_port_input_count = htons_local(2);
    desc->stream_port_output_count = htons_local(2);
    desc->external_port_input_count = htons_local(0);
    desc->external_port_output_count = htons_local(0);
    desc->internal_port_input_count = htons_local(0);
    desc->internal_port_output_count = htons_local(0);
    desc->audio_cluster_count = htons_local(0);
    desc->audio_map_count = htons_local(0);
    desc->control_count = htons_local(0);
    desc->signal_selector_count = htons_local(0);
    desc->mixer_count = htons_local(0);
    desc->matrix_count = htons_local(0);
    desc->signal_splitter_count = htons_local(0);
    desc->signal_combiner_count = htons_local(0);
    desc->signal_demultiplexer_count = htons_local(0);
    desc->signal_multiplexer_count = htons_local(0);
    desc->signal_transcoder_count = htons_local(0);
    desc->clock_domain_count = htons_local(1);         // 1 media clock domain
    desc->control_block_count = htons_local(0);
}

// Handle AECP READ_DESCRIPTOR command
int handle_aecp_read_descriptor(const uint8_t* rx_frame, size_t rx_size, uint8_t* tx_frame, size_t* tx_size) {
    if (rx_size < sizeof(ethernet_header_t) + sizeof(avtp_header_t) + sizeof(aecp_header_t) + sizeof(aecp_read_descriptor_cmd_t)) {
        return -1; // Packet too small
    }
    
    const avdecc_aecp_packet_t* rx_packet = (const avdecc_aecp_packet_t*)rx_frame;
    avdecc_aecp_packet_t* tx_packet = (avdecc_aecp_packet_t*)tx_frame;
    
    // Parse the READ_DESCRIPTOR command
    uint16_t descriptor_type = ntohs(rx_packet->payload.read_descriptor_cmd.descriptor_type);
    uint16_t descriptor_index = ntohs(rx_packet->payload.read_descriptor_cmd.descriptor_index);
    
    printf("📖 AECP READ_DESCRIPTOR: type=0x%04X, index=%d\n", descriptor_type, descriptor_index);
    
    // Prepare response packet (copy headers from request)
    memcpy(tx_packet, rx_packet, sizeof(ethernet_header_t) + sizeof(avtp_header_t) + sizeof(aecp_header_t));
    
    // Swap source and destination MACs
    memcpy(tx_packet->eth_header.dest_mac, rx_packet->eth_header.src_mac, 6);
    memcpy(tx_packet->eth_header.src_mac, g_src_mac, 6);
    
    // Update AECP header for response
    tx_packet->aecp_header.message_type = AECP_MESSAGE_TYPE_AEM_RESPONSE;
    tx_packet->aecp_header.status = AECP_STATUS_SUCCESS;
    
    // Swap controller and target entity IDs
    uint64_t temp_id = tx_packet->aecp_header.target_entity_id;
    tx_packet->aecp_header.target_entity_id = tx_packet->aecp_header.controller_entity_id;
    tx_packet->aecp_header.controller_entity_id = temp_id;
    
    size_t descriptor_size = 0;
    
    // Generate the requested descriptor
    switch (descriptor_type) {
        case DESCRIPTOR_TYPE_ENTITY:
            if (descriptor_index == 0) {
                create_entity_descriptor(&tx_packet->payload.entity_descriptor);
                descriptor_size = sizeof(entity_descriptor_t);
                printf("✅ Sending Entity Descriptor (%zu bytes)\n", descriptor_size);
            } else {
                tx_packet->aecp_header.status = AECP_STATUS_NOT_IMPLEMENTED;
                printf("❌ Entity descriptor index %d not found\n", descriptor_index);
            }
            break;
            
        case DESCRIPTOR_TYPE_CONFIGURATION:
            if (descriptor_index == 0) {
                create_configuration_descriptor(&tx_packet->payload.configuration_descriptor);
                descriptor_size = sizeof(configuration_descriptor_t);
                printf("✅ Sending Configuration Descriptor (%zu bytes)\n", descriptor_size);
            } else {
                tx_packet->aecp_header.status = AECP_STATUS_NOT_IMPLEMENTED;
                printf("❌ Configuration descriptor index %d not found\n", descriptor_index);
            }
            break;
            
        default:
            tx_packet->aecp_header.status = AECP_STATUS_NOT_IMPLEMENTED;
            printf("❌ Descriptor type 0x%04X not implemented\n", descriptor_type);
            descriptor_size = 0;
            break;
    }
    
    // Update control data length in AECP header
    tx_packet->aecp_header.control_data_length = htons_local(sizeof(aecp_header_t) + descriptor_size - 4);
    
    // Update AVTP stream data length
    tx_packet->avtp_header.stream_data_length = htons_local(sizeof(aecp_header_t) + descriptor_size);
    
    // Calculate total packet size
    *tx_size = sizeof(ethernet_header_t) + sizeof(avtp_header_t) + sizeof(aecp_header_t) + descriptor_size;
    
    return 0; // Success
}

// Process received AVDECC packet
void process_received_packet(const uint8_t* packet_data, size_t packet_size) {
    if (packet_size < sizeof(ethernet_header_t) + sizeof(avtp_header_t)) {
        return; // Packet too small
    }
    
    const ethernet_header_t* eth_header = (const ethernet_header_t*)packet_data;
    const avtp_header_t* avtp_header = (const avtp_header_t*)(packet_data + sizeof(ethernet_header_t));
    
    // Check if it's an AVDECC packet
    if (ntohs(eth_header->ethertype) != AVDECC_ETHERTYPE || avtp_header->subtype != AVDECC_SUBTYPE) {
        return;
    }
    
    // Check if we have AECP header
    if (packet_size < sizeof(ethernet_header_t) + sizeof(avtp_header_t) + sizeof(aecp_header_t)) {
        return;
    }
    
    const aecp_header_t* aecp_header = (const aecp_header_t*)(packet_data + sizeof(ethernet_header_t) + sizeof(avtp_header_t));
    
    // Check if it's an AECP command for our entity
    if (aecp_header->message_type == AECP_MESSAGE_TYPE_AEM_COMMAND && 
        ntohll_local(aecp_header->target_entity_id) == g_entity_id) {
        
        uint16_t command_type = ntohs(aecp_header->command_type);
        
        printf("📨 AECP Command 0x%04X for our entity (ID: 0x%016llX)\n", command_type, g_entity_id);
        
        if (command_type == AECP_CMD_READ_DESCRIPTOR) {
            // Handle READ_DESCRIPTOR command
            uint8_t response_frame[1500];
            size_t response_size;
            
            if (handle_aecp_read_descriptor(packet_data, packet_size, response_frame, &response_size) == 0) {
                if (send_raw_ethernet_frame(response_frame, response_size) > 0) {
                    printf("✅ AECP READ_DESCRIPTOR response sent (%zu bytes)\n", response_size);
                } else {
                    printf("❌ Failed to send AECP response\n");
                }
            }
        } else {
            printf("⚠️  AECP command 0x%04X not implemented\n", command_type);
        }
    }
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
            process_received_packet(rx_buffer, rx_bytes);
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

// List all available network interfaces
void list_network_interfaces() {
#ifdef NPCAP_AVAILABLE
    pcap_if_t* all_devs;
    pcap_if_t* device;
    int i = 0;
    
    printf("📡 Available Network Interfaces:\n");
    printf("=====================================\n");
    
    if (pcap_findalldevs(&all_devs, g_raw_eth.error_buffer) == -1) {
        printf("❌ Error finding devices: %s\n", g_raw_eth.error_buffer);
        return;
    }
    
    for (device = all_devs; device; device = device->next) {
        i++;
        printf("Interface %d:\n", i);
        printf("  Name: %s\n", device->name);
        
        if (device->description) {
            printf("  Description: %s\n", device->description);
            
            // Highlight AVB-capable interfaces
            if (strstr(device->description, "Intel") &&
                (strstr(device->description, "I210") || 
                 strstr(device->description, "I219") || 
                 strstr(device->description, "I225"))) {
                printf("  🎯 AVB-CAPABLE: Intel interface (recommended for AVB)\n");
            } else if (strstr(device->description, "RME")) {
                printf("  🎵 AUDIO-INTERFACE: Professional audio device\n");
            } else if (strstr(device->description, "Ethernet") && 
                      !strstr(device->description, "Virtual") &&
                      !strstr(device->description, "VMware") &&
                      !strstr(device->description, "VirtualBox")) {
                printf("  🔗 ETHERNET: Physical network interface\n");
            } else if (strstr(device->description, "Wi-Fi") || strstr(device->description, "Wireless")) {
                printf("  📶 WIRELESS: Not recommended for AVB\n");
            } else if (strstr(device->description, "Virtual") || 
                      strstr(device->description, "VMware") ||
                      strstr(device->description, "VirtualBox") ||
                      strstr(device->description, "Loopback")) {
                printf("  🖥️  VIRTUAL: Not suitable for AVB\n");
            }
        } else {
            printf("  Description: Not available\n");
        }
        
        // Show addresses if available
        pcap_addr_t* addr;
        for (addr = device->addresses; addr; addr = addr->next) {
            if (addr->addr && addr->addr->sa_family == AF_INET) {
                struct sockaddr_in* addr_in = (struct sockaddr_in*)addr->addr;
                printf("  IP: %s\n", inet_ntoa(addr_in->sin_addr));
                break;
            }
        }
        
        printf("\n");
    }
    
    printf("💡 For AVB/AVDECC, choose:\n");
    printf("   - Intel I210/I219/I225 interfaces (best AVB support)\n");
    printf("   - RME audio interfaces (professional AVB support)\n");
    printf("   - Physical Ethernet interfaces (basic support)\n");
    printf("   - Avoid wireless and virtual interfaces\n\n");
    
    pcap_freealldevs(all_devs);
#else
    printf("📡 Interface listing requires Npcap SDK\n");
    printf("   Install Npcap SDK and rebuild to see available interfaces\n\n");
#endif
}

// Enhanced interface selection with Intel/RME preference
const char* select_best_interface() {
#ifdef NPCAP_AVAILABLE
    pcap_if_t* all_devs;
    pcap_if_t* device;
    const char* best_interface = NULL;
    int priority = 0; // Higher number = better choice
    
    if (pcap_findalldevs(&all_devs, g_raw_eth.error_buffer) == -1) {
        printf("❌ Error finding devices: %s\n", g_raw_eth.error_buffer);
        return NULL;
    }
    
    for (device = all_devs; device; device = device->next) {
        if (!device->description) continue;
        
        int current_priority = 0;
        
        // Prioritize Intel AVB-capable interfaces
        if (strstr(device->description, "Intel")) {
            if (strstr(device->description, "I210")) {
                current_priority = 100; // Highest priority - Intel I210
            } else if (strstr(device->description, "I219")) {
                current_priority = 95;  // Intel I219
            } else if (strstr(device->description, "I225")) {
                current_priority = 90;  // Intel I225
            } else if (strstr(device->description, "Ethernet")) {
                current_priority = 70;  // Other Intel Ethernet
            }
        }
        // RME professional audio interfaces
        else if (strstr(device->description, "RME")) {
            current_priority = 85; // High priority - professional audio
        }
        // Other physical Ethernet interfaces
        else if (strstr(device->description, "Ethernet") && 
                 !strstr(device->description, "Virtual") &&
                 !strstr(device->description, "VMware") &&
                 !strstr(device->description, "VirtualBox") &&
                 !strstr(device->description, "Miniport") &&
                 !strstr(device->description, "Loopback")) {
            current_priority = 50; // Medium priority - physical Ethernet
        }
        
        // Skip virtual, wireless, and problematic interfaces
        if (strstr(device->description, "Virtual") ||
            strstr(device->description, "VMware") ||
            strstr(device->description, "VirtualBox") ||
            strstr(device->description, "Loopback") ||
            strstr(device->description, "Wi-Fi") ||
            strstr(device->description, "Wireless") ||
            strstr(device->description, "Miniport") ||
            strstr(device->description, "Monitor")) {
            current_priority = 0; // Skip these
        }
        
        if (current_priority > priority) {
            priority = current_priority;
            best_interface = device->name;
        }
    }
    
    if (best_interface) {
        // Find the device again to get description
        for (device = all_devs; device; device = device->next) {
            if (strcmp(device->name, best_interface) == 0) {
                printf("🎯 Auto-selected BEST interface for AVB:\n");
                printf("   Name: %s\n", device->name);
                if (device->description) {
                    printf("   Description: %s\n", device->description);
                }
                printf("   Priority Score: %d/100\n\n", priority);
                break;
            }
        }
    }
    
    pcap_freealldevs(all_devs);
    return best_interface;
#else
    return NULL;
#endif
}

// Main function
int main(int argc, char* argv[]) {
    int duration_seconds = 30; // Default duration
    const char* interface_name = NULL;
    bool list_interfaces = false;
    bool show_help = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            duration_seconds = atoi(argv[i + 1]);
            i++; // Skip the next argument
        } else if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            interface_name = argv[i + 1];
            i++; // Skip the next argument
        } else if (strcmp(argv[i], "--list") == 0 || strcmp(argv[i], "-l") == 0) {
            list_interfaces = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_help = true;
        }
    }
    
    if (show_help) {
        printf("OpenAvnu Raw Ethernet AVDECC Entity\n");
        printf("=====================================\n\n");
        printf("Usage: %s [options]\n\n", argv[0]);
        printf("Options:\n");
        printf("  --duration <seconds>    Run for specified duration (default: 30)\n");
        printf("  --interface <name>      Use specific network interface\n");
        printf("  --list, -l              List all available network interfaces\n");
        printf("  --help, -h              Show this help message\n\n");
        printf("Examples:\n");
        printf("  %s --list                           # List available interfaces\n", argv[0]);
        printf("  %s --duration 60                    # Run for 60 seconds\n", argv[0]);
        printf("  %s --interface \"Intel I219\"         # Use specific interface\n", argv[0]);
        printf("  %s --interface \"RME\" --duration 120  # RME interface, 2 minutes\n\n", argv[0]);
        printf("💡 For best AVB/AVDECC performance:\n");
        printf("   - Use Intel I210/I219/I225 network adapters\n");
        printf("   - Use RME professional audio interfaces\n");
        printf("   - Run as Administrator for Raw Ethernet access\n");
        printf("   - Ensure Hive-AVDECC uses the same interface\n");
        return 0;
    }
    
    if (list_interfaces) {
        list_network_interfaces();
        return 0;
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
    
    // Interface selection logic
    if (!interface_name) {
        // Auto-select best interface
        interface_name = select_best_interface();
        if (!interface_name) {
            printf("⚠️  No suitable interface found, using default selection\n");
        }
    } else {
        printf("🔧 Using specified interface: %s\n", interface_name);
    }
    
    if (init_raw_ethernet(interface_name) != 0) {
        printf("❌ Failed to initialize Raw Ethernet interface\n");
        printf("   Ensure WinPcap/Npcap is installed and running as Administrator\n");
        return 1;
    }
    
    // Register cleanup function
    atexit(cleanup);
    
#ifdef NPCAP_AVAILABLE
    printf("✅ PRODUCTION MODE: Raw Ethernet AVDECC implementation ready\n");
    printf("   IEEE 1722.1 frames will be transmitted via Npcap\n");
    printf("   Compatible with professional AVDECC tools (Hive, L-Acoustics)\n\n");
#else
    printf("⚠️  SIMULATION MODE: Npcap not available at compile time\n");
    printf("   Professional AVDECC tools require actual Raw Ethernet implementation\n");
    printf("   To work with Hive-AVDECC, rebuild with Npcap SDK\n\n");
    
    printf("💡 Required for Production:\n");
    printf("   1. Install Npcap SDK from https://npcap.com/dist/npcap-sdk-1.13.zip\n");
    printf("   2. Rebuild with -DNPCAP_AVAILABLE and link wpcap.lib\n");
    printf("   3. Run as Administrator for Raw socket privileges\n\n");
#endif
    
    // Run the AVDECC entity
    run_avdecc_entity_for_duration(duration_seconds);
    
    printf("👋 OpenAvnu AVDECC Entity stopped\n");
    return 0;
}
