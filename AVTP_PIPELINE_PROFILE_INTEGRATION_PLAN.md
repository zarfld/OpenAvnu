# AVTP Pipeline Profile Integration Plan

## 🎯 **Integration Objective**
Integrate the completed AVTP Profile System with the existing AVTP pipeline to enable profile-aware stream configuration, automatic validation, and runtime profile selection.

## 🏗️ **Architecture Overview**

### Current AVTP Pipeline Structure
```
openavb_host/harness
├── TL (Talker/Listener) Module
├── Map Modules (AAF, H.264, MJPEG, etc.)
├── Interface Modules (ALSA, GStreamer, etc.)
└── AVTP Layer (Open1722 integration)
```

### Enhanced Profile-Aware Architecture
```
openavb_host/harness + Profile System
├── Profile-Aware TL Module
│   ├── Profile Selection Engine
│   ├── Stream Validation
│   └── Configuration Override
├── Profile-Enhanced Map Modules
├── Profile-Aware Interface Modules
└── Profile-Integrated AVTP Layer
```

## 📋 **Implementation Phases**

### **Phase 1: Core Integration Foundation**

#### 1.1 Profile-Aware TL Configuration Extension
**File**: `lib/avtp_pipeline/include/openavb_tl_profile_cfg.h`
```c
// Extension to openavb_tl_cfg_t for profile support
typedef struct {
    openavb_tl_cfg_t base_cfg;
    
    // Profile system integration
    openavb_profile_type_t profile_type;
    openavb_profile_cfg_t *profile_cfg;
    bool auto_profile_detection;
    bool profile_validation_enabled;
    bool profile_override_allowed;
    
    // Profile-specific constraints
    openavb_latency_constraints_t latency_constraints;
    openavb_format_requirements_t format_requirements;
    openavb_validation_criteria_t validation_criteria;
    
    // Runtime profile management
    char profile_config_file[256];
    bool dynamic_profile_switching;
    
} openavb_tl_profile_cfg_t;
```

#### 1.2 Profile-Aware Stream Factory
**File**: `lib/avtp_pipeline/tl/openavb_tl_profile_factory.h`
```c
// Profile-aware stream creation and configuration
typedef struct {
    openavb_profile_type_t profile_type;
    openavb_stream_validation_result_t validation_result;
    openavb_profile_compliance_status_t compliance_status;
    void *profile_private_data;
} openavb_tl_profile_context_t;

// Core factory functions
tl_handle_t openavbTLOpenWithProfile(openavb_profile_type_t profile_type);
bool openavbTLConfigureWithProfile(tl_handle_t handle, 
                                   openavb_tl_profile_cfg_t *pProfileCfg,
                                   openavb_tl_cfg_name_value_t *pNVCfg);
bool openavbTLValidateProfileCompliance(tl_handle_t handle);
```

### **Phase 2: Profile-Enhanced Map Module Integration**

#### 2.1 AAF Audio Profile Integration
**File**: `lib/avtp_pipeline/map_aaf_audio/openavb_map_aaf_audio_profile.c`
```c
// Integrate Milan and ProAV profiles with AAF audio mapping
bool openavbMapAAFProfileInit(media_q_t *pMediaQ, 
                              openavb_map_cb_t *pMapCB, 
                              openavb_profile_cfg_t *pProfileCfg,
                              U32 inMaxTransitUsec)
{
    // Milan Profile: 2ms latency, specific sample rates
    if (pProfileCfg->profile_type == OPENAVB_PROFILE_MILAN) {
        // Configure AAF for Milan compliance
        // - Set 48kHz/96kHz sample rates
        // - Configure 2ms presentation time
        // - Enable stream reservation
    }
    
    // ProAV Profile: High-resolution audio, 5ms latency
    if (pProfileCfg->profile_type == OPENAVB_PROFILE_PROAV) {
        // Configure AAF for ProAV compliance
        // - Support 192kHz sample rates
        // - Configure 5ms presentation time
        // - Enable high-precision timing
    }
    
    return openavbMapAAFAudioInitialize(pMediaQ, pMapCB, inMaxTransitUsec);
}
```

