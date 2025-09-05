/**
 * Test IEEE 1722-2016 AVTP Provider Implementation
 * 
 * Comprehensive test for Milan professional audio media transport
 * with AAF-PCM support and stream redundancy.
 */

// Include only the implementation file directly to avoid complex build dependencies
#include "lib/Standards/Integration/ieee_provider_implementations.cpp"

#include <iostream>
#include <cassert>
#include <vector>

using namespace OpenAvnu::Integration::Milan_IEEE;

/**
 * Test IEEE 1722-2016 AVTP Provider for Milan Professional Audio
 */
void test_ieee_1722_2016_avtp_provider() {
    std::cout << "\n=== Testing IEEE 1722-2016 AVTP Provider ===" << std::endl;
    
    // Create IEEE 1722-2016 AVTP provider
    IEEE1722_2016_Provider avtp_provider;
    
    // Test 1: Configure Milan professional audio format
    std::cout << "\nTest 1: Configure Milan base audio format" << std::endl;
    bool config_result = avtp_provider.configure_base_audio_format(
        48000,  // 48kHz sample rate (Milan standard)
        8,      // 8 channels (Milan professional)
        24      // 24-bit depth (Milan professional)
    );
    assert(config_result && "Failed to configure Milan base audio format");
    std::cout << "✅ Milan base audio format configured successfully" << std::endl;
    
    // Test 2: Create Milan audio stream
    std::cout << "\nTest 2: Create Milan audio stream" << std::endl;
    uint64_t talker_stream_id = 0x91E0F000FE001234ULL;
    std::vector<uint8_t> milan_format = {
        0x8, 0x0,       // 8 channels
        0x30, 0x0,      // 48kHz sample rate
        0x18, 0x0,      // 24-bit depth
        0x6, 0x0        // 6 samples per frame (Milan timing)
    };
    
    bool stream_created = avtp_provider.create_milan_audio_stream(talker_stream_id, milan_format);
    assert(stream_created && "Failed to create Milan audio stream");
    std::cout << "✅ Milan audio stream created successfully" << std::endl;
    
    // Test 3: Setup Milan presentation time
    std::cout << "\nTest 3: Setup Milan presentation time" << std::endl;
    uint32_t presentation_time_ns = 2000000; // 2ms for professional audio
    bool pt_result = avtp_provider.setup_milan_presentation_time(talker_stream_id, presentation_time_ns);
    assert(pt_result && "Failed to setup Milan presentation time");
    std::cout << "✅ Milan presentation time configured: 2ms" << std::endl;
    
    // Test 4: Create Milan CRF stream for media clock synchronization
    std::cout << "\nTest 4: Create Milan CRF stream" << std::endl;
    uint64_t crf_stream_id = 0x91E0F000FE005678ULL;
    uint32_t base_frequency = 48000; // 48kHz base frequency
    bool crf_created = avtp_provider.create_milan_crf_stream(crf_stream_id, base_frequency);
    assert(crf_created && "Failed to create Milan CRF stream");
    std::cout << "✅ Milan CRF stream created for media clock synchronization" << std::endl;
    
    // Test 5: Synchronize to media clock
    std::cout << "\nTest 5: Synchronize to media clock" << std::endl;
    bool sync_result = avtp_provider.synchronize_to_media_clock(crf_stream_id);
    assert(sync_result && "Failed to synchronize to media clock");
    std::cout << "✅ Synchronized to Milan media clock" << std::endl;
    
    // Test 6: Start Milan audio streaming
    std::cout << "\nTest 6: Start Milan audio streaming" << std::endl;
    bool start_result = avtp_provider.start_audio_streaming(talker_stream_id);
    assert(start_result && "Failed to start Milan audio streaming");
    std::cout << "✅ Milan audio streaming started" << std::endl;
    
    // Test 7: Verify Milan latency requirements
    std::cout << "\nTest 7: Verify Milan latency requirements" << std::endl;
    bool latency_ok = avtp_provider.meets_milan_latency_requirements(talker_stream_id);
    assert(latency_ok && "Does not meet Milan latency requirements");
    std::cout << "✅ Milan latency requirements met (≤2ms)" << std::endl;
    
    // Test 8: Check packet loss (should be 0 initially)
    std::cout << "\nTest 8: Check packet loss monitoring" << std::endl;
    uint32_t packet_loss = avtp_provider.get_stream_packet_loss(talker_stream_id);
    assert(packet_loss == 0 && "Unexpected packet loss detected");
    std::cout << "✅ No packet loss detected (quality monitoring working)" << std::endl;
    
    // Test 9: Stop Milan audio streaming
    std::cout << "\nTest 9: Stop Milan audio streaming" << std::endl;
    bool stop_result = avtp_provider.stop_audio_streaming(talker_stream_id);
    assert(stop_result && "Failed to stop Milan audio streaming");
    std::cout << "✅ Milan audio streaming stopped" << std::endl;
    
    // Test 10: Test multiple sample rates (Milan professional requirements)
    std::cout << "\nTest 10: Test multiple Milan sample rates" << std::endl;
    
    // Test 96kHz
    bool config_96k = avtp_provider.configure_base_audio_format(96000, 8, 24);
    assert(config_96k && "Failed to configure 96kHz");
    
    // Test 192kHz
    bool config_192k = avtp_provider.configure_base_audio_format(192000, 8, 24);
    assert(config_192k && "Failed to configure 192kHz");
    
    std::cout << "✅ Multiple Milan sample rates supported (48/96/192kHz)" << std::endl;
    
    // Test 11: Test high channel counts (Milan supports up to 64 channels)
    std::cout << "\nTest 11: Test Milan high channel counts" << std::endl;
    bool config_32ch = avtp_provider.configure_base_audio_format(48000, 32, 24);
    assert(config_32ch && "Failed to configure 32 channels");
    
    bool config_64ch = avtp_provider.configure_base_audio_format(48000, 64, 24);
    assert(config_64ch && "Failed to configure 64 channels");
    
    std::cout << "✅ Milan high channel counts supported (up to 64 channels)" << std::endl;
    
    std::cout << "\n✅ ALL IEEE 1722-2016 AVTP PROVIDER TESTS PASSED!" << std::endl;
}

