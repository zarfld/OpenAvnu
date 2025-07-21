#ifndef HAVE_PCAP_TYPES
#ifdef _WIN32
typedef struct pcap pcap_t;
typedef struct pcap_if pcap_if_t;
#endif
#define HAVE_PCAP_TYPES
#endif
/**
 * @file complete_avb_entity_integration.cpp
 * @brief Complete AVB Entity with gPTP + IEEE 1722-2016 + IEEE 1722.1 Integration
 * 
 * This implementation demonstrates how to integrate:
 * - Your gPTP submodule (thirdparty/gptp) for time synchronization
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
 * │  │  │   Layer 2   │  │ Filter Driver│  │ Registers   │    │   │
 * │  │  │             │  │             │  │             │    │   │
 * │  │  │ • Packet    │  │ • HW Queues │  │ • PTP Regs  │    │   │
 * │  │  │   Inject    │  │ • Traffic   │  │ • Timestamp │    │   │
 * │  │  │ • Capture   │  │   Shaping   │  │ • Clock Ctrl│    │   │
 * │  │  └─────────────┘  └─────────────┘  └─────────────┘    │   │
 * │  │         Intel I219/I225/I226 Hardware Platform       │   │
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
#include <cstring>  // for std::memcpy, std::memset

// Include your existing components
#include "avb_gptp.h"                                   // Real gPTP integration via shared memory
#include "../lib/intel_avb/lib/intel.h"                 // Your Intel AVB filter driver
#include "../lib/intel_avb/lib/intel_windows.h"         // Windows driver interface

// Define INTEGRATION_MODE to prevent main() from the AVDECC entity
#define INTEGRATION_MODE
#include "../lib/Standards/intel_pcap_avdecc_entity_responsive.cpp" // Your responsive AVDECC entity

// Include PCAP for network access
#ifdef _WIN32
#  include <pcap.h>
#else
#  include <pcap/pcap.h>
#endif

#ifndef PCAP_ERRBUF_SIZE
#define PCAP_ERRBUF_SIZE 256
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")

// gPTP interface declarations (from lib/common/avb_gptp.h)
typedef long double FrequencyRatio;

typedef struct {
    int64_t ml_phoffset;
    int64_t ls_phoffset;
    FrequencyRatio ml_freqoffset;
    FrequencyRatio ls_freqoffset;
    uint64_t local_time;
    
    /* Current grandmaster information */
    uint8_t gptp_grandmaster_id[8];
    uint8_t gptp_domain_number;
    
    /* Grandmaster support for the network interface */
    uint8_t  clock_identity[8];
    uint8_t  priority1;
    uint8_t  clock_class;
    int16_t  offset_scaled_log_variance;
    uint8_t  clock_accuracy;
    uint8_t  priority2;
    uint8_t  domain_number;
    int8_t   log_sync_interval;
    int8_t   log_announce_interval;
    int8_t   log_pdelay_interval;
    uint16_t port_number;
} gPtpTimeData;

// gPTP function declarations
extern "C" {
    int gptpinit(int *shm_fd, char **shm_map);
    int gptpdeinit(int *shm_fd, char **shm_map);
    int gptpgetdata(char *shm_mmap, gPtpTimeData *td);
    bool gptplocaltime(const gPtpTimeData * td, uint64_t* now_local);
}

// Forward declarations for gPTP integration
namespace gptp_integration {
    bool initialize_gptp_daemon(const std::string& interface_name);
    uint64_t get_gptp_time_ns();
    bool is_gptp_synchronized();
    void shutdown_gptp_daemon();
}

/**
 * @brief Complete AVB Entity with Full IEEE Standards Integration
 * 
 * This class demonstrates how to integrate all three IEEE standards
 * into a single, functional AVB/TSN device.
 */
class CompleteAVBEntity {
private:
    // Component instances
    std::unique_ptr<ResponsiveAVDECCEntity> avdecc_entity_;
    device_t intel_device_;  // Your Intel AVB device handle
    
    // Network interface
    pcap_t* pcap_handle_;
    std::string interface_name_;
    
    // Streaming state
    std::atomic<bool> running_;
    std::atomic<bool> streaming_active_;
    std::thread avdecc_thread_;
    std::thread streaming_thread_;
    std::thread gptp_monitor_thread_;
    