#### 2.2 CVF Video Profile Integration
**File**: `lib/avtp_pipeline/map_h264/openavb_map_h264_profile.c`
```c
// Integrate Automotive profile with H.264 CVF mapping
bool openavbMapH264ProfileInit(media_q_t *pMediaQ, 
                               openavb_map_cb_t *pMapCB, 
                               openavb_profile_cfg_t *pProfileCfg,
                               U32 inMaxTransitUsec)
{
    // Automotive Profile: H.264 video with 50ms latency
    if (pProfileCfg->profile_type == OPENAVB_PROFILE_AUTOMOTIVE) {
        // Configure H.264 for automotive compliance
        // - Set appropriate bitrates for automotive networks
        // - Configure 50ms presentation time
        // - Enable robust error correction
    }
    
    return openavbMapH264Initialize(pMediaQ, pMapCB, inMaxTransitUsec);
}
```

### **Phase 3: Profile-Aware Interface Module Enhancement**

#### 3.1 ALSA Audio Interface Profile Support
**File**: `lib/avtp_pipeline/intf_alsa/openavb_intf_alsa_profile.c`
```c
// Profile-aware ALSA configuration
bool openavbIntfAlsaProfileInit(media_q_t *pMediaQ, 
                                openavb_intf_cb_t *pIntfCB,
                                openavb_profile_cfg_t *pProfileCfg)
{
    // Configure ALSA based on profile requirements
    switch(pProfileCfg->profile_type) {
        case OPENAVB_PROFILE_MILAN:
            // Milan: Configure for 2ms buffer, specific sample rates
            break;
        case OPENAVB_PROFILE_PROAV:
            // ProAV: Configure for high-resolution audio
            break;
        case OPENAVB_PROFILE_AUTOMOTIVE:
            // Automotive: Configure for robust automotive audio
            break;
    }
    
    return openavbIntfAlsaInitialize(pMediaQ, pIntfCB);
}
```

### **Phase 4: Configuration File Profile Integration**

#### 4.1 Profile-Aware INI Configuration
**Example**: `examples/milan_audio_talker_profile.ini`
```ini
# Milan Profile Audio Talker Configuration
role = talker
initial_state = running

# Profile specification
profile_type = milan
profile_validation = enabled
profile_auto_detect = disabled

# Milan-specific constraints (automatically applied)
# max_transit_usec = 2000  # 2ms Milan requirement
# sr_class = a             # Class A for Milan

# Standard configuration
ifname = eth0
stream_addr = 00:25:64:12:34:56
stream_uid = 1001
dest_addr = 91:e0:f0:00:fe:12

# Map and interface modules (profile-enhanced)
map_lib = libopenavb_map_aaf_audio.so
map_fn = openavbMapAAFAudioInitialize
intf_lib = libopenavb_intf_alsa.so
intf_fn = openavbIntfAlsaInitialize

# ALSA specific (profile will validate these)
intf_nv_device_name = hw:0
intf_nv_audio_rate = 48000
intf_nv_audio_bit_depth = 24
intf_nv_audio_channels = 8
```

#### 4.2 Profile Configuration Parser Extension
**File**: `lib/avtp_pipeline/tl/openavb_tl_profile_cfg_parser.c`
```c
// Parse profile-specific configuration
bool openavbTLParseProfileConfig(const char *configFile,
                                openavb_tl_profile_cfg_t *pProfileCfg)
{
    // Parse profile_type directive
    // Load profile-specific constraints
    // Validate configuration against profile requirements
    // Apply automatic profile-based overrides
}
```

### **Phase 5: Runtime Profile Management**

