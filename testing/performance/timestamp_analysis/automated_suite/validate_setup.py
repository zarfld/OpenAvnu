#!/usr/bin/env python3
"""
Quick validation script to test the I219 timestamp analysis tools.
Performs basic functionality testing and setup validation.
"""

import os
import sys
import subprocess
import platform
from pathlib import Path

def check_python_dependencies():
    """Check if required Python packages are available"""
    required_packages = ['matplotlib', 'numpy', 'psutil']
    missing_packages = []
    
    for package in required_packages:
        try:
            __import__(package)
            print(f"✓ {package} available")
        except ImportError:
            missing_packages.append(package)
            print(f"✗ {package} missing")
    
    if missing_packages:
        print(f"\nTo install missing packages:")
        print(f"pip install {' '.join(missing_packages)}")
        return False
    
    return True

def check_gptp_build():
    """Check if gPTP build is available"""
    possible_paths = [
        'build-intel-hal/thirdparty/gptp',
        'build/thirdparty/gptp',
        '../../build-intel-hal/thirdparty/gptp',
        '../../build/thirdparty/gptp'
    ]
    
    for path in possible_paths:
        full_path = os.path.abspath(path)
        if os.path.exists(full_path):
            print(f"✓ gPTP build found: {full_path}")
            return True
    
    print("✗ gPTP build not found")
    print("Expected locations:")
    for path in possible_paths:
        print(f"  - {os.path.abspath(path)}")
    return False

def check_network_interface():
    """Check for available network interfaces"""
    try:
        import psutil
        interfaces = psutil.net_if_addrs()
        
        print(f"✓ Available network interfaces:")
        for name, addrs in interfaces.items():
            for addr in addrs:
                if addr.family == psutil.AF_LINK:  # MAC address
                    print(f"  - {name}: {addr.address}")
        
        return len(interfaces) > 0
    except Exception as e:
        print(f"✗ Error checking network interfaces: {e}")
        return False

def test_basic_functionality():
    """Test basic functionality of analysis tools"""
    print("\nTesting basic tool functionality...")
    
    # Test timestamp_analyzer import
    try:
        sys.path.insert(0, os.path.dirname(__file__))
        from timestamp_analyzer import TimestampAnalyzer
        analyzer = TimestampAnalyzer()
        print("✓ TimestampAnalyzer import successful")
    except Exception as e:
        print(f"✗ TimestampAnalyzer import failed: {e}")
        return False
    
    # Test performance_benchmark import
    try:
        from performance_benchmark import PerformanceBenchmark, BenchmarkConfig
        config = BenchmarkConfig(test_duration=1)
        benchmark = PerformanceBenchmark(config)
        print("✓ PerformanceBenchmark import successful")
    except Exception as e:
        print(f"✗ PerformanceBenchmark import failed: {e}")
        return False
    
    # Test automated_test_suite import
    try:
        from automated_test_suite import I219TestSuite
        suite = I219TestSuite()
        print("✓ I219TestSuite import successful")
    except Exception as e:
        print(f"✗ I219TestSuite import failed: {e}")
        return False
    
    return True

def run_quick_test():
    """Run a quick test of the timestamp analyzer"""
    print("\nRunning quick timestamp analyzer test...")
    
    try:
        sys.path.insert(0, os.path.dirname(__file__))
        from timestamp_analyzer import TimestampAnalyzer
        
        analyzer = TimestampAnalyzer()
        
        # Test log parsing
        test_line = "sync message: offset = 1500.5 ns, sequence = 123"
        reading = analyzer.parse_log_line(test_line)
        
        if reading and reading.sync_offset == 1500.5:
            print("✓ Log parsing working correctly")
            return True
        else:
            print("✗ Log parsing test failed")
            return False
            
    except Exception as e:
        print(f"✗ Quick test failed: {e}")
        return False

def main():
    print("Intel I219-LM Timestamp Analysis Tools - Setup Validation")
    print("=" * 60)
    
    all_checks_passed = True
    
    # Check Python dependencies
    print("\n1. Checking Python Dependencies")
    if not check_python_dependencies():
        all_checks_passed = False
    
    # Check gPTP build
    print("\n2. Checking gPTP Build")
    if not check_gptp_build():
        all_checks_passed = False
        print("Note: Tools will work but automated tests may be limited")
    
    # Check network interfaces
    print("\n3. Checking Network Interfaces")
    if not check_network_interface():
        all_checks_passed = False
    
    # Test basic functionality
    print("\n4. Testing Basic Functionality")
    if not test_basic_functionality():
        all_checks_passed = False
    
    # Run quick test
    print("\n5. Quick Functionality Test")
    if not run_quick_test():
        all_checks_passed = False
    
    # Summary
    print("\n" + "=" * 60)
    if all_checks_passed:
        print("✓ Setup validation PASSED")
        print("\nTools are ready to use!")
        print("\nNext steps:")
        print("1. Run automated test suite: python automated_test_suite.py")
        print("2. Perform baseline benchmark: python performance_benchmark.py")
        print("3. Monitor real-time performance: python timestamp_analyzer.py")
    else:
        print("✗ Setup validation FAILED")
        print("\nPlease address the issues above before using the tools.")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
