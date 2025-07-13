/**
 * @file avtp_profile.cpp
 * @brief Unified AVTP Profile Configuration Implementation
 * 
 * This implementation provides concrete profile configurations based on
 * industry standards and specifications from the attached documents.
 */

#include "include/avtp_profile.hpp"
#include <algorithm>
#include <cstring>
#include <string>

// AVTPProfile implementation
AVTPProfile::AVTPProfile() {
    // Default to Standard IEEE 1722-2016 profile
    profile_name = "standard";
    profile_version = "2016";
    profile_description = "Standard IEEE 1722-2016 Profile";
    profile_type = AVTPProfileType::STANDARD;
    
    // Standard stream format support - basic AAF
    supported_talker_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0
    };
    supported_listener_formats = supported_talker_formats;
    mandatory_formats = {AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS};
    optional_formats = {AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0};
    
    // Standard AVTP packet configuration
    supports_open1722_mode = true;
    supports_legacy_mode = true;
    allows_runtime_mode_switching = true;
    default_to_open1722 = true;
    
    // Standard timing requirements
    max_presentation_time_offset_ns = 50000000;  // 50ms
    target_wakeup_time_ns = 1000000;             // 1ms
    max_sync_uncertainty_ns = 1000;              // 1μs
    requires_media_clock_recovery = false;
    supports_media_clock_distribution = true;
    
    // Standard stream class - Class A equivalent
    AVTPStreamClassRequirements class_a;
    class_a.class_name = "Class A";
    class_a.max_frame_size = 1500;
    class_a.max_interval_frames = 1;
    class_a.max_transit_time_us = 2000;          // 2ms
    class_a.max_timing_uncertainty_us = 125;     // 125μs
    class_a.supports_redundancy = false;
    class_a.requires_presentation_time = true;
    stream_classes["Class A"] = class_a;
    
    // Standard stream limits
    max_streams_per_talker = 1;
    max_streams_per_listener = 1;
    max_concurrent_streams = 8;
    supports_dynamic_stream_changes = false;
    
    // Standard protocol behavior
    strict_stream_id_validation = false;
    supports_avdecc_fast_connect = false;
    requires_stream_backup_support = false;
    supports_seamless_stream_switching = false;
    
    // Standard media requirements
    media_requirements.supported_sample_rates = {48000};
    media_requirements.supported_bit_depths = {16, 24};
    media_requirements.supported_channel_counts = {1, 2, 4, 6, 8};
    media_requirements.supports_multi_stream_audio = false;
    media_requirements.requires_sample_rate_conversion = false;
    media_requirements.supported_crf_rates = {48000};
    media_requirements.supports_multiple_clock_domains = false;
    media_requirements.requires_crf_timestamp_validation = false;
    
    // Standard hardware requirements
    hardware_requirements.requires_hardware_timestamping = false;
    hardware_requirements.requires_credit_based_shaper = true;
    hardware_requirements.requires_time_aware_traffic = false;
    hardware_requirements.requires_frame_preemption = false;
    hardware_requirements.min_link_speed_mbps = 100;
    hardware_requirements.supported_phys = {"1000BASE-T", "100BASE-TX"};
    
    // Initialize profile-specific configs to defaults
    automotive_config = {};
    milan_config = {};
    proav_config = {};
    industrial_config = {};
    
    // Standard performance limits
    max_stream_setup_time_ms = 2000;       // 2 seconds
    max_stream_start_time_ms = 1000;       // 1 second
    target_packet_processing_time_ns = 10000; // 10μs
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
}

bool AVTPProfile::supports_stream_format(AVTPStreamFormat format, bool is_talker) const {
    const auto& formats = is_talker ? supported_talker_formats : supported_listener_formats;
    return std::find(formats.begin(), formats.end(), format) != formats.end();
}

const AVTPStreamClassRequirements* AVTPProfile::get_stream_class_requirements(AVTPStreamFormat format) const {
    // Default mapping - can be overridden by specific profiles
    switch (format) {
        case AVTPStreamFormat::AAF_PCM_44_1_UP_TO_8_CHANNELS:
        case AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS:
        case AVTPStreamFormat::AAF_PCM_88_2_UP_TO_8_CHANNELS:
        case AVTPStreamFormat::AAF_PCM_96_0_UP_TO_8_CHANNELS:
        case AVTPStreamFormat::AAF_PCM_176_4_UP_TO_8_CHANNELS:
        case AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS:
            if (stream_classes.find("Class A") != stream_classes.end()) {
                return &stream_classes.at("Class A");
            }
            break;
        case AVTPStreamFormat::CVF_H264:
        case AVTPStreamFormat::CVF_MJPEG:
        case AVTPStreamFormat::MPEG2_TS:
            if (stream_classes.find("Class B") != stream_classes.end()) {
                return &stream_classes.at("Class B");
            }
            break;
        default:
            break;
    }
    return nullptr;
}