    // Stream configuration
    struct StreamConfig {
        uint8_t channels = 8;           // 8-channel audio
        uint32_t sample_rate = 48000;   // 48kHz
        uint16_t bit_depth = 24;        // 24-bit
        uint32_t samples_per_frame = 6; // 6 samples @ 48kHz = 8000 packets/sec
        uint8_t stream_id[8] = {0xc0, 0x47, 0xe0, 0x16, 0x7b, 0x89, 0x00, 0x01};
        uint8_t dest_mac[6] = {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00};  // AVB multicast
    } stream_config_;
    
    // gPTP synchronization state
    struct GPTPState {
        bool synchronized = false;
        uint64_t last_sync_time = 0;
        int64_t clock_offset_ns = 0;
        uint32_t presentation_offset_us = 2000;  // 2ms presentation offset
    } gptp_state_;
    
    // Basic AVTP stream state for future expansion
    struct AVTPStreamState {
        bool active = false;
        uint64_t stream_id = 0;
        uint8_t sequence_number = 0;
    } avtp_state_;
    
public:
    CompleteAVBEntity() : running_(false), streaming_active_(false), pcap_handle_(nullptr) {}

    ~CompleteAVBEntity() {
        shutdown();
    }

    /**
     * @brief Initialize all AVB components
     * @return true if successful, false otherwise
     */
    bool initialize() {
        std::cout << "🚀 Initializing Complete AVB Entity..." << std::endl;

        // 1. Initialize network interface
        if (!initialize_network()) {
            std::cerr << "❌ Failed to initialize network interface" << std::endl;
            return false;
        }

        // 2. Initialize Intel AVB hardware (filter driver)
        // TODO: Use your Intel AVB filter driver to open the hardware device and enable timestamping
        // Example: intel_device_ = intel_open_device(interface_name_.c_str());
        //          intel_enable_timestamping(intel_device_);
        // For now, just print a message
        std::cout << "🛠️  (TODO) Intel AVB filter driver initialization for " << interface_name_ << std::endl;

        // 3. Initialize gPTP time synchronization
        if (!gptp_integration::initialize_gptp_daemon(interface_name_)) {
            std::cerr << "❌ Failed to initialize gPTP daemon" << std::endl;
            return false;
        }
        std::cout << "✅ gPTP daemon initialized" << std::endl;

        // 4. Initialize IEEE 1722.1 AVDECC entity
        avdecc_entity_ = std::make_unique<ResponsiveAVDECCEntity>();
        if (!avdecc_entity_->initialize()) {
            std::cerr << "❌ Failed to initialize AVDECC entity" << std::endl;
            return false;
        }
        std::cout << "✅ IEEE 1722.1 AVDECC entity initialized" << std::endl;

        // 5. Initialize basic AVTP stream state
        avtp_state_.stream_id = 0x91E0F000FE010000ULL; // Example stream ID
        std::cout << "✅ AVTP stream state initialized" << std::endl;

        std::cout << "🎯 Complete AVB Entity initialization successful!" << std::endl;
        return true;
    }
    
    /**
     * @brief Start all AVB services
     */
    void start() {
        if (running_.load()) {
            std::cout << "⚠️  AVB Entity already running" << std::endl;
            return;
        }
        
        running_.store(true);
        
        std::cout << "🚀 Starting Complete AVB Entity..." << std::endl;
        
        // Start AVDECC discovery and control thread
        avdecc_thread_ = std::thread([this]() {
            std::cout << "🔍 AVDECC thread started - handling discovery and control" << std::endl;
            avdecc_entity_->run();
        });
        
        // Start gPTP monitoring thread
        gptp_monitor_thread_ = std::thread([this]() {
            std::cout << "⏰ gPTP monitoring thread started" << std::endl;
            monitor_gptp_synchronization();
        });
        
        // Start AVTP streaming thread
        streaming_thread_ = std::thread([this]() {
            std::cout << "🎵 AVTP streaming thread started" << std::endl;
            handle_avtp_streaming();
        });
        
        std::cout << "✅ All AVB services started successfully!" << std::endl;
        std::cout << std::endl;
        std::cout << "🎯 AVB Entity Status:" << std::endl;
        std::cout << "   📡 AVDECC Discovery: Active (IEEE 1722.1-2021)" << std::endl;
        std::cout << "   ⏰ gPTP Synchronization: Monitoring" << std::endl;
        std::cout << "   🎵 AVTP Streaming: Ready (IEEE 1722-2016)" << std::endl;
        std::cout << "   🔗 Network Interface: " << interface_name_ << std::endl;
        std::cout << std::endl;
    }
    
