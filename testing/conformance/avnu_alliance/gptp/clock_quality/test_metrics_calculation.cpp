#include <gtest/gtest.h>
#include "gptp_clock_quality.hpp"
#include <cmath>
#include <vector>
#include <random>

class MetricsCalculationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Seed random number generator for reproducible tests
        std::srand(12345);
    }

    std::vector<int64_t> generate_normal_distribution(int count, double mean, double std_dev) {
        std::vector<int64_t> values;
        std::default_random_engine generator(12345);
        std::normal_distribution<double> distribution(mean, std_dev);
        
        for (int i = 0; i < count; ++i) {
            values.push_back(static_cast<int64_t>(distribution(generator)));
        }
        return values;
    }

    std::vector<int64_t> generate_uniform_distribution(int count, int64_t min_val, int64_t max_val) {
        std::vector<int64_t> values;
        for (int i = 0; i < count; ++i) {
            int64_t range = max_val - min_val;
            values.push_back(min_val + (std::rand() % range));
        }
        return values;
    }

    ClockQualityMetrics calculate_metrics_from_values(const std::vector<int64_t>& values) {
        IngressEventMonitor monitor;
        monitor.enable_monitoring(125);
        
        for (size_t i = 0; i < values.size(); ++i) {
            uint64_t t1 = 1000000000ULL + i * 125000000ULL;
            uint64_t t2 = t1 + 1000000ULL + values[i]; // 1ms path delay + error
            uint64_t path_delay = 1000000ULL;
            
            monitor.record_sync_ingress(t1, t2, path_delay);
        }
        
        return monitor.compute_metrics(300);
    }
};

TEST_F(MetricsCalculationTest, MeanCalculation) {
    std::vector<int64_t> values = {10, 20, 30, 40, 50};
    auto metrics = calculate_metrics_from_values(values);
    
    EXPECT_EQ(metrics.mean_time_error_ns, 30); // (10+20+30+40+50)/5 = 30
    EXPECT_EQ(metrics.total_measurements, 5);
}

TEST_F(MetricsCalculationTest, MinMaxCalculation) {
    std::vector<int64_t> values = {-50, 75, 22, -15, 100, 8};
    auto metrics = calculate_metrics_from_values(values);
    
    EXPECT_EQ(metrics.max_time_error_ns, 100);
    EXPECT_EQ(metrics.min_time_error_ns, -50);
}

TEST_F(MetricsCalculationTest, StandardDeviationCalculation) {
    // Known values: 10, 20, 30, 40, 50
    // Mean = 30, variance = 200, std dev = sqrt(200) ≈ 14.14
    std::vector<int64_t> values = {10, 20, 30, 40, 50};
    auto metrics = calculate_metrics_from_values(values);
    
    EXPECT_NEAR(metrics.std_dev_ns, 14.14, 0.1);
}

TEST_F(MetricsCalculationTest, ZeroStandardDeviation) {
    std::vector<int64_t> values = {25, 25, 25, 25, 25}; // All same value
    auto metrics = calculate_metrics_from_values(values);
    
    EXPECT_EQ(metrics.std_dev_ns, 0.0);
    EXPECT_EQ(metrics.mean_time_error_ns, 25);
}

TEST_F(MetricsCalculationTest, LargeDatasetStatistics) {
    // Generate 1000 normally distributed values around mean=50, std_dev=15
    auto values = generate_normal_distribution(1000, 50.0, 15.0);
    auto metrics = calculate_metrics_from_values(values);
    
    EXPECT_NEAR(metrics.mean_time_error_ns, 50, 5); // Should be close to target mean
    EXPECT_NEAR(metrics.std_dev_ns, 15.0, 3.0); // Should be close to target std dev
    EXPECT_EQ(metrics.total_measurements, 1000);
}

TEST_F(MetricsCalculationTest, AccuracyRequirement80ns) {
    // Test values within ±80ns requirement
    std::vector<int64_t> values_pass = {-79, -40, 0, 35, 78};
    auto metrics_pass = calculate_metrics_from_values(values_pass);
    EXPECT_TRUE(metrics_pass.meets_80ns_requirement);
    
    // Test values that exceed ±80ns requirement
    std::vector<int64_t> values_fail = {-85, -40, 0, 35, 78};
    auto metrics_fail = calculate_metrics_from_values(values_fail);
    EXPECT_FALSE(metrics_fail.meets_80ns_requirement);
}

TEST_F(MetricsCalculationTest, RMSErrorCalculation) {
    // RMS = sqrt(mean of squares)
    std::vector<int64_t> values = {3, 4, 5}; // RMS = sqrt((9+16+25)/3) = sqrt(50/3) ≈ 4.08
    auto metrics = calculate_metrics_from_values(values);
    
    double expected_rms = std::sqrt((9.0 + 16.0 + 25.0) / 3.0);
    EXPECT_NEAR(metrics.rms_error_ns, expected_rms, 0.1);
}

TEST_F(MetricsCalculationTest, FrequencyStabilityEstimation) {
    // Simulate gradual frequency drift
    std::vector<int64_t> values;
    for (int i = 0; i < 100; ++i) {
        // Linear drift: 0.5ns per measurement
        int64_t drift = static_cast<int64_t>(i * 0.5);
        int64_t noise = (std::rand() % 6) - 3; // ±3ns noise
        values.push_back(drift + noise);
    }
    
    auto metrics = calculate_metrics_from_values(values);
    
    // Should detect frequency drift (non-zero frequency stability)
    EXPECT_NE(metrics.frequency_stability_ppb, 0.0);
    EXPECT_LT(std::abs(metrics.frequency_stability_ppb), 100.0); // Reasonable range
}

