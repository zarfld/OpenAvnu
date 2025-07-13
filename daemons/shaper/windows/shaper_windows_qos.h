/*
 * OpenAvnu Windows QoS Manager
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Windows QoS integration for traffic shaping
 */

#ifndef OPENAVNU_SHAPER_WINDOWS_QOS_H
#define OPENAVNU_SHAPER_WINDOWS_QOS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdint>
#include <memory>
#include <unordered_map>

// QoS definitions (simplified for compatibility)
typedef UINT32 QOS_FLOWID;
#define QOS_NON_ADAPTIVE_FLOW 0x00000002

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

// QoS statistics
struct QoSStats {
    uint64_t bytes_processed;
    uint64_t packets_processed;
    uint32_t flows_active;
    uint32_t qos_errors;
};

// Windows QoS flow handle
struct QoSFlowHandle {
    QOS_FLOWID flow_id;
    HANDLE qos_handle;
    bool active;
};

class WindowsQoSManager {
public:
    WindowsQoSManager();
    ~WindowsQoSManager();
    
    // Initialize QoS subsystem
    bool initialize();
    
    // Start QoS processing
    bool start();
    
    // Stop QoS processing
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
    QoSStats get_statistics() const;
    
private:
    bool m_initialized;
    bool m_running;
    
    // QoS version and capabilities
    UINT32 m_qos_version;
    
    // Flow tracking
    std::unordered_map<uint32_t, std::unique_ptr<QoSFlowHandle>> m_flows;
    
    // Statistics
    mutable QoSStats m_stats;
    
    // Internal methods
    bool check_qos_support();
    bool create_qos_flow(const ShaperFlow& flow, QoSFlowHandle& handle);
    void cleanup_flow(QoSFlowHandle& handle);
};

} // namespace shaper
} // namespace openavnu

#endif // OPENAVNU_SHAPER_WINDOWS_QOS_H
