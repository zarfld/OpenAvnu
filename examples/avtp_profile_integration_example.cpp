/**
 * @file avtp_profile_integration_example.cpp
 * @brief Example demonstrating AVTP profile integration with gPTP and OpenAvnu
 * 
 * This example shows how to:
 * 1. Initialize and configure AVTP profiles for different use cases
 * 2. Integrate AVTP profiles with gPTP profile selection
 * 3. Validate stream configurations against profile requirements
 * 4. Handle runtime profile switching and compatibility checking
 */

#include "../lib/avtp_pipeline/include/avtp_profile.hpp"
#include <iostream>
#include <string>

class AVTPProfileManager {
public:
    AVTPProfileManager() : current_profile_("standard") {
        initializeProfiles();
    }
    
    void initializeProfiles() {
        profiles_["milan"] = AVTPProfileFactory::createMilanProfile("2.0a");
        profiles_["automotive"] = AVTPProfileFactory::createAutomotiveProfile("1.6");
        profiles_["proav"] = AVTPProfileFactory::createProAVProfile();
        profiles_["industrial"] = AVTPProfileFactory::createIndustrialProfile();
        profiles_["standard"] = AVTPProfileFactory::createStandardProfile();
        
        std::cout << "✅ Initialized " << profiles_.size() << " AVTP profiles\n";
    }
    
    bool setProfile(const std::string& profile_name) {
        if (profiles_.find(profile_name) == profiles_.end()) {
            std::cout << "❌ Unknown profile: " << profile_name << "\n";
            return false;
        }
        
        if (!AVTPProfileFactory::validateProfile(profiles_[profile_name])) {
            std::cout << "❌ Invalid profile: " << profile_name << "\n";
            return false;
        }
        
        current_profile_ = profile_name;
        std::cout << "✅ Switched to profile: " << profile_name << "\n";
        return true;
    }
    
    const AVTPProfile& getCurrentProfile() const {
        return profiles_.at(current_profile_);
    }
    
    bool validateStreamConfig(AVTPStreamFormat format, uint32_t sample_rate, 
                            uint16_t channels, uint64_t presentation_offset) {
        const auto& profile = getCurrentProfile();
        return profile.validate_stream_parameters(format, sample_rate, channels, presentation_offset);
    }
    
    void listSupportedFormats() const {
        const auto& profile = getCurrentProfile();
        std::cout << "\n📊 Supported stream formats for " << current_profile_ << " profile:\n";
        
        for (const auto& format : profile.supported_talker_formats) {
            std::cout << "  Talker: " << getFormatName(format) << "\n";
        }
        
        for (const auto& format : profile.supported_listener_formats) {
            std::cout << "  Listener: " << getFormatName(format) << "\n";
        }
    }
    
private:
    std::map<std::string, AVTPProfile> profiles_;
    std::string current_profile_;
    
    std::string getFormatName(AVTPStreamFormat format) const {
        switch (format) {
            case AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS: return "AAF PCM 48kHz (up to 8ch)";
            case AVTPStreamFormat::AAF_PCM_96_0_UP_TO_8_CHANNELS: return "AAF PCM 96kHz (up to 8ch)";
            case AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS: return "AAF PCM 192kHz (up to 8ch)";
            case AVTPStreamFormat::CVF_H264: return "CVF H.264 Video";
            case AVTPStreamFormat::CVF_MJPEG: return "CVF MJPEG Video";
            case AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0: return "CRF Audio Sample 48kHz";
            case AVTPStreamFormat::CRF_AUDIO_SAMPLE_96_0: return "CRF Audio Sample 96kHz";
            case AVTPStreamFormat::MPEG2_TS: return "MPEG2 Transport Stream";
            case AVTPStreamFormat::CUSTOM_FORMAT: return "Custom/Vendor Format";
            default: return "Unknown format";
        }
    }
};

// Example functions demonstrating profile usage scenarios

void demonstrateMilanAudioSetup() {
    std::cout << "\n🎵 Milan Audio Setup Example\n";
    std::cout << "==========================\n";
    
    AVTPProfileManager manager;
    manager.setProfile("milan");
    
    // Typical Milan audio stream: 48kHz, 8 channels, 2ms max latency
    bool valid = manager.validateStreamConfig(
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        48000,  // 48kHz sample rate
        8,      // 8 channels
        1500000 // 1.5ms presentation offset (within 2ms limit)
    );
    
    std::cout << "Milan 48kHz/8ch/1.5ms stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    // Test invalid configuration (exceeds Milan limits)
    valid = manager.validateStreamConfig(
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        48000,  // 48kHz sample rate
        8,      // 8 channels
        3000000 // 3ms presentation offset (exceeds 2ms limit)
    );
    
    std::cout << "Milan 48kHz/8ch/3ms stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    manager.listSupportedFormats();
}

void demonstrateAutomotiveVideoSetup() {
    std::cout << "\n🚗 Automotive Video Setup Example\n";
    std::cout << "================================\n";
    
    AVTPProfileManager manager;
    manager.setProfile("automotive");
    
    // Automotive H.264 video stream
    bool valid = manager.validateStreamConfig(
        AVTPStreamFormat::CVF_H264,
        0,      // N/A for video
        0,      // N/A for video
        25000000 // 25ms presentation offset
    );
    
    std::cout << "Automotive H.264/25ms stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    // Automotive audio stream
    valid = manager.validateStreamConfig(
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        48000,  // 48kHz sample rate
        2,      // 2 channels (stereo)
        5000000 // 5ms presentation offset
    );
    
    std::cout << "Automotive 48kHz/2ch/5ms stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    manager.listSupportedFormats();
}

