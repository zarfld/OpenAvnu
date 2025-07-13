/*
 * OpenAvnu Windows Traffic Shaper Implementation
 * 
 * Copyright (c) 2024, The OpenAvnu Contributors
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Windows Traffic Shaper with Intel TSN integration and QoS support
 * Provides advanced traffic shaping for AVB/TSN streams on Windows
 */

#include "shaper_windows.h"
#include "shaper_windows_qos.h"
#include "shaper_intel_tsn.h"
#include "shaper_common.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

namespace openavnu {
namespace shaper {

// Global shaper instance
static std::unique_ptr<WindowsShaper> g_shaper_instance;

WindowsShaper::WindowsShaper() 
    : m_initialized(false)
    , m_running(false)
    , m_qos_manager(nullptr)
    , m_intel_tsn(nullptr)
    , m_stats{}
{
    ZeroMemory(&m_config, sizeof(m_config));
}

WindowsShaper::~WindowsShaper()
{
    shutdown();
}

bool WindowsShaper::initialize(const ShaperConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_config = config;
    
    // Initialize Windows QoS manager
    m_qos_manager = std::make_unique<WindowsQoSManager>();
    if (!m_qos_manager->initialize()) {
        std::cerr << "Failed to initialize Windows QoS manager" << std::endl;
        return false;
    }
    
    // Initialize Intel TSN if available
    #ifdef OPENAVNU_BUILD_INTEL_HAL
    m_intel_tsn = std::make_unique<IntelTSNManager>();
    if (m_intel_tsn->initialize()) {
        std::cout << "Intel TSN integration initialized successfully" << std::endl;
        m_config.intel_tsn_enabled = true;
    } else {
        std::cout << "Intel TSN not available, using software shaping" << std::endl;
        m_config.intel_tsn_enabled = false;
    }
    #else
    std::cout << "Intel HAL not built, using software shaping only" << std::endl;
    m_config.intel_tsn_enabled = false;
    #endif
    
    // Configure traffic classes
    if (!configure_traffic_classes()) {
        std::cerr << "Failed to configure traffic classes" << std::endl;
        return false;
    }
    
    // Initialize statistics
    ZeroMemory(&m_stats, sizeof(m_stats));
    m_stats.start_time = GetTickCount64();
    
    m_initialized = true;
    std::cout << "Windows Shaper initialized successfully" << std::endl;
    
    return true;
}

bool WindowsShaper::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        std::cerr << "Shaper not initialized" << std::endl;
        return false;
    }
    
    if (m_running) {
        return true;
    }
    
    // Start QoS manager
    if (!m_qos_manager->start()) {
        std::cerr << "Failed to start QoS manager" << std::endl;
        return false;
    }
    
    // Start Intel TSN if enabled
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        if (!m_intel_tsn->start()) {
            std::cerr << "Failed to start Intel TSN manager" << std::endl;
            return false;
        }
    }
    
    m_running = true;
    
    // Start worker thread
    m_worker_thread = std::thread(&WindowsShaper::worker_thread, this);
    
    std::cout << "Windows Shaper started successfully" << std::endl;
    return true;
}

void WindowsShaper::stop()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_running) {
        return;
    }
    
    m_running = false;
    lock.unlock();
    
    // Stop worker thread
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
    
    lock.lock();
    
    // Stop Intel TSN
    if (m_intel_tsn) {
        m_intel_tsn->stop();
    }
    
    // Stop QoS manager
    if (m_qos_manager) {
        m_qos_manager->stop();
    }
    
    std::cout << "Windows Shaper stopped" << std::endl;
}

void WindowsShaper::shutdown()
{
    stop();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    // Cleanup Intel TSN
    if (m_intel_tsn) {
        m_intel_tsn->shutdown();
        m_intel_tsn.reset();
    }
    
    // Cleanup QoS manager
    if (m_qos_manager) {
        m_qos_manager->shutdown();
        m_qos_manager.reset();
    }
    
    m_initialized = false;
    std::cout << "Windows Shaper shutdown complete" << std::endl;
}

bool WindowsShaper::add_flow(const ShaperFlow& flow)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return false;
    }
    
    // Add to QoS manager
    if (!m_qos_manager->add_flow(flow)) {
        std::cerr << "Failed to add flow to QoS manager" << std::endl;
        return false;
    }
    
    // Add to Intel TSN if enabled
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        if (!m_intel_tsn->add_flow(flow)) {
            std::cerr << "Failed to add flow to Intel TSN" << std::endl;
            // Continue anyway, QoS will handle it
        }
    }
    
    m_flows[flow.id] = flow;
    m_stats.flows_configured++;
    
    std::cout << "Added shaper flow ID: " << flow.id 
              << ", Priority: " << flow.priority
              << ", Bandwidth: " << flow.max_bandwidth_bps << " bps" << std::endl;
    
    return true;
}

bool WindowsShaper::remove_flow(uint32_t flow_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_flows.find(flow_id);
    if (it == m_flows.end()) {
        return false;
    }
    
    // Remove from QoS manager
    if (m_qos_manager) {
        m_qos_manager->remove_flow(flow_id);
    }
    
    // Remove from Intel TSN
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        m_intel_tsn->remove_flow(flow_id);
    }
    
    m_flows.erase(it);
    m_stats.flows_removed++;
    
    std::cout << "Removed shaper flow ID: " << flow_id << std::endl;
    return true;
}