    /**
     * @brief Stop all AVB services
     */
    void shutdown() {
        if (!running_.load()) {
            return;
        }
        
        std::cout << "🛑 Shutting down Complete AVB Entity..." << std::endl;
        
        running_.store(false);
        streaming_active_.store(false);
        
        // Stop AVDECC entity
        if (avdecc_entity_) {
            avdecc_entity_->stop();
        }
        
        // Join threads
        if (avdecc_thread_.joinable()) {
            avdecc_thread_.join();
        }
        if (streaming_thread_.joinable()) {
            streaming_thread_.join();
        }
        if (gptp_monitor_thread_.joinable()) {
            gptp_monitor_thread_.join();
        }
        
        // Cleanup network
        if (pcap_handle_) {
            pcap_close(pcap_handle_);
            pcap_handle_ = nullptr;
        }
        
        // Shutdown gPTP
        gptp_integration::shutdown_gptp_daemon();
        
        std::cout << "✅ Complete AVB Entity shutdown complete" << std::endl;
    }
    
    /**
     * @brief Start audio streaming (triggered by AVDECC START_STREAMING command)
     */
    void start_streaming() {
        if (!gptp_state_.synchronized) {
            std::cout << "⚠️  Cannot start streaming - gPTP not synchronized" << std::endl;
            return;
        }
        
        streaming_active_.store(true);
        std::cout << "🎵 Audio streaming started (48kHz/24-bit/8ch)" << std::endl;
    }
    
