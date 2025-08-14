/**
 * @file milan_avdecc_entity_example.cpp
 * @brief Example Milan v1.2 AVDECC Entity Implementation
 * 
 * This example demonstrates how to create and use a Milan-compliant
 * Professional Audio AVB Device (PAAD) Entity using the complete
 * Milan v1.2 implementation.
 * 
 * @date July 2025
 * @copyright OpenAvnu Project / AVnu Alliance
 */

#include "../lib/Standards/AVnu/Milan/1.2-2023/AVnu_Milan_1_2-2023_complete.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace AVnu::Milan::_1_2_2023;

/**
 * @brief Example Milan AVDECC Entity Usage
 */
int main() {
    std::cout << "Milan v1.2 AVDECC Entity Example" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Create a Milan PAAD Entity
    uint64_t entity_id = 0x001B21FFFE000001ULL;  // Example L-Acoustics OUI
    uint64_t entity_model_id = 0x001B21FFFE000100ULL;
    
    std::cout << "Creating Milan PAAD Entity..." << std::endl;
    std::cout << "Entity ID: 0x" << std::hex << std::setfill('0') << std::setw(16) << entity_id << std::endl;
    std::cout << "Entity Model ID: 0x" << std::hex << std::setfill('0') << std::setw(16) << entity_model_id << std::endl;
    
    auto milan_entity = std::make_unique<MilanPAADEntity>(entity_id, entity_model_id);
    
    // Configure Milan capabilities
    uint32_t capabilities = static_cast<uint32_t>(MilanCapabilityFlags::BASELINE_INTEROPERABILITY) |
                           static_cast<uint32_t>(MilanCapabilityFlags::DISCOVERY_CONNECTION_CONTROL) |
                           static_cast<uint32_t>(MilanCapabilityFlags::BASE_STREAM_FORMATS) |
                           static_cast<uint32_t>(MilanCapabilityFlags::MEDIA_CLOCKING);
    
    uint16_t features = static_cast<uint16_t>(MilanProtocolFeatures::MILAN_BASELINE) |
                       static_cast<uint16_t>(MilanProtocolFeatures::FAST_CONNECT) |
                       static_cast<uint16_t>(MilanProtocolFeatures::PROFESSIONAL_AUDIO);
    
    milan_entity->set_milan_capabilities(capabilities);
    milan_entity->set_milan_features(features);
    milan_entity->set_device_name("OpenAvnu Milan Demo Device");
    
    std::cout << std::dec;
    std::cout << "Milan Capabilities: " << MilanUtils::capabilities_to_string(capabilities) << std::endl;
    std::cout << "Milan Features: " << MilanUtils::features_to_string(features) << std::endl;
    std::cout << "Device Name: " << milan_entity->get_device_name() << std::endl;
    
    // Test Milan compliance
    std::cout << "\nMilan Compliance Check:" << std::endl;
    if (milan_entity->validate_milan_compliance()) {
        std::cout << "✅ Entity is Milan v1.2 compliant!" << std::endl;
    } else {
        std::cout << "❌ Entity is NOT Milan compliant:" << std::endl;
        auto issues = milan_entity->get_compliance_issues();
        for (const auto& issue : issues) {
            std::cout << "  - " << issue << std::endl;
        }
    }
    
    // Test Milan MVU command handling
    std::cout << "\nTesting Milan MVU Commands:" << std::endl;
    
    // Create a GET_MILAN_INFO command
    GetMilanInfoCommand get_info_cmd;
    get_info_cmd.command_type = static_cast<uint16_t>(MilanMVUCommandType::GET_MILAN_INFO);
    get_info_cmd.milan_version_major = MILAN_MAJOR_VERSION;
    get_info_cmd.milan_version_minor = MILAN_MINOR_VERSION;
    
    std::vector<uint8_t> command_data;
    if (get_info_cmd.serialize(command_data)) {
        std::cout << "✅ GET_MILAN_INFO command serialized successfully" << std::endl;
        
        // Handle the command
        auto response_data = milan_entity->handle_milan_mvu_command(command_data);
        if (!response_data.empty()) {
            std::cout << "✅ GET_MILAN_INFO response received (" << response_data.size() << " bytes)" << std::endl;
            
            GetMilanInfoResponse response;
            if (response.deserialize(response_data)) {
                std::cout << "  Milan Version: " << response.milan_version_major 
                         << "." << response.milan_version_minor << std::endl;
                std::cout << "  Capabilities: 0x" << std::hex << response.milan_capabilities << std::endl;
                std::cout << "  Features: 0x" << std::hex << response.milan_features << std::endl;
                std::cout << std::dec;
            }
        } else {
            std::cout << "❌ GET_MILAN_INFO command failed" << std::endl;
        }
    } else {
        std::cout << "❌ Failed to serialize GET_MILAN_INFO command" << std::endl;
    }
    
    // Test stream format validation
    std::cout << "\nTesting Milan Stream Format Validation:" << std::endl;
    
    std::vector<uint64_t> test_formats = {
        static_cast<uint64_t>(MilanBaseAudioFormat::AAF_PCM_24BIT_48KHZ_2CH),
        static_cast<uint64_t>(MilanBaseAudioFormat::AAF_PCM_24BIT_48KHZ_8CH),
        static_cast<uint64_t>(MilanBaseAudioFormat::AAF_PCM_32BIT_48KHZ_2CH),
        static_cast<uint64_t>(MilanBaseAudioFormat::IEC61883_6_AM824_48KHZ_2CH),
        0x1234567890ABCDEFULL  // Invalid format
    };
    
    for (auto format : test_formats) {
        bool is_valid = MilanStreamFormatValidator::is_valid_milan_format(format);
        std::string format_name = MilanStreamFormatValidator::format_to_string(format);
        std::cout << "  0x" << std::hex << std::setfill('0') << std::setw(16) << format 
                 << ": " << (is_valid ? "✅" : "❌") << " " << format_name << std::endl;
    }
    std::cout << std::dec;
    
    // Test professional tool compatibility
    std::cout << "\nProfessional Tool Compatibility:" << std::endl;
    auto compatible_tools = MilanUtils::get_professional_tool_compatibility();
    for (const auto& tool : compatible_tools) {
        std::cout << "✅ Compatible with " << tool << std::endl;
    }
    
    // Performance statistics
    std::cout << "\nPerformance Statistics:" << std::endl;
    std::cout << "Commands Processed: " << milan_entity->get_commands_processed() << std::endl;
    std::cout << "MVU Commands Processed: " << milan_entity->get_mvu_commands_processed() << std::endl;
    std::cout << "Average Processing Time: " 
             << milan_entity->get_average_command_processing_time().count() << " ns" << std::endl;
    
    std::cout << "\n" << MilanPAADEntity::get_milan_version_string() << std::endl;
    std::cout << "Milan implementation complete! 🎵" << std::endl;
    
    return 0;
}
