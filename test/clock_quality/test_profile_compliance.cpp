#include <gtest/gtest.h>
#include "gptp_profile.hpp"
#include "gptp_clock_quality.hpp"

class ProfileComplianceTest : public ::testing::Test {
protected:
    void SetUp() override {
        milan_profile = gPTPProfileFactory::create_milan_profile();
        automotive_profile = gPTPProfileFactory::create_automotive_profile();
        base_profile = gPTPProfileFactory::create_base_profile();
    }

    void simulate_quality_measurements(gPTPProfile& profile, int count, int64_t mean_error, int64_t jitter) {
        profile.enable_clock_quality_monitoring();
        
        for (int i = 0; i < count; ++i) {
            ClockQualityMeasurement measurement;
            measurement.time_error_ns = mean_error + (rand() % (2 * jitter)) - jitter;
            measurement.timestamp = std::chrono::steady_clock::now().time_since_epoch().count() + i * 125000000ULL;
            measurement.is_valid = true;
            
            // Simulate adding measurement through profile interface
            // This would normally come from actual gPTP message processing
            profile.record_clock_quality_measurement(measurement);
        }
    }

    std::unique_ptr<gPTPProfile> milan_profile;
    std::unique_ptr<gPTPProfile> automotive_profile;
    std::unique_ptr<gPTPProfile> base_profile;
};

TEST_F(ProfileComplianceTest, MilanProfileCreation) {
    EXPECT_EQ(milan_profile->get_profile_type(), ProfileType::MILAN);
    EXPECT_EQ(milan_profile->get_sync_interval_log(), -3); // 125ms
    EXPECT_EQ(milan_profile->get_pdelay_interval_log(), 0); // 1s
    EXPECT_EQ(milan_profile->get_announce_interval_log(), 1); // 2s
    
    // Milan-specific requirements
    EXPECT_EQ(milan_profile->get_late_response_threshold_ms(), 15);
    EXPECT_EQ(milan_profile->get_accuracy_requirement_ns(), 80);
}

TEST_F(ProfileComplianceTest, AutomotiveProfileCreation) {
    EXPECT_EQ(automotive_profile->get_profile_type(), ProfileType::AUTOMOTIVE);
    EXPECT_TRUE(automotive_profile->requires_immediate_ascapable());
    EXPECT_EQ(automotive_profile->get_accuracy_requirement_ns(), 50); // Stricter than Milan
}

TEST_F(ProfileComplianceTest, BaseProfileCreation) {
    EXPECT_EQ(base_profile->get_profile_type(), ProfileType::AVNU_BASE);
    EXPECT_GE(base_profile->get_min_pdelay_successes(), 2);
    EXPECT_LE(base_profile->get_max_pdelay_successes(), 10);
}

TEST_F(ProfileComplianceTest, MilanCompliance_Pass) {
    // Simulate measurements that meet Milan requirements
    simulate_quality_measurements(*milan_profile, 1000, 20, 25); // ±25ns around 20ns
    
    auto metrics = milan_profile->get_clock_quality_metrics();
    
    EXPECT_TRUE(metrics.meets_80ns_requirement);
    EXPECT_TRUE(metrics.meets_lock_time_requirement);
    EXPECT_TRUE(milan_profile->validate_certification_compliance());
    
    // Milan-specific validation
    EXPECT_TRUE(milan_profile->validate_milan_requirements());
}

TEST_F(ProfileComplianceTest, MilanCompliance_Fail) {
    // Simulate measurements that exceed Milan ±80ns requirement
    simulate_quality_measurements(*milan_profile, 100, 0, 120); // ±120ns jitter
    
    auto metrics = milan_profile->get_clock_quality_metrics();
    
    EXPECT_FALSE(metrics.meets_80ns_requirement);
    EXPECT_FALSE(milan_profile->validate_certification_compliance());
    EXPECT_FALSE(milan_profile->validate_milan_requirements());
}

TEST_F(ProfileComplianceTest, AutomotiveCompliance_Pass) {
    // Automotive requires ±50ns accuracy
    simulate_quality_measurements(*automotive_profile, 500, 15, 20); // ±20ns around 15ns
    
    auto metrics = automotive_profile->get_clock_quality_metrics();
    
    EXPECT_TRUE(metrics.meets_80ns_requirement); // Should also meet relaxed requirement
    EXPECT_TRUE(automotive_profile->validate_automotive_requirements());
    EXPECT_TRUE(automotive_profile->validate_certification_compliance());
}

TEST_F(ProfileComplianceTest, AutomotiveCompliance_Fail) {
    // Simulate measurements that meet Milan but fail Automotive requirements
    simulate_quality_measurements(*automotive_profile, 100, 0, 65); // ±65ns - exceeds 50ns
    
    auto metrics = automotive_profile->get_clock_quality_metrics();
    
    // Might pass general 80ns requirement but fail automotive-specific
    EXPECT_FALSE(automotive_profile->validate_automotive_requirements());
}

