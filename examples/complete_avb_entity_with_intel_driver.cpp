/**
 * @file complete_avb_entity_with_intel_driver.cpp
 * @brief Complete AVB Entity Using Your Intel Filter Driver + gPTP + IEEE 1722-2016
 * 
 * This implementation demonstrates proper integration of:
 * - Your gPTP submodule (thirdparty/gptp) for time synchronization
 * - Your Intel AVB filter driver (lib/intel_avb/lib/) for hardware control
 * - Your IEEE 1722-2016 library (lib/Standards/) for AVTP streaming  
 * - Your responsive AVDECC entity for IEEE 1722.1 compliance
 * 
 * ARCHITECTURE:
 * ┌─────────────────────────────────────────────────────────────────┐
 * │                     Complete AVB Entity                         │
 * │                                                                 │
 * │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
 * │  │   IEEE 1722.1   │  │   IEEE 1722     │  │     gPTP        │ │
 * │  │   (AVDECC)      │  │   (AVTP)        │  │  (IEEE 802.1AS) │ │
 * │  │                 │  │                 │  │                 │ │
 * │  │ • Discovery     │  │ • AAF Audio     │  │ • Time Sync     │ │
 * │  │ • Enumeration   │  │ • CVF Video     │  │ • Presentation  │ │
 * │  │ • Control       │  │ • CRF Clock     │  │ • HW Timestamps │ │
 * │  │ • State Mgmt    │  │ • Stream Mgmt   │  │ • Clock Domain  │ │
 * │  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
 * │           │                     │                     │        │
 * │           └─────────────────────┼─────────────────────┘        │
 * │                                 │                              │
 * │  ┌─────────────────────────────────────────────────────────┐   │
 * │  │              Network Layer (Multi-Level)               │   │
 * │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │   │
 * │  │  │    PCAP     │  │ Intel AVB   │  │ Intel HAL   │    │   │
 * │  │  │   Layer 2   │  │Filter Driver│  │ Registers   │    │   │
 * │  │  │             │  │             │  │             │    │   │
 * │  │  │ • Packet    │  │ • HW Queues │  │ • PTP Regs  │    │   │
 * │  │  │   Inject    │  │ • Traffic   │  │ • Timestamp │    │   │
 * │  │  │ • Capture   │  │   Shaping   │  │ • Clock Ctrl│    │   │
 * │  │  └─────────────┘  └─────────────┘  └─────────────┘    │   │
 * │  │         Intel I210/I219/I225/I226 Hardware Platform  │   │
 * │  └─────────────────────────────────────────────────────────┘   │
 * └─────────────────────────────────────────────────────────────────┘
 */

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>

// Include your Intel AVB filter driver
#include "../lib/intel_avb/lib/intel.h"
#include "../lib/intel_avb/lib/intel_windows.h"

// Include PCAP for fallback network access
#include <pcap.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")

// Forward declaration of your AVDECC entity
class ResponsiveAVDECCEntity {
public:
    bool initialize();
    void run();
    void stop();
};

// Forward declarations for gPTP integration
namespace gptp_integration {
    bool initialize_gptp_daemon();
    uint64_t get_gptp_time_ns();
    bool is_gptp_synchronized();
    void shutdown_gptp_daemon();
}

/**
 * @brief Complete AVB Entity with Intel Filter Driver Integration
 * 
 * This class demonstrates how to integrate all three IEEE standards
 * using your Intel AVB filter driver for optimal performance.
 */
class CompleteAVBEntity {
private:
    // Component instances
    std::unique_ptr<ResponsiveAVDECCEntity> avdecc_entity_;
    device_t intel_device_;  // Your Intel AVB device handle
    
    // Network interface fallback
    pcap_t* pcap_handle_;
    std::string interface_name_;
    
    // Thread management
    std::atomic<bool> running_;
    std::atomic<bool> streaming_active_;
    std::thread avdecc_thread_;
    std::thread streaming_thread_;
    std::thread gptp_monitor_thread_;
    
    // Stream configuration
    struct {
        uint32_t sample_rate = 48000;   // 48kHz
        uint16_t channels = 8;          // 8-channel audio
        uint16_t bit_depth = 24;        // 24-bit
        uint64_t stream_id = 0xc047e0fffe167b89ULL;  // Consistent with AVDECC entity
        uint16_t samples_per_frame = 6; // 6 samples per AVTP frame (125μs @ 48kHz)
    } stream_config_;
    
    // gPTP synchronization state
    struct {
        bool synchronized = false;
        uint64_t master_time_ns = 0;
        int64_t offset_ns = 0;
        uint32_t presentation_offset_us = 2000; // 2ms presentation time
    } gptp_state_;
    
public:
    CompleteAVBEntity() : 
        avdecc_entity_(nullptr),
        pcap_handle_(nullptr),
        running_(false),
        streaming_active_(false) {
        
        // Initialize Intel device structure
        std::memset(&intel_device_, 0, sizeof(intel_device_));
        intel_device_.pci_vendor_id = INTEL_VENDOR_ID;
    }
    
