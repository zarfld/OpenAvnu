#include "../lib/Standards/1722_1-2021_clean.h"
#include <iostream>
#include <cassert>

using namespace IEEE::_1722_1::_2021;

void test_acmp_protocol() {
    std::cout << "🧪 Testing IEEE 1722.1-2021 ACMP Protocol Implementation..." << std::endl;
    
    // Test 1: Create an ACMP Connect TX Command
    ACMP::ATDECCConnectionManagementProtocolPDU acmp_cmd;
    
    // Set up a realistic connection request
    acmp_cmd.message_type = ACMP::ACMP_Message_type::CONNECT_TX_COMMAND;
    acmp_cmd.status = ACMP::ACMP_Status::SUCCESS;
    acmp_cmd.stream_id = 0x1234567890ABCDEF;
    acmp_cmd.controller_entity_id = 0x0123456789ABCDEF;
    acmp_cmd.talker_entity_id = 0xFEDCBA9876543210;
    acmp_cmd.listener_entity_id = 0x1111222233334444;
    acmp_cmd.talker_unique_id = 0x0001;
    acmp_cmd.listener_unique_id = 0x0002;
    acmp_cmd.sequence_id = 0x1234;
    acmp_cmd.flags = 0x0003; // Fast connect + Class B
    acmp_cmd.stream_vlan_id = 2;
    
    // Set stream destination MAC address
    uint8_t dest_mac[6] = {0x91, 0xE0, 0xF0, 0x00, 0x12, 0x34};
    memcpy(acmp_cmd.stream_dest_mac, dest_mac, 6);
    
    acmp_cmd.serialize();
    
    std::cout << "✅ ACMP Command Created:" << std::endl;
    std::cout << "   Message Type: " << static_cast<int>(acmp_cmd.message_type) << " (CONNECT_TX_COMMAND)" << std::endl;
    std::cout << "   Stream ID: 0x" << std::hex << acmp_cmd.stream_id << std::dec << std::endl;
    std::cout << "   Talker Entity: 0x" << std::hex << acmp_cmd.talker_entity_id << std::dec << std::endl;
    std::cout << "   Listener Entity: 0x" << std::hex << acmp_cmd.listener_entity_id << std::dec << std::endl;
    
    // Test 2: Verify serialization/deserialization round-trip
    uint8_t* serialized_data = acmp_cmd.get_raw_octets();
    size_t data_size = acmp_cmd.get_size();
    
    ACMP::ATDECCConnectionManagementProtocolPDU acmp_parsed(serialized_data, data_size);
    
    // Verify all fields match
    assert(acmp_parsed.message_type == acmp_cmd.message_type);
    assert(acmp_parsed.stream_id == acmp_cmd.stream_id);
    assert(acmp_parsed.controller_entity_id == acmp_cmd.controller_entity_id);
    assert(acmp_parsed.talker_entity_id == acmp_cmd.talker_entity_id);
    assert(acmp_parsed.listener_entity_id == acmp_cmd.listener_entity_id);
    assert(acmp_parsed.sequence_id == acmp_cmd.sequence_id);
    assert(acmp_parsed.flags == acmp_cmd.flags);
    assert(acmp_parsed.stream_vlan_id == acmp_cmd.stream_vlan_id);
    
    std::cout << "✅ Serialization/Deserialization: PASSED" << std::endl;
    
    // Test 3: Create response from command
    ACMP::ATDECCConnectionManagementProtocolPDU acmp_response = acmp_cmd;
    acmp_response.create_response(ACMP::ACMP_Status::SUCCESS);
    
    assert(acmp_response.is_response());
    assert(!acmp_response.is_command());
    assert(acmp_response.message_type == ACMP::ACMP_Message_type::CONNECT_TX_RESPONSE);
    assert(acmp_response.status == ACMP::ACMP_Status::SUCCESS);
    
    std::cout << "✅ Command to Response Conversion: PASSED" << std::endl;
    
    // Test 4: Test different status codes
    ACMP::ATDECCConnectionManagementProtocolPDU acmp_error = acmp_cmd;
    acmp_error.create_response(ACMP::ACMP_Status::TALKER_NO_BANDWIDTH);
    
    assert(acmp_error.status == ACMP::ACMP_Status::TALKER_NO_BANDWIDTH);
    
    std::cout << "✅ Error Status Handling: PASSED" << std::endl;
    
    std::cout << "🎉 IEEE 1722.1-2021 ACMP Protocol Implementation: ALL TESTS PASSED!" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "📋 Implementation Status:" << std::endl;
    std::cout << "   ✅ IEEE 1722.1-2021 ADP (Discovery Protocol) - COMPLETE" << std::endl;
    std::cout << "   ✅ IEEE 1722.1-2021 ACMP (Connection Management) - COMPLETE" << std::endl;
    std::cout << "   ⏳ IEEE 1722.1-2021 AECP (Enumeration & Control) - NEXT" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "🎯 Next Step: Implement AECP protocol to complete IEEE 1722.1-2021" << std::endl;
}

int main() {
    test_acmp_protocol();
    return 0;
}
