#include <gtest/gtest.h>
#include "gptp_clock_quality.hpp"
#include <chrono>
#include <thread>

class IngressMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor = std::make_unique<IngressEventMonitor>();
    }

    void TearDown() override {
        monitor.reset();
    }

    void simulate_sync_measurements(int count, int64_t base_error_ns, int64_t jitter_ns = 10) {
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < count; ++i) {
            // Simulate timing with some jitter
            int64_t error = base_error_ns + (rand() % (2 * jitter_ns)) - jitter_ns;
            
            uint64_t t1_master = 1000000000ULL + i * 125000000ULL; // 125ms intervals
            uint64_t t2_slave = t1_master + 1000000ULL + error; // 1ms path delay + error
            uint64_t path_delay = 1000000ULL; // 1ms path delay
            
            monitor->record_sync_ingress(t1_master, t2_slave, path_delay);
            
            // Small delay to simulate real timing
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    std::unique_ptr<IngressEventMonitor> monitor;
};

TEST_F(IngressMonitorTest, EnableDisableMonitoring) {
    EXPECT_FALSE(monitor->is_monitoring_enabled());
    
    monitor->enable_monitoring(125);
    EXPECT_TRUE(monitor->is_monitoring_enabled());
    
    monitor->disable_monitoring();
    EXPECT_FALSE(monitor->is_monitoring_enabled());
}

TEST_F(IngressMonitorTest, BasicMeasurementRecording) {
    monitor->enable_monitoring(125);
    
    // Record a single measurement
    uint64_t t1 = 1000000000ULL;
    uint64_t t2 = 1001050000ULL; // 1ms path delay + 50ns error
    uint64_t path_delay = 1000000ULL;
    
    monitor->record_sync_ingress(t1, t2, path_delay);
    
    auto metrics = monitor->compute_metrics(60);
    EXPECT_EQ(metrics.total_measurements, 1);
    EXPECT_EQ(metrics.mean_time_error_ns, 50);
    EXPECT_EQ(metrics.max_time_error_ns, 50);
    EXPECT_EQ(metrics.min_time_error_ns, 50);
}

TEST_F(IngressMonitorTest, AccuracyRequirement80ns) {
    monitor->enable_monitoring(125);
    
    // Simulate measurements within ±80ns requirement
    simulate_sync_measurements(100, 0, 70); // Mean 0ns, ±70ns jitter
    
    auto metrics = monitor->compute_metrics(60);
    EXPECT_LE(std::abs(metrics.mean_time_error_ns), 80);
    EXPECT_LE(metrics.max_time_error_ns, 80);
    EXPECT_GE(metrics.min_time_error_ns, -80);
    EXPECT_TRUE(metrics.meets_80ns_requirement);
}

TEST_F(IngressMonitorTest, AccuracyRequirementFailure) {
    monitor->enable_monitoring(125);
    
    // Simulate measurements exceeding ±80ns requirement
    simulate_sync_measurements(10, 0, 150); // ±150ns jitter - should fail
    
    auto metrics = monitor->compute_metrics(60);
    EXPECT_FALSE(metrics.meets_80ns_requirement);
}

TEST_F(IngressMonitorTest, StatisticalAnalysis) {
    monitor->enable_monitoring(125);
    
    // Simulate consistent measurements with known statistics
    simulate_sync_measurements(1000, 25, 15); // 25ns mean, ±15ns jitter
    
    auto metrics = monitor->compute_metrics(300);
    
    // Mean should be close to 25ns
    EXPECT_NEAR(metrics.mean_time_error_ns, 25, 5);
    
    // Standard deviation should be reasonable for the jitter
    EXPECT_LT(metrics.std_dev_ns, 20.0);
    EXPECT_GT(metrics.std_dev_ns, 8.0);
    
    // Should have recorded all measurements
    EXPECT_EQ(metrics.total_measurements, 1000);
}

TEST_F(IngressMonitorTest, LockTimeDetection) {
    monitor->enable_monitoring(125);
    
    // Simulate initial large errors (unlocked state)
    simulate_sync_measurements(20, 500, 200); // Large errors
    
    // Then simulate convergence to locked state
    simulate_sync_measurements(50, 10, 5); // Small errors
    
    auto metrics = monitor->compute_metrics(60);
    
    // Should detect lock state
    EXPECT_TRUE(metrics.is_locked);
    EXPECT_LT(metrics.lock_time_seconds, 10); // Should lock quickly in simulation
}

TEST_F(IngressMonitorTest, WindowedAnalysis) {
    monitor->enable_monitoring(125);
    
    // Record measurements over a longer period
    simulate_sync_measurements(500, 15, 8);
    
    // Test different analysis windows
    auto metrics_60s = monitor->compute_metrics(60);
    auto metrics_300s = monitor->compute_metrics(300);
    
    // Longer window should include more measurements
    EXPECT_GE(metrics_300s.total_measurements, metrics_60s.total_measurements);
    
    // Both should be within accuracy requirements
    EXPECT_TRUE(metrics_60s.meets_80ns_requirement);
    EXPECT_TRUE(metrics_300s.meets_80ns_requirement);
}

TEST_F(IngressMonitorTest, TLVExport) {
    monitor->enable_monitoring(125);
    
    // Record some measurements
    simulate_sync_measurements(100, 20, 10);
    
    // Export TLV data
    auto tlv_data = monitor->export_tlv_data();
    
    // Should have reasonable TLV size
    EXPECT_GT(tlv_data.size(), 100);
    EXPECT_LT(tlv_data.size(), 10000);
    
    // TLV should start with proper header
    EXPECT_EQ(tlv_data[0], 0x80); // Custom TLV type high byte
    EXPECT_EQ(tlv_data[1], 0x01); // Custom TLV type low byte
}

TEST_F(IngressMonitorTest, HistoryManagement) {
    monitor->enable_monitoring(125);
    
    // Record many measurements to test history limits
    simulate_sync_measurements(2000, 30, 12);
    
    auto metrics = monitor->compute_metrics(300);
    
    // Should not exceed maximum history size
    EXPECT_LE(metrics.total_measurements, 2000);
    
    // Should still provide accurate analysis
    EXPECT_NEAR(metrics.mean_time_error_ns, 30, 10);
}