TEST_F(MetricsCalculationTest, OutlierImpact) {
    // Test how outliers affect statistics
    std::vector<int64_t> values_normal = {20, 22, 18, 21, 19}; // Tight distribution
    auto metrics_normal = calculate_metrics_from_values(values_normal);
    
    std::vector<int64_t> values_outlier = {20, 22, 18, 21, 200}; // One large outlier
    auto metrics_outlier = calculate_metrics_from_values(values_outlier);
    
    // Outlier should significantly increase mean and std dev
    EXPECT_GT(metrics_outlier.mean_time_error_ns, metrics_normal.mean_time_error_ns);
    EXPECT_GT(metrics_outlier.std_dev_ns, metrics_normal.std_dev_ns);
    EXPECT_GT(metrics_outlier.max_time_error_ns, metrics_normal.max_time_error_ns);
}

TEST_F(MetricsCalculationTest, LockDetectionLogic) {
    IngressEventMonitor monitor;
    monitor.enable_monitoring(125);
    
    // Simulate initial large errors (unlocked state)
    for (int i = 0; i < 10; ++i) {
        uint64_t t1 = 1000000000ULL + i * 125000000ULL;
        uint64_t t2 = t1 + 1000000ULL + 300; // 300ns error - clearly unlocked
        monitor.record_sync_ingress(t1, t2, 1000000ULL);
    }
    
    auto metrics_unlocked = monitor.compute_metrics(60);
    EXPECT_FALSE(metrics_unlocked.is_locked);
    
    // Now simulate locked state with small errors
    for (int i = 10; i < 60; ++i) {
        uint64_t t1 = 1000000000ULL + i * 125000000ULL;
        uint64_t t2 = t1 + 1000000ULL + 15; // 15ns error - locked
        monitor.record_sync_ingress(t1, t2, 1000000ULL);
    }
    
    auto metrics_locked = monitor.compute_metrics(60);
    EXPECT_TRUE(metrics_locked.is_locked);
}

TEST_F(MetricsCalculationTest, ConsecutiveGoodMeasurements) {
    IngressEventMonitor monitor;
    monitor.enable_monitoring(125);
    
    // Add mostly good measurements with a few bad ones
    for (int i = 0; i < 100; ++i) {
        uint64_t t1 = 1000000000ULL + i * 125000000ULL;
        int64_t error;
        
        if (i == 20 || i == 50) {
            error = 150; // Bad measurement
        } else {
            error = 25; // Good measurement
        }
        
        uint64_t t2 = t1 + 1000000ULL + error;
        monitor.record_sync_ingress(t1, t2, 1000000ULL);
    }
    
    auto metrics = monitor.compute_metrics(300);
    
    // Should track consecutive good measurements
    EXPECT_GT(metrics.consecutive_good_measurements, 40);
    EXPECT_LT(metrics.consecutive_good_measurements, 100); // Reset by bad measurements
}

TEST_F(MetricsCalculationTest, WindowedAnalysisAccuracy) {
    IngressEventMonitor monitor;
    monitor.enable_monitoring(125);
    
    // Add measurements over a long period
    for (int i = 0; i < 1000; ++i) {
        uint64_t t1 = 1000000000ULL + i * 125000000ULL;
        uint64_t t2 = t1 + 1000000ULL + 30; // Consistent 30ns error
        monitor.record_sync_ingress(t1, t2, 1000000ULL);
    }
    
    // Test different window sizes
    auto metrics_60s = monitor.compute_metrics(60);
    auto metrics_300s = monitor.compute_metrics(300);
    
    // Both should have consistent mean error
    EXPECT_EQ(metrics_60s.mean_time_error_ns, 30);
    EXPECT_EQ(metrics_300s.mean_time_error_ns, 30);
    
    // Longer window should include more measurements
    EXPECT_LT(metrics_60s.total_measurements, metrics_300s.total_measurements);
}

TEST_F(MetricsCalculationTest, NumericalStability) {
    // Test with very large values to ensure no overflow
    std::vector<int64_t> large_values;
    for (int i = 0; i < 100; ++i) {
        large_values.push_back(1000000 + i); // Large but valid nanosecond values
    }
    
    auto metrics = calculate_metrics_from_values(large_values);
    
    // Should handle large values without overflow
    EXPECT_GT(metrics.mean_time_error_ns, 1000000);
    EXPECT_LT(metrics.mean_time_error_ns, 1100000);
    EXPECT_GT(metrics.std_dev_ns, 25.0); // Should be around sqrt(833) ≈ 28.9
}

TEST_F(MetricsCalculationTest, EdgeCaseEmptyDataset) {
    IngressEventMonitor monitor;
    monitor.enable_monitoring(125);
    
    // No measurements added
    auto metrics = monitor.compute_metrics(300);
    
    EXPECT_EQ(metrics.total_measurements, 0);
    EXPECT_EQ(metrics.mean_time_error_ns, 0);
    EXPECT_EQ(metrics.max_time_error_ns, 0);
    EXPECT_EQ(metrics.min_time_error_ns, 0);
    EXPECT_EQ(metrics.std_dev_ns, 0.0);
    EXPECT_FALSE(metrics.is_locked);
}
