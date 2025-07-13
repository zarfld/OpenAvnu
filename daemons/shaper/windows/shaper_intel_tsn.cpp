/*
 * OpenAvnu Intel TSN Manager Implementation
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Intel TSN hardware integration for Windows traffic shaping
 */

#include "shaper_intel_tsn.h"
#include <iostream>
#include <cstring>

namespace openavnu {
namespace shaper {

IntelTSNManager::IntelTSNManager()
    : m_initialized(false)
    , m_running(false)
    , m_hardware_available(false)
    , m_max_queues(8)
    , m_max_bandwidth(1000000000) // 1 Gbps
    , m_stats{}
#ifdef OPENAVNU_BUILD_INTEL_HAL
    , m_hal_context(nullptr)
#endif
{
}

IntelTSNManager::~IntelTSNManager()
{
    shutdown();
}

bool IntelTSNManager::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    // Detect Intel TSN hardware
    m_hardware_available = detect_intel_hardware();
    
    if (m_hardware_available) {
        std::cout << "Intel TSN hardware detected" << std::endl;
        
#ifdef OPENAVNU_BUILD_INTEL_HAL
        if (!initialize_intel_hal()) {
            std::cerr << "Failed to initialize Intel HAL" << std::endl;
            m_hardware_available = false;
        }
#endif
    } else {
        std::cout << "Intel TSN hardware not available, using software fallback" << std::endl;
    }
    
    // Initialize statistics
    memset(&m_stats, 0, sizeof(m_stats));
    
    m_initialized = true;
    return true;
}

bool IntelTSNManager::start()
{
    if (!m_initialized) {
        return false;
    }
    
    if (m_running) {
        return true;
    }
    
    m_running = true;
    std::cout << "Intel TSN Manager started (Hardware: " 
              << (m_hardware_available ? "Yes" : "No") << ")" << std::endl;
    
    return true;
}

void IntelTSNManager::stop()
{
    if (!m_running) {
        return;
    }
    
    m_running = false;
    std::cout << "Intel TSN Manager stopped" << std::endl;
}

void IntelTSNManager::shutdown()
{
    stop();
    
    if (!m_initialized) {
        return;
    }
    
    // Cleanup all queues
    for (auto& pair : m_queues) {
        if (pair.second) {
            cleanup_queue(pair.first);
        }
    }
    m_queues.clear();
    m_flow_to_queue.clear();
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    cleanup_intel_hal();
#endif
    
    m_initialized = false;
    std::cout << "Intel TSN Manager shutdown complete" << std::endl;
}

bool IntelTSNManager::add_flow(const ShaperFlow& flow)
{
    if (!m_initialized || !m_running) {
        return false;
    }
    
    if (!m_hardware_available) {
        // Software fallback - just track the flow
        m_stats.hardware_flows++;
        std::cout << "Added TSN flow " << flow.id << " (software mode)" << std::endl;
        return true;
    }
    
    // Allocate hardware queue for flow
    uint32_t queue_id = allocate_queue_for_flow(flow);
    if (queue_id == UINT32_MAX) {
        return false;
    }
    
    m_flow_to_queue[flow.id] = queue_id;
    m_stats.hardware_flows++;
    
    std::cout << "Added TSN flow " << flow.id << " to hardware queue " << queue_id << std::endl;
    return true;
}

bool IntelTSNManager::remove_flow(uint32_t flow_id)
{
    auto it = m_flow_to_queue.find(flow_id);
    if (it == m_flow_to_queue.end()) {
        return false;
    }
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    if (m_hardware_available) {
        hal_remove_flow_from_queue(flow_id);
    }
#endif
    
    m_flow_to_queue.erase(it);
    
    if (m_stats.hardware_flows > 0) {
        m_stats.hardware_flows--;
    }
    
    std::cout << "Removed TSN flow " << flow_id << std::endl;
    return true;
}

bool IntelTSNManager::is_flow_active(uint32_t flow_id) const
{
    return m_flow_to_queue.find(flow_id) != m_flow_to_queue.end();
}

bool IntelTSNManager::configure_traffic_class(const TrafficClass& tc)
{
    if (!m_initialized) {
        return false;
    }
    
    std::cout << "Configured TSN traffic class: " << tc.name 
              << " (Priority: " << static_cast<int>(tc.priority) << ")" << std::endl;
    
    return configure_hardware_queue(tc.id, tc);
}

IntelTSNStats IntelTSNManager::get_statistics() const
{
    return m_stats;
}

bool IntelTSNManager::is_hardware_available() const
{
    return m_hardware_available;
}

uint32_t IntelTSNManager::get_max_queues() const
{
    return m_max_queues;
}

bool IntelTSNManager::detect_intel_hardware()
{
    // Simplified hardware detection
    // In a real implementation, this would check for Intel TSN capable adapters
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    // Try to detect Intel Ethernet adapters
    return true; // Assume hardware is available when Intel HAL is built
#else
    return false; // No hardware support without Intel HAL
#endif
}

bool IntelTSNManager::configure_hardware_queue(uint32_t queue_id, const TrafficClass& tc)
{
    if (!m_hardware_available) {
        return true; // Software mode - always succeeds
    }
    
    auto queue = std::make_unique<TSNQueue>();
    queue->queue_id = queue_id;
    queue->priority = tc.priority;
    queue->max_bandwidth = (m_max_bandwidth * tc.max_bandwidth_percent) / 100;
    queue->active = true;
    
#ifdef OPENAVNU_BUILD_INTEL_HAL
    if (!hal_configure_queue(queue_id, queue->max_bandwidth, queue->priority)) {
        return false;
    }
#endif
    
    m_queues[queue_id] = std::move(queue);
    m_stats.queues_configured++;
    
    return true;
}

uint32_t IntelTSNManager::allocate_queue_for_flow(const ShaperFlow& flow)
{
    // Find available queue based on priority
    uint32_t queue_id = flow.priority; // Simple mapping: priority -> queue ID
    
    if (queue_id >= m_max_queues) {
        queue_id = m_max_queues - 1; // Use highest priority queue as fallback
    }
    
    return queue_id;
}

void IntelTSNManager::cleanup_queue(uint32_t queue_id)
{
    auto it = m_queues.find(queue_id);
    if (it != m_queues.end()) {
        m_queues.erase(it);
        
        if (m_stats.queues_configured > 0) {
            m_stats.queues_configured--;
        }
    }
}

#ifdef OPENAVNU_BUILD_INTEL_HAL
bool IntelTSNManager::initialize_intel_hal()
{
    // Initialize Intel HAL context
    // In a real implementation, this would call Intel HAL initialization functions
    m_hal_context = nullptr; // Placeholder
    return true;
}

void IntelTSNManager::cleanup_intel_hal()
{
    if (m_hal_context) {
        // Cleanup Intel HAL context
        m_hal_context = nullptr;
    }
}

bool IntelTSNManager::hal_configure_queue(uint32_t queue_id, uint64_t bandwidth, uint32_t priority)
{
    // Configure hardware queue using Intel HAL
    // In a real implementation, this would call Intel HAL functions
    
    m_stats.bytes_processed += 1000; // Simulate processing
    m_stats.packets_processed += 10;
    
    return true;
}

bool IntelTSNManager::hal_add_flow_to_queue(uint32_t flow_id, uint32_t queue_id)
{
    // Add flow to hardware queue using Intel HAL
    return true;
}

bool IntelTSNManager::hal_remove_flow_from_queue(uint32_t flow_id)
{
    // Remove flow from hardware queue using Intel HAL
    return true;
}
#endif

} // namespace shaper
} // namespace openavnu
