/**
 * @file intel_hal_timestamp_interface.h
 * @brief Intel HAL Hardware Timestamp Interface Implementation
 * @details Intel hardware-specific timestamp interface with HAL integration
 * 
 * This file provides Intel HAL specific implementation of the generic
 * HardwareTimestampInterface for use with IEEE 802.1AS-2021 standards.
 * 
 * @author OpenAvnu Hardware Integration Team
 * @date 2025
 * @copyright Intel HAL Integration - Hardware-specific implementation
 */

#ifndef INTEL_HAL_TIMESTAMP_INTERFACE_H
#define INTEL_HAL_TIMESTAMP_INTERFACE_H

#include "../../../lib/Standards/IEEE/802.1AS/2021/core/time_sync_engine.h"
#include <memory>
#include <string>

namespace OpenAvnu {
namespace HAL {
namespace Intel {

/**
 * @brief Intel HAL Timestamp Interface Implementation
 * @details Intel hardware-specific timestamp interface with HAL integration
 * 
 * This class provides Intel NIC specific implementation of the abstract
 * HardwareTimestampInterface defined in IEEE 802.1AS-2021 standards.
 * It uses Intel Ethernet HAL for direct hardware access.
 */
class IntelHALTimestampInterface : public IEEE::_802_1AS::_2021::HardwareTimestampInterface {
public:
    /**
     * @brief Constructor
     * @param interface_name Network interface name (e.g., "eth0", "Intel(R) Ethernet Controller I219-V")
     */
    explicit IntelHALTimestampInterface(const std::string& interface_name);
    
    /**
     * @brief Destructor
     */
    virtual ~IntelHALTimestampInterface();

    // Hardware timestamp capture - IEEE 802.1AS-2021 interface implementation
    bool capture_tx_timestamp(uint16_t sequence_id, TimestampCapture& capture) override;
    bool capture_rx_timestamp(uint16_t sequence_id, TimestampCapture& capture) override;
    
    // Hardware clock adjustment - IEEE 802.1AS-2021 interface implementation
    bool adjust_clock_frequency(int32_t ppb_adjustment) override;
    bool adjust_clock_phase(IEEE::_802_1AS::_2021::TimeInterval phase_adjustment) override;
    bool set_clock_time(const IEEE::_802_1AS::_2021::Timestamp& time) override;
    bool get_clock_time(IEEE::_802_1AS::_2021::Timestamp& time) override;
    
    // Hardware capabilities - IEEE 802.1AS-2021 interface implementation
    bool supports_one_step() const override;
    bool supports_two_step() const override;
    IEEE::_802_1AS::_2021::TimeInterval get_timestamp_accuracy() const override;

    // Intel HAL specific methods
    bool initialize_hal();
    bool shutdown_hal();
    bool is_hal_initialized() const;
    
    // Intel specific diagnostics
    bool get_hardware_info(std::string& device_info) const;
    bool is_intel_nic_supported() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pImpl;
};

} // namespace Intel
} // namespace HAL
} // namespace OpenAvnu

#endif // INTEL_HAL_TIMESTAMP_INTERFACE_H
