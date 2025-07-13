/*
 * OpenAvnu Intel TSN Manager
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Intel TSN hardware integration for Windows traffic shaping
 */

#ifndef OPENAVNU_SHAPER_INTEL_TSN_H
#define OPENAVNU_SHAPER_INTEL_TSN_H

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace openavnu {
namespace shaper {

// Forward declarations
struct ShaperFlow {
    uint32_t id;
    uint8_t priority;
    uint64_t max_bandwidth_bps;
    uint32_t max_latency_us;
    char source_addr[46];
    char dest_addr[46];
    uint16_t source_port;
    uint16_t dest_port;
    uint8_t protocol;
    bool active;
};

struct TrafficClass {
    uint32_t id;
    uint8_t priority;
    uint32_t max_bandwidth_percent;
    uint32_t max_latency_us;
    char name[32];
};

// Intel TSN statistics
struct IntelTSNStats {
    uint32_t hardware_flows;
    uint32_t queues_configured;
    uint64_t bytes_processed;
    uint64_t packets_processed;
    uint32_t tsn_errors;
};

// Intel TSN hardware queue
struct TSNQueue {
    uint32_t queue_id;
    uint32_t priority;
    uint64_t max_bandwidth;
    bool active;
};

class IntelTSNManager {
public:
    IntelTSNManager();
    ~IntelTSNManager();
    
    // Initialize Intel TSN hardware
    bool initialize();
    
    // Start TSN processing
    bool start();
    
    // Stop TSN processing
    void stop();
    
    // Shutdown and cleanup
    void shutdown();
    
    // Flow management
    bool add_flow(const ShaperFlow& flow);
    bool remove_flow(uint32_t flow_id);
    bool is_flow_active(uint32_t flow_id) const;
    
    // Traffic class configuration
    bool configure_traffic_class(const TrafficClass& tc);
    
    // Statistics
    IntelTSNStats get_statistics() const;
    
    // Hardware capabilities
    bool is_hardware_available() const;
    uint32_t get_max_queues() const;
    
private:
    bool m_initialized;
    bool m_running;
    bool m_hardware_available;
    
    // Hardware capabilities
    uint32_t m_max_queues;
    uint32_t m_max_bandwidth;
    
    // Queue management
    std::unordered_map<uint32_t, std::unique_ptr<TSNQueue>> m_queues;
    std::unordered_map<uint32_t, uint32_t> m_flow_to_queue;
    
    // Statistics
    mutable IntelTSNStats m_stats;
    
    // Internal methods
    bool detect_intel_hardware();
    bool configure_hardware_queue(uint32_t queue_id, const TrafficClass& tc);
    uint32_t allocate_queue_for_flow(const ShaperFlow& flow);
    void cleanup_queue(uint32_t queue_id);
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Intel HAL integration
    void* m_hal_context;
    bool initialize_intel_hal();
    void cleanup_intel_hal();
    bool hal_configure_queue(uint32_t queue_id, uint64_t bandwidth, uint32_t priority);
    bool hal_add_flow_to_queue(uint32_t flow_id, uint32_t queue_id);
    bool hal_remove_flow_from_queue(uint32_t flow_id);
#endif
};

} // namespace shaper
} // namespace openavnu

#endif // OPENAVNU_SHAPER_INTEL_TSN_H
