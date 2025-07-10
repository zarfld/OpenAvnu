# I225 gPTP Master-Slave Test Implementation Plan

**Date**: July 10, 2025  
**Status**: 🚧 **WORK IN PROGRESS**  
**Hardware**: Two Intel I225-V adapters ready for testing

## 🎯 **Objective**

Validate gPTP synchronization between two I225-V adapters in a direct Master-Slave configuration, building on the successful I219 validation from yesterday.

## 🔧 **Hardware Setup**

### **Available I225 Hardware**
- **Master**: Intel(R) Ethernet Controller (3) I225-V (`Ethernet 2`)
- **Slave**: Intel(R) Ethernet Controller (3) I225-V #2 (`Ethernet 5`)
- **Device ID**: 0x15F3 (confirmed via PnP)
- **Speed**: 2.5 Gbps per adapter
- **Connection**: Direct Ethernet cable between adapters

### **Physical Connection**
```
[System] ─── I225-V #1 (Master) ←──── Cat5e/6 Cable ────→ I225-V #2 (Slave) ─── [Same System]
         Ethernet 2                                                    Ethernet 5
```

## 📋 **Implementation Steps**

### **Phase 1: Hardware Preparation (Today)**
1. **Connect Hardware**
   - Physical Ethernet cable between both I225 adapters
   - Verify link establishment at 2.5 Gbps

2. **Configure Network Interfaces**
   - Master: Static IP (e.g., 192.168.100.1/24)
   - Slave: Static IP (e.g., 192.168.100.2/24)
   - Verify ping connectivity

### **Phase 2: gPTP Configuration (1-2 Days)**
1. **Build gPTP with I225 Support**
   - Use existing OpenAvnu gPTP daemon
   - Configure for I225 hardware timestamping
   - Test compilation and basic functionality

2. **Master Configuration**
   ```ini
   # Master gPTP Configuration
   priority1 = 50
   clockClass = 6
   # I225-specific settings
   timestamping = hardware
   interface = Ethernet 2
   ```

3. **Slave Configuration**
   ```ini
   # Slave gPTP Configuration  
   priority1 = 100
   clockClass = 248
   # I225-specific settings
   timestamping = hardware
   interface = Ethernet 5
   ```

### **Phase 3: Synchronization Testing (2-3 Days)**
1. **Start Master Daemon**
   - Initialize gPTP on Ethernet 2
   - Verify master role establishment
   - Monitor timestamp quality

2. **Start Slave Daemon**
   - Initialize gPTP on Ethernet 5
   - Verify slave role and master discovery
   - Monitor synchronization convergence

3. **Performance Measurement**
   - Convergence time (target: < 100ms)
   - Synchronization accuracy (target: < 1μs)
   - Timestamp precision validation

## 🔍 **Expected I225 Advantages**

### **Hardware Capabilities**
- **Enhanced Timestamping**: Better than I219 baseline
- **2.5 Gbps Bandwidth**: Reduced network latency
- **Hardware TSN Support**: Native time-sensitive networking
- **Dedicated PTP Logic**: On-chip precision timing

### **Performance Targets**
- **Convergence**: < 50ms (better than I219)
- **Accuracy**: < 500ns (sub-microsecond precision)
- **Stability**: Long-term drift < 100ppb

## ⚠️ **Potential Challenges**

### **Known I225 Issues**
- **Driver Compatibility**: Ensure latest Intel drivers
- **Windows NDIS**: Timestamp support varies by driver version
- **Dual-Adapter**: Interference between same-family adapters

### **Mitigation Strategies**
- **Isolated Network**: Use dedicated switch if interference occurs
- **Driver Updates**: Ensure I225 drivers support hardware timestamping
- **Fallback Plan**: Software timestamping if hardware fails

## 📊 **Success Criteria**

### **Functional Requirements**
- ✅ Both I225 adapters detected and accessible
- ✅ gPTP Master-Slave role establishment
- ✅ Clock synchronization within target accuracy
- ✅ Stable long-term operation (> 1 hour)

### **Performance Requirements**
- **Convergence**: < 100ms from start to sync
- **Accuracy**: < 1μs average offset
- **Precision**: < 100ns standard deviation
- **Stability**: < 1ms drift over 24 hours

## 🚀 **Next Actions**

### **Immediate (Today)**
1. Connect I225 adapters with Ethernet cable
2. Configure static IP addresses
3. Test basic network connectivity

### **Short-term (Tomorrow)**
1. Build OpenAvnu gPTP daemon
2. Configure I225-specific parameters
3. Start initial gPTP testing

### **Medium-term (This Week)**
1. Performance measurement and optimization
2. Documentation of results
3. Comparison with I219 baseline performance

## 📝 **Documentation Plan**

Results will be documented in:
- **Success**: `docs/completed/I225_gPTP_Master_Slave_Test_Success.md`
- **Detailed Results**: `docs/completed/I225_gPTP_Performance_Analysis.md`
- **Issues**: `docs/work-in-progress/I225_gPTP_Troubleshooting.md` (if needed)

This test will validate I225 hardware capabilities and establish a baseline for advanced TSN features testing.

## 📊 **Status Update - July 10, 2025, 15:30**

### ✅ **Phase 1: Hardware Preparation - COMPLETED**

**Network Configuration Success:**
- ✅ **Master (Ethernet 2)**: 192.168.100.1/24, Status UP, 100 Mbps
- ✅ **Slave (Ethernet 5)**: 192.168.100.2/24, Status UP, 100 Mbps  
- ✅ **Bidirectional Connectivity**: < 1ms latency, 0% packet loss
- ✅ **Stable Link**: Both I225-V adapters connected and operational

**Test Results:**
```
ping 192.168.100.2: Min=0ms, Max=0ms, Average=0ms, Loss=0%
ping 192.168.100.1: Min=0ms, Max=0ms, Average=0ms, Loss=0%
```

### 🔧 **Phase 2: gPTP Configuration - IN PROGRESS**

**Configuration Files Created:**
- ✅ `i225_master_config.ini` - Master gPTP configuration for Ethernet 2
- ✅ `i225_slave_config.ini` - Slave gPTP configuration for Ethernet 5

**Key Configuration Features:**
- **Hardware Timestamping**: Enabled for both I225 adapters
- **Enhanced Precision**: I225-V specific optimizations
- **Master Priority**: 50 (low number = high priority)
- **Slave Priority**: 100 (high number = low priority, will be slave)
- **Sync Interval**: 8 packets/second (-3 log interval)

### 🚀 **Next Immediate Steps:**

1. **Build gPTP Daemon**: Compile OpenAvnu gPTP with I225 support
2. **Start Master**: Launch gPTP daemon on Ethernet 2 with master config
3. **Start Slave**: Launch gPTP daemon on Ethernet 5 with slave config
4. **Monitor Synchronization**: Validate master-slave role establishment
5. **Measure Performance**: Record convergence time and sync accuracy
