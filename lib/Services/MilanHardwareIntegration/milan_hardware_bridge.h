/**
 * Milan Hardware Integration Bridge
 * 
 * Service Layer bridge that connects Standards layer IEEE providers
 * to Hardware layer (intel-ethernet-hal) while maintaining proper
 * architectural separation.
 * 
 * ARCHITECTURE COMPLIANCE:
 * - Standards Layer: Pure protocol logic (hardware agnostic)
 * - Service Layer: THIS FILE - bridges Standards to Hardware
 * - HAL Layer: intel-ethernet-hal (hardware access only)
 * 
 * CRITICAL: Standards layer NEVER directly includes hardware headers!
 */

#ifndef MILAN_HARDWARE_BRIDGE_H
#define MILAN_HARDWARE_BRIDGE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// Standards Layer (hardware agnostic) - forward declarations
namespace OpenAvnu {
namespace Integration {
namespace Milan_IEEE {
    class IEEE802_1AS_2021_Provider;
    class IEEE1722_1_2021_Provider;
    class IEEE1722_2016_Provider;
}
}
}

// Forward declarations to avoid including hardware headers in this header
// CORRECT CHAIN: intel-ethernet-hal → intel_avb → NDISIntelFilterDriver
struct intel_hal_device;
typedef struct intel_hal_device intel_hal_device_t;

namespace OpenAvnu {
namespace Services {
namespace MilanHardwareIntegration {

/**
 * @brief Hardware abstraction callbacks for Standards layer
 * 
 * These function pointers allow Standards providers to access hardware
 * capabilities without directly depending on hardware libraries.
 */
struct HardwareCapabilities {
    // Timing and synchronization callbacks
    std::function<uint64_t()> get_hardware_timestamp;
    std::function<bool()> is_hardware_synchronized;
    std::function<double()> get_hardware_sync_accuracy;
    
    // Packet transmission callbacks
    std::function<bool(const std::vector<uint8_t>&)> transmit_packet;
    std::function<bool(const std::vector<uint8_t>&, uint64_t)> transmit_packet_with_timestamp;
    
    // Hardware status callbacks
    std::function<bool()> is_hardware_available;
    std::function<std::string()> get_hardware_info;
    std::function<std::vector<std::string>()> enumerate_network_interfaces;
};

/**
 * @brief Intel Hardware Integration Service
 * 
 * Service layer that connects Standards providers to Intel hardware
 * through existing intel-ethernet-hal infrastructure.
 */
class IntelHardwareIntegrationService {
private:
    intel_hal_device_t* intel_hal_device_;
    bool hardware_initialized_;
    std::string active_interface_;
    HardwareCapabilities capabilities_;
    
    // Standards providers (injected)
    std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE802_1AS_2021_Provider> gptp_provider_;
    std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE1722_1_2021_Provider> avdecc_provider_;
    std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE1722_2016_Provider> avtp_provider_;
    
public:
    IntelHardwareIntegrationService();
    ~IntelHardwareIntegrationService();
    
    // Hardware initialization
    bool initialize_intel_hardware();
    bool detect_intel_nics();
    bool select_interface(const std::string& interface_name);
    
    // Standards provider injection
    void inject_gptp_provider(std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE802_1AS_2021_Provider> provider);
    void inject_avdecc_provider(std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE1722_1_2021_Provider> provider);
    void inject_avtp_provider(std::unique_ptr<OpenAvnu::Integration::Milan_IEEE::IEEE1722_2016_Provider> provider);
    
    // Hardware-integrated operations
    bool start_milan_professional_audio_system();
    bool stop_milan_professional_audio_system();
    
    // Real hardware testing
    bool run_hardware_validation_tests();
    
    // Hardware status
    bool is_hardware_ready() const { return hardware_initialized_; }
    std::string get_hardware_status() const;
    
private:
    // Hardware capability implementations
    uint64_t get_intel_hardware_timestamp();
    bool is_intel_hardware_synchronized();
    double get_intel_sync_accuracy();
    bool transmit_intel_packet(const std::vector<uint8_t>& packet);
    bool transmit_intel_packet_with_timestamp(const std::vector<uint8_t>& packet, uint64_t timestamp);
    
    // Hardware setup helpers
    void setup_hardware_capabilities();
    bool configure_intel_ptp();
    bool configure_intel_avb_queues();
};

/**
 * @brief gPTP Hardware Service
 * 
 * Bridges IEEE 802.1AS-2021 Standards provider to Intel hardware timing.
 */
class GPTPHardwareService {
private:
    intel_hal_device_t* intel_hal_device_;
    OpenAvnu::Integration::Milan_IEEE::IEEE802_1AS_2021_Provider* standards_provider_;
    
public:
    explicit GPTPHardwareService(intel_hal_device_t* device);
    
    bool initialize_hardware_sync();
    bool configure_milan_timing_requirements();
    bool start_hardware_synchronization();
    bool validate_milan_timing_accuracy();
    
private:
    void setup_hardware_timestamp_callbacks();
    uint64_t get_ptp_hardware_timestamp();
    bool check_grandmaster_stability();
};

/**
 * @brief AVDECC Hardware Service
 * 
 * Bridges IEEE 1722.1-2021 Standards provider to Intel hardware discovery.
 */
class AVDECCHardwareService {
private:
    intel_hal_device_t* intel_hal_device_;
    OpenAvnu::Integration::Milan_IEEE::IEEE1722_1_2021_Provider* standards_provider_;
    
public:
    explicit AVDECCHardwareService(intel_hal_device_t* device);
    
    bool initialize_hardware_discovery();
    bool start_entity_advertisement();
    bool configure_milan_entity_hardware();
    
private:
    void setup_packet_transmission_callbacks();
    bool transmit_avdecc_packet(const std::vector<uint8_t>& packet);
};

/**
 * @brief AVTP Hardware Service
 * 
 * Bridges IEEE 1722-2016 Standards provider to Intel hardware streaming.
 */
class AVTPHardwareService {
private:
    intel_hal_device_t* intel_hal_device_;
    OpenAvnu::Integration::Milan_IEEE::IEEE1722_2016_Provider* standards_provider_;
    
public:
    explicit AVTPHardwareService(intel_hal_device_t* device);
    
    bool initialize_hardware_streaming();
    bool configure_milan_audio_hardware();
    bool start_professional_audio_streaming();
    bool validate_stream_quality();
    
private:
    void setup_media_streaming_callbacks();
    bool transmit_avtp_packet(const std::vector<uint8_t>& packet, uint64_t presentation_time);
    bool configure_hardware_queues_for_milan();
};

/**
 * @brief Hardware Test Suite
 * 
 * Comprehensive hardware validation tests for Milan professional audio.
 */
class MilanHardwareTestSuite {
private:
    IntelHardwareIntegrationService* integration_service_;
    
public:
    explicit MilanHardwareTestSuite(IntelHardwareIntegrationService* service);
    
    // Hardware detection tests
    bool test_intel_nic_detection();
    bool test_hardware_capabilities();
    
    // Timing tests
    bool test_hardware_timestamping();
    bool test_milan_timing_accuracy();
    bool test_gptp_synchronization();
    
    // Streaming tests
    bool test_avtp_packet_transmission();
    bool test_milan_audio_streaming();
    bool test_stream_redundancy();
    
    // Integration tests
    bool test_end_to_end_hardware_integration();
    
    // Generate comprehensive test report
    std::string generate_hardware_test_report();
};

} // namespace MilanHardwareIntegration
} // namespace Services
} // namespace OpenAvnu

#endif // MILAN_HARDWARE_BRIDGE_H