    ~CompleteAVBEntity() {
        shutdown();
    }
    
    /**
     * @brief Initialize all AVB components
     */
    bool initialize() {
        std::cout << "🚀 Initializing Complete AVB Entity..." << std::endl;
        
        // 1. Initialize Intel AVB filter driver
        if (!initialize_intel_driver()) {
            std::cerr << "❌ Failed to initialize Intel AVB filter driver" << std::endl;
            return false;
        }
        
        // 2. Initialize gPTP daemon
        if (!gptp_integration::initialize_gptp_daemon()) {
            std::cerr << "❌ Failed to initialize gPTP daemon" << std::endl;
            return false;
        }
        
        // 3. Initialize AVDECC entity
        avdecc_entity_ = std::make_unique<ResponsiveAVDECCEntity>();
        if (!avdecc_entity_->initialize()) {
            std::cerr << "❌ Failed to initialize AVDECC entity" << std::endl;
            return false;
        }
        
        // 4. Initialize network interface (fallback)
        if (!initialize_network_interface()) {
            std::cerr << "❌ Failed to initialize network interface" << std::endl;
            return false;
        }
        
        std::cout << "✅ Complete AVB Entity initialization successful!" << std::endl;
        return true;
    }
    
    /**
     * @brief Start all AVB services
     */
    void start() {
        running_ = true;
        
        std::cout << "🎯 Starting Complete AVB Entity services..." << std::endl;
        
        // Start AVDECC discovery and control
        avdecc_thread_ = std::thread([this]() {
            avdecc_entity_->run();
        });
        
        // Start gPTP monitoring
        gptp_monitor_thread_ = std::thread([this]() {
            monitor_gptp_synchronization();
        });
        
        // Start streaming capability (when requested)
        streaming_thread_ = std::thread([this]() {
            streaming_service();
        });
        
        std::cout << "✅ All AVB services started!" << std::endl;
        
        // Main loop
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Monitor overall system health
            if (running_.load()) {
                check_system_health();
            }
        }
    }
    
    /**
     * @brief Stop all services
     */
    void shutdown() {
        std::cout << "🛑 Shutting down Complete AVB Entity..." << std::endl;
        
        running_ = false;
        streaming_active_ = false;
        
        // Stop threads
        if (avdecc_thread_.joinable()) {
            avdecc_entity_->stop();
            avdecc_thread_.join();
        }
        
        if (gptp_monitor_thread_.joinable()) {
            gptp_monitor_thread_.join();
        }
        
        if (streaming_thread_.joinable()) {
            streaming_thread_.join();
        }
        
        // Cleanup components
        gptp_integration::shutdown_gptp_daemon();
        cleanup_intel_driver();
        cleanup_network_interface();
        
        std::cout << "✅ Complete AVB Entity shutdown complete!" << std::endl;
    }

