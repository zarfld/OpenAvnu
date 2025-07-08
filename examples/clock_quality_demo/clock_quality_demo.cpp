/**
 * @file clock_quality_demo.cpp
 * @brief Demonstration of OpenAvnu gPTP Clock Quality Testing
 * 
 * This demo shows how to use the clock quality monitoring capabilities
 * integrated into the OpenAvnu gPTP implementation.
 */

#include "../../thirdparty/gptp/common/gptp_profile.hpp"
#include "../../thirdparty/gptp/common/gptp_clock_quality.hpp"
#include "gptp_log_stub.hpp" // Provide logging stub for demo
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>

using namespace OpenAvnu::gPTP;

/**
 * @brief Simulate Sync message ingress events with realistic timing
 */
class SyncMessageSimulator {
private:
    std::mt19937 rng_;
    std::normal_distribution<double> time_error_dist_;
    std::uniform_int_distribution<uint64_t> path_delay_dist_;
    std::uniform_int_distribution<uint16_t> sequence_id_dist_;
    
    uint64_t base_time_;
    uint16_t last_sequence_id_;
    
public:
    SyncMessageSimulator(double mean_error_ns = 10.0, double std_dev_ns = 25.0)
        : rng_(std::chrono::steady_clock::now().time_since_epoch().count())
        , time_error_dist_(mean_error_ns, std_dev_ns)
        , path_delay_dist_(50000, 150000)  // 50-150μs path delay
        , sequence_id_dist_(1, 65535)
        , base_time_(std::chrono::steady_clock::now().time_since_epoch().count())
        , last_sequence_id_(0)
    {
    }
    
    struct SyncEvent {
        uint64_t t1_master_tx;
        uint64_t t2_slave_rx;
        uint64_t path_delay;
        uint64_t correction_field;
        uint16_t sequence_id;
    };
    
    SyncEvent generate_sync_event() {
        SyncEvent event;
        
        // Generate realistic timestamps
        uint64_t current_time = std::chrono::steady_clock::now().time_since_epoch().count();
        event.path_delay = path_delay_dist_(rng_);
        event.correction_field = 0; // Simplified - no correction field
        
        // Master transmit time (slightly in the past)
        event.t1_master_tx = current_time - event.path_delay - 1000; // 1μs processing delay
        
        // Slave receive time with simulated clock error
        double time_error = time_error_dist_(rng_);
        event.t2_slave_rx = event.t1_master_tx + event.path_delay + static_cast<int64_t>(time_error);
        
        // Increment sequence ID
        event.sequence_id = ++last_sequence_id_;
        
        return event;
    }
    
    void simulate_lock_sequence(int& error_trend) {
        // Simulate initial lock acquisition with improving accuracy
        if (error_trend > 0) {
            error_trend -= 5; // Improve accuracy over time
            time_error_dist_ = std::normal_distribution<double>(
                static_cast<double>(error_trend), 
                std::max(5.0, static_cast<double>(error_trend) / 3.0)
            );
        }
    }
};

/**
 * @brief Demo function for Milan profile clock quality testing
 */
void demo_milan_clock_quality() {
    std::cout << "=== Milan Profile Clock Quality Demo ===\n\n";
    
    // Create Milan profile with clock quality monitoring enabled
    gPTPProfile milan_profile = gPTPProfileFactory::createMilanProfile();
    milan_profile.clock_quality_monitoring_enabled = true;
    milan_profile.clock_quality_target_accuracy_ns = 80; // ±80ns requirement
    milan_profile.clock_quality_measurement_interval_ms = 125; // 125ms sync interval
    
    // Enable monitoring
    milan_profile.enable_clock_quality_monitoring();
    
    if (!milan_profile.is_clock_quality_monitoring_active()) {
        std::cout << "ERROR: Failed to enable clock quality monitoring\n";
        return;
    }
    
    std::cout << "Profile: " << milan_profile.profile_description << "\n";
    std::cout << "Monitoring enabled with 125ms measurement interval\n";
    std::cout << "Target accuracy: ±" << milan_profile.clock_quality_target_accuracy_ns << "ns\n\n";
    
    // Simulate lock acquisition and stable operation
    SyncMessageSimulator simulator(100.0, 50.0); // Start with 100ns mean error
    int error_trend = 100; // Will decrease over time to simulate lock
    
    auto start_time = std::chrono::steady_clock::now();
    int sync_count = 0;
    
    std::cout << "Simulating gPTP lock acquisition and stability...\n";
    std::cout << "Time(s) | Count | Mean Error | Max Error | Std Dev | Locked | Compliant\n";
    std::cout << "--------|-------|------------|-----------|---------|--------|----------\n";
    
    // Run simulation for 5 minutes (300 seconds) as per certification requirement
    while (std::chrono::steady_clock::now() - start_time < std::chrono::minutes(5)) {
        // Generate and record sync event
        auto sync_event = simulator.generate_sync_event();
        milan_profile.record_sync_ingress_event(
            sync_event.t1_master_tx,
            sync_event.t2_slave_rx,
            sync_event.path_delay,
            sync_event.correction_field,
            sync_event.sequence_id
        );
        
        sync_count++;
        
        // Simulate lock acquisition process
        simulator.simulate_lock_sequence(error_trend);
        
        // Display metrics every 10 seconds
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        if (sync_count % 80 == 0 || elapsed % 30 == 0) { // Every ~10s (80 * 125ms) or 30s
            auto metrics = milan_profile.get_clock_quality_metrics(60); // Last 60 seconds
            
            std::cout << std::setw(7) << elapsed << " | "
                     << std::setw(5) << metrics.total_measurements << " | "
                     << std::setw(10) << metrics.mean_time_error_ns << " | "
                     << std::setw(9) << metrics.max_time_error_ns << " | "
                     << std::setw(7) << std::fixed << std::setprecision(1) << metrics.std_dev_ns << " | "
                     << std::setw(6) << (metrics.is_locked ? "YES" : "NO") << " | "
                     << std::setw(9) << (metrics.meets_80ns_requirement ? "YES" : "NO") << "\n";
        }
        
        // Sleep for sync interval (125ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(125));
    }
    
    std::cout << "\n=== Final Certification Results ===\n\n";
    
    // Generate final compliance report
    auto final_metrics = milan_profile.get_clock_quality_metrics();
    std::string compliance_report = milan_profile.generate_clock_quality_report();
    std::cout << compliance_report << "\n";
    
    // Certification validation
    bool certified = milan_profile.validate_clock_quality_certification();
    std::cout << "=== MILAN CERTIFICATION STATUS ===\n";
    std::cout << "Result: " << (certified ? "PASS - CERTIFIED" : "FAIL - NOT CERTIFIED") << "\n\n";
}