bool AVTPProfile::validate_stream_parameters(AVTPStreamFormat format, uint32_t sample_rate,
                                           uint8_t channels, uint32_t presentation_time_offset) const {
    // Validate format support
    if (!supports_stream_format(format, true)) {
        return false;
    }
    
    // Validate sample rate for audio formats
    if (format >= AVTPStreamFormat::AAF_PCM_44_1_UP_TO_8_CHANNELS && 
        format <= AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS) {
        auto& rates = media_requirements.supported_sample_rates;
        if (std::find(rates.begin(), rates.end(), sample_rate) == rates.end()) {
            return false;
        }
    }
    
    // Validate channel count
    auto& channel_counts = media_requirements.supported_channel_counts;
    if (std::find(channel_counts.begin(), channel_counts.end(), channels) == channel_counts.end()) {
        return false;
    }
    
    // Validate presentation time offset
    if (presentation_time_offset > max_presentation_time_offset_ns) {
        return false;
    }
    
    return true;
}

AVTPProfile::AVTPPacketConfig AVTPProfile::get_recommended_packet_config(AVTPStreamFormat format) const {
    AVTPPacketConfig config;
    
    // Default packet configuration
    config.max_packet_size = 1500;
    config.packets_per_frame = 1;
    config.packet_interval_ns = 125000;  // 125μs (8kHz)
    config.use_presentation_time = true;
    
    // Format-specific optimizations
    switch (format) {
        case AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS:
            config.packet_interval_ns = 1000000;  // 1ms for 48kHz
            break;
        case AVTPStreamFormat::AAF_PCM_96_0_UP_TO_8_CHANNELS:
            config.packet_interval_ns = 500000;   // 0.5ms for 96kHz
            break;
        case AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS:
            config.packet_interval_ns = 250000;   // 0.25ms for 192kHz
            break;
        default:
            break;
    }
    
    return config;
}

namespace AVTPProfileFactory {

AVTPProfile createMilanProfile(const std::string& version) {
    AVTPProfile profile;
    
    // Milan profile identification
    profile.profile_name = "milan";
    profile.profile_version = version;
    profile.profile_description = "Milan Baseline Interoperability Profile v" + version;
    profile.profile_type = (version == "1.0" || version == "1.1") ? 
                          AVTPProfileType::MILAN_V1 : AVTPProfileType::MILAN_V2;
    
    // Milan stream format support - comprehensive audio formats
    profile.supported_talker_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_96_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_96_0,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_192_0
    };
    profile.supported_listener_formats = profile.supported_talker_formats;
    
