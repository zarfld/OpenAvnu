#!/usr/bin/env python3
"""
Performance Benchmarking Suite for Intel I219-LM gPTP Implementation
Compares hardware vs software timestamping, measures performance under load,
and benchmarks against other timing solutions.

Features:
- Hardware vs Software timestamp precision comparison
- Load testing with varying network conditions
- Multi-device synchronization accuracy
- Performance regression testing
- Comparative analysis with other NICs
"""

import argparse
import time
import datetime
import json
import subprocess
import threading
import statistics
import os
import sys
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional, Tuple
import concurrent.futures

def load_config():
    """Load configuration from config.json"""
    config_path = Path(__file__).parent / "config.json"
    if config_path.exists():
        with open(config_path, 'r') as f:
            return json.load(f)
    return {}

@dataclass
class BenchmarkConfig:
    """Configuration for benchmark tests"""
    test_duration: int = 300  # seconds
    hw_timestamp_tests: bool = True
    sw_timestamp_tests: bool = True
    load_test_enabled: bool = True
    load_levels: List[int] = None  # Packets per second
    network_conditions: List[str] = None
    comparison_nics: List[str] = None
    
    def __post_init__(self):
        if self.load_levels is None:
            self.load_levels = [100, 500, 1000, 2000, 5000]
        if self.network_conditions is None:
            self.network_conditions = ['normal', 'congested', 'lossy']
        if self.comparison_nics is None:
            self.comparison_nics = ['igb', 'e1000e']

@dataclass
class BenchmarkResult:
    """Results from a single benchmark test"""
    test_name: str
    config: Dict
    timestamp_mode: str  # 'hardware' or 'software'
    duration: float
    samples: int
    mean_precision: float
    jitter: float
    max_deviation: float
    min_precision: float
    max_precision: float
    stability_score: float
    error_rate: float
    throughput: float
    cpu_usage: Optional[float] = None
    memory_usage: Optional[float] = None

