/**
 * @file avtp_profile.hpp
 * @brief Unified AVTP Profile Configuration Structure
 * 
 * This file defines a unified AVTP profile system that works alongside 
 * the existing gPTP profile framework. It contains all AVTP-specific
 * profile settings for Milan, Automotive, ProAV, and Industrial profiles.
 * 
 * This builds on the proven gPTP profile architecture from gptp_profile.hpp
 * to provide consistent profile management across the entire OpenAvnu stack.
 */

#ifndef AVTP_PROFILE_HPP
#define AVTP_PROFILE_HPP

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>

/**
 * @brief AVTP Stream Format Types
 */
enum class AVTPStreamFormat {
    AAF_PCM_44_1_UP_TO_8_CHANNELS,     // AAF PCM 44.1kHz up to 8 channels
    AAF_PCM_48_0_UP_TO_8_CHANNELS,     // AAF PCM 48.0kHz up to 8 channels  
    AAF_PCM_88_2_UP_TO_8_CHANNELS,     // AAF PCM 88.2kHz up to 8 channels
    AAF_PCM_96_0_UP_TO_8_CHANNELS,     // AAF PCM 96.0kHz up to 8 channels
    AAF_PCM_176_4_UP_TO_8_CHANNELS,    // AAF PCM 176.4kHz up to 8 channels
    AAF_PCM_192_0_UP_TO_8_CHANNELS,    // AAF PCM 192.0kHz up to 8 channels
    CRF_AUDIO_SAMPLE_48_0,             // CRF Audio Sample 48.0kHz
    CRF_AUDIO_SAMPLE_96_0,             // CRF Audio Sample 96.0kHz
    CRF_AUDIO_SAMPLE_192_0,            // CRF Audio Sample 192.0kHz
    CVF_H264,                          // CVF H.264 Video Format
    CVF_MJPEG,                         // CVF Motion JPEG Format
    MPEG2_TS,                          // MPEG2 Transport Stream
    CUSTOM_FORMAT                       // Custom/Vendor-specific format
};

/**
 * @brief AVTP Profile Types matching industry standards
 */
enum class AVTPProfileType {
    MILAN_V1,           // Milan Baseline Interoperability v1.x
    MILAN_V2,           // Milan Baseline Interoperability v2.x  
    AVNU_BASE,          // AVnu Base/ProAV Functional Interoperability
    AUTOMOTIVE_V1_5,    // Automotive Ethernet AVB v1.5
    AUTOMOTIVE_V1_6,    // Automotive Ethernet AVB v1.6
    INDUSTRIAL,         // Industrial Ethernet AVB Profile
    STANDARD            // Standard IEEE 1722-2016
};

/**
 * @brief AVTP Stream Class Requirements
 */
struct AVTPStreamClassRequirements {
    std::string class_name;             // "Class A", "Class B", "CDT"
    uint32_t max_frame_size;            // Maximum frame size (bytes)
    uint32_t max_interval_frames;       // Maximum interval frames
    uint32_t max_transit_time_us;       // Maximum transit time (microseconds) 
    uint32_t max_timing_uncertainty_us; // Maximum timing uncertainty (microseconds)
    bool supports_redundancy;           // Support for seamless redundancy
    bool requires_presentation_time;    // Requires presentation time
};

/**
 * @brief AVTP Hardware Requirements per Profile
 */
struct AVTPHardwareRequirements {
    bool requires_hardware_timestamping; // Hardware timestamping required
    bool requires_credit_based_shaper;   // CBS required
    bool requires_time_aware_traffic;    // TSN time-aware traffic control
    bool requires_frame_preemption;     // Frame preemption support
    uint32_t min_link_speed_mbps;       // Minimum link speed (Mbps)
    std::vector<std::string> supported_phys; // Supported PHY types
};

/**
 * @brief Unified AVTP Profile Configuration
 * 
 * This structure contains all AVTP-specific profile settings and behaviors.
 * Different profiles (Milan, Automotive, ProAV, Industrial) are just
 * different initializations of this same structure.
 */
struct AVTPProfile {
    // Profile identification
    std::string profile_name;              // "milan", "automotive", "proav", "industrial", "standard"
    std::string profile_version;           // Profile version string (e.g. "2.0a", "1.6")
    std::string profile_description;       // Human-readable description
    AVTPProfileType profile_type;          // Enumerated profile type
    
    // Stream format configuration
    std::vector<AVTPStreamFormat> supported_talker_formats;   // Supported talker formats
    std::vector<AVTPStreamFormat> supported_listener_formats; // Supported listener formats
    std::vector<AVTPStreamFormat> mandatory_formats;          // Mandatory format support
    std::vector<AVTPStreamFormat> optional_formats;           // Optional format support
    