    // Milan mandatory formats based on Milan spec
    profile.mandatory_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0
    };
    
    // Milan AVTP configuration
    profile.supports_open1722_mode = true;
    profile.supports_legacy_mode = false;  // Milan prefers standards-compliant
    profile.allows_runtime_mode_switching = false;
    profile.default_to_open1722 = true;
    
    // Milan timing requirements - strict professional audio
    profile.max_presentation_time_offset_ns = 2000000;  // 2ms max (Milan requirement)
    profile.target_wakeup_time_ns = 125000;             // 125μs target
    profile.max_sync_uncertainty_ns = 1000;             // 1μs max uncertainty
    profile.requires_media_clock_recovery = true;
    profile.supports_media_clock_distribution = true;
    
    // Milan stream classes - strict Class A requirements
    AVTPStreamClassRequirements milan_class_a;
    milan_class_a.class_name = "Milan Class A";
    milan_class_a.max_frame_size = 1500;
    milan_class_a.max_interval_frames = 1;
    milan_class_a.max_transit_time_us = 2000;           // 2ms strict
    milan_class_a.max_timing_uncertainty_us = 125;      // 125μs max
    milan_class_a.supports_redundancy = true;           // Milan redundancy support
    milan_class_a.requires_presentation_time = true;
    profile.stream_classes["Milan Class A"] = milan_class_a;
    
    // Milan stream limits - professional requirements
    profile.max_streams_per_talker = 64;   // Milan supports many streams
    profile.max_streams_per_listener = 64;
    profile.max_concurrent_streams = 256;
    profile.supports_dynamic_stream_changes = true;
    
    // Milan protocol behavior - strict compliance
    profile.strict_stream_id_validation = true;
    profile.supports_avdecc_fast_connect = true;
    profile.requires_stream_backup_support = true;
    profile.supports_seamless_stream_switching = true;
    
    // Milan media requirements - professional audio
    profile.media_requirements.supported_sample_rates = {44100, 48000, 88200, 96000, 176400, 192000};
    profile.media_requirements.supported_bit_depths = {16, 24, 32};
    profile.media_requirements.supported_channel_counts = {1, 2, 4, 6, 8, 16, 24, 32};
    profile.media_requirements.supports_multi_stream_audio = true;
    profile.media_requirements.requires_sample_rate_conversion = true;
    profile.media_requirements.supported_crf_rates = {44100, 48000, 88200, 96000, 176400, 192000};
    profile.media_requirements.supports_multiple_clock_domains = true;
    profile.media_requirements.requires_crf_timestamp_validation = true;
    
    // Milan hardware requirements - professional grade
    profile.hardware_requirements.requires_hardware_timestamping = true;
    profile.hardware_requirements.requires_credit_based_shaper = true;
    profile.hardware_requirements.requires_time_aware_traffic = false;
    profile.hardware_requirements.requires_frame_preemption = false;
    profile.hardware_requirements.min_link_speed_mbps = 100;
    profile.hardware_requirements.supported_phys = {"1000BASE-T", "100BASE-TX"};
    
    // Milan-specific configuration
    profile.milan_config.requires_milan_entity_model = true;
    profile.milan_config.supports_milan_redundancy = true;
    profile.milan_config.requires_milan_discovery = true;
    profile.milan_config.supports_milan_control_protocol = true;
    profile.milan_config.milan_certification_level = version;
    
    // Milan performance requirements
    profile.max_stream_setup_time_ms = 500;        // 500ms max setup
    profile.max_stream_start_time_ms = 100;        // 100ms max start
    profile.target_packet_processing_time_ns = 1000; // 1μs target processing
    
    return profile;
}