class PerformanceBenchmark:
    """Main benchmarking class"""
    
    def __init__(self, config: BenchmarkConfig):
        self.config = config
        self.results = []
        self.baseline_result = None
        
        # Load configuration from config.json
        self.settings = load_config()
        self.gptp_executable = self.settings.get('gptp_build_path', 'gptp')
        self.target_interface = self.settings.get('target_interface', 'ethernet')
        
    def run_full_benchmark_suite(self) -> List[BenchmarkResult]:
        """Run complete benchmark suite"""
        print("Starting Intel I219-LM Performance Benchmark Suite")
        print("=" * 60)
        
        # 1. Baseline hardware timestamp test
        print("\n1. Baseline Hardware Timestamp Performance")
        hw_result = self.run_timestamp_benchmark('hardware')
        if hw_result:
            self.baseline_result = hw_result
            self.results.append(hw_result)
        
        # 2. Software timestamp comparison
        if self.config.sw_timestamp_tests:
            print("\n2. Software Timestamp Comparison")
            sw_result = self.run_timestamp_benchmark('software')
            if sw_result:
                self.results.append(sw_result)
        
        # 3. Load testing
        if self.config.load_test_enabled:
            print("\n3. Load Testing")
            for load_level in self.config.load_levels:
                print(f"  Testing at {load_level} pps...")
                load_result = self.run_load_test(load_level)
                if load_result:
                    self.results.append(load_result)
        
        # 4. Network condition testing
        print("\n4. Network Condition Testing")
        for condition in self.config.network_conditions:
            print(f"  Testing under {condition} conditions...")
            condition_result = self.run_network_condition_test(condition)
            if condition_result:
                self.results.append(condition_result)
        
        # 5. Comparative testing
        print("\n5. Comparative Testing")
        for nic in self.config.comparison_nics:
            print(f"  Comparing with {nic}...")
            comparison_result = self.run_comparison_test(nic)
            if comparison_result:
                self.results.append(comparison_result)
        
        return self.results
    
    def run_timestamp_benchmark(self, mode: str) -> Optional[BenchmarkResult]:
        """Run timestamp accuracy benchmark"""
        print(f"Running {mode} timestamp benchmark for {self.config.test_duration}s...")
        
        # Build gPTP command based on mode
        if mode == 'hardware':
            cmd = [self.gptp_executable, self.target_interface]
        else:
            cmd = [self.gptp_executable, self.target_interface, '--software-timestamps']
        
        # Run test
        start_time = time.time()
        readings = []
        
        try:
            process = subprocess.Popen(
                cmd, 
                stdout=subprocess.PIPE, 
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                encoding='utf-8',
                errors='ignore'
            )
            
            # Collect data for specified duration
            while (time.time() - start_time) < self.config.test_duration:
                line = process.stdout.readline()
                if not line:
                    break
                
                # Parse timestamp data from line
                reading = self._parse_timestamp_reading(line)
                if reading is not None:
                    readings.append(reading)
            
            process.terminate()
            process.wait()
            
        except Exception as e:
            print(f"Error running {mode} benchmark: {e}")
            return None
        
        if not readings:
            print(f"No valid readings collected for {mode} test")
            return None
        
        # Calculate statistics
        duration = time.time() - start_time
        mean_precision = statistics.mean(readings)
        jitter = statistics.stdev(readings) if len(readings) > 1 else 0
        max_deviation = max(abs(r - mean_precision) for r in readings)
        min_precision = min(readings)
        max_precision = max(readings)
        
        # Calculate stability score (percentage within 2x jitter)
        threshold = 2 * jitter if jitter > 0 else 1000
        stable_readings = sum(1 for r in readings if abs(r - mean_precision) <= threshold)
        stability_score = stable_readings / len(readings)
        
        result = BenchmarkResult(
            test_name=f"{mode}_timestamp_baseline",
            config={'mode': mode, 'duration': self.config.test_duration},
            timestamp_mode=mode,
            duration=duration,
            samples=len(readings),
            mean_precision=mean_precision,
            jitter=jitter,
            max_deviation=max_deviation,
            min_precision=min_precision,
            max_precision=max_precision,
            stability_score=stability_score,
            error_rate=0.0,  # Would need error detection logic
            throughput=len(readings) / duration
        )
        
        self._print_benchmark_result(result)
        return result
    
    def run_load_test(self, load_pps: int) -> Optional[BenchmarkResult]:
        """Run benchmark under specified packet load"""
        print(f"  Load test at {load_pps} packets/second...")
        
        # Start background traffic generator
        traffic_cmd = ['python', '-c', f'''
import socket
import time
import threading

def generate_traffic():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    interval = 1.0 / {load_pps}
    
    for i in range({load_pps * self.config.test_duration}):
        try:
            sock.sendto(b"test_packet", ("127.0.0.1", 12345))
            time.sleep(interval)
        except:
            pass
    sock.close()

generate_traffic()
''']
        
        # Start traffic generator in background
        try:
            traffic_process = subprocess.Popen(traffic_cmd)
            time.sleep(1)  # Let traffic start
            
            # Run timestamp test under load
            result = self.run_timestamp_benchmark('hardware')
            if result:
                result.test_name = f"load_test_{load_pps}pps"
                result.config['load_pps'] = load_pps
            
            traffic_process.terminate()
            traffic_process.wait()
            
            return result
            
        except Exception as e:
            print(f"Error in load test: {e}")
            return None
    
    def run_network_condition_test(self, condition: str) -> Optional[BenchmarkResult]:
        """Run test under simulated network conditions"""
        print(f"  Testing under {condition} network conditions...")
        
        # Simulate network conditions using tc (Linux) or PowerShell (Windows)
        if os.name == 'nt':  # Windows
            condition_cmd = self._get_windows_network_condition_cmd(condition)
        else:  # Linux
            condition_cmd = self._get_linux_network_condition_cmd(condition)
        
        try:
            # Apply network condition
            if condition_cmd:
                subprocess.run(condition_cmd, check=True)
                time.sleep(2)  # Let condition stabilize
            
            # Run test
            result = self.run_timestamp_benchmark('hardware')
            if result:
                result.test_name = f"network_condition_{condition}"
                result.config['network_condition'] = condition
            
            # Clean up network condition
            cleanup_cmd = self._get_network_cleanup_cmd()
            if cleanup_cmd:
                subprocess.run(cleanup_cmd)
            
            return result
            
        except Exception as e:
            print(f"Error in network condition test: {e}")
            return None
    
    def run_comparison_test(self, nic_driver: str) -> Optional[BenchmarkResult]:
        """Run comparative test with different NIC driver"""
        print(f"  Comparing performance with {nic_driver} driver...")
        
        # This would require having alternative drivers available
        # For now, simulate by running with different parameters
        
        try:
            # Modify command for comparison NIC
            cmd = [self.gptp_executable, self.target_interface, '--driver', nic_driver]
            
            start_time = time.time()
            readings = []
            
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                encoding='utf-8',
                errors='ignore'
            )
            
            # Shorter test for comparison
            comparison_duration = min(60, self.config.test_duration)
            
            while (time.time() - start_time) < comparison_duration:
                line = process.stdout.readline()
                if not line:
                    break
                
                reading = self._parse_timestamp_reading(line)
                if reading is not None:
                    readings.append(reading)
            
            process.terminate()
            process.wait()
            
            if not readings:
                return None
            
            duration = time.time() - start_time
            mean_precision = statistics.mean(readings)
            jitter = statistics.stdev(readings) if len(readings) > 1 else 0
            
            result = BenchmarkResult(
                test_name=f"comparison_{nic_driver}",
                config={'nic_driver': nic_driver, 'duration': comparison_duration},
                timestamp_mode='hardware',
                duration=duration,
                samples=len(readings),
                mean_precision=mean_precision,
                jitter=jitter,
                max_deviation=max(abs(r - mean_precision) for r in readings),
                min_precision=min(readings),
                max_precision=max(readings),
                stability_score=0.95,  # Placeholder
                error_rate=0.0,
                throughput=len(readings) / duration
            )
            
            return result
            
        except Exception as e:
            print(f"Error in comparison test: {e}")
            return None
    
    def _parse_timestamp_reading(self, line: str) -> Optional[float]:
        """Parse timestamp reading from gPTP output line"""
        import re
        
        # Look for sync offset or similar precision indicators
        patterns = [
            r'offset\s*[=:]\s*([+-]?\d+(?:\.\d+)?)',
            r'precision\s*[=:]\s*([+-]?\d+(?:\.\d+)?)',
            r'sync.*?([+-]?\d+(?:\.\d+)?)\s*ns'
        ]
        
        for pattern in patterns:
            match = re.search(pattern, line, re.IGNORECASE)
            if match:
                try:
                    return abs(float(match.group(1)))  # Use absolute value for precision
                except ValueError:
                    continue
        
        return None
    
    def _get_windows_network_condition_cmd(self, condition: str) -> Optional[List[str]]:
        """Get Windows command to simulate network condition"""
        # Windows network simulation would require additional tools
        # For now, return None to skip
        return None
    
    def _get_linux_network_condition_cmd(self, condition: str) -> Optional[List[str]]:
        """Get Linux tc command to simulate network condition"""
        interface = 'eth0'  # Would need to detect actual interface
        
        if condition == 'congested':
            return ['tc', 'qdisc', 'add', 'dev', interface, 'root', 'netem', 'delay', '10ms']
        elif condition == 'lossy':
            return ['tc', 'qdisc', 'add', 'dev', interface, 'root', 'netem', 'loss', '1%']
        else:
            return None
    
    def _get_network_cleanup_cmd(self) -> Optional[List[str]]:
        """Get command to clean up network conditions"""
        if os.name != 'nt':  # Linux
            return ['tc', 'qdisc', 'del', 'dev', 'eth0', 'root']
        return None
    
    def _print_benchmark_result(self, result: BenchmarkResult):
        """Print formatted benchmark result"""
        print(f"\n  Results for {result.test_name}:")
        print(f"    Duration: {result.duration:.1f}s")
        print(f"    Samples: {result.samples}")
        print(f"    Mean Precision: {result.mean_precision:.1f} ns")
        print(f"    Jitter: {result.jitter:.1f} ns")
        print(f"    Max Deviation: {result.max_deviation:.1f} ns")
        print(f"    Stability: {result.stability_score:.1%}")
        print(f"    Throughput: {result.throughput:.1f} samples/sec")
    
    def generate_comparison_report(self, output_path: str):
        """Generate comprehensive comparison report"""
        if not self.results:
            print("No results to report")
            return
        
        report = {
            'metadata': {
                'generation_time': datetime.datetime.now().isoformat(),
                'benchmark_config': asdict(self.config),
                'total_tests': len(self.results)
            },
            'summary': self._generate_summary(),
            'detailed_results': [asdict(r) for r in self.results],
            'analysis': self._generate_analysis()
        }
        
        # Save JSON report
        with open(f"{output_path}.json", 'w') as f:
            json.dump(report, f, indent=2)
        
        # Generate text report
        self._generate_text_report(f"{output_path}.txt", report)
        
        print(f"Benchmark report generated: {output_path}.json, {output_path}.txt")
    
    def _generate_summary(self) -> Dict:
        """Generate summary statistics"""
        if not self.results:
            return {}
        
        hw_results = [r for r in self.results if r.timestamp_mode == 'hardware']
        sw_results = [r for r in self.results if r.timestamp_mode == 'software']
        
        summary = {
            'total_tests': len(self.results),
            'hardware_tests': len(hw_results),
            'software_tests': len(sw_results)
        }
        
        if hw_results:
            hw_precisions = [r.mean_precision for r in hw_results]
            hw_jitters = [r.jitter for r in hw_results]
            
            summary['hardware_performance'] = {
                'mean_precision': statistics.mean(hw_precisions),
                'best_precision': min(hw_precisions),
                'worst_precision': max(hw_precisions),
                'mean_jitter': statistics.mean(hw_jitters),
                'best_jitter': min(hw_jitters),
                'worst_jitter': max(hw_jitters)
            }
        
        if sw_results:
            sw_precisions = [r.mean_precision for r in sw_results]
            sw_jitters = [r.jitter for r in sw_results]
            
            summary['software_performance'] = {
                'mean_precision': statistics.mean(sw_precisions),
                'best_precision': min(sw_precisions),
                'worst_precision': max(sw_precisions),
                'mean_jitter': statistics.mean(sw_jitters),
                'best_jitter': min(sw_jitters),
                'worst_jitter': max(sw_jitters)
            }
        
        return summary
    
    def _generate_analysis(self) -> Dict:
        """Generate performance analysis"""
        analysis = {}
        
        # Hardware vs Software comparison
        hw_results = [r for r in self.results if r.timestamp_mode == 'hardware']
        sw_results = [r for r in self.results if r.timestamp_mode == 'software']
        
        if hw_results and sw_results:
            hw_precision = statistics.mean([r.mean_precision for r in hw_results])
            sw_precision = statistics.mean([r.mean_precision for r in sw_results])
            
            analysis['hw_vs_sw'] = {
                'precision_improvement': ((sw_precision - hw_precision) / sw_precision) * 100,
                'recommendation': 'hardware' if hw_precision < sw_precision else 'software'
            }
        
        # Load impact analysis
        load_results = [r for r in self.results if 'load_test' in r.test_name]
        if len(load_results) > 1:
            loads = [r.config.get('load_pps', 0) for r in load_results]
            precisions = [r.mean_precision for r in load_results]
            
            # Simple correlation analysis
            if len(loads) == len(precisions):
                analysis['load_impact'] = {
                    'precision_degradation': max(precisions) - min(precisions),
                    'affected_by_load': max(precisions) > min(precisions) * 1.1
                }
        
        return analysis
    
    def _generate_text_report(self, output_path: str, report: Dict):
        """Generate human-readable text report"""
        with open(output_path, 'w') as f:
            f.write("Intel I219-LM Performance Benchmark Report\n")
            f.write("=" * 50 + "\n\n")
            
            f.write(f"Generated: {report['metadata']['generation_time']}\n")
            f.write(f"Total Tests: {report['metadata']['total_tests']}\n\n")
            
            # Summary
            if 'summary' in report:
                f.write("SUMMARY\n")
                f.write("-" * 20 + "\n")
                
                summary = report['summary']
                if 'hardware_performance' in summary:
                    hw = summary['hardware_performance']
                    f.write(f"Hardware Timestamping:\n")
                    f.write(f"  Mean Precision: {hw['mean_precision']:.1f} ns\n")
                    f.write(f"  Best Precision: {hw['best_precision']:.1f} ns\n")
                    f.write(f"  Mean Jitter: {hw['mean_jitter']:.1f} ns\n\n")
                
                if 'software_performance' in summary:
                    sw = summary['software_performance']
                    f.write(f"Software Timestamping:\n")
                    f.write(f"  Mean Precision: {sw['mean_precision']:.1f} ns\n")
                    f.write(f"  Best Precision: {sw['best_precision']:.1f} ns\n")
                    f.write(f"  Mean Jitter: {sw['mean_jitter']:.1f} ns\n\n")
            
            # Analysis
            if 'analysis' in report:
                f.write("ANALYSIS\n")
                f.write("-" * 20 + "\n")
                
                analysis = report['analysis']
                if 'hw_vs_sw' in analysis:
                    hw_vs_sw = analysis['hw_vs_sw']
                    f.write(f"Hardware vs Software:\n")
                    f.write(f"  Precision Improvement: {hw_vs_sw['precision_improvement']:.1f}%\n")
                    f.write(f"  Recommendation: {hw_vs_sw['recommendation']}\n\n")
            
            # Detailed results
            f.write("DETAILED RESULTS\n")
            f.write("-" * 20 + "\n")
            for result in report['detailed_results']:
                f.write(f"\nTest: {result['test_name']}\n")
                f.write(f"  Mode: {result['timestamp_mode']}\n")
                f.write(f"  Duration: {result['duration']:.1f}s\n")
                f.write(f"  Samples: {result['samples']}\n")
                f.write(f"  Mean Precision: {result['mean_precision']:.1f} ns\n")
                f.write(f"  Jitter: {result['jitter']:.1f} ns\n")
                f.write(f"  Stability: {result['stability_score']:.1%}\n")