    // AVTP packet configuration  
    bool supports_open1722_mode;          // Open1722 library integration support
    bool supports_legacy_mode;            // Legacy AVTP implementation support
    bool allows_runtime_mode_switching;   // Runtime switching between Open1722/legacy
    bool default_to_open1722;             // Default mode selection
    
    // Timing and synchronization requirements
    uint32_t max_presentation_time_offset_ns;  // Maximum presentation time offset (ns)
    uint32_t target_wakeup_time_ns;           // Target wakeup time for talkers (ns)
    uint32_t max_sync_uncertainty_ns;         // Maximum sync uncertainty (ns)
    bool requires_media_clock_recovery;       // Media clock recovery required
    bool supports_media_clock_distribution;   // Media clock distribution via CRF
    
    // Stream reservation requirements  
    std::map<std::string, AVTPStreamClassRequirements> stream_classes; // Stream class definitions
    uint32_t max_streams_per_talker;         // Maximum streams per talker
    uint32_t max_streams_per_listener;       // Maximum streams per listener
    uint32_t max_concurrent_streams;         // Maximum concurrent streams
    bool supports_dynamic_stream_changes;    // Dynamic stream parameter changes
    
    // Protocol behavior configuration
    bool strict_stream_id_validation;        // Strict stream ID validation
    bool supports_avdecc_fast_connect;      // AVDECC fast connect support
    bool requires_stream_backup_support;     // Stream backup/redundancy required
    bool supports_seamless_stream_switching; // Seamless stream switching
    
    // Media format specific requirements
    struct {
        // Audio format requirements
        std::vector<uint32_t> supported_sample_rates;    // Supported audio sample rates
        std::vector<uint8_t> supported_bit_depths;       // Supported bit depths (16, 24, 32)
        std::vector<uint8_t> supported_channel_counts;   // Supported channel counts
        bool supports_multi_stream_audio;               // Multi-stream audio support
        bool requires_sample_rate_conversion;           // SRC support required
        
        // Video format requirements (for CVF support)
        std::vector<std::string> supported_video_codecs; // Supported video codecs
        uint32_t max_video_resolution_width;            // Maximum video width
        uint32_t max_video_resolution_height;           // Maximum video height
        uint32_t max_video_framerate;                   // Maximum video framerate
        bool supports_compressed_video;                 // Compressed video support
        
        // Clock reference requirements
        std::vector<uint32_t> supported_crf_rates;      // Supported CRF rates
        bool supports_multiple_clock_domains;           // Multiple clock domain support
        bool requires_crf_timestamp_validation;         // CRF timestamp validation
    } media_requirements;
    
    // Hardware and platform requirements
    AVTPHardwareRequirements hardware_requirements;
    
    // Automotive-specific configuration
    struct {
        bool disable_avdecc_entity_discovery;  // Disable AVDECC entity discovery 
        bool use_automotive_addressing;        // Use automotive addressing scheme
        bool supports_diagnostic_counters;     // Diagnostic counter support
        bool requires_deterministic_latency;   // Deterministic latency required
        uint32_t max_end_to_end_latency_us;    // Maximum end-to-end latency (us)
        std::vector<std::string> supported_vehicle_networks; // CAN, FlexRay, etc.
    } automotive_config;
    
    // Milan-specific configuration  
    struct {
        bool requires_milan_entity_model;      // Milan AEM model required
        bool supports_milan_redundancy;        // Milan redundancy support
        bool requires_milan_discovery;         // Milan discovery protocol
        bool supports_milan_control_protocol;  // Milan control protocol
        std::string milan_certification_level; // Milan certification level
    } milan_config;
    
    // ProAV-specific configuration
    struct {
        bool supports_professional_audio;      // Professional audio features
        bool requires_low_latency_audio;       // Low latency audio required
        bool supports_dante_interop;           // Dante interoperability
        bool supports_aes67_compatibility;     // AES67 compatibility mode
        uint32_t max_audio_latency_ms;         // Maximum audio latency (ms)
    } proav_config;
    
    // Industrial-specific configuration
    struct {
        bool supports_industrial_protocols;    // Industrial protocol integration
        bool requires_real_time_guarantees;    // Real-time guarantee requirements
        bool supports_safety_functions;        // Safety function support
        uint32_t max_industrial_cycle_time_us; // Maximum cycle time (us)
        std::vector<std::string> supported_industrial_standards; // IEC 61158, etc.
    } industrial_config;
    
    // Performance and compliance limits
    uint32_t max_stream_setup_time_ms;        // Maximum stream setup time (ms)
    uint32_t max_stream_start_time_ms;        // Maximum stream start time (ms)
    uint32_t target_packet_processing_time_ns; // Target packet processing time (ns)
    