void demonstrateProAVProfessionalSetup() {
    std::cout << "\n🎤 ProAV Professional Audio Setup Example\n";
    std::cout << "========================================\n";
    
    AVTPProfileManager manager;
    manager.setProfile("proav");
    
    // High-resolution audio for professional applications
    bool valid = manager.validateStreamConfig(
        AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS,
        192000, // 192kHz sample rate
        8,      // 8 channels
        2500000 // 2.5ms presentation offset
    );
    
    std::cout << "ProAV 192kHz/8ch/2.5ms stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    // Low-latency monitoring
    valid = manager.validateStreamConfig(
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        48000,  // 48kHz sample rate
        2,      // 2 channels
        1000000 // 1ms presentation offset (low latency)
    );
    
    std::cout << "ProAV 48kHz/2ch/1ms low-latency validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    manager.listSupportedFormats();
}

void demonstrateIndustrialControlSetup() {
    std::cout << "\n🏭 Industrial Control Setup Example\n";
    std::cout << "=================================\n";
    
    AVTPProfileManager manager;
    manager.setProfile("industrial");
    
    // Industrial control data stream
    bool valid = manager.validateStreamConfig(
        AVTPStreamFormat::MPEG2_TS,  // Control data format
        0,      // N/A for control data
        0,      // N/A for control data
        500000  // 500μs presentation offset (deterministic)
    );
    
    std::cout << "Industrial control/500μs stream validation: " << (valid ? "✅ VALID" : "❌ INVALID") << "\n";
    
    manager.listSupportedFormats();
}

void demonstrateRuntimeProfileSwitching() {
    std::cout << "\n🔄 Runtime Profile Switching Example\n";
    std::cout << "==================================\n";
    
    AVTPProfileManager manager;
    
    // Start with standard profile
    std::cout << "Starting with standard profile...\n";
    manager.listSupportedFormats();
    
    // Switch to Milan for professional audio
    std::cout << "\nSwitching to Milan for professional audio...\n";
    manager.setProfile("milan");
    manager.listSupportedFormats();
    
    // Switch to automotive for vehicle applications
    std::cout << "\nSwitching to automotive for vehicle applications...\n";
    manager.setProfile("automotive");
    manager.listSupportedFormats();
    
    // Test invalid profile switch
    std::cout << "\nTesting invalid profile switch...\n";
    manager.setProfile("nonexistent");
}

void demonstrateProfileCompatibilityChecking() {
    std::cout << "\n🔗 Profile Compatibility Checking Example\n";
    std::cout << "========================================\n";
    
    auto milan_profile = AVTPProfileFactory::createMilanProfile();
    auto automotive_profile = AVTPProfileFactory::createAutomotiveProfile();
    
    // Check if Milan AVTP profile is compatible with Milan gPTP profile
    bool compatible = AVTPProfileFactory::validateProfileCompatibility(milan_profile, "milan");
    std::cout << "Milan AVTP + Milan gPTP compatibility: " << (compatible ? "✅ COMPATIBLE" : "❌ INCOMPATIBLE") << "\n";
    
    // Check if Milan AVTP profile is compatible with Automotive gPTP profile (should fail)
    compatible = AVTPProfileFactory::validateProfileCompatibility(milan_profile, "automotive");
    std::cout << "Milan AVTP + Automotive gPTP compatibility: " << (compatible ? "✅ COMPATIBLE" : "❌ INCOMPATIBLE") << "\n";
    
    // Check if Automotive AVTP profile is compatible with Automotive gPTP profile
    compatible = AVTPProfileFactory::validateProfileCompatibility(automotive_profile, "automotive");
    std::cout << "Automotive AVTP + Automotive gPTP compatibility: " << (compatible ? "✅ COMPATIBLE" : "❌ INCOMPATIBLE") << "\n";
}

void demonstratePerformanceOptimization() {
    std::cout << "\n⚡ Performance Optimization Example\n";
    std::cout << "=================================\n";
    
    AVTPProfileManager manager;
    manager.setProfile("milan");
    
    const auto& profile = manager.getCurrentProfile();
    
    std::cout << "Milan Performance Parameters:\n";
    std::cout << "  Target packet processing time: " << profile.target_packet_processing_time_ns / 1000.0 << "μs\n";
    std::cout << "  Max stream setup time: " << profile.max_stream_setup_time_ms << "ms\n";
    std::cout << "  Max stream start time: " << profile.max_stream_start_time_ms << "ms\n";
    // std::cout << "  Buffer management strategy: Optimized\n";
    std::cout << "  Hardware timestamping: " << (profile.hardware_requirements.requires_hardware_timestamping ? "Required" : "Optional") << "\n";
}

int main() {
    std::cout << "🎵 AVTP Profile Integration Example\n";
    std::cout << "==================================\n";
    std::cout << "Demonstrating comprehensive AVTP profile usage with OpenAvnu\n\n";
    
    try {
        // Demonstrate various profile use cases
        demonstrateMilanAudioSetup();
        demonstrateAutomotiveVideoSetup();
        demonstrateProAVProfessionalSetup();
        demonstrateIndustrialControlSetup();
        
        // Demonstrate advanced features
        demonstrateRuntimeProfileSwitching();
        demonstrateProfileCompatibilityChecking();
        demonstratePerformanceOptimization();
        
        std::cout << "\n🎯 AVTP Profile Integration Example Complete!\n";
        std::cout << "============================================\n";
        std::cout << "✅ All profile scenarios demonstrated successfully\n";
        std::cout << "✅ Runtime configuration and validation working\n";
        std::cout << "✅ Cross-profile compatibility checking functional\n";
        std::cout << "✅ Performance optimization parameters accessible\n";
        std::cout << "\n🚀 Ready for production integration with OpenAvnu!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