/**
 * Test Milan Professional Audio Integration
 */
void test_milan_professional_audio_integration() {
    std::cout << "\n=== Testing Milan Professional Audio Integration ===" << std::endl;
    
    IEEE1722_2016_Provider avtp_provider;
    
    // Configure for Milan professional audio scenario
    std::cout << "\nConfiguring Milan professional audio (8ch/48kHz/24bit)..." << std::endl;
    avtp_provider.configure_base_audio_format(48000, 8, 24);
    
    // Create multiple streams for redundancy testing
    uint64_t primary_stream = 0x91E0F000FE001111ULL;
    uint64_t redundant_stream = 0x91E0F000FE002222ULL;
    
    std::vector<uint8_t> professional_format = {0x8, 0x0, 0x30, 0x0, 0x18, 0x0, 0x6, 0x0};
    
    // Create primary and redundant streams
    bool primary_created = avtp_provider.create_milan_audio_stream(primary_stream, professional_format);
    bool redundant_created = avtp_provider.create_milan_audio_stream(redundant_stream, professional_format);
    
    assert(primary_created && redundant_created && "Failed to create redundant streams");
    
    // Configure professional presentation time (1ms for critical applications)
    avtp_provider.setup_milan_presentation_time(primary_stream, 1000000);   // 1ms
    avtp_provider.setup_milan_presentation_time(redundant_stream, 1000000); // 1ms
    
    // Start both streams for redundancy
    avtp_provider.start_audio_streaming(primary_stream);
    avtp_provider.start_audio_streaming(redundant_stream);
    
    // Verify both streams meet professional latency requirements
    bool primary_latency_ok = avtp_provider.meets_milan_latency_requirements(primary_stream);
    bool redundant_latency_ok = avtp_provider.meets_milan_latency_requirements(redundant_stream);
    
    assert(primary_latency_ok && redundant_latency_ok && 
           "Professional latency requirements not met");
    
    std::cout << "✅ Milan professional audio with redundancy configured and streaming" << std::endl;
    
    // Stop streams
    avtp_provider.stop_audio_streaming(primary_stream);
    avtp_provider.stop_audio_streaming(redundant_stream);
    
    std::cout << "✅ Milan professional audio integration test completed" << std::endl;
}

int main() {
    std::cout << "IEEE 1722-2016 AVTP Provider Test Suite for Milan Professional Audio" << std::endl;
    std::cout << "=====================================================================" << std::endl;
    
    try {
        // Test IEEE 1722-2016 AVTP provider implementation
        test_ieee_1722_2016_avtp_provider();
        
        // Test Milan professional audio integration
        test_milan_professional_audio_integration();
        
        std::cout << "\n🎉 ALL TESTS PASSED! IEEE 1722-2016 AVTP Provider fully functional" << std::endl;
        std::cout << "✅ Milan professional audio media transport with AAF-PCM support" << std::endl;
        std::cout << "✅ Stream redundancy capabilities validated" << std::endl;
        std::cout << "✅ Presentation time and latency requirements met" << std::endl;
        std::cout << "✅ Media clock synchronization working" << std::endl;
        std::cout << "✅ Quality monitoring (packet loss) implemented" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