    // Runtime statistics (mutable, updated during operation)
    mutable struct {
        uint64_t total_packets_transmitted;    // Total AVTP packets transmitted
        uint64_t total_packets_received;       // Total AVTP packets received
        uint64_t total_stream_connections;     // Total stream connections established
        uint64_t total_format_negotiations;    // Total format negotiations
        uint32_t current_active_streams;       // Currently active stream count
        uint32_t current_packet_rate;          // Current packet rate (packets/second)
        uint64_t last_stream_activity_time;    // Last stream activity timestamp
        bool profile_compliance_validated;     // Profile compliance validated
        std::vector<std::string> compliance_violations; // Compliance violations found
    } stats;
    
    // Constructor with defaults (Standard IEEE 1722-2016 profile)
    AVTPProfile();
    
    // Destructor
    ~AVTPProfile() = default;
    
    // Copy control
    AVTPProfile(const AVTPProfile&) = default;
    AVTPProfile& operator=(const AVTPProfile&) = default;
    AVTPProfile(AVTPProfile&&) = default;
    AVTPProfile& operator=(AVTPProfile&&) = default;
    
    /**
     * @brief Validate if a stream format is supported by this profile
     * @param format AVTP stream format to validate
     * @param is_talker true if checking talker support, false for listener
     * @return true if format is supported
     */
    bool supports_stream_format(AVTPStreamFormat format, bool is_talker) const;
    
    /**
     * @brief Get stream class requirements for a given format
     * @param format AVTP stream format
     * @return Stream class requirements, or nullptr if not supported
     */
    const AVTPStreamClassRequirements* get_stream_class_requirements(AVTPStreamFormat format) const;
    
    /**
     * @brief Validate stream parameters against profile requirements
     * @param format Stream format
     * @param sample_rate Sample rate (for audio)
     * @param channels Channel count (for audio)
     * @param presentation_time_offset Presentation time offset (ns)
     * @return true if parameters are valid for this profile
     */
    bool validate_stream_parameters(AVTPStreamFormat format, uint32_t sample_rate,
                                  uint8_t channels, uint32_t presentation_time_offset) const;
    
    /**
     * @brief Get recommended AVTP packet configuration for this profile
     * @param format Stream format
     * @return Recommended packet configuration parameters
     */
    struct AVTPPacketConfig {
        uint16_t max_packet_size;
        uint8_t packets_per_frame;
        uint32_t packet_interval_ns;
        bool use_presentation_time;
    };
    AVTPPacketConfig get_recommended_packet_config(AVTPStreamFormat format) const;
};

/**
 * @brief Profile factory functions to create specific AVTP profile configurations
 */
namespace AVTPProfileFactory {
    
    /**
     * @brief Create Milan Baseline Interoperability Profile configuration
     * @param version Milan version ("1.0", "2.0a", etc.)
     */
    AVTPProfile createMilanProfile(const std::string& version = "2.0a");
    
    /**
     * @brief Create AVnu Base/ProAV Functional Interoperability Profile configuration
     */
    AVTPProfile createProAVProfile();
    
    /**
     * @brief Create Automotive Ethernet AVB Profile configuration  
     * @param version Automotive version ("1.5", "1.6", etc.)
     */
    AVTPProfile createAutomotiveProfile(const std::string& version = "1.6");
    
    /**
     * @brief Create Industrial Ethernet AVB Profile configuration
     */
    AVTPProfile createIndustrialProfile();
    
    /**
     * @brief Create Standard IEEE 1722-2016 Profile configuration
     */
    AVTPProfile createStandardProfile();
    
    /**
     * @brief Create profile from configuration file
     * @param config_path Path to profile configuration file
     */
    AVTPProfile createProfileFromConfig(const std::string& config_path);
    
    /**
     * @brief Create profile by name and version
     * @param profile_name Profile name ("milan", "automotive", "proav", "industrial", "standard")
     * @param version Profile version string
     */
    AVTPProfile createProfileByName(const std::string& profile_name, const std::string& version = "");
    
    /**
     * @brief Validate AVTP profile configuration
     * @param profile Profile to validate
     * @return true if profile is valid and compliant
     */
    bool validateProfile(const AVTPProfile& profile);
    
    /**
     * @brief Get profile description string with compliance status
     * @param profile Profile to describe
     */
    std::string getProfileDescription(const AVTPProfile& profile);
    
    /**
     * @brief Check compatibility between AVTP and gPTP profiles
     * @param avtp_profile AVTP profile
     * @param gptp_profile_name gPTP profile name
     * @return true if profiles are compatible
     */
    bool validateProfileCompatibility(const AVTPProfile& avtp_profile, const std::string& gptp_profile_name);
    
    /**
     * @brief Get list of all available profile names
     */
    std::vector<std::string> getAvailableProfiles();
}

#endif // AVTP_PROFILE_HPP
