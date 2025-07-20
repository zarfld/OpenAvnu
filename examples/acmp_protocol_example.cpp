/**
 * @file acmp_protocol_example.cpp
 * @brief Example usage of IEEE 1722.1-2021 ACMP Protocol Implementation
 * 
 * This example demonstrates how to use the clean ACMP implementation
 * for AVDECC stream connection management.
 */

#include "1722_1-2021_clean.h"
#include <iostream>
#include <iomanip>

using namespace IEEE::_1722_1::_2021;

void demonstrate_acmp_usage() {
    std::cout << "=== IEEE 1722.1-2021 ACMP Protocol Example ===" << std::endl;
    
    // Example 1: Create a CONNECT_TX_COMMAND
    std::cout << "\n1. Creating CONNECT_TX_COMMAND" << std::endl;
    ACMP::ATDECCConnectionManagementProtocolPDU connect_cmd;
    
    // Set up the connection request
    connect_cmd.message_type = ACMP::ACMP_Message_type::CONNECT_TX_COMMAND;
    connect_cmd.stream_id = 0x1234567890ABCDEF;
    connect_cmd.controller_entity_id = 0x0000001122334455;
    connect_cmd.talker_entity_id = 0xAABBCCDDEEFF0011;
    connect_cmd.listener_entity_id = 0x1111222233334444;
    connect_cmd.talker_unique_id = 0x0001;
    connect_cmd.listener_unique_id = 0x0002;
    connect_cmd.sequence_id = 0x1234;
    
    // Set flags for Class B + Fast Connect
    connect_cmd.flags = 0x0003;
    connect_cmd.stream_vlan_id = 2;
    
    // Set multicast destination MAC for AVB
    uint8_t avb_mac[6] = {0x91, 0xE0, 0xF0, 0x00, 0x12, 0x34};
    memcpy(connect_cmd.stream_dest_mac, avb_mac, 6);
    
    connect_cmd.serialize();
    
    std::cout << "   Stream ID: 0x" << std::hex << connect_cmd.stream_id << std::endl;
    std::cout << "   Talker Entity: 0x" << std::hex << connect_cmd.talker_entity_id << std::endl;
    std::cout << "   Listener Entity: 0x" << std::hex << connect_cmd.listener_entity_id << std::endl;
    std::cout << "   Flags: 0x" << std::hex << connect_cmd.flags << " (Class B + Fast Connect)" << std::endl;
    
    // Example 2: Simulate network transmission and parsing
    std::cout << "\n2. Network Transmission Simulation" << std::endl;
    uint8_t* packet_data = connect_cmd.get_raw_octets();
    size_t packet_size = connect_cmd.get_size();
    
    std::cout << "   Serialized packet size: " << std::dec << packet_size << " bytes" << std::endl;
    std::cout << "   First 16 bytes: ";
    for (int i = 0; i < 16; i++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)packet_data[i] << " ";
    }
    std::cout << std::endl;
    
    // Example 3: Parse received packet
    std::cout << "\n3. Parsing Received Packet" << std::endl;
    ACMP::ATDECCConnectionManagementProtocolPDU received_cmd(packet_data, packet_size);
    
    std::cout << "   Parsed Message Type: " << std::dec << (int)received_cmd.message_type;
    std::cout << " (" << (received_cmd.is_command() ? "COMMAND" : "RESPONSE") << ")" << std::endl;
    std::cout << "   Parsed Stream ID: 0x" << std::hex << received_cmd.stream_id << std::endl;
    std::cout << "   Parsed Sequence ID: " << std::dec << received_cmd.sequence_id << std::endl;
    
    // Example 4: Create response
    std::cout << "\n4. Creating CONNECT_TX_RESPONSE" << std::endl;
    ACMP::ATDECCConnectionManagementProtocolPDU response = received_cmd;
    response.create_response(ACMP::ACMP_Status::SUCCESS);
    
    std::cout << "   Response Message Type: " << std::dec << (int)response.message_type;
    std::cout << " (" << (response.is_response() ? "RESPONSE" : "COMMAND") << ")" << std::endl;
    std::cout << "   Response Status: " << (int)response.status << " (SUCCESS)" << std::endl;
    
    // Example 5: Error handling
    std::cout << "\n5. Error Response Example" << std::endl;
    ACMP::ATDECCConnectionManagementProtocolPDU error_response = received_cmd;
    error_response.create_response(ACMP::ACMP_Status::TALKER_NO_BANDWIDTH);
    
    std::cout << "   Error Status: " << (int)error_response.status << " (TALKER_NO_BANDWIDTH)" << std::endl;
    
    std::cout << "\n=== ACMP Protocol Example Complete ===" << std::endl;
    std::cout << "\nThis implementation provides:" << std::endl;
    std::cout << "✅ IEEE 1722.1-2021 compliant ACMP protocol" << std::endl;
    std::cout << "✅ All message types and status codes" << std::endl;
    std::cout << "✅ Network byte order serialization" << std::endl;
    std::cout << "✅ Command/Response pattern support" << std::endl;
    std::cout << "✅ Ready for state machine integration" << std::endl;
}

// Note: This is example code. In a real implementation, you would:
// 1. Integrate with network sockets for actual transmission
// 2. Connect to AVDECC state machines
// 3. Handle timing and retry logic
// 4. Validate entity IDs and stream parameters
// 5. Integrate with stream reservation protocols (SRP)