/**
 * @brief Demo function for Automotive profile clock quality testing
 */
void demo_automotive_clock_quality() {
    std::cout << "=== Automotive Profile Clock Quality Demo ===\n\n";
    
    // Create Automotive profile with enhanced requirements
    gPTPProfile auto_profile = gPTPProfileFactory::createAutomotiveProfile();
    auto_profile.clock_quality_monitoring_enabled = true;
    auto_profile.clock_quality_target_accuracy_ns = 50; // Stricter ±50ns requirement
    auto_profile.clock_quality_max_lock_time_s = 1; // Immediate asCapable requirement
    
    auto_profile.enable_clock_quality_monitoring();
    
    std::cout << "Profile: " << auto_profile.profile_description << "\n";
    std::cout << "Enhanced requirements: ±50ns accuracy, immediate asCapable\n\n";
    
    // Simulate immediate lock (automotive requirement)
    SyncMessageSimulator simulator(5.0, 15.0); // Very good accuracy from start
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Run for shorter period (automotive typically has faster validation)
    while (std::chrono::steady_clock::now() - start_time < std::chrono::minutes(2)) {
        auto sync_event = simulator.generate_sync_event();
        auto_profile.record_sync_ingress_event(
            sync_event.t1_master_tx,
            sync_event.t2_slave_rx,
            sync_event.path_delay,
            sync_event.correction_field,
            sync_event.sequence_id
        );
        
        std::this_thread::sleep_for(std::chrono::milliseconds(125));
    }
    
    // Generate automotive compliance report
    std::string auto_report = auto_profile.generate_clock_quality_report();
    std::cout << auto_report << "\n";
    
    bool auto_certified = auto_profile.validate_clock_quality_certification();
    std::cout << "=== AUTOMOTIVE CERTIFICATION STATUS ===\n";
    std::cout << "Result: " << (auto_certified ? "PASS - CERTIFIED" : "FAIL - NOT CERTIFIED") << "\n\n";
}

/**
 * @brief Demo function showing TLV export for remote monitoring
 */
void demo_remote_monitoring() {
    std::cout << "=== Remote Monitoring TLV Export Demo ===\n\n";
    
    gPTPProfile profile = gPTPProfileFactory::createStandardProfile();
    profile.clock_quality_monitoring_enabled = true;
    profile.enable_clock_quality_monitoring();
    
    // Generate some sample data
    SyncMessageSimulator simulator(20.0, 10.0);
    for (int i = 0; i < 50; i++) {
        auto sync_event = simulator.generate_sync_event();
        profile.record_sync_ingress_event(
            sync_event.t1_master_tx,
            sync_event.t2_slave_rx,
            sync_event.path_delay,
            sync_event.correction_field,
            sync_event.sequence_id
        );
    }
    
    // Export TLV data
    auto tlv_data = profile.export_clock_quality_tlv();
    
    std::cout << "Generated TLV data for remote monitoring:\n";
    std::cout << "TLV Size: " << tlv_data.size() << " bytes\n";
    std::cout << "First 32 bytes (hex): ";
    for (size_t i = 0; i < std::min(tlv_data.size(), size_t(32)); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(tlv_data[i]) << " ";
    }
    std::cout << std::dec << "\n\n";
    
    std::cout << "This TLV data can be transmitted over the network for remote monitoring\n";
    std::cout << "and analysis by certification test equipment.\n\n";
}

int main(int argc, char* argv[]) {
    std::cout << "OpenAvnu gPTP Clock Quality Testing Framework Demo\n";
    std::cout << "Based on Avnu Alliance '802.1AS Recovered Clock Quality Testing v1.0'\n";
    std::cout << "================================================================\n\n";
    
    if (argc > 1) {
        std::string demo_type = argv[1];
        
        if (demo_type == "milan") {
            demo_milan_clock_quality();
        } else if (demo_type == "automotive") {
            demo_automotive_clock_quality();
        } else if (demo_type == "remote") {
            demo_remote_monitoring();
        } else {
            std::cout << "Usage: " << argv[0] << " [milan|automotive|remote]\n";
            return 1;
        }
    } else {
        // Run all demos
        demo_milan_clock_quality();
        std::cout << "\n" << std::string(60, '=') << "\n\n";
        demo_automotive_clock_quality();
        std::cout << "\n" << std::string(60, '=') << "\n\n";
        demo_remote_monitoring();
    }
    
    std::cout << "Demo completed successfully!\n";
    std::cout << "\nNext steps:\n";
    std::cout << "1. Integrate with real gPTP message processing\n";
    std::cout << "2. Add hardware 1PPS support for additional validation\n";
    std::cout << "3. Implement Reverse Sync method for bidirectional testing\n";
    std::cout << "4. Add automated certification test suite\n";
    
    return 0;
}
