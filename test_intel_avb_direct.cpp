#include <iostream>
#include <windows.h>
#include "lib/intel_avb/intelavb.h"

int main() {
    std::cout << "🔧 Testing intel_avb functions directly..." << std::endl;
    
    // Test intel_probe()
    std::cout << "🔧 Calling intel_probe()..." << std::endl;
    int result = intel_probe();
    std::cout << "🔧 intel_probe() returned: " << result << std::endl;
    
    if (result > 0) {
        std::cout << "✅ Found " << result << " Intel devices!" << std::endl;
        
        // Test intel_gettime()
        std::cout << "🔧 Calling intel_gettime()..." << std::endl;
        struct timespec ts;
        if (intel_gettime(&ts) == 0) {
            std::cout << "✅ intel_gettime() success: " << ts.tv_sec << "." << ts.tv_nsec << std::endl;
        } else {
            std::cout << "❌ intel_gettime() failed" << std::endl;
        }
    } else {
        std::cout << "❌ intel_probe() failed - no devices found" << std::endl;
    }
    
    return 0;
}
