#include "lib/Standards/IEEE/1722/2016/core/avtpdu.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>

using namespace IEEE::_1722::_2016;

int main() {
    std::cout << "=== IEEE 1722-2016 Deserialization Debug ===" << std::endl;
    
    // Create and configure an AudioAVTPDU
    AudioAVTPDU original(0x123456789ABCDEF0ULL, AudioFormat::SAF);
    original.set_sequence_num(123);
    original.set_timestamp_valid(true);
    original.set_avtp_timestamp(0x87654321);
    original.configure_audio(AudioFormat::SAF, 2, 24, SampleRate::RATE_48000);
    
    std::cout << "Original configuration:" << std::endl;
    std::cout << "  Stream ID: 0x" << std::hex << original.get_stream_id() << std::dec << std::endl;
    std::cout << "  Sequence: " << original.get_sequence_num() << std::endl;
    std::cout << "  Timestamp Valid: " << original.get_timestamp_valid() << std::endl;
    std::cout << "  Timestamp: 0x" << std::hex << original.get_avtp_timestamp() << std::dec << std::endl;
    std::cout << "  Format: " << static_cast<int>(original.get_format()) << std::endl;
    std::cout << "  Channels: " << static_cast<int>(original.get_channels()) << std::endl;
    std::cout << "  Bit Depth: " << static_cast<int>(original.get_bit_depth()) << std::endl;
    std::cout << "  Sample Rate: " << static_cast<int>(original.get_sample_rate()) << std::endl;
    
    // Add payload
    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC, 0xDD};
    original.set_payload(payload);
    
    // Serialize
    std::vector<uint8_t> buffer(1500);
    size_t size = original.serialize(buffer.data(), buffer.size());
    std::cout << "\nSerialized size: " << size << " bytes" << std::endl;
    
    // Show serialized bytes
    std::cout << "First 32 bytes: ";
    for (size_t i = 0; i < std::min(size, size_t(32)); ++i) {
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // Try to deserialize
    AudioAVTPDU deserialized;
    std::cout << "\nAttempting deserialization..." << std::endl;
    bool success = deserialized.deserialize(buffer.data(), size);
    std::cout << "Deserialization result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
    
    if (success) {
        std::cout << "\nDeserialized configuration:" << std::endl;
        std::cout << "  Stream ID: 0x" << std::hex << deserialized.get_stream_id() << std::dec << std::endl;
        std::cout << "  Sequence: " << deserialized.get_sequence_num() << std::endl;
        std::cout << "  Timestamp Valid: " << deserialized.get_timestamp_valid() << std::endl;
        std::cout << "  Timestamp: 0x" << std::hex << deserialized.get_avtp_timestamp() << std::dec << std::endl;
        std::cout << "  Format: " << static_cast<int>(deserialized.get_format()) << std::endl;
        std::cout << "  Channels: " << static_cast<int>(deserialized.get_channels()) << std::endl;
        std::cout << "  Bit Depth: " << static_cast<int>(deserialized.get_bit_depth()) << std::endl;
        std::cout << "  Sample Rate: " << static_cast<int>(deserialized.get_sample_rate()) << std::endl;
        
        // Compare fields
        std::cout << "\nField comparison:" << std::endl;
        std::cout << "  Stream ID match: " << (deserialized.get_stream_id() == original.get_stream_id()) << std::endl;
        std::cout << "  Sequence match: " << (deserialized.get_sequence_num() == original.get_sequence_num()) << std::endl;
        std::cout << "  Timestamp valid match: " << (deserialized.get_timestamp_valid() == original.get_timestamp_valid()) << std::endl;
        std::cout << "  Timestamp match: " << (deserialized.get_avtp_timestamp() == original.get_avtp_timestamp()) << std::endl;
        std::cout << "  Format match: " << (deserialized.get_format() == original.get_format()) << std::endl;
        std::cout << "  Channels match: " << (deserialized.get_channels() == original.get_channels()) << std::endl;
        std::cout << "  Payload match: " << (deserialized.get_payload() == original.get_payload()) << std::endl;
    }
    
    return 0;
}