AVTPProfile createAutomotiveProfile(const std::string& version) {
    AVTPProfile profile;
    
    // Automotive profile identification
    profile.profile_name = "automotive";
    profile.profile_version = version;
    profile.profile_description = "Automotive Ethernet AVB Profile v" + version;
    profile.profile_type = (version == "1.5") ? 
                          AVTPProfileType::AUTOMOTIVE_V1_5 : AVTPProfileType::AUTOMOTIVE_V1_6;
    
    // Automotive stream format support - focus on essential audio/video
    profile.supported_talker_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_44_1_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CVF_H264,
        AVTPStreamFormat::CVF_MJPEG,
        AVTPStreamFormat::MPEG2_TS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0
    };
    profile.supported_listener_formats = profile.supported_talker_formats;
    
    // Automotive mandatory formats
    profile.mandatory_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS
    };
    
    // Automotive AVTP configuration
    profile.supports_open1722_mode = true;
    profile.supports_legacy_mode = true;
    profile.allows_runtime_mode_switching = false;  // Automotive prefers stability
    profile.default_to_open1722 = true;
    
    // Automotive timing requirements - deterministic
    profile.max_presentation_time_offset_ns = 50000000;  // 50ms max
    profile.target_wakeup_time_ns = 2000000;             // 2ms target
    profile.max_sync_uncertainty_ns = 1000;              // 1μs max
    profile.requires_media_clock_recovery = false;       // Not required in automotive
    profile.supports_media_clock_distribution = true;
    
    // Automotive stream classes
    AVTPStreamClassRequirements auto_class_a;
    auto_class_a.class_name = "Automotive Class A";
    auto_class_a.max_frame_size = 1500;
    auto_class_a.max_interval_frames = 1;
    auto_class_a.max_transit_time_us = 50000;           // 50ms for automotive
    auto_class_a.max_timing_uncertainty_us = 125;
    auto_class_a.supports_redundancy = false;           // Automotive doesn't require redundancy
    auto_class_a.requires_presentation_time = true;
    profile.stream_classes["Automotive Class A"] = auto_class_a;
    
    // Automotive stream limits - moderate requirements
    profile.max_streams_per_talker = 8;
    profile.max_streams_per_listener = 8;
    profile.max_concurrent_streams = 16;
    profile.supports_dynamic_stream_changes = false;    // Automotive prefers static config
    
    // Automotive protocol behavior
    profile.strict_stream_id_validation = false;
    profile.supports_avdecc_fast_connect = false;       // Automotive may disable AVDECC
    profile.requires_stream_backup_support = false;
    profile.supports_seamless_stream_switching = false;
    
    // Automotive media requirements - basic audio/video
    profile.media_requirements.supported_sample_rates = {44100, 48000};
    profile.media_requirements.supported_bit_depths = {16, 24};
    profile.media_requirements.supported_channel_counts = {1, 2, 4, 6, 8};
    profile.media_requirements.supports_multi_stream_audio = false;
    profile.media_requirements.requires_sample_rate_conversion = false;
    profile.media_requirements.supported_video_codecs = {"H.264", "MJPEG"};
    profile.media_requirements.max_video_resolution_width = 1920;
    profile.media_requirements.max_video_resolution_height = 1080;
    profile.media_requirements.max_video_framerate = 30;
    profile.media_requirements.supports_compressed_video = true;
    
    // Automotive hardware requirements
    profile.hardware_requirements.requires_hardware_timestamping = true;
    profile.hardware_requirements.requires_credit_based_shaper = true;
    profile.hardware_requirements.requires_time_aware_traffic = false;
    profile.hardware_requirements.requires_frame_preemption = false;
    profile.hardware_requirements.min_link_speed_mbps = 100;
    profile.hardware_requirements.supported_phys = {"100BASE-T1", "1000BASE-T1"};
    
    // Automotive-specific configuration
    profile.automotive_config.disable_avdecc_entity_discovery = true;  // Automotive may disable AVDECC
    profile.automotive_config.use_automotive_addressing = true;
    profile.automotive_config.supports_diagnostic_counters = true;
    profile.automotive_config.requires_deterministic_latency = true;
    profile.automotive_config.max_end_to_end_latency_us = 100000;      // 100ms max
    profile.automotive_config.supported_vehicle_networks = {"CAN", "FlexRay", "Ethernet"};
    
    // Automotive performance requirements
    profile.max_stream_setup_time_ms = 5000;           // 5 seconds max for automotive
    profile.max_stream_start_time_ms = 2000;           // 2 seconds max start
    profile.target_packet_processing_time_ns = 50000;  // 50μs target processing
    
    return profile;
}

AVTPProfile createProAVProfile() {
    AVTPProfile profile;
    
    // ProAV profile identification
    profile.profile_name = "proav";
    profile.profile_version = "2.0";
    profile.profile_description = "AVnu ProAV Functional Interoperability Profile";
    profile.profile_type = AVTPProfileType::AVNU_BASE;
    
    // ProAV stream format support - comprehensive professional audio
    profile.supported_talker_formats = {
        AVTPStreamFormat::AAF_PCM_44_1_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_88_2_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_96_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_176_4_UP_TO_8_CHANNELS,
        AVTPStreamFormat::AAF_PCM_192_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_96_0,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_192_0
    };
    profile.supported_listener_formats = profile.supported_talker_formats;
    
    // ProAV mandatory formats
    profile.mandatory_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0
    };
    
    // ProAV configuration - similar to Milan but more flexible
    profile.supports_open1722_mode = true;
    profile.supports_legacy_mode = true;
    profile.allows_runtime_mode_switching = true;
    profile.default_to_open1722 = true;
    
    // ProAV timing requirements - professional but not as strict as Milan
    profile.max_presentation_time_offset_ns = 5000000;  // 5ms max
    profile.target_wakeup_time_ns = 250000;             // 250μs target
    profile.max_sync_uncertainty_ns = 1000;             // 1μs max
    profile.requires_media_clock_recovery = true;
    profile.supports_media_clock_distribution = true;
    
    // ProAV-specific configuration
    profile.proav_config.supports_professional_audio = true;
    profile.proav_config.requires_low_latency_audio = true;
    profile.proav_config.supports_dante_interop = false;        // Optional
    profile.proav_config.supports_aes67_compatibility = true;
    profile.proav_config.max_audio_latency_ms = 10;             // 10ms max
    
    return profile;
}