ShaperStats WindowsShaper::get_statistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    ShaperStats stats = m_stats;
    stats.uptime_ms = GetTickCount64() - stats.start_time;
    stats.active_flows = static_cast<uint32_t>(m_flows.size());
    
    // Aggregate QoS statistics
    if (m_qos_manager) {
        auto qos_stats = m_qos_manager->get_statistics();
        stats.bytes_shaped += qos_stats.bytes_processed;
        stats.packets_shaped += qos_stats.packets_processed;
    }
    
    // Aggregate Intel TSN statistics
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        auto tsn_stats = m_intel_tsn->get_statistics();
        stats.intel_tsn_active = true;
        stats.hardware_queues_used = tsn_stats.queues_configured;
    }
    
    return stats;
}

bool WindowsShaper::configure_traffic_classes()
{
    // Configure standard AVB traffic classes
    TrafficClass class_a = {
        .id = 0,
        .priority = 3,
        .max_bandwidth_percent = 75,
        .max_latency_us = 2000,
        .name = "AVB Class A"
    };
    
    TrafficClass class_b = {
        .id = 1,
        .priority = 2,
        .max_bandwidth_percent = 25,
        .max_latency_us = 50000,
        .name = "AVB Class B"
    };
    
    TrafficClass best_effort = {
        .id = 2,
        .priority = 0,
        .max_bandwidth_percent = 0,  // Remaining bandwidth
        .max_latency_us = UINT32_MAX,
        .name = "Best Effort"
    };
    
    // Configure in QoS manager
    if (!m_qos_manager->configure_traffic_class(class_a) ||
        !m_qos_manager->configure_traffic_class(class_b) ||
        !m_qos_manager->configure_traffic_class(best_effort)) {
        return false;
    }
    
    // Configure in Intel TSN if available
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        m_intel_tsn->configure_traffic_class(class_a);
        m_intel_tsn->configure_traffic_class(class_b);
        m_intel_tsn->configure_traffic_class(best_effort);
    }
    
    return true;
}

void WindowsShaper::worker_thread()
{
    std::cout << "Windows Shaper worker thread started" << std::endl;
    
    while (m_running) {
        // Update statistics
        update_statistics();
        
        // Check flow health
        check_flow_health();
        
        // Sleep for monitoring interval
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    std::cout << "Windows Shaper worker thread stopped" << std::endl;
}

void WindowsShaper::update_statistics()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update from QoS manager
    if (m_qos_manager) {
        auto qos_stats = m_qos_manager->get_statistics();
        m_stats.bytes_shaped = qos_stats.bytes_processed;
        m_stats.packets_shaped = qos_stats.packets_processed;
    }
    
    // Update from Intel TSN
    if (m_config.intel_tsn_enabled && m_intel_tsn) {
        auto tsn_stats = m_intel_tsn->get_statistics();
        m_stats.hardware_accelerated_flows = tsn_stats.hardware_flows;
    }
}

void WindowsShaper::check_flow_health()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& [flow_id, flow] : m_flows) {
        // Check if flow is still active
        bool qos_active = m_qos_manager ? m_qos_manager->is_flow_active(flow_id) : false;
        bool tsn_active = (m_config.intel_tsn_enabled && m_intel_tsn) ? 
                         m_intel_tsn->is_flow_active(flow_id) : false;
        
        if (!qos_active && !tsn_active) {
            std::cerr << "Warning: Flow " << flow_id << " appears inactive" << std::endl;
        }
    }
}

} // namespace shaper
} // namespace openavnu

// C API implementation for compatibility
extern "C" {

int shaper_windows_init(const char* config_file)
{
    if (openavnu::shaper::g_shaper_instance) {
        return 1; // Already initialized
    }
    
    openavnu::shaper::g_shaper_instance = std::make_unique<openavnu::shaper::WindowsShaper>();
    
    // Load configuration
    openavnu::shaper::ShaperConfig config = {};
    if (config_file) {
        // Load from file (implementation would parse config file)
        strncpy_s(config.interface_name, sizeof(config.interface_name), "eth0", _TRUNCATE);
    } else {
        // Use defaults
        strncpy_s(config.interface_name, sizeof(config.interface_name), "eth0", _TRUNCATE);
        config.intel_tsn_enabled = false;
        config.max_flows = 64;
    }
    
    return openavnu::shaper::g_shaper_instance->initialize(config) ? 0 : -1;
}

int shaper_windows_start(void)
{
    if (!openavnu::shaper::g_shaper_instance) {
        return -1;
    }
    
    return openavnu::shaper::g_shaper_instance->start() ? 0 : -1;
}

void shaper_windows_stop(void)
{
    if (openavnu::shaper::g_shaper_instance) {
        openavnu::shaper::g_shaper_instance->stop();
    }
}

void shaper_windows_shutdown(void)
{
    if (openavnu::shaper::g_shaper_instance) {
        openavnu::shaper::g_shaper_instance->shutdown();
        openavnu::shaper::g_shaper_instance.reset();
    }
}

int shaper_windows_add_flow(uint32_t flow_id, uint8_t priority, uint64_t max_bandwidth)
{
    if (!openavnu::shaper::g_shaper_instance) {
        return -1;
    }
    
    openavnu::shaper::ShaperFlow flow = {};
    flow.id = flow_id;
    flow.priority = priority;
    flow.max_bandwidth_bps = max_bandwidth;
    
    return openavnu::shaper::g_shaper_instance->add_flow(flow) ? 0 : -1;
}

int shaper_windows_remove_flow(uint32_t flow_id)
{
    if (!openavnu::shaper::g_shaper_instance) {
        return -1;
    }
    
    return openavnu::shaper::g_shaper_instance->remove_flow(flow_id) ? 0 : -1;
}

} // extern "C"