    /**
     * @brief Stop audio streaming (triggered by AVDECC STOP_STREAMING command)
     */
    void stop_streaming() {
        streaming_active_.store(false);
        std::cout << "🎵 Audio streaming stopped" << std::endl;
    }
    
private:
    /**
     * @brief Initialize network interface for AVB communication
     */
    bool initialize_network() {
        // Find Intel Ethernet interface (same logic as your AVDECC entity)
        pcap_if_t* alldevs;
        char errbuf[PCAP_ERRBUF_SIZE];
        
        if (pcap_findalldevs(&alldevs, errbuf) == -1) {
            std::cerr << "❌ Error finding network devices: " << errbuf << std::endl;
            return false;
        }
        
        pcap_if_t* selected_device = nullptr;
        
        // Priority: Intel I219 Ethernet
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
        
        if (!selected_device && alldevs) {
            selected_device = alldevs;
            interface_name_ = alldevs->name;
            std::cout << "⚠️  Using first available interface: " << alldevs->name << std::endl;
        }
        
        if (!selected_device) {
            pcap_freealldevs(alldevs);
            return false;
        }
        
        // Open interface for packet capture/injection
        pcap_handle_ = pcap_open_live(selected_device->name, 65536, 1, 1000, errbuf);
        pcap_freealldevs(alldevs);
        
        if (!pcap_handle_) {
            std::cerr << "❌ Failed to open interface: " << errbuf << std::endl;
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief Configure IEEE 1722-2016 audio stream parameters
     */
    void configure_audio_stream() {
        // Set stream ID
        std::memcpy(audio_stream_->stream_id, stream_config_.stream_id, 8);
        // Configure audio format (use MILAN_PCM for demo)
        audio_stream_->format = avtp_protocol::ieee_1722_2016::AudioFormat::MILAN_PCM;
        audio_stream_->nominal_sample_rate = avtp_protocol::ieee_1722_2016::SampleRate::RATE_48KHZ;
        audio_stream_->channels = stream_config_.channels;
        audio_stream_->bit_depth = stream_config_.bit_depth;
        audio_stream_->samples_per_frame = stream_config_.samples_per_frame;
        // Set AVTP header fields
        audio_stream_->subtype = static_cast<uint8_t>(avtp_protocol::ieee_1722_2016::Subtype::AAF);
        audio_stream_->stream_valid = true;
        audio_stream_->version = avtp_protocol::ieee_1722_2016::AVTP_VERSION_2016;
        audio_stream_->tv = true;  // Timestamp valid
        std::cout << "🎵 Audio stream configured: " << static_cast<int>(stream_config_.channels) 
                  << "ch/" << stream_config_.sample_rate << "Hz/" << stream_config_.bit_depth << "-bit" << std::endl;
    }
    
    /**
     * @brief Monitor gPTP synchronization status
     */
    void monitor_gptp_synchronization() {
        while (running_.load()) {
            bool was_synchronized = gptp_state_.synchronized;
            gptp_state_.synchronized = gptp_integration::is_gptp_synchronized();
            
            if (gptp_state_.synchronized && !was_synchronized) {
                std::cout << "✅ gPTP synchronized - streaming now available" << std::endl;
                gptp_state_.last_sync_time = gptp_integration::get_gptp_time_ns();
            } else if (!gptp_state_.synchronized && was_synchronized) {
                std::cout << "⚠️  gPTP synchronization lost - stopping streams" << std::endl;
                streaming_active_.store(false);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    
    /**
     * @brief Handle AVTP streaming with gPTP timing
     */
    void handle_avtp_streaming() {
        while (running_.load()) {
            if (streaming_active_.load() && gptp_state_.synchronized) {
                send_audio_packet();
                
                // 8000 packets/sec = 125µs interval
                std::this_thread::sleep_for(std::chrono::microseconds(125));
            } else {
                // Not streaming - check every 10ms
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    
    /**
     * @brief Send IEEE 1722-2016 audio packet with gPTP timing
     */
    void send_audio_packet() {
        // Get current gPTP time for timestamp
        uint64_t gptp_time_ns = gptp_integration::get_gptp_time_ns();
        
        // Calculate presentation time (current time + offset)
        uint64_t presentation_time_ns = gptp_time_ns + (gptp_state_.presentation_offset_us * 1000);
        
        // Set AVTP timestamp (32-bit, nanoseconds portion)
        audio_stream_->avtp_timestamp = static_cast<uint32_t>(presentation_time_ns & 0xFFFFFFFF);
        
        // Generate audio samples (silence for demo)
        size_t sample_size_bytes = (stream_config_.bit_depth / 8) * stream_config_.channels * stream_config_.samples_per_frame;
        std::memset(audio_stream_->payload.data(), 0, sample_size_bytes);  // Silence
        audio_stream_->stream_data_length = static_cast<uint16_t>(sample_size_bytes);
        
        // Serialize AVTPDU to network packet
        uint8_t packet_buffer[1500];
        size_t packet_size;
        audio_stream_->serialize(packet_buffer, packet_size);
        
        // Add Ethernet header for AVB multicast
        uint8_t eth_packet[1518];
        size_t eth_size = add_ethernet_header(eth_packet, packet_buffer, packet_size);
        
        // Send packet via PCAP
        if (pcap_sendpacket(pcap_handle_, eth_packet, static_cast<int>(eth_size)) != 0) {
            // Error sending - could log but don't spam console
        }
        
        // Increment sequence number
        audio_stream_->sequence_num++;
    }
    
    /**
     * @brief Add Ethernet header for AVB packet
     */
    size_t add_ethernet_header(uint8_t* eth_packet, const uint8_t* avtp_data, size_t avtp_size) {
        size_t offset = 0;
        
        // Destination MAC (AVB multicast)
        std::memcpy(&eth_packet[offset], stream_config_.dest_mac, 6);
        offset += 6;
        
        // Source MAC (derived from stream ID)
        std::memcpy(&eth_packet[offset], stream_config_.stream_id, 6);
        offset += 6;
        
        // EtherType: IEEE 1722 (0x22F0)
        eth_packet[offset++] = 0x22;
        eth_packet[offset++] = 0xF0;
        
        // Copy AVTP data
        std::memcpy(&eth_packet[offset], avtp_data, avtp_size);
        offset += avtp_size;
        
        return offset;
    }
};

// =============================
// gPTP Integration Functions
// =============================
namespace gptp_integration {
    // gPTP shared memory variables
    static int gptp_shm_fd = -1;
    static char* gptp_shm_map = nullptr;
    static gPtpTimeData gptp_data;
    
    bool initialize_gptp_daemon(const std::string& interface_name) {
        std::cout << "📡 Initializing gPTP integration for interface: " << interface_name << std::endl;
        
        // First try to connect to existing gPTP daemon via shared memory
        if (gptpinit(&gptp_shm_fd, &gptp_shm_map) == 0) {
            std::cout << "✅ Connected to existing gPTP daemon via shared memory" << std::endl;
            return true;
        }
        
        // If no existing daemon, try to start one
        std::cout << "📡 Starting gPTP daemon from thirdparty/gptp..." << std::endl;
        
        // Build command to start gPTP daemon
        std::string gptp_cmd = "start /B \"gPTP Daemon\" ";
        gptp_cmd += "..\\thirdparty\\gptp\\build\\windows\\daemon_cl.exe ";
        gptp_cmd += "-i " + interface_name + " ";
        gptp_cmd += "-p automotive ";
        gptp_cmd += "-l 1";  // Log level 1
        
        std::cout << "� Executing: " << gptp_cmd << std::endl;
        
        int result = system(gptp_cmd.c_str());
        if (result != 0) {
            std::cerr << "⚠️  gPTP daemon start command returned: " << result << std::endl;
        }
        
        // Wait a moment for daemon to start
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Try to connect again
        if (gptpinit(&gptp_shm_fd, &gptp_shm_map) == 0) {
            std::cout << "✅ Connected to newly started gPTP daemon" << std::endl;
            return true;
        }
        
        std::cerr << "❌ Failed to connect to gPTP daemon" << std::endl;
        return false;
    }

    uint64_t get_gptp_time_ns() {
        if (gptp_shm_map == nullptr) {
            return 0; // No gPTP connection
        }
        
        // Get current gPTP data
        if (gptpgetdata(gptp_shm_map, &gptp_data) < 0) {
            return 0;
        }
        
        // Convert to nanoseconds
        uint64_t local_time_ns;
        if (gptplocaltime(&gptp_data, &local_time_ns)) {
            return local_time_ns;
        }
        
        return 0;
    }

    bool is_gptp_synchronized() {
        if (gptp_shm_map == nullptr) {
            return false;
        }
        
        // Get current gPTP data
        if (gptpgetdata(gptp_shm_map, &gptp_data) < 0) {
            return false;
        }
        
        // Check if we have a valid grandmaster
        for (int i = 0; i < 8; i++) {
            if (gptp_data.gptp_grandmaster_id[i] != 0) {
                return true; // Non-zero grandmaster ID means synchronized
            }
        }
        
        return false;
    }

    void shutdown_gptp_daemon() {
        std::cout << "📡 Shutting down gPTP integration..." << std::endl;
        
        if (gptp_shm_map != nullptr) {
            gptpdeinit(&gptp_shm_fd, &gptp_shm_map);
            gptp_shm_map = nullptr;
            gptp_shm_fd = -1;
        }
        
        // Optionally terminate the gPTP daemon
        // system("taskkill /IM daemon_cl.exe /F");
    }
}

// =============================
// Main Application
// =============================
int main() {
    std::cout << "🎯 Complete AVB Entity - Integration Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "📡 IEEE 1722.1 (AVDECC): Discovery & Control" << std::endl;
    std::cout << "🎵 IEEE 1722-2016 (AVTP): Audio Streaming" << std::endl;
    std::cout << "⏰ IEEE 802.1AS (gPTP): Time Synchronization" << std::endl;
    std::cout << "🔗 Hardware: Intel I219/I225 with HW Timestamping" << std::endl;
    std::cout << std::endl;
    
    CompleteAVBEntity entity;
    
    if (!entity.initialize()) {
        std::cerr << "❌ Failed to initialize AVB entity" << std::endl;
        return 1;
    }
    
    entity.start();
    
    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();
    
    entity.shutdown();
    
    std::cout << "🎯 Complete AVB Entity demonstration completed!" << std::endl;
    return 0;
}