TEST_F(ProfileComplianceTest, LockTimeRequirement) {
    // Test lock time requirement (≤6 seconds)
    milan_profile->enable_clock_quality_monitoring();
    
    // Simulate initial unlock state with large errors
    for (int i = 0; i < 20; ++i) {
        ClockQualityMeasurement measurement;
        measurement.time_error_ns = 500 - i * 20; // Decreasing from 500ns to 100ns
        measurement.timestamp = std::chrono::steady_clock::now().time_since_epoch().count() + i * 125000000ULL;
        measurement.is_valid = true;
        milan_profile->record_clock_quality_measurement(measurement);
    }
    
    // Then simulate locked state
    simulate_quality_measurements(*milan_profile, 100, 15, 10);
    
    auto metrics = milan_profile->get_clock_quality_metrics();
    
    EXPECT_TRUE(metrics.is_locked);
    EXPECT_LE(metrics.lock_time_seconds, 6);
    EXPECT_TRUE(metrics.meets_lock_time_requirement);
}

TEST_F(ProfileComplianceTest, StabilityRequirement) {
    // Test 5-minute stability requirement
    simulate_quality_measurements(*milan_profile, 2400, 30, 8); // 5 minutes worth of stable data
    
    auto metrics = milan_profile->get_clock_quality_metrics();
    
    EXPECT_TRUE(metrics.meets_stability_requirement);
    EXPECT_GE(metrics.observation_window_seconds, 300); // 5 minutes
    EXPECT_LT(metrics.std_dev_ns, 15.0); // Should be stable
}

TEST_F(ProfileComplianceTest, ProfileSpecificParameters) {
    // Test that each profile has correct parameters
    
    // Milan Profile
    EXPECT_EQ(milan_profile->get_sync_interval_log(), -3); // 125ms
    EXPECT_EQ(milan_profile->get_announce_interval_log(), 1); // 2s
    EXPECT_EQ(milan_profile->get_late_response_threshold_ms(), 15);
    
    // Automotive Profile  
    EXPECT_TRUE(automotive_profile->requires_immediate_ascapable());
    EXPECT_LT(automotive_profile->get_accuracy_requirement_ns(), milan_profile->get_accuracy_requirement_ns());
    
    // Base Profile
    EXPECT_GE(base_profile->get_min_pdelay_successes(), 2);
    EXPECT_LE(base_profile->get_max_pdelay_successes(), 10);
}

TEST_F(ProfileComplianceTest, ClockQualityMonitoringControl) {
    EXPECT_FALSE(milan_profile->is_clock_quality_monitoring_enabled());
    
    milan_profile->enable_clock_quality_monitoring();
    EXPECT_TRUE(milan_profile->is_clock_quality_monitoring_enabled());
    
    milan_profile->disable_clock_quality_monitoring();
    EXPECT_FALSE(milan_profile->is_clock_quality_monitoring_enabled());
}

TEST_F(ProfileComplianceTest, ComplianceReporting) {
    simulate_quality_measurements(*milan_profile, 1000, 25, 15);
    
    std::string report = milan_profile->generate_compliance_report();
    
    // Report should contain key information
    EXPECT_NE(report.find("Milan"), std::string::npos);
    EXPECT_NE(report.find("Compliance"), std::string::npos);
    EXPECT_NE(report.find("Time Error"), std::string::npos);
    EXPECT_NE(report.find("Lock Time"), std::string::npos);
    
    // Should be reasonably detailed
    EXPECT_GT(report.length(), 500);
}

TEST_F(ProfileComplianceTest, TLVExportImport) {
    simulate_quality_measurements(*milan_profile, 200, 20, 12);
    
    // Export clock quality data as TLV
    auto tlv_data = milan_profile->export_clock_quality_tlv();
    EXPECT_GT(tlv_data.size(), 50);
    
    // Create new profile and import TLV data
    auto test_profile = gPTPProfileFactory::create_milan_profile();
    test_profile->import_clock_quality_tlv(tlv_data);
    
    // Should have similar metrics (this is a basic test - real implementation would be more complex)
    auto original_metrics = milan_profile->get_clock_quality_metrics();
    auto imported_metrics = test_profile->get_clock_quality_metrics();
    
    EXPECT_EQ(original_metrics.measurement_method, imported_metrics.measurement_method);
}

TEST_F(ProfileComplianceTest, MultiProfileComparison) {
    // Apply same measurement pattern to all profiles
    simulate_quality_measurements(*milan_profile, 500, 30, 18);
    simulate_quality_measurements(*automotive_profile, 500, 30, 18);
    simulate_quality_measurements(*base_profile, 500, 30, 18);
    
    auto milan_metrics = milan_profile->get_clock_quality_metrics();
    auto automotive_metrics = automotive_profile->get_clock_quality_metrics();
    auto base_metrics = base_profile->get_clock_quality_metrics();
    
    // All should have similar basic metrics
    EXPECT_NEAR(milan_metrics.mean_time_error_ns, automotive_metrics.mean_time_error_ns, 5);
    EXPECT_NEAR(milan_metrics.mean_time_error_ns, base_metrics.mean_time_error_ns, 5);
    
    // But compliance may differ based on profile requirements
    bool milan_compliant = milan_profile->validate_certification_compliance();
    bool automotive_compliant = automotive_profile->validate_certification_compliance();
    bool base_compliant = base_profile->validate_certification_compliance();
    
    // Automotive has stricter requirements, so might fail where others pass
    if (milan_compliant) {
        EXPECT_TRUE(base_compliant); // Base is most lenient
    }
}