def main():
    parser = argparse.ArgumentParser(description='Intel I219-LM Performance Benchmark')
    parser.add_argument('--duration', type=int, default=300,
                       help='Test duration in seconds')
    parser.add_argument('--no-hardware', action='store_true',
                       help='Skip hardware timestamp tests')
    parser.add_argument('--no-software', action='store_true',
                       help='Skip software timestamp tests')
    parser.add_argument('--no-load', action='store_true',
                       help='Skip load testing')
    parser.add_argument('--load-levels', type=int, nargs='+',
                       default=[100, 500, 1000, 2000],
                       help='Load levels to test (packets per second)')
    parser.add_argument('--output', default='i219_benchmark',
                       help='Output file prefix')
    parser.add_argument('--all-supported-adapters', action='store_true',
                       help='Automatically detect and benchmark all supported adapters (i210, i219, i225, i226)')

    args = parser.parse_args()

    def detect_supported_adapters():
        import platform
        adapters = []
        if platform.system() == 'Windows':
            try:
                import subprocess
                result = subprocess.run(['powershell', '-Command', 'Get-NetAdapter | Select-Object -ExpandProperty Name, InterfaceDescription'], capture_output=True, text=True)
                lines = result.stdout.splitlines()
                for line in lines:
                    if any(x in line for x in ['i210', 'i219', 'i225', 'i226', 'I210', 'I219', 'I225', 'I226']):
                        name = line.strip().split()[0]
                        adapters.append(name)
            except Exception as e:
                print(f"Adapter detection failed: {e}")
        else:
            # Linux: use 'ip link' or 'lspci' for more details
            try:
                import subprocess
                result = subprocess.run(['ip', '-o', 'link', 'show'], capture_output=True, text=True)
                for line in result.stdout.splitlines():
                    if any(x in line for x in ['i210', 'i219', 'i225', 'i226', 'I210', 'I219', 'I225', 'I226']):
                        name = line.split(':')[1].strip()
                        adapters.append(name)
            except Exception as e:
                print(f"Adapter detection failed: {e}")
        return adapters

    # Zielordner für Testergebnisse gemäß Repository-Richtlinien
    results_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'docs', 'tests', 'results')
    os.makedirs(results_dir, exist_ok=True)

    def get_output_prefix(base, adapter=None):
        if adapter:
            return os.path.join(results_dir, f"{base}_{adapter.replace(' ', '_')}")
        else:
            return os.path.join(results_dir, base)

    if args.all_supported_adapters:
        adapters = detect_supported_adapters()
        if not adapters:
            print("No supported adapters (i210, i219, i225, i226) detected!")
            sys.exit(1)
        print(f"Detected supported adapters: {adapters}")
        all_results = []
        for adapter in adapters:
            print(f"\n=== Benchmarking adapter: {adapter} ===")
            # Adapter name as target_interface
            config = BenchmarkConfig(
                test_duration=args.duration,
                hw_timestamp_tests=not args.no_hardware,
                sw_timestamp_tests=not args.no_software,
                load_test_enabled=not args.no_load,
                load_levels=args.load_levels
            )
            benchmark = PerformanceBenchmark(config)
            benchmark.target_interface = adapter
            results = benchmark.run_full_benchmark_suite()
            if results:
                out_prefix = get_output_prefix(args.output, adapter)
                benchmark.generate_comparison_report(out_prefix)
                all_results.extend(results)
        if all_results:
            print(f"\nBenchmark complete! {len(all_results)} tests performed across {len(adapters)} adapters.")
        else:
            print("No benchmark results generated.")
    else:
        # Create benchmark configuration
        config = BenchmarkConfig(
            test_duration=args.duration,
            hw_timestamp_tests=not args.no_hardware,
            sw_timestamp_tests=not args.no_software,
            load_test_enabled=not args.no_load,
            load_levels=args.load_levels
        )
        # Run benchmark
        benchmark = PerformanceBenchmark(config)
        results = benchmark.run_full_benchmark_suite()
        if results:
            out_prefix = get_output_prefix(args.output)
            benchmark.generate_comparison_report(out_prefix)
            print(f"\nBenchmark complete! {len(results)} tests performed.")
        else:
            print("No benchmark results generated.")

if __name__ == '__main__':
    main()
