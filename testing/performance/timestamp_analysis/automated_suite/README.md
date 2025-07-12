# Intel I219-LM Timestamp Analysis Tools

This directory contains automated tools for analyzing and benchmarking the Intel I219-LM Hardware Abstraction Layer (HAL) implementation in OpenAvnu.

## Tools Overview

### 1. timestamp_analyzer.py
Real-time timestamp accuracy measurement and analysis tool.

**Features:**
- Real-time gPTP log analysis
- Hardware vs software timestamp comparison
- Statistical analysis (jitter, drift, stability)
- Performance visualization
- Automated reporting

**Usage:**
```bash
# Basic analysis with default gPTP daemon
python timestamp_analyzer.py

# Custom gPTP command and configuration
python timestamp_analyzer.py --daemon-cmd gptp -i eth0 --hardware-timestamps --config config.json

# Generate visualizations
python timestamp_analyzer.py --visualize --output detailed_analysis

# Time-limited analysis
python timestamp_analyzer.py --duration 300 --log-output gptp_session.log
```

**Output:**
- Real-time analysis summaries
- JSON report with detailed statistics
- CSV data for further analysis
- Performance visualization plots (with --visualize)

### 2. performance_benchmark.py
Comprehensive performance benchmarking suite comparing different configurations.

**Features:**
- Hardware vs software timestamp precision comparison
- Load testing with varying network conditions
- Multi-device synchronization accuracy
- Performance regression testing
- Comparative analysis with other NICs

**Usage:**
```bash
# Full benchmark suite
python performance_benchmark.py --duration 300

# Hardware timestamps only
python performance_benchmark.py --no-software --duration 180

# Custom load levels
python performance_benchmark.py --load-levels 100 500 1000 5000

# Skip load testing
python performance_benchmark.py --no-load --output baseline_comparison

# Automatisch alle unterstützten Adapter (i210, i219, i225, i226) testen
python performance_benchmark.py --all-supported-adapters --duration 300
```

**Hinweis:**
Mit der Option `--all-supported-adapters` erkennt das Skript automatisch alle Netzwerkschnittstellen mit den Bezeichnungen i210, i219, i225 und i226 und führt den Benchmark für jede dieser Schnittstellen durch. Die Ergebnisse werden für jede getestete Schnittstelle separat ausgegeben und dokumentiert.

**Output:**
- JSON report with benchmark results
- Text summary with performance comparison
- Detailed analysis and recommendations

### 3. automated_test_suite.py
Automated validation suite for I219 HAL integration testing.

**Features:**
- Automated hardware detection and validation
- gPTP daemon integration testing
- Hardware timestamp accuracy verification
- System compatibility testing
- Regression testing framework

**Usage:**
```bash
# Full test suite
python automated_test_suite.py

# Specific interface
python automated_test_suite.py --interface "Ethernet 2"

# Verbose output with detailed logging
python automated_test_suite.py --verbose --duration 120

# Custom output location
python automated_test_suite.py --output validation_results_2025_01_10
```

**Output:**
- JSON test report with detailed results
- Text summary with pass/fail status
- System and hardware configuration details

## Configuration

### config.json
Default configuration file for analysis parameters:

```json
{
  "analysis_window": 300,
  "jitter_threshold": 1000,
  "drift_threshold": 100,
  "stability_threshold": 0.95,
  "performance_thresholds": {
    "excellent": {"jitter": 500, "drift": 50},
    "good": {"jitter": 1000, "drift": 100},
    "fair": {"jitter": 2000, "drift": 200}
  }
}
```

## Installation Requirements

```bash
# Python dependencies
pip install matplotlib numpy psutil

# Windows PowerShell (for hardware detection)
# Linux: ethtool, lspci utilities
```

## Example Workflow

### 1. Initial Validation
```bash
# Run automated test suite to verify system setup
python automated_test_suite.py --verbose

# If tests pass, proceed to performance analysis
```

### 2. Baseline Performance Measurement
```bash
# Measure baseline performance
python performance_benchmark.py --duration 300 --output baseline_2025_01_10

# Analyze real-time performance
python timestamp_analyzer.py --duration 600 --visualize --output realtime_analysis
```

### 3. Continuous Monitoring
```bash
# Monitor during extended operation
python timestamp_analyzer.py --daemon-cmd gptp -i "Ethernet 2" --hardware-timestamps --duration 3600
```

## Output Analysis

### Performance Grades
- **Excellent**: Jitter < 500ns, Drift < 50ns/s
- **Good**: Jitter < 1000ns, Drift < 100ns/s  
- **Fair**: Jitter < 2000ns, Drift < 200ns/s
- **Poor**: Above fair thresholds

### Key Metrics
- **Mean Sync Offset**: Average synchronization offset in nanoseconds
- **Jitter**: Standard deviation of sync offsets (timing precision)
- **Drift Rate**: Linear trend in offsets over time (clock stability)
- **Stability**: Percentage of readings within threshold
- **Hardware Timestamp Ratio**: Percentage using hardware timestamps

### Typical Results (I219-LM)
Based on validation testing:
- Hardware timestamp jitter: 1400-2700ns
- Sync window quality: 85/100
- Hardware timestamp ratio: >95%
- Stability: >98% within 5μs threshold

## Troubleshooting

### Common Issues
1. **No interface detected**: Check I219 driver installation
2. **gPTP build not found**: Verify build paths in automated_test_suite.py
3. **Permission errors**: Run with appropriate privileges for network access
4. **Missing dependencies**: Install matplotlib, numpy, psutil

### Debug Mode
Add `--verbose` flag to any tool for detailed debug output.

## Integration with Development Workflow

These tools are designed to support the Intel I219 development phases:

**Phase 1**: Use automated_test_suite.py for validation
**Phase 2**: Use performance_benchmark.py for comparison testing  
**Phase 3**: Use timestamp_analyzer.py for continuous monitoring
**Phase 4**: Combine all tools for comprehensive analysis

## Contributing

When adding new test scenarios or analysis features:
1. Follow the existing dataclass pattern for results
2. Add comprehensive error handling
3. Include both JSON and human-readable output formats
4. Update this README with new features
