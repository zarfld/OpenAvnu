# OpenAvnu Quick Start Guide - SuperMicro X8DA6 with Intel NICs

**Hardware Configuration**: SuperMicro X8DA6 with Intel i210 and i225 NICs  
**Operating System**: Windows 10  
**Created**: July 8, 2025

## 🎯 **Quick Start Overview**

You have excellent hardware for OpenAvnu testing! Your configuration includes:
- ✅ **2x SuperMicro X8DA6** with Intel i210 NICs
- ✅ **1x SuperMicro X8DA6** with Intel i225 dual port NIC
- ✅ **All systems running Windows 10**

This gives you a 3-node test network with both i210 and i225 NICs - perfect for comprehensive testing.

## 🚀 **Immediate Next Steps**

### **Step 1: Network Setup (30 minutes)**
1. **Connect all systems** to the same network switch
2. **Verify basic connectivity** between all systems
3. **Document IP addresses** and network configuration

### **Step 2: Driver Installation (1 hour)**
1. **Download Intel drivers**:
   - Intel i210 drivers for Windows 10
   - Intel i225 drivers for Windows 10
2. **Install drivers** on respective systems
3. **Verify installation** in Device Manager

### **Step 3: OpenAvnu Installation (1 hour)**
1. **Download OpenAvnu** from repository
2. **Install on all 3 systems**
3. **Basic configuration** for Windows 10

### **Step 4: First Test (30 minutes)**
1. **Start gPTP** on one system
2. **Verify basic operation**
3. **Check for errors**

## 📋 **Detailed Setup Instructions**

### **Network Topology Setup**
```
Network Switch
├── System 1: SuperMicro X8DA6 + Intel i210 (Master)
├── System 2: SuperMicro X8DA6 + Intel i210 (Slave)
└── System 3: SuperMicro X8DA6 + Intel i225 (Advanced testing)
```

### **Driver Installation**

#### **For Intel i210 (Systems 1 & 2)**
1. **Download** Intel i210 drivers from Intel website
2. **Install** using Device Manager or installer
3. **Verify** NIC appears in Device Manager
4. **Test** basic network connectivity

#### **For Intel i225 (System 3)**
1. **Download** Intel i225 drivers from Intel website
2. **Install** both ports if dual port
3. **Verify** both NICs appear in Device Manager
4. **Test** connectivity on both ports

### **OpenAvnu Installation**

#### **Download and Setup**
1. **Clone repository**:
   ```cmd
   git clone https://github.com/AVnu/OpenAvnu.git
   cd OpenAvnu
   ```

2. **Build for Windows** (if needed):
   ```cmd
   cd lib\common
   [Follow Windows build instructions]
   ```

#### **Configuration**
1. **Edit gPTP configuration**:
   - Modify `gptp\windows\gptp.ini`
   - Set correct network interface
   - Configure master/slave roles

2. **Test configuration**:
   ```cmd
   cd gptp\windows
   gptp.exe -F gptp.ini
   ```

### **First Test Execution**

#### **Basic Functionality Test**
1. **On System 1 (Master)**:
   ```cmd
   cd OpenAvnu\gptp\windows
   gptp.exe -F gptp.ini
   ```

2. **Check for**:
   - [ ] Successful startup
   - [ ] Interface detection
   - [ ] No critical errors
   - [ ] Basic timing operation

#### **Two-Node Synchronization Test**
1. **Configure System 1 as master**
2. **Configure System 2 as slave**
3. **Start gPTP on both systems**
4. **Monitor synchronization**

## 🔧 **Expected Challenges and Solutions**

### **Common Issues**

#### **Driver Compatibility**
- **Issue**: NIC not recognized
- **Solution**: Install latest Intel drivers
- **Verification**: Check Device Manager

#### **Network Configuration**
- **Issue**: Systems can't communicate
- **Solution**: Verify network setup and firewall
- **Verification**: Ping between systems

#### **gPTP Configuration**
- **Issue**: gPTP won't start
- **Solution**: Check configuration file and permissions
- **Verification**: Review log files

### **Hardware-Specific Considerations**

#### **SuperMicro X8DA6**
- **PCIe Compatibility**: Verify NIC properly seated
- **BIOS Settings**: Check network boot settings
- **Driver Support**: Ensure Windows 10 compatibility

#### **Intel i210**
- **Well-supported**: Should work out of the box
- **Hardware timestamping**: Verify precision support
- **Driver version**: Use latest stable version

#### **Intel i225**
- **Dual port**: Test both ports independently
- **Advanced features**: Verify full functionality
- **Driver compatibility**: Ensure Windows 10 support

## 📊 **Success Metrics**

### **Basic Success**
- [ ] All NICs recognized by Windows
- [ ] gPTP daemon starts successfully
- [ ] Basic network communication works
- [ ] No critical errors in logs

### **Advanced Success**
- [ ] Clock synchronization achieved
- [ ] Hardware timestamping functional
- [ ] All 3 systems can communicate
- [ ] i225 dual port features working

### **Production Readiness**
- [ ] Synchronization accuracy within ±80ns
- [ ] Stable operation for 24+ hours
- [ ] Full hardware timestamping support
- [ ] Comprehensive test results documented

## 🎯 **Testing Priority**

### **Week 1: Basic Setup**
- [ ] Network setup and driver installation
- [ ] OpenAvnu installation and configuration
- [ ] Basic functionality testing

### **Week 2: Synchronization**
- [ ] Two-node synchronization testing
- [ ] Hardware timestamping validation
- [ ] Performance measurement

### **Week 3: Advanced Testing**
- [ ] Three-node network testing
- [ ] i225 dual port testing
- [ ] Long-term stability testing

## 📞 **Need Help?**

### **If You Encounter Issues**
1. **Check the detailed test plan**: `docs/HARDWARE_TEST_PLAN_SUPERMICRO_X8DA6.md`
2. **Review OpenAvnu documentation**: Project documentation
3. **Check hardware compatibility**: Intel NIC specifications
4. **Verify network setup**: Basic connectivity tests

### **For Technical Support**
- **OpenAvnu community**: GitHub issues and discussions
- **Intel support**: For NIC-specific problems
- **Hardware documentation**: SuperMicro X8DA6 manual

## 🎉 **This is Excellent Progress!**

Your hardware configuration is perfect for OpenAvnu testing:
- **Multiple test nodes** for comprehensive testing
- **Different NIC types** (i210 and i225) for compatibility validation
- **Consistent platform** (Windows 10) for reliable testing
- **Professional hardware** (SuperMicro) for stability

This setup will allow you to **definitively validate** OpenAvnu's production readiness and update all documentation with **real hardware test results**.

---

**Next Step**: Start with network setup and driver installation  
**Expected Timeline**: 1-2 weeks for comprehensive testing  
**Success Probability**: High (excellent hardware configuration)
