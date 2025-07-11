#include <gtest/gtest.h>
#include "gptp_clock_quality.hpp"
#include <vector>
#include <cmath>

class ClockQualityAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        analyzer = std::make_unique<ClockQualityAnalyzer>();
    }

    void TearDown() override {
        analyzer.reset();
    }

    void add_measurement(int64_t time_error_ns, uint64_t timestamp_offset = 0) {
        ClockQualityMeasurement measurement;
        measurement.time_error_ns = time_error_ns;
        measurement.timestamp = std::chrono::steady_clock::now().time_since_epoch().count() + timestamp_offset;
        measurement.is_valid = true;
        
        analyzer->add_measurement(measurement);
    }

    void add_multiple_measurements(const std::vector<int64_t>& errors) {
        for (size_t i = 0; i < errors.size(); ++i) {
            add_measurement(errors[i], i * 125000000ULL); // 125ms apart
        }
    }

    std::unique_ptr<ClockQualityAnalyzer> analyzer;
};

TEST_F(ClockQualityAnalyzerTest, EmptyAnalysis) {
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_EQ(metrics.total_measurements, 0);
    EXPECT_EQ(metrics.mean_time_error_ns, 0);
    EXPECT_EQ(metrics.max_time_error_ns, 0);
    EXPECT_EQ(metrics.min_time_error_ns, 0);
    EXPECT_EQ(metrics.std_dev_ns, 0.0);
    EXPECT_FALSE(metrics.is_locked);
    EXPECT_FALSE(metrics.meets_80ns_requirement);
}

TEST_F(ClockQualityAnalyzerTest, SingleMeasurement) {
    add_measurement(45);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_EQ(metrics.total_measurements, 1);
    EXPECT_EQ(metrics.mean_time_error_ns, 45);
    EXPECT_EQ(metrics.max_time_error_ns, 45);
    EXPECT_EQ(metrics.min_time_error_ns, 45);
    EXPECT_EQ(metrics.std_dev_ns, 0.0);
    EXPECT_TRUE(metrics.meets_80ns_requirement);
}