#### 5.1 Profile-Aware Host Application
**File**: `lib/avtp_pipeline/platform/Linux/avb_host/openavb_profile_host.c`
```c
// Enhanced host application with profile support
int main(int argc, char *argv[])
{
    // Standard initialization
    osalAVBInitialize(optLogFileName, optIfnameGlobal);
    
    // Register profile-enhanced modules
    registerProfileMapModule(openavbMapAAFAudioProfileInitialize);
    registerProfileMapModule(openavbMapH264ProfileInitialize);
    registerProfileIntfModule(openavbIntfAlsaProfileInitialize);
    
    // Open streams with profile awareness
    for (i1 = 0; i1 < tlCount; i1++) {
        openavb_profile_type_t profile = detectProfileFromConfig(argv[i1 + iniIdx]);
        tlHandleList[i1] = openavbTLOpenWithProfile(profile);
    }
    
    // Configure with profile validation
    for (i1 = 0; i1 < tlCount; i1++) {
        openavb_tl_profile_cfg_t profileCfg;
        if (!openavbTLReadProfileIniFile(tlHandleList[i1], argv[i1 + iniIdx], &profileCfg)) {
            AVB_LOGF_ERROR("Profile configuration error: %s\n", argv[i1 + 1]);
            exit(-1);
        }
        
        if (!openavbTLConfigureWithProfile(tlHandleList[i1], &profileCfg, &NVCfg)) {
            AVB_LOGF_ERROR("Profile validation failed: %s\n", argv[i1 + 1]);
            exit(-1);
        }
        
        if (!openavbTLValidateProfileCompliance(tlHandleList[i1])) {
            AVB_LOGF_ERROR("Stream does not comply with profile: %s\n", argv[i1 + 1]);
            exit(-1);
        }
    }
    
    // Run streams with profile monitoring
    for (i1 = 0; i1 < tlCount; i1++) {
        if (openavbTLGetInitialState(tlHandleList[i1]) != TL_INIT_STATE_STOPPED) {
            openavbTLRunWithProfileMonitoring(tlHandleList[i1]);
        }
    }
}
```

## 🧪 **Testing and Validation Strategy**

### **Integration Test Cases**
1. **Milan Profile Audio Stream**: 2ms latency AAF audio validation
2. **Automotive Profile Video Stream**: 50ms latency H.264 CVF validation  
3. **ProAV Profile High-Res Audio**: 192kHz audio stream validation
4. **Profile Auto-Detection**: Automatic profile recognition from configuration
5. **Profile Compliance Monitoring**: Runtime validation and error reporting
6. **Mixed Profile Streams**: Multiple streams with different profiles

### **Performance Benchmarks**
- Profile validation overhead measurement
- Stream setup time with profile processing
- Runtime profile monitoring impact
- Memory usage for profile-enhanced streams

## 🎉 **Expected Benefits**

### **For Developers**
- **Simplified Configuration**: Profiles automatically set optimal parameters
- **Validation at Build Time**: Catch configuration errors early
- **Industry Standard Compliance**: Built-in Milan/Automotive/ProAV support

### **For System Integrators** 
- **Plug-and-Play Compatibility**: Profile-based interoperability
- **Reduced Configuration Errors**: Profile constraints prevent misconfigurations
- **Runtime Monitoring**: Continuous compliance verification

### **For End Users**
- **Reliable Performance**: Profile-guaranteed latency and quality
- **Seamless Interoperability**: Cross-vendor compatibility through profiles
- **Simplified Troubleshooting**: Profile-aware error reporting

## 🚀 **Implementation Timeline**
- **Phase 1**: Core Integration (1-2 weeks)
- **Phase 2**: Map Module Enhancement (1 week)  
- **Phase 3**: Interface Module Enhancement (1 week)
- **Phase 4**: Configuration Integration (1 week)
- **Phase 5**: Runtime Management & Testing (1 week)

**Total Estimated Time**: 5-6 weeks for complete integration

This integration will provide a production-ready, profile-aware AVTP pipeline that maintains backward compatibility while adding powerful industry-standard profile support.
