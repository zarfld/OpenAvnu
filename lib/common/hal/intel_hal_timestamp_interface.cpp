/**
 * @file intel_hal_timestamp_interface.cpp
 * @brief Intel HAL Hardware Timestamp Interface Implementation
 * @details Intel hardware-specific timestamp interface with HAL integration
 * 
 * @author OpenAvnu Hardware Integration Team
 * @date 2025
 * @copyright Intel HAL Integration - Hardware-specific implementation
 */

#include "intel_hal_timestamp_interface.h"
#include <chrono>
#include <string>

#ifdef OPENAVNU_BUILD_INTEL_HAL
#include "../../../thirdparty/intel-ethernet-hal/include/intel_ethernet_hal.h"
#endif

namespace OpenAvnu {
namespace HAL {
namespace Intel {

/**
 * @brief Intel HAL Timestamp Interface Implementation
 * @details Internal implementation class for IntelHALTimestampInterface
 */
class IntelHALTimestampInterface::Implementation {
public:
    Implementation(const std::string& interface_name)
        : interface_name_(interface_name)
        , hal_initialized_(false)
        , device_handle_(nullptr) {}

    ~Implementation() {
        shutdown_hal();
    }

    bool initialize_hal() {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (hal_initialized_) return true;

        // Initialize Intel HAL for the specific interface
        device_handle_ = intel_hal_open_device(interface_name_.c_str());
        if (!device_handle_) {
            return false;
        }

        // Configure hardware timestamping
        intel_hal_config_t config = {};
        config.enable_tx_timestamping = true;
        config.enable_rx_timestamping = true;
        config.timestamp_accuracy = INTEL_HAL_TIMESTAMP_ACCURACY_1NS;
        
        if (intel_hal_configure_device(device_handle_, &config) != INTEL_HAL_SUCCESS) {
            intel_hal_close_device(device_handle_);
            device_handle_ = nullptr;
            return false;
        }

        hal_initialized_ = true;
        return true;
#else
        // Fallback implementation for systems without Intel HAL
        hal_initialized_ = true;
        return true;
#endif
    }

    bool shutdown_hal() {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_) return true;

        if (device_handle_) {
            intel_hal_close_device(device_handle_);
            device_handle_ = nullptr;
        }

        hal_initialized_ = false;
        return true;
#else
        hal_initialized_ = false;
        return true;
#endif
    }

    bool capture_tx_timestamp(uint16_t sequence_id, IEEE::_802_1AS::_2021::HardwareTimestampInterface::TimestampCapture& capture) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;

        intel_hal_timestamp_t hw_timestamp = {};
        if (intel_hal_get_tx_timestamp(device_handle_, sequence_id, &hw_timestamp) == INTEL_HAL_SUCCESS) {
            // Convert Intel HAL timestamp to IEEE 802.1AS timestamp
            capture.timestamp.seconds_field = hw_timestamp.seconds;
            capture.timestamp.nanoseconds_field = hw_timestamp.nanoseconds;
            capture.sequence_id = sequence_id;
            capture.valid = true;
            capture.capture_time = std::chrono::steady_clock::now();
            return true;
        }
        return false;
#else
        // Simulate timestamp for testing without hardware
        auto now = std::chrono::steady_clock::now();
        auto epoch = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(epoch - seconds);

        capture.timestamp.seconds_field = static_cast<uint64_t>(seconds.count());
        capture.timestamp.nanoseconds_field = static_cast<uint32_t>(nanoseconds.count());
        capture.sequence_id = sequence_id;
        capture.valid = true;
        capture.capture_time = now;
        return true;
#endif
    }

    bool capture_rx_timestamp(uint16_t sequence_id, IEEE::_802_1AS::_2021::HardwareTimestampInterface::TimestampCapture& capture) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;

        intel_hal_timestamp_t hw_timestamp = {};
        if (intel_hal_get_rx_timestamp(device_handle_, sequence_id, &hw_timestamp) == INTEL_HAL_SUCCESS) {
            capture.timestamp.seconds_field = hw_timestamp.seconds;
            capture.timestamp.nanoseconds_field = hw_timestamp.nanoseconds;
            capture.sequence_id = sequence_id;
            capture.valid = true;
            capture.capture_time = std::chrono::steady_clock::now();
            return true;
        }
        return false;
#else
        // Simulate timestamp for testing
        return capture_tx_timestamp(sequence_id, capture);
#endif
    }

    bool adjust_clock_frequency(int32_t ppb_adjustment) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;
        return intel_hal_adjust_clock_frequency(device_handle_, ppb_adjustment) == INTEL_HAL_SUCCESS;
#else
        // Simulate successful adjustment
        simulated_frequency_adjustment_ = ppb_adjustment;
        return true;
#endif
    }

    bool adjust_clock_phase(IEEE::_802_1AS::_2021::TimeInterval phase_adjustment) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;
        return intel_hal_adjust_clock_phase(device_handle_, phase_adjustment) == INTEL_HAL_SUCCESS;
#else
        // Simulate successful adjustment
        simulated_phase_adjustment_ = phase_adjustment;
        return true;