TEST_F(ClockQualityAnalyzerTest, StatisticalCalculations) {
    // Add measurements with known statistics
    std::vector<int64_t> errors = {10, 20, 30, 40, 50}; // Mean = 30, std dev ≈ 15.81
    add_multiple_measurements(errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_EQ(metrics.total_measurements, 5);
    EXPECT_EQ(metrics.mean_time_error_ns, 30);
    EXPECT_EQ(metrics.max_time_error_ns, 50);
    EXPECT_EQ(metrics.min_time_error_ns, 10);
    EXPECT_NEAR(metrics.std_dev_ns, 15.81, 0.1);
    EXPECT_TRUE(metrics.meets_80ns_requirement);
}

TEST_F(ClockQualityAnalyzerTest, NegativeErrors) {
    std::vector<int64_t> errors = {-30, -20, -10, 10, 20, 30}; // Mean = 0
    add_multiple_measurements(errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_EQ(metrics.mean_time_error_ns, 0);
    EXPECT_EQ(metrics.max_time_error_ns, 30);
    EXPECT_EQ(metrics.min_time_error_ns, -30);
    EXPECT_TRUE(metrics.meets_80ns_requirement);
}

TEST_F(ClockQualityAnalyzerTest, AccuracyRequirement80ns_Pass) {
    // All measurements within ±80ns
    std::vector<int64_t> errors = {-75, -40, -10, 15, 45, 78};
    add_multiple_measurements(errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_TRUE(metrics.meets_80ns_requirement);
    EXPECT_LE(metrics.max_time_error_ns, 80);
    EXPECT_GE(metrics.min_time_error_ns, -80);
}

TEST_F(ClockQualityAnalyzerTest, AccuracyRequirement80ns_Fail) {
    // Some measurements exceed ±80ns
    std::vector<int64_t> errors = {-75, -40, 85, 15, 45, 78}; // 85ns exceeds limit
    add_multiple_measurements(errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_FALSE(metrics.meets_80ns_requirement);
    EXPECT_GT(metrics.max_time_error_ns, 80);
}

TEST_F(ClockQualityAnalyzerTest, LockDetection) {
    // Simulate initial large errors (unlocked)
    std::vector<int64_t> large_errors = {200, 180, 220, 190, 210};
    add_multiple_measurements(large_errors);
    
    // Then add small errors (locked)
    std::vector<int64_t> small_errors;
    for (int i = 0; i < 50; ++i) {
        small_errors.push_back(10 + (i % 20) - 10); // ±10ns around 10ns
    }
    add_multiple_measurements(small_errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_TRUE(metrics.is_locked);
    EXPECT_GT(metrics.consecutive_good_measurements, 40);
}

TEST_F(ClockQualityAnalyzerTest, LockTimeCalculation) {
    // Add measurements that simulate lock acquisition
    for (int i = 0; i < 100; ++i) {
        int64_t error;
        if (i < 20) {
            // Initial large errors (unlocked state)
            error = 200 - i * 8; // Gradually decreasing from 200ns to ~40ns
        } else {
            // Locked state with small errors
            error = (i % 20) - 10; // ±10ns variation
        }
        add_measurement(error, i * 125000000ULL); // 125ms intervals
    }
    
    auto metrics = analyzer->analyze_window(300);
    
    EXPECT_TRUE(metrics.is_locked);
    EXPECT_LT(metrics.lock_time_seconds, 6); // Should meet 6s requirement
    EXPECT_GT(metrics.lock_time_seconds, 1); // Should take some time to lock
}

TEST_F(ClockQualityAnalyzerTest, WindowedAnalysis) {
    // Add measurements over different time periods
    for (int i = 0; i < 500; ++i) {
        add_measurement(20 + (i % 10) - 5, i * 125000000ULL); // Every 125ms
    }
    
    // Test different window sizes
    auto metrics_60s = analyzer->analyze_window(60);
    auto metrics_300s = analyzer->analyze_window(300);
    
    // Shorter window should have fewer measurements
    EXPECT_LT(metrics_60s.total_measurements, metrics_300s.total_measurements);
    
    // Both should detect similar accuracy characteristics
    EXPECT_NEAR(metrics_60s.mean_time_error_ns, metrics_300s.mean_time_error_ns, 5);
}

TEST_F(ClockQualityAnalyzerTest, FrequencyStabilityCalculation) {
    // Add measurements that simulate frequency drift
    for (int i = 0; i < 100; ++i) {
        // Simulate gradual frequency drift: 1ns per measurement
        int64_t drift_error = i * 1; // 1ns/measurement = ~8ppb over time
        int64_t noise = (i % 6) - 3; // ±3ns noise
        add_measurement(drift_error + noise, i * 125000000ULL);
    }
    
    auto metrics = analyzer->analyze_window(300);
    
    // Should detect frequency stability issues
    EXPECT_NE(metrics.frequency_stability_ppb, 0.0);
    EXPECT_LT(std::abs(metrics.frequency_stability_ppb), 50.0); // Reasonable range
}

TEST_F(ClockQualityAnalyzerTest, OutlierDetection) {
    // Add mostly good measurements with a few outliers
    std::vector<int64_t> errors;
    for (int i = 0; i < 90; ++i) {
        errors.push_back(15 + (i % 10) - 5); // ±5ns around 15ns
    }
    // Add some outliers
    errors.push_back(150); // Large positive outlier
    errors.push_back(-120); // Large negative outlier
    
    add_multiple_measurements(errors);
    
    auto metrics = analyzer->analyze_window(300);
    
    // Should still pass requirements due to majority good measurements
    EXPECT_TRUE(metrics.meets_80ns_requirement);
    EXPECT_GT(metrics.consecutive_good_measurements, 80);
    
    // But outliers should be reflected in max/min
    EXPECT_GE(metrics.max_time_error_ns, 150);
    EXPECT_LE(metrics.min_time_error_ns, -120);
}

TEST_F(ClockQualityAnalyzerTest, StabilityRequirement) {
    // Add measurements that maintain stability over 5 minutes
    for (int i = 0; i < 2400; ++i) { // 5 minutes at 125ms intervals
        int64_t error = 25 + (i % 8) - 4; // ±4ns around 25ns - very stable
        add_measurement(error, i * 125000000ULL);
    }
    
    auto metrics = analyzer->analyze_window(300); // 5 minute window
    
    EXPECT_TRUE(metrics.meets_stability_requirement);
    EXPECT_LT(metrics.std_dev_ns, 10.0); // Should be very stable
    EXPECT_GT(metrics.consecutive_good_measurements, 2300);
}

TEST_F(ClockQualityAnalyzerTest, HistoryManagement) {
    // Test that analyzer handles large numbers of measurements
    for (int i = 0; i < 5000; ++i) {
        add_measurement(30 + (i % 20) - 10, i * 125000000ULL);
    }
    
    auto metrics = analyzer->analyze_window(300);
    
    // Should handle large datasets efficiently
    EXPECT_GT(metrics.total_measurements, 2000);
    EXPECT_LT(metrics.total_measurements, 5001);
    EXPECT_NEAR(metrics.mean_time_error_ns, 30, 5);
}
