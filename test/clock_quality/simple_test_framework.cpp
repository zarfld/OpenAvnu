// Simple testing framework for clock quality tests
#ifndef SIMPLE_TEST_FRAMEWORK_H
#define SIMPLE_TEST_FRAMEWORK_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <cmath>

class SimpleTest {
public:
    static int total_tests;
    static int passed_tests;
    static int failed_tests;
    static std::vector<std::string> failures;

    static void assert_true(bool condition, const std::string& message) {
        total_tests++;
        if (condition) {
            passed_tests++;
            std::cout << "[PASS] " << message << std::endl;
        } else {
            failed_tests++;
            failures.push_back(message);
            std::cout << "[FAIL] " << message << std::endl;
        }
    }

    static void assert_false(bool condition, const std::string& message) {
        assert_true(!condition, message);
    }

    static void assert_equal(int64_t expected, int64_t actual, const std::string& message) {
        bool condition = (expected == actual);
        std::string full_message = message + " (expected: " + std::to_string(expected) + ", actual: " + std::to_string(actual) + ")";
        assert_true(condition, full_message);
    }

    static void assert_near(double expected, double actual, double tolerance, const std::string& message) {
        bool condition = std::abs(expected - actual) <= tolerance;
        std::string full_message = message + " (expected: " + std::to_string(expected) + ", actual: " + std::to_string(actual) + ", tolerance: " + std::to_string(tolerance) + ")";
        assert_true(condition, full_message);
    }

    static void assert_greater(int64_t value, int64_t threshold, const std::string& message) {
        bool condition = value > threshold;
        std::string full_message = message + " (" + std::to_string(value) + " > " + std::to_string(threshold) + ")";
        assert_true(condition, full_message);
    }

    static void assert_less(int64_t value, int64_t threshold, const std::string& message) {
        bool condition = value < threshold;
        std::string full_message = message + " (" + std::to_string(value) + " < " + std::to_string(threshold) + ")";
        assert_true(condition, full_message);
    }

    static void run_test(const std::string& test_name, std::function<void()> test_func) {
        std::cout << "\n=== Running " << test_name << " ===" << std::endl;
        try {
            test_func();
        } catch (const std::exception& e) {
            failed_tests++;
            failures.push_back(test_name + " threw exception: " + e.what());
            std::cout << "[EXCEPTION] " << test_name << ": " << e.what() << std::endl;
        }
    }

    static void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;
        
        if (!failures.empty()) {
            std::cout << "\nFailures:" << std::endl;
            for (const auto& failure : failures) {
                std::cout << "  - " << failure << std::endl;
            }
        }
        
        std::cout << "\nResult: " << (failed_tests == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    }

    static int get_exit_code() {
        return failed_tests == 0 ? 0 : 1;
    }
};

#endif // SIMPLE_TEST_FRAMEWORK_H
