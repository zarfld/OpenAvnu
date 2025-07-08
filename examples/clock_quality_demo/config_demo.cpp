// Clock Quality Configuration Demo
// OpenAvnu gPTP Clock Quality Testing Framework

#include "gptp_clock_quality_config.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "OpenAvnu gPTP Clock Quality Configuration Demo" << std::endl;
    std::cout << "Based on Avnu Alliance '802.1AS Recovered Clock Quality Testing v1.0'" << std::endl;
    std::cout << "=================================================================" << std::endl;

    // Get the configuration manager instance
    auto& config_manager = OpenAvnu::gPTP::ClockQualityConfigManager::get_instance();
    
    std::cout << "\n=== Default Configuration ===" << std::endl;
    config_manager.print_config_summary();
    
    // Try to load configuration from file
    std::cout << "\n=== Loading Configuration from File ===" << std::endl;
    const std::string config_file = "clock_quality_config.ini";
    
    if (config_manager.load_config_file(config_file)) {
        std::cout << "Successfully loaded configuration from: " << config_file << std::endl;
    } else {
        std::cout << "Could not load " << config_file << ", using defaults" << std::endl;
    }
    
    std::cout << "\n=== Updated Configuration ===" << std::endl;
    config_manager.print_config_summary();
    
    // Validate configuration
    std::cout << "\n=== Configuration Validation ===" << std::endl;
    if (config_manager.validate_config()) {
        std::cout << "Configuration is valid!" << std::endl;
    } else {
        std::cout << "Configuration has errors:" << std::endl;
        auto errors = config_manager.get_validation_errors();
        for (const auto& error : errors) {
            std::cout << "  - " << error << std::endl;
        }
    }
    
    // Test profile-specific configurations
    std::cout << "\n=== Profile-Specific Configurations ===" << std::endl;
    
    auto milan_config = config_manager.get_profile_config("Milan");
    std::cout << "Milan Profile:" << std::endl;
    std::cout << "  Accuracy: ±" << milan_config.accuracy_requirement_ns << "ns" << std::endl;
    std::cout << "  Lock time: ≤" << milan_config.max_lock_time_seconds << "s" << std::endl;
    std::cout << "  Late response: " << milan_config.late_response_threshold_ms << "ms" << std::endl;
    
    auto automotive_config = config_manager.get_profile_config("Automotive");
    std::cout << "\nAutomotive Profile:" << std::endl;
    std::cout << "  Accuracy: ±" << automotive_config.accuracy_requirement_ns << "ns" << std::endl;
    std::cout << "  Lock time: ≤" << automotive_config.max_lock_time_seconds << "s" << std::endl;
    std::cout << "  Immediate asCapable: " << (automotive_config.immediate_ascapable_required ? "Yes" : "No") << std::endl;
    
    auto base_config = config_manager.get_profile_config("AVnu_Base");
    std::cout << "\nAVnu Base Profile:" << std::endl;
    std::cout << "  Accuracy: ±" << base_config.accuracy_requirement_ns << "ns" << std::endl;
    std::cout << "  Lock time: ≤" << base_config.max_lock_time_seconds << "s" << std::endl;
    std::cout << "  PDelay successes: " << base_config.min_pdelay_successes 
              << " to " << base_config.max_pdelay_successes << std::endl;
    
    // Test runtime configuration updates
    std::cout << "\n=== Runtime Configuration Updates ===" << std::endl;
    
    std::cout << "Original measurement interval: " << config_manager.get_config().measurement_interval_ms << "ms" << std::endl;
    config_manager.update_measurement_interval(250);
    std::cout << "Updated measurement interval: " << config_manager.get_config().measurement_interval_ms << "ms" << std::endl;
    
    std::cout << "Original analysis window: " << config_manager.get_config().analysis_window_seconds << "s" << std::endl;
    config_manager.update_analysis_window(600);
    std::cout << "Updated analysis window: " << config_manager.get_config().analysis_window_seconds << "s" << std::endl;
    
    // Test measurement method conversion
    std::cout << "\n=== Measurement Method Testing ===" << std::endl;
    
    auto methods = {
        OpenAvnu::gPTP::MeasurementMethod::INGRESS_REPORTING,
        OpenAvnu::gPTP::MeasurementMethod::REVERSE_SYNC,
        OpenAvnu::gPTP::MeasurementMethod::PPS_HARDWARE,
        OpenAvnu::gPTP::MeasurementMethod::COMBINED
    };
    
    for (auto method : methods) {
        std::string method_str = OpenAvnu::gPTP::ClockQualityConfigManager::method_to_string(method);
        auto parsed_method = OpenAvnu::gPTP::ClockQualityConfigManager::string_to_method(method_str);
        std::cout << "Method: " << method_str << " (roundtrip: " 
                  << (method == parsed_method ? "OK" : "FAIL") << ")" << std::endl;
    }
    
    // Test environment variable override
    std::cout << "\n=== Environment Variable Testing ===" << std::endl;
    std::cout << "Set GPTP_CLOCK_QUALITY_METHOD=reverse_sync to test environment override" << std::endl;
    std::cout << "Set GPTP_CLOCK_QUALITY_INTERVAL=1000 to test interval override" << std::endl;
    
    if (config_manager.load_config_from_environment()) {
        std::cout << "Found environment configuration overrides" << std::endl;
        config_manager.print_config_summary();
    } else {
        std::cout << "No environment configuration found" << std::endl;
    }
    
    // Test configuration save
    std::cout << "\n=== Configuration Persistence ===" << std::endl;
    std::string save_path = "clock_quality_config_saved.ini";
    if (config_manager.save_config_file(save_path)) {
        std::cout << "Configuration saved to: " << save_path << std::endl;
    } else {
        std::cout << "Failed to save configuration" << std::endl;
    }
    
    std::cout << "\n=== Demo Completed Successfully ===" << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "1. Test with different configuration files" << std::endl;
    std::cout << "2. Integrate with actual gPTP message processing" << std::endl;
    std::cout << "3. Add configuration validation for hardware-specific settings" << std::endl;
    std::cout << "4. Implement configuration change notifications" << std::endl;
    
    return 0;
}
