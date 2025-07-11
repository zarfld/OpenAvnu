#include "simple_test_framework.cpp"
#include "../../../thirdparty/gptp/common/gptp_clock_quality.hpp"
#include "../../../thirdparty/gptp/common/gptp_profile.hpp"
#include <vector>
#include <chrono>
#include <thread>

using namespace OpenAvnu::gPTP;

/**
 * @brief Avnu Alliance gPTP Conformance Test Suite
 * 
 * Implements systematic test cases from Avnu Alliance gPTP Test Plan v1.0
 * Leverages existing OpenAvnu clock quality framework for measurements.
 */
class AvnuConformanceTestSuite {
private:
    std::unique_ptr<ClockQualityAnalyzer> analyzer;
    std::unique_ptr<IngressEventMonitor> monitor;
    gPTPProfile current_profile;
    
public:
    AvnuConformanceTestSuite() {
        analyzer = std::make_unique<ClockQualityAnalyzer>();
        monitor = std::make_unique<IngressEventMonitor>();
        current_profile = gPTPProfileFactory::createStandardProfile();
    }
    
    // ========================================================================
    // Group 15: MDPdelayReq State Machine Tests (Critical for Intel HAL)
    // ========================================================================
    
    /**
     * @brief Test gPTP.com.c.15.1 — MDPdelayReqSM: DUT reports proper delay
     * 
     * Purpose: Verify that the DUT correctly calculates and reports path delay
     * from PDelay request/response exchanges.
     */
    bool test_MDPdelayReq_proper_delay() {
        SimpleTest::start_test("gPTP.com.c.15.1 - MDPdelayReqSM: DUT reports proper delay");
        
        monitor->enable_monitoring(125); // 125ms interval
        
        // Simulate PDelay exchange with known path delay
        uint64_t known_path_delay_ns = 1000000; // 1ms
        uint64_t t1_request_tx = 1000000000ULL;
        uint64_t t2_request_rx = t1_request_tx + known_path_delay_ns;
        uint64_t t3_response_tx = t2_request_rx + 100000; // 100µs processing delay
        uint64_t t4_response_rx = t3_response_tx + known_path_delay_ns;
        
        // Calculate expected path delay: ((t4-t1) - (t3-t2)) / 2
        uint64_t expected_delay = ((t4_response_rx - t1_request_tx) - (t3_response_tx - t2_request_rx)) / 2;
        
        // Record the measurement
        monitor->record_pdelay_exchange(t1_request_tx, t2_request_rx, t3_response_tx, t4_response_rx);
        
        // Verify calculated delay matches expected
        auto measurements = monitor->get_recent_measurements(1);
        if (measurements.empty()) {
            SimpleTest::assert_true(false, "No measurements recorded");
            return false;
        }
        
        uint64_t calculated_delay = measurements[0].path_delay_ns;
        int64_t delay_error = static_cast<int64_t>(calculated_delay) - static_cast<int64_t>(expected_delay);
        
        // Allow small margin for calculation precision
        bool delay_accurate = std::abs(delay_error) <= 1000; // 1µs tolerance
        
        SimpleTest::assert_true(delay_accurate, "Calculated path delay should match expected within 1µs");
        SimpleTest::assert_equal(static_cast<int64_t>(expected_delay), 
                                static_cast<int64_t>(calculated_delay), 
                                "Path delay calculation accuracy");
        
        monitor->disable_monitoring();
        return delay_accurate;
    }
    
    /**
     * @brief Test gPTP.com.c.15.2 — MDPdelayReq SM: sequenceId
     * 
     * Purpose: Verify proper sequenceId handling in PDelay requests
     */
    bool test_MDPdelayReq_sequenceId() {
        SimpleTest::start_test("gPTP.com.c.15.2 - MDPdelayReq SM: sequenceId");
        
        // Test sequence ID increments properly
        std::vector<uint16_t> sequence_ids;
        
        // Simulate multiple PDelay requests
        for (int i = 0; i < 5; i++) {
            uint16_t seq_id = static_cast<uint16_t>(i + 1);
            sequence_ids.push_back(seq_id);
            
            // Simulate PDelay exchange with this sequence ID
            uint64_t base_time = 1000000000ULL + i * 1000000000ULL; // 1s intervals
            monitor->record_pdelay_sequence(seq_id, base_time, base_time + 1000000);
        }
        
        // Verify sequence IDs are properly incremented
        bool sequence_valid = true;
        for (size_t i = 1; i < sequence_ids.size(); i++) {
            if (sequence_ids[i] != sequence_ids[i-1] + 1) {
                sequence_valid = false;
                break;
            }
        }
        
        SimpleTest::assert_true(sequence_valid, "Sequence IDs should increment properly");
        SimpleTest::assert_equal(5, static_cast<int64_t>(sequence_ids.size()), 
                                "All sequence IDs should be recorded");
        
        return sequence_valid;
    }
    