#endif
    }

    bool set_clock_time(const IEEE::_802_1AS::_2021::Timestamp& time) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;
        intel_hal_timestamp_t hw_time = {time.seconds_field, time.nanoseconds_field};
        return intel_hal_set_clock_time(device_handle_, &hw_time) == INTEL_HAL_SUCCESS;
#else
        simulated_clock_time_ = time;
        return true;
#endif
    }

    bool get_clock_time(IEEE::_802_1AS::_2021::Timestamp& time) {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) return false;
        intel_hal_timestamp_t hw_time = {};
        if (intel_hal_get_clock_time(device_handle_, &hw_time) == INTEL_HAL_SUCCESS) {
            time.seconds_field = hw_time.seconds;
            time.nanoseconds_field = hw_time.nanoseconds;
            return true;
        }
        return false;
#else
        time = simulated_clock_time_;
        return true;
#endif
    }

    bool supports_one_step() const {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        return true; // Intel NICs typically support one-step
#else
        return false;
#endif
    }

    bool supports_two_step() const {
        return true; // Always supported
    }

    IEEE::_802_1AS::_2021::TimeInterval get_timestamp_accuracy() const {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        return 1; // 1ns accuracy for Intel hardware
#else
        return 1000; // 1µs simulated accuracy
#endif
    }

    bool is_hal_initialized() const {
        return hal_initialized_;
    }

    bool get_hardware_info(std::string& device_info) const {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!hal_initialized_ || !device_handle_) {
            device_info = "Intel HAL not initialized";
            return false;
        }
        
        // Get Intel device information
        intel_hal_device_info_t info = {};
        if (intel_hal_get_device_info(device_handle_, &info) == INTEL_HAL_SUCCESS) {
            device_info = "Intel NIC: " + std::string(info.device_name) + 
                         " (PCI: " + std::string(info.pci_id) + ")";
            return true;
        }
#endif
        device_info = "Intel HAL not available or device not found";
        return false;
    }

    bool is_intel_nic_supported() const {
#ifdef OPENAVNU_BUILD_INTEL_HAL
        return hal_initialized_ && device_handle_ != nullptr;
#else
        return false;
#endif
    }

private:
    std::string interface_name_;
    bool hal_initialized_;
    void* device_handle_;
    
    // Simulation variables for non-HAL builds
    int32_t simulated_frequency_adjustment_{0};
    IEEE::_802_1AS::_2021::TimeInterval simulated_phase_adjustment_{0};
    IEEE::_802_1AS::_2021::Timestamp simulated_clock_time_{0, 0};
};

// IntelHALTimestampInterface Implementation
IntelHALTimestampInterface::IntelHALTimestampInterface(const std::string& interface_name)
    : pImpl(std::make_unique<Implementation>(interface_name)) {}

IntelHALTimestampInterface::~IntelHALTimestampInterface() = default;

bool IntelHALTimestampInterface::capture_tx_timestamp(uint16_t sequence_id, TimestampCapture& capture) {
    return pImpl->capture_tx_timestamp(sequence_id, capture);
}

bool IntelHALTimestampInterface::capture_rx_timestamp(uint16_t sequence_id, TimestampCapture& capture) {
    return pImpl->capture_rx_timestamp(sequence_id, capture);
}

bool IntelHALTimestampInterface::adjust_clock_frequency(int32_t ppb_adjustment) {
    return pImpl->adjust_clock_frequency(ppb_adjustment);
}

bool IntelHALTimestampInterface::adjust_clock_phase(IEEE::_802_1AS::_2021::TimeInterval phase_adjustment) {
    return pImpl->adjust_clock_phase(phase_adjustment);
}

bool IntelHALTimestampInterface::set_clock_time(const IEEE::_802_1AS::_2021::Timestamp& time) {
    return pImpl->set_clock_time(time);
}

bool IntelHALTimestampInterface::get_clock_time(IEEE::_802_1AS::_2021::Timestamp& time) {
    return pImpl->get_clock_time(time);
}

bool IntelHALTimestampInterface::supports_one_step() const {
    return pImpl->supports_one_step();
}

bool IntelHALTimestampInterface::supports_two_step() const {
    return pImpl->supports_two_step();
}

IEEE::_802_1AS::_2021::TimeInterval IntelHALTimestampInterface::get_timestamp_accuracy() const {
    return pImpl->get_timestamp_accuracy();
}

bool IntelHALTimestampInterface::initialize_hal() {
    return pImpl->initialize_hal();
}

bool IntelHALTimestampInterface::shutdown_hal() {
    return pImpl->shutdown_hal();
}

bool IntelHALTimestampInterface::is_hal_initialized() const {
    return pImpl->is_hal_initialized();
}

bool IntelHALTimestampInterface::get_hardware_info(std::string& device_info) const {
    return pImpl->get_hardware_info(device_info);
}

bool IntelHALTimestampInterface::is_intel_nic_supported() const {
    return pImpl->is_intel_nic_supported();
}

} // namespace Intel
} // namespace HAL
} // namespace OpenAvnu