private:
    /**
     * @brief Initialize your Intel AVB filter driver
     */
    bool initialize_intel_driver() {
        std::cout << "🔧 Initializing Intel AVB filter driver..." << std::endl;
        
        // Detect Intel hardware (prioritize I219, then I225, I210)
        const uint16_t intel_devices[] = {
            0x0dc7, // I219-LM (your specific device)
            0x15f2, // I225-LM  
            0x1533, // I210
            0x15b7, // I219-V
            0x15f3  // I225-V
        };
        
        bool device_found = false;
        for (uint16_t device_id : intel_devices) {
            intel_device_.pci_device_id = device_id;
            
            int result = intel_probe(&intel_device_);
            if (result == 0) {
                std::cout << "✅ Found Intel device: " << intel_get_device_name(&intel_device_) << std::endl;
                std::cout << "   Device ID: 0x" << std::hex << device_id << std::dec << std::endl;
                std::cout << "   Capabilities: 0x" << std::hex << intel_get_capabilities(&intel_device_) << std::dec << std::endl;
                device_found = true;
                break;
            }
        }
        
        if (!device_found) {
            std::cerr << "❌ No supported Intel Ethernet device found" << std::endl;
            return false;
        }
        
        // Initialize common Intel HAL
        int result = intel_common_init(&intel_device_);
        if (result != 0) {
            std::cerr << "❌ Failed to initialize Intel common HAL: " << result << std::endl;
            return false;
        }
        
        // Initialize device-specific features
        result = intel_init(&intel_device_);
        if (result != 0) {
            std::cerr << "❌ Failed to initialize Intel device: " << result << std::endl;
            return false;
        }
        
        // Check for IEEE 1588 PTP capabilities
        if (intel_has_capability(&intel_device_, INTEL_CAP_BASIC_1588)) {
            std::cout << "✅ IEEE 1588 PTP timestamping available" << std::endl;
        } else {
            std::cout << "⚠️  IEEE 1588 PTP timestamping not available" << std::endl;
        }
        
        // Check for TSN capabilities (I225/I226 only)
        if (intel_has_capability(&intel_device_, INTEL_CAP_TSN_TAS)) {
            std::cout << "✅ TSN Time Aware Shaper (TAS) available" << std::endl;
        }
        
        if (intel_has_capability(&intel_device_, INTEL_CAP_TSN_FP)) {
            std::cout << "✅ TSN Frame Preemption available" << std::endl;
        }
        
        std::cout << "✅ Intel AVB filter driver initialized successfully!" << std::endl;
        return true;
    }
    
    /**
     * @brief Initialize network interface (PCAP fallback)
     */
    bool initialize_network_interface() {
        std::cout << "🌐 Initializing network interface fallback..." << std::endl;
        
        pcap_if_t* alldevs;
        char errbuf[PCAP_ERRBUF_SIZE];
        
        // Find all network devices
        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            std::cerr << "❌ Error finding devices: " << errbuf << std::endl;
            return false;
        }
        
        // Find Intel Ethernet interface
        pcap_if_t* selected_device = nullptr;
        
        // Priority 1: Intel I219
        for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
            if (d->description && 
                strstr(d->description, "Intel") &&
                strstr(d->description, "I219") &&
                strstr(d->description, "Ethernet")) {
                selected_device = d;
                interface_name_ = d->name;
                std::cout << "✅ Selected Intel I219 interface: " << d->name << std::endl;
                break;
            }
        }
        
        // Priority 2: Any Intel Ethernet
        if (!selected_device) {
            for (pcap_if_t* d = alldevs; d != nullptr; d = d->next) {
                if (d->description && 
                    strstr(d->description, "Intel") &&
                    strstr(d->description, "Ethernet")) {
                    selected_device = d;
                    interface_name_ = d->name;
                    std::cout << "✅ Selected Intel Ethernet interface: " << d->name << std::endl;
                    break;
                }
            }
        }
        
        if (!selected_device) {
            std::cerr << "❌ No Intel Ethernet interface found" << std::endl;
            pcap_freealldevs(alldevs);
            return false;
        }
        
        // Open interface for packet capture/injection
        pcap_handle_ = pcap_open_live(interface_name_.c_str(), 65536, 1, 1000, errbuf);
        pcap_freealldevs(alldevs);
        
        if (!pcap_handle_) {
            std::cerr << "❌ Failed to open interface: " << errbuf << std::endl;
            return false;
        }
        
        std::cout << "✅ Network interface fallback initialized!" << std::endl;
        return true;
    }
    
    /**
     * @brief Monitor gPTP synchronization status
     */
    void monitor_gptp_synchronization() {
        std::cout << "📡 Starting gPTP synchronization monitoring..." << std::endl;
        
        while (running_) {
            // Check gPTP synchronization status
            bool was_synchronized = gptp_state_.synchronized;
            gptp_state_.synchronized = gptp_integration::is_gptp_synchronized();
            
            if (gptp_state_.synchronized) {
                gptp_state_.master_time_ns = gptp_integration::get_gptp_time_ns();
                
                if (!was_synchronized) {
                    std::cout << "✅ gPTP synchronization achieved!" << std::endl;
                    std::cout << "   Master time: " << gptp_state_.master_time_ns << " ns" << std::endl;
                }
            } else if (was_synchronized) {
                std::cout << "⚠️  gPTP synchronization lost!" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    /**
     * @brief Streaming service (activated by AVDECC commands)
     */
    void streaming_service() {
        std::cout << "🎵 Streaming service ready..." << std::endl;
        
        while (running_) {
            if (streaming_active_.load() && gptp_state_.synchronized) {
                // Perform actual AVTP streaming using Intel driver
                send_avtp_audio_frame();
                
                // Stream at 8000 Hz (125μs intervals)
                std::this_thread::sleep_for(std::chrono::microseconds(125));
            } else {
                // Wait for streaming activation
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    
    /**
     * @brief Send AVTP audio frame using Intel driver
     */
    void send_avtp_audio_frame() {
        // This would use your Intel driver for optimal performance
        // For now, this is a placeholder showing the integration approach
        
        // 1. Get hardware timestamp from Intel device
        uint64_t hw_timestamp = 0;
        if (intel_has_capability(&intel_device_, INTEL_CAP_BASIC_1588)) {
            // Read hardware timestamp register
            uint32_t ts_low, ts_high;
            if (intel_read_reg(&intel_device_, 0x0B618, &ts_low) == 0 &&
                intel_read_reg(&intel_device_, 0x0B61C, &ts_high) == 0) {
                hw_timestamp = ((uint64_t)ts_high << 32) | ts_low;
            }
        }
        
        // 2. Calculate presentation time
        uint64_t presentation_time = gptp_state_.master_time_ns + 
                                   (gptp_state_.presentation_offset_us * 1000);
        
        // 3. Prepare AVTP packet (simplified)
        struct avtp_audio_packet {
            // Ethernet header
            uint8_t dest_mac[6] = {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00}; // AVB multicast
            uint8_t src_mac[6] = {0xc0, 0x47, 0xe0, 0x16, 0x7b, 0x89};   // Entity ID based
            uint16_t ethertype = 0x22f0; // IEEE 1722
            
            // AVTP header
            uint8_t subtype = 0x02;      // AAF
            uint8_t sv_version = 0x00;   // sv=0, version=0
            uint8_t mr_tv_seq = 0x80;    // mr=1, tv=0, seq=0 (will be updated)
            uint8_t reserved = 0x00;
            uint64_t stream_id = stream_config_.stream_id;
            uint32_t avtp_timestamp = (uint32_t)(presentation_time & 0xFFFFFFFF);
            uint32_t format_info = (stream_config_.channels << 8) | stream_config_.bit_depth;
            uint16_t packet_info = stream_config_.samples_per_frame;
            
            // Audio data (placeholder)
            uint8_t audio_data[144]; // 6 samples * 8 channels * 3 bytes (24-bit)
        } packet;
        
        // 4. Send packet using Intel driver (high performance path)
        struct intel_packet intel_pkt;
        intel_pkt.data = (uint8_t*)&packet;
        intel_pkt.len = sizeof(packet);
        intel_pkt.timestamp = hw_timestamp;
        
        if (intel_xmit(&intel_device_, 0, &intel_pkt) == 0) {
            // Success - packet sent via Intel driver
        } else {
            // Fallback to PCAP injection
            if (pcap_handle_) {
                pcap_inject(pcap_handle_, &packet, sizeof(packet));
            }
        }
    }
    
    /**
     * @brief Check overall system health
     */
    void check_system_health() {
        static int health_check_counter = 0;
        
        if (++health_check_counter % 50 == 0) { // Every 5 seconds
            std::cout << "💊 System Health Check:" << std::endl;
            std::cout << "   gPTP Sync: " << (gptp_state_.synchronized ? "✅" : "❌") << std::endl;
            std::cout << "   Streaming: " << (streaming_active_.load() ? "🎵" : "⏸️") << std::endl;
            std::cout << "   Intel Driver: " << (intel_device_.device_type != 0 ? "✅" : "❌") << std::endl;
        }
    }
    
    /**
     * @brief Cleanup Intel driver
     */
    void cleanup_intel_driver() {
        if (intel_device_.device_type != 0) {
            intel_detach(&intel_device_);
            intel_common_cleanup(&intel_device_);
        }
    }
    
    /**
     * @brief Cleanup network interface
     */
    void cleanup_network_interface() {
        if (pcap_handle_) {
            pcap_close(pcap_handle_);
            pcap_handle_ = nullptr;
        }
    }

public:
    /**
     * @brief Start/Stop streaming (called by AVDECC commands)
     */
    void start_streaming() {
        if (gptp_state_.synchronized) {
            streaming_active_ = true;
            std::cout << "🎵 Audio streaming started!" << std::endl;
        } else {
            std::cout << "⚠️  Cannot start streaming - gPTP not synchronized" << std::endl;
        }
    }
    
    void stop_streaming() {
        streaming_active_ = false;
        std::cout << "⏹️  Audio streaming stopped!" << std::endl;
    }
    
    bool is_streaming() const {
        return streaming_active_.load();
    }
    
    bool is_synchronized() const {
        return gptp_state_.synchronized;
    }
};

/**
 * @brief Application entry point
 */
int main() {
    std::cout << "🎯 Complete AVB Entity with Intel Filter Driver Integration" << std::endl;
    std::cout << "=============================================================" << std::endl;
    std::cout << "Architecture:" << std::endl;
    std::cout << "  📡 IEEE 1722.1 (AVDECC) - Discovery & Control" << std::endl;
    std::cout << "  🎵 IEEE 1722 (AVTP) - Audio/Video Streaming" << std::endl;  
    std::cout << "  ⏰ IEEE 802.1AS (gPTP) - Time Synchronization" << std::endl;
    std::cout << "  🔧 Intel AVB Filter Driver - Hardware Control" << std::endl;
    std::cout << std::endl;
    
    CompleteAVBEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "❌ Failed to initialize Complete AVB Entity" << std::endl;
        return 1;
    }
    
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;
    
    try {
        entity.start();
    } catch (const std::exception& e) {
        std::cerr << "💥 Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