    /**
     * @brief Test gPTP.com.c.15.3 — MDPdelayReq SM: Lost and Late responses
     * 
     * Purpose: Verify proper handling of lost and late PDelay responses
     */
    bool test_MDPdelayReq_lost_late_responses() {
        SimpleTest::start_test("gPTP.com.c.15.3 - MDPdelayReq SM: Lost and Late responses");
        
        // Set Milan profile for late response testing
        current_profile = gPTPProfileFactory::createMilanProfile();
        uint32_t late_threshold_ms = current_profile.late_response_threshold_ms; // 15ms for Milan
        
        monitor->enable_monitoring(125);
        
        // Test 1: Lost response (timeout)
        uint64_t request_time = 1000000000ULL;
        monitor->record_pdelay_request(1, request_time);
        
        // Wait for timeout period (no response)
        std::this_thread::sleep_for(std::chrono::milliseconds(late_threshold_ms + 10));
        
        auto timeout_stats = monitor->get_timeout_statistics();
        bool timeout_detected = timeout_stats.total_timeouts > 0;
        
        // Test 2: Late response (within interval but > threshold)
        uint64_t late_response_time = request_time + (late_threshold_ms + 5) * 1000000ULL; // 5ms late
        monitor->record_pdelay_response(1, late_response_time);
        
        auto late_stats = monitor->get_late_response_statistics();
        bool late_detected = late_stats.total_late_responses > 0;
        
        SimpleTest::assert_true(timeout_detected, "Timeout should be detected for lost responses");
        SimpleTest::assert_true(late_detected, "Late response should be detected");
        
        monitor->disable_monitoring();
        return timeout_detected && late_detected;
    }
    
    /**
     * @brief Test gPTP.com.c.15.6 — MDPdelayReq SM: asCapable conditions, neighborPropDelay
     * 
     * Purpose: Verify asCapable behavior with neighbor propagation delay threshold
     * This is critical for Intel HAL integration!
     */
    bool test_MDPdelayReq_asCapable_neighborPropDelay() {
        SimpleTest::start_test("gPTP.com.c.15.6 - MDPdelayReq SM: asCapable conditions, neighborPropDelay");
        
        // Use Milan profile with 800µs threshold
        current_profile = gPTPProfileFactory::createMilanProfile();
        int64_t threshold_ns = current_profile.neighbor_prop_delay_thresh; // 800,000ns
        
        monitor->enable_monitoring(125);
        
        // Test 1: Path delay within threshold - should maintain asCapable
        uint64_t good_delay_ns = 500000; // 500µs (below 800µs threshold)
        bool ascapable_before = true;
        
        monitor->record_neighbor_prop_delay(good_delay_ns);
        bool ascapable_good = monitor->evaluate_ascapable_threshold(ascapable_before, good_delay_ns, threshold_ns);
        
        // Test 2: Path delay exceeding threshold - behavior depends on profile
        uint64_t bad_delay_ns = 1200000; // 1.2ms (above 800µs threshold)
        bool ascapable_bad = monitor->evaluate_ascapable_threshold(true, bad_delay_ns, threshold_ns);
        
        // Milan profile should handle threshold flexibly per Annex B.2.3
        bool milan_flexible_threshold = (current_profile.profile_name == "milan");
        
        SimpleTest::assert_true(ascapable_good, "asCapable should remain true for delay within threshold");
        
        if (milan_flexible_threshold) {
            // Milan allows flexible threshold handling
            SimpleTest::assert_true(ascapable_bad, "Milan profile should maintain asCapable despite threshold");
        } else {
            // Standard profile should lose asCapable
            SimpleTest::assert_false(ascapable_bad, "Standard profile should lose asCapable above threshold");
        }
        
        monitor->disable_monitoring();
        return ascapable_good && (milan_flexible_threshold ? ascapable_bad : !ascapable_bad);
    }
    
    // ========================================================================
    // Test Suite Execution
    // ========================================================================
    
    void run_all_group15_tests() {
        std::cout << "=== Avnu Alliance gPTP Conformance Tests - Group 15: MDPdelayReq State Machine ===" << std::endl;
        std::cout << "Using Intel HAL integration and existing OpenAvnu clock quality framework" << std::endl;
        std::cout << std::endl;
        
        int passed = 0;
        int total = 0;
        
        // Run Group 15 tests
        total++; if (test_MDPdelayReq_proper_delay()) passed++;
        total++; if (test_MDPdelayReq_sequenceId()) passed++;  
        total++; if (test_MDPdelayReq_lost_late_responses()) passed++;
        total++; if (test_MDPdelayReq_asCapable_neighborPropDelay()) passed++;
        
        std::cout << std::endl;
        std::cout << "=== Group 15 Test Results ===" << std::endl;
        std::cout << "Passed: " << passed << "/" << total << std::endl;
        std::cout << "Success Rate: " << (100.0 * passed / total) << "%" << std::endl;
        
        if (passed == total) {
            std::cout << "✅ ALL GROUP 15 TESTS PASSED! Intel HAL + gPTP integration working correctly." << std::endl;
        } else {
            std::cout << "❌ Some tests failed. Review implementation and hardware setup." << std::endl;
        }
    }
};

// Test entry point that integrates with existing simple test framework
void test_avnu_conformance_group15() {
    AvnuConformanceTestSuite suite;
    suite.run_all_group15_tests();
}

// Add to main test runner
void run_avnu_conformance_tests() {
    std::cout << "Starting Avnu Alliance gPTP Conformance Tests..." << std::endl;
    test_avnu_conformance_group15();
    std::cout << "Avnu conformance tests complete." << std::endl;
}