AVTPProfile createIndustrialProfile() {
    AVTPProfile profile;
    
    // Industrial profile identification
    profile.profile_name = "industrial";
    profile.profile_version = "1.0";
    profile.profile_description = "Industrial Ethernet AVB Profile";
    profile.profile_type = AVTPProfileType::INDUSTRIAL;
    
    // Industrial stream format support - basic audio, focus on control
    profile.supported_talker_formats = {
        AVTPStreamFormat::AAF_PCM_48_0_UP_TO_8_CHANNELS,
        AVTPStreamFormat::CRF_AUDIO_SAMPLE_48_0,
        AVTPStreamFormat::CUSTOM_FORMAT  // Industrial control data
    };
    profile.supported_listener_formats = profile.supported_talker_formats;
    
    // Industrial hardware requirements - rugged
    profile.hardware_requirements.requires_hardware_timestamping = true;
    profile.hardware_requirements.requires_credit_based_shaper = true;
    profile.hardware_requirements.requires_time_aware_traffic = true;   // TSN required
    profile.hardware_requirements.requires_frame_preemption = true;     // Industrial needs preemption
    profile.hardware_requirements.min_link_speed_mbps = 100;
    
    // Industrial-specific configuration
    profile.industrial_config.supports_industrial_protocols = true;
    profile.industrial_config.requires_real_time_guarantees = true;
    profile.industrial_config.supports_safety_functions = true;
    profile.industrial_config.max_industrial_cycle_time_us = 1000;      // 1ms max cycle
    profile.industrial_config.supported_industrial_standards = {"IEC 61158", "IEC 61784"};
    
    return profile;
}

AVTPProfile createStandardProfile() {
    // Return the default constructor profile (already standard)
    return AVTPProfile();
}

AVTPProfile createProfileFromConfig(const std::string& config_path) {
    // TODO: Implement configuration file parsing
    // For now, return standard profile
    return createStandardProfile();
}

AVTPProfile createProfileByName(const std::string& profile_name, const std::string& version) {
    if (profile_name == "milan") {
        return createMilanProfile(version.empty() ? "2.0a" : version);
    } else if (profile_name == "automotive") {
        return createAutomotiveProfile(version.empty() ? "1.6" : version);
    } else if (profile_name == "proav" || profile_name == "avnu_base") {
        return createProAVProfile();
    } else if (profile_name == "industrial") {
        return createIndustrialProfile();
    } else {
        return createStandardProfile();
    }
}

bool validateProfile(const AVTPProfile& profile) {
    // Basic validation checks
    if (profile.profile_name.empty() || profile.profile_version.empty()) {
        return false;
    }
    
    if (profile.mandatory_formats.empty()) {
        return false;
    }
    
    if (profile.stream_classes.empty()) {
        return false;
    }
    
    // Validate mandatory formats are supported
    for (const auto& format : profile.mandatory_formats) {
        if (!profile.supports_stream_format(format, true)) {
            return false;
        }
    }
    
    return true;
}

std::string getProfileDescription(const AVTPProfile& profile) {
    std::string desc = profile.profile_description + "\n";
    desc += "Supported talker formats: " + std::to_string(profile.supported_talker_formats.size()) + "\n";
    desc += "Supported listener formats: " + std::to_string(profile.supported_listener_formats.size()) + "\n";
    desc += "Mandatory formats: " + std::to_string(profile.mandatory_formats.size()) + "\n";
    desc += "Stream classes: " + std::to_string(profile.stream_classes.size()) + "\n";
    desc += "Open1722 support: " + std::string(profile.supports_open1722_mode ? "Yes" : "No") + "\n";
    desc += "Legacy support: " + std::string(profile.supports_legacy_mode ? "Yes" : "No") + "\n";
    return desc;
}

bool validateProfileCompatibility(const AVTPProfile& avtp_profile, const std::string& gptp_profile_name) {
    // Check common compatibility rules
    if (avtp_profile.profile_name == "milan" && gptp_profile_name == "milan") {
        return true;
    }
    if (avtp_profile.profile_name == "automotive" && gptp_profile_name == "automotive") {
        return true;
    }
    if (avtp_profile.profile_name == "proav" && gptp_profile_name == "avnu_base") {
        return true;
    }
    
    // Standard profiles are generally compatible
    if (avtp_profile.profile_name == "standard" || gptp_profile_name == "standard") {
        return true;
    }
    
    return false;
}

std::vector<std::string> getAvailableProfiles() {
    return {"milan", "automotive", "proav", "industrial", "standard"};
}

} // namespace AVTPProfileFactory
