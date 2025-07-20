# Hive-AVDECC Discovery Test Plan
## REAL SUCCESS CRITERIA

### ❌ WHAT IS NOT SUCCESS:
- Building without errors ❌
- Interface initialization ❌ 
- MAC address detection ❌
- Packet transmission attempts ❌
- Validation scripts passing ❌

### ✅ WHAT IS ACTUAL SUCCESS:
**OpenAvnu Raw Ethernet AVDECC Entity appears in Hive-AVDECC device list**

## Test Setup Requirements

### 1. Network Configuration
- Intel I219 (MAC: C0:47:0E:16:7B:89) connected to RME
- RME powered ON and connected
- Same network segment as Hive-AVDECC host

### 2. Software Requirements
- Hive-AVDECC running on network
- OpenAvnu Raw Ethernet AVDECC entity running
- Both on same Ethernet segment

### 3. Test Procedure
```bash
# Step 1: Start OpenAvnu AVDECC Entity
cd d:\Repos\OpenAvnu\build\examples\Release
.\raw_ethernet_avdecc_entity.exe 30

# Step 2: Start Hive-AVDECC on same network
# Step 3: Check if OpenAvnu entity appears in Hive device list
```

### 4. Success Criteria
- [ ] OpenAvnu entity visible in Hive-AVDECC
- [ ] Entity ID matches: 0x001B21481B2F193D
- [ ] MAC matches: C0:47:0E:16:7B:89
- [ ] MILAN compliance detected
- [ ] Stream capabilities visible

### 5. Failure Analysis
If entity does NOT appear in Hive:
- Check network connectivity
- Verify ADP advertisements sent
- Analyze packet capture
- Debug IEEE 1722.1 compliance

## Current Status: IEEE 1722.1 COMPLIANCE FIXED - STILL TESTING
**Progress made:**
- ✅ Fixed AVTP header structure (packet size now 105 bytes vs 84)
- ✅ Added proper stream ID, gateway flags, control data length
- ❌ Still not detected by Hive-AVDECC - investigation ongoing

**Next steps:**
- Analyze remaining compliance issues
- Check AEM descriptor requirements
- Verify MILAN-specific fields
