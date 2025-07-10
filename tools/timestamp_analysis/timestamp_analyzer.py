#!/usr/bin/env python3
"""
Automated Timestamp Accuracy Measurement Tool for Intel I219-LM
Analyzes gPTP daemon output to measure hardware timestamping precision,
jitter, drift, and synchronization quality over time.

Usage:
    python timestamp_analyzer.py [options]
    
Features:
- Real-time gPTP log analysis
- Hardware vs software timestamp comparison
- Statistical analysis (jitter, drift, stability)
- Performance visualization
- Automated reporting
"""

import argparse
import re
import statistics
import time
import datetime
import json
import csv
import threading
import queue
import subprocess
import sys
import os
import shlex
from pathlib import Path
from collections import defaultdict, deque
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional, Tuple
import matplotlib.pyplot as plt
import numpy as np

@dataclass
class TimestampReading:
    """Single timestamp measurement"""
    timestamp: float
    sync_offset: Optional[float] = None
    delay_offset: Optional[float] = None
    rate_ratio: Optional[float] = None
    hardware_ts: Optional[bool] = None
    message_type: Optional[str] = None
    sequence_id: Optional[int] = None
    
@dataclass
class AnalysisResult:
    """Results from timestamp analysis"""
    duration_seconds: float
    total_samples: int
    mean_sync_offset: float
    sync_jitter: float  # Standard deviation
    max_sync_deviation: float
    drift_rate: float  # ns/second
    stability_metric: float
    hardware_timestamp_ratio: float
    message_stats: Dict[str, int]
    performance_grade: str

class TimestampAnalyzer:
    """Main analyzer class for gPTP timestamp data"""
    
    def __init__(self, config_path: Optional[str] = None):
        self.readings = deque(maxlen=10000)  # Keep last 10k readings
        self.start_time = None
        self.config = self._load_config(config_path)
        self.running = False
        self.analysis_queue = queue.Queue()
        
        # Analysis patterns for gPTP log parsing
        self.patterns = {
            'sync_msg': re.compile(r'sync message.*offset\s*[=:]\s*([+-]?\d+(?:\.\d+)?)'),
            'delay_msg': re.compile(r'delay.*offset\s*[=:]\s*([+-]?\d+(?:\.\d+)?)'),
            'hardware_ts': re.compile(r'hardware.*timestamp|hw.*ts'),
            'software_ts': re.compile(r'software.*timestamp|sw.*ts'),
            'sequence_id': re.compile(r'sequence\s*[=:]\s*(\d+)'),
            'rate_ratio': re.compile(r'rate.*ratio\s*[=:]\s*([+-]?\d+(?:\.\d+)?)')
        }
    
    def _load_config(self, config_path: Optional[str]) -> Dict:
        """Load configuration from file or use defaults"""
        default_config = {
            'analysis_window': 300,  # seconds
            'jitter_threshold': 1000,  # ns
            'drift_threshold': 100,  # ns/s
            'stability_threshold': 0.95,
            'performance_thresholds': {
                'excellent': {'jitter': 500, 'drift': 50},
                'good': {'jitter': 1000, 'drift': 100},
                'fair': {'jitter': 2000, 'drift': 200},
            }
        }
        
        if config_path and os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    user_config = json.load(f)
                default_config.update(user_config)
            except Exception as e:
                print(f"Warning: Could not load config from {config_path}: {e}")
        
        return default_config
    
    def parse_log_line(self, line: str) -> Optional[TimestampReading]:
        """Parse a single gPTP log line into a TimestampReading"""
        timestamp = time.time()
        reading = TimestampReading(timestamp=timestamp)
        
        # Extract sync offset
        if match := self.patterns['sync_msg'].search(line):
            reading.sync_offset = float(match.group(1))
            reading.message_type = 'sync'
        
        # Extract delay offset
        elif match := self.patterns['delay_msg'].search(line):
            reading.delay_offset = float(match.group(1))
            reading.message_type = 'delay'
        
        # Check for hardware/software timestamping
        if self.patterns['hardware_ts'].search(line):
            reading.hardware_ts = True
        elif self.patterns['software_ts'].search(line):
            reading.hardware_ts = False
        
        # Extract sequence ID
        if match := self.patterns['sequence_id'].search(line):
            reading.sequence_id = int(match.group(1))
        
        # Extract rate ratio
        if match := self.patterns['rate_ratio'].search(line):
            reading.rate_ratio = float(match.group(1))
        
        # Only return reading if we found useful data
        if reading.sync_offset is not None or reading.delay_offset is not None:
            return reading
        
        return None
    
    def add_reading(self, reading: TimestampReading):
        """Add a new timestamp reading to the analysis"""
        if self.start_time is None:
            self.start_time = reading.timestamp
        
        self.readings.append(reading)
        
        # Trigger analysis every 100 readings
        if len(self.readings) % 100 == 0:
            self.analysis_queue.put('analyze')
    
    def analyze_current_data(self) -> Optional[AnalysisResult]:
        """Analyze current timestamp data and return results"""
        if len(self.readings) < 10:
            return None
        
        # Extract sync offsets for analysis
        sync_offsets = [r.sync_offset for r in self.readings if r.sync_offset is not None]
        
        if not sync_offsets:
            return None
        
        # Calculate basic statistics
        duration = self.readings[-1].timestamp - self.readings[0].timestamp
        mean_offset = statistics.mean(sync_offsets)
        jitter = statistics.stdev(sync_offsets) if len(sync_offsets) > 1 else 0
        max_deviation = max(abs(x - mean_offset) for x in sync_offsets)
        
        # Calculate drift rate (linear regression over time)
        timestamps = [r.timestamp - self.start_time for r in self.readings if r.sync_offset is not None]
        if len(timestamps) > 5:
            drift_rate = self._calculate_drift_rate(timestamps, sync_offsets)
        else:
            drift_rate = 0
        
        # Calculate stability metric (percentage of readings within threshold)
        threshold = self.config['jitter_threshold']
        stable_readings = sum(1 for x in sync_offsets if abs(x - mean_offset) <= threshold)
        stability = stable_readings / len(sync_offsets)
        
        # Hardware timestamp ratio
        hw_readings = sum(1 for r in self.readings if r.hardware_ts is True)
        hw_ratio = hw_readings / len(self.readings) if self.readings else 0
        
        # Message type statistics
        message_stats = defaultdict(int)
        for reading in self.readings:
            if reading.message_type:
                message_stats[reading.message_type] += 1
        
        # Performance grading
        performance_grade = self._calculate_performance_grade(jitter, abs(drift_rate))
        
        return AnalysisResult(
            duration_seconds=duration,
            total_samples=len(sync_offsets),
            mean_sync_offset=mean_offset,
            sync_jitter=jitter,
            max_sync_deviation=max_deviation,
            drift_rate=drift_rate,
            stability_metric=stability,
            hardware_timestamp_ratio=hw_ratio,
            message_stats=dict(message_stats),
            performance_grade=performance_grade
        )
    
    def _calculate_drift_rate(self, timestamps: List[float], offsets: List[float]) -> float:
        """Calculate drift rate using linear regression"""
        if len(timestamps) < 2:
            return 0
        
        # Simple linear regression: y = mx + b, we want m (slope)
        n = len(timestamps)
        sum_x = sum(timestamps)
        sum_y = sum(offsets)
        sum_xy = sum(x * y for x, y in zip(timestamps, offsets))
        sum_x2 = sum(x * x for x in timestamps)
        
        denominator = n * sum_x2 - sum_x * sum_x
        if abs(denominator) < 1e-10:
            return 0
        
        slope = (n * sum_xy - sum_x * sum_y) / denominator
        return slope  # ns/second
    
    def _calculate_performance_grade(self, jitter: float, drift: float) -> str:
        """Calculate performance grade based on jitter and drift"""
        thresholds = self.config['performance_thresholds']
        
        if jitter <= thresholds['excellent']['jitter'] and drift <= thresholds['excellent']['drift']:
            return 'Excellent'
        elif jitter <= thresholds['good']['jitter'] and drift <= thresholds['good']['drift']:
            return 'Good'
        elif jitter <= thresholds['fair']['jitter'] and drift <= thresholds['fair']['drift']:
            return 'Fair'
        else:
            return 'Poor'
    
    def monitor_gptp_daemon(self, daemon_cmd: str, output_file: Optional[str] = None):
        """Monitor gPTP daemon output and analyze timestamps in real-time"""
        print(f"Starting gPTP daemon monitoring: {daemon_cmd}")
        
        self.running = True
        
        # Start daemon process
        try:
            # Convert string command to list for subprocess
            if isinstance(daemon_cmd, str):
                # For Windows, split on spaces but handle quoted paths properly
                if os.name == 'nt':  # Windows
                    # Use shlex with posix=False for Windows compatibility
                    cmd_args = shlex.split(daemon_cmd, posix=False)
                else:
                    cmd_args = shlex.split(daemon_cmd)
            else:
                cmd_args = daemon_cmd
                
            process = subprocess.Popen(
                cmd_args,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )
        except Exception as e:
            print(f"Error starting daemon: {e}")
            return
        
        # Start analysis thread
        analysis_thread = threading.Thread(target=self._analysis_worker)
        analysis_thread.daemon = True
        analysis_thread.start()
        
        # Monitor output
        output_log = []
        try:
            for line in process.stdout:
                if not self.running:
                    break
                
                line = line.strip()
                output_log.append(f"{datetime.datetime.now().isoformat()}: {line}")
                
                # Parse and add reading
                reading = self.parse_log_line(line)
                if reading:
                    self.add_reading(reading)
                
                print(line)  # Real-time output
                
        except KeyboardInterrupt:
            print("\nStopping monitoring...")
        finally:
            self.running = False
            process.terminate()
            process.wait()
        
        # Save output log if requested
        if output_file:
            with open(output_file, 'w') as f:
                f.write('\n'.join(output_log))
    
    def _analysis_worker(self):
        """Background worker for continuous analysis"""
        while self.running:
            try:
                # Wait for analysis trigger or timeout
                self.analysis_queue.get(timeout=30)
                
                result = self.analyze_current_data()
                if result:
                    self._print_analysis_summary(result)
                    
            except queue.Empty:
                # Periodic analysis even without trigger
                result = self.analyze_current_data()
                if result and result.total_samples > 0:
                    self._print_analysis_summary(result)
    
    def _print_analysis_summary(self, result: AnalysisResult):
        """Print a concise analysis summary"""
        print(f"\n{'='*60}")
        print(f"TIMESTAMP ANALYSIS SUMMARY")
        print(f"{'='*60}")
        print(f"Duration: {result.duration_seconds:.1f}s | Samples: {result.total_samples}")
        print(f"Mean Sync Offset: {result.mean_sync_offset:.1f} ns")
        print(f"Jitter (StdDev): {result.sync_jitter:.1f} ns")
        print(f"Max Deviation: {result.max_sync_deviation:.1f} ns")
        print(f"Drift Rate: {result.drift_rate:.2f} ns/s")
        print(f"Stability: {result.stability_metric:.1%}")
        print(f"HW Timestamps: {result.hardware_timestamp_ratio:.1%}")
        print(f"Performance: {result.performance_grade}")
        print(f"{'='*60}\n")
    
    def generate_report(self, output_path: str):
        """Generate comprehensive analysis report"""
        result = self.analyze_current_data()
        if not result:
            print("No data available for report generation")
            return
        
        report = {
            'metadata': {
                'generation_time': datetime.datetime.now().isoformat(),
                'analysis_duration': result.duration_seconds,
                'total_samples': result.total_samples,
                'config': self.config
            },
            'results': asdict(result),
            'raw_data': [asdict(r) for r in list(self.readings)]
        }
        
        # Save JSON report
        json_path = f"{output_path}.json"
        with open(json_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        # Save CSV data
        csv_path = f"{output_path}.csv"
        with open(csv_path, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'sync_offset', 'delay_offset', 'hardware_ts', 'message_type'])
            for reading in self.readings:
                writer.writerow([
                    reading.timestamp,
                    reading.sync_offset,
                    reading.delay_offset,
                    reading.hardware_ts,
                    reading.message_type
                ])
        
        print(f"Report generated: {json_path}, {csv_path}")
    
    def create_visualizations(self, output_dir: str):
        """Create performance visualization plots"""
        if len(self.readings) < 10:
            print("Insufficient data for visualization")
            return
        
        os.makedirs(output_dir, exist_ok=True)
        
        # Extract data for plotting
        timestamps = []
        sync_offsets = []
        
        for reading in self.readings:
            if reading.sync_offset is not None:
                timestamps.append(reading.timestamp - self.start_time)
                sync_offsets.append(reading.sync_offset)
        
        if not timestamps:
            return
        
        # Plot 1: Sync offset over time
        plt.figure(figsize=(12, 8))
        
        plt.subplot(2, 2, 1)
        plt.plot(timestamps, sync_offsets, 'b-', alpha=0.7, linewidth=1)
        plt.title('Sync Offset Over Time')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Sync Offset (ns)')
        plt.grid(True, alpha=0.3)
        
        # Plot 2: Sync offset histogram
        plt.subplot(2, 2, 2)
        plt.hist(sync_offsets, bins=50, alpha=0.7, color='blue', edgecolor='black')
        plt.title('Sync Offset Distribution')
        plt.xlabel('Sync Offset (ns)')
        plt.ylabel('Frequency')
        plt.grid(True, alpha=0.3)
        
        # Plot 3: Rolling jitter
        window_size = min(100, len(sync_offsets) // 10)
        if window_size > 5:
            rolling_jitter = []
            for i in range(window_size, len(sync_offsets)):
                window = sync_offsets[i-window_size:i]
                rolling_jitter.append(statistics.stdev(window))
            
            plt.subplot(2, 2, 3)
            plt.plot(timestamps[window_size:], rolling_jitter, 'r-', linewidth=2)
            plt.title(f'Rolling Jitter (window={window_size})')
            plt.xlabel('Time (seconds)')
            plt.ylabel('Jitter (ns)')
            plt.grid(True, alpha=0.3)
        
        # Plot 4: Drift trend
        plt.subplot(2, 2, 4)
        # Linear trend line
        z = np.polyfit(timestamps, sync_offsets, 1)
        p = np.poly1d(z)
        plt.plot(timestamps, sync_offsets, 'b.', alpha=0.5, markersize=2)
        plt.plot(timestamps, p(timestamps), 'r-', linewidth=2, label=f'Drift: {z[0]:.2f} ns/s')
        plt.title('Drift Analysis')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Sync Offset (ns)')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'timestamp_analysis.png'), dpi=300, bbox_inches='tight')
        plt.close()
        
        print(f"Visualizations saved to {output_dir}")

def main():
    parser = argparse.ArgumentParser(description='Analyze gPTP timestamp accuracy')
    parser.add_argument('--config', help='Configuration file path')
    parser.add_argument('--daemon-cmd', default='gptp', 
                       help='gPTP daemon executable path or command')
    parser.add_argument('--output', default='timestamp_analysis',
                       help='Output file prefix for reports')
    parser.add_argument('--log-output', help='Save daemon output to file')
    parser.add_argument('--duration', type=int, default=0,
                       help='Analysis duration in seconds (0 = until interrupted)')
    parser.add_argument('--visualize', action='store_true',
                       help='Generate visualization plots')
    
    args = parser.parse_args()
    
    # Create analyzer
    analyzer = TimestampAnalyzer(args.config)
    
    try:
        # Run monitoring
        if args.duration > 0:
            # Time-limited monitoring
            def stop_after_duration():
                time.sleep(args.duration)
                analyzer.running = False
            
            timer = threading.Timer(args.duration, stop_after_duration)
            timer.start()
        
        analyzer.monitor_gptp_daemon(args.daemon_cmd, args.log_output)
        
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        # Generate final report
        analyzer.generate_report(args.output)
        
        if args.visualize:
            analyzer.create_visualizations(f"{args.output}_plots")

if __name__ == '__main__':
    main()
