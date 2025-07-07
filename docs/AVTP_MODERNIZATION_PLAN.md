# OpenAvnu AVTP Modernization Plan

## Overview
This document outlines the plan to modernize OpenAvnu's AVTP implementation by integrating COVESA's Open1722 library as a submodule.

## Current State Analysis

### OpenAvnu AVTP Implementation Issues
- **Legacy Design**: Current implementation in `lib/avtp_pipeline/avtp/` is tightly coupled to legacy kernel modules
- **Limited Standards Compliance**: Not fully aligned with IEEE 1722-2016
- **Maintenance Burden**: Custom AVTP parsing/creation code that duplicates industry standards
- **Limited Format Support**: Missing support for modern AVTP formats and automotive control formats

### Current AVTP Components
```
lib/avtp_pipeline/
├── avtp/
│   ├── openavb_avtp.c/h          # Main AVTP implementation
│   └── ...
├── map_*/                        # Format-specific mapping modules
├── aem/                          # AVDECC Entity Model
└── platform/                    # Platform-specific code
```

## Integration Strategy: Open1722

### Why Open1722?
1. **Automotive Focus**: Supports AVTP Control Formats (ACF) for CAN, LIN, FlexRay
2. **Active Development**: Regular updates and community support
3. **CMake Integration**: Compatible build system
4. **Enhanced Media Support**: MJPEG, JPEG2000, AAF, CVF, CRF, RVF, VSF
5. **Standards Compliance**: Full IEEE 1722-2016 implementation
6. **Modular Design**: Clean separation of packet creation/parsing

### Phase 1: Submodule Integration

#### Step 1.1: Add Open1722 Submodule
```bash
git submodule add https://github.com/COVESA/Open1722.git thirdparty/open1722
git submodule update --init --recursive
```

#### Step 1.2: CMake Integration
Update `CMakeLists.txt`:
```cmake
# Add Open1722 submodule
option(OPENAVNU_USE_OPEN1722 "Use Open1722 for AVTP implementation" ON)

if(OPENAVNU_USE_OPEN1722)
    add_subdirectory("thirdparty/open1722")
    set(OPEN1722_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/open1722/include")
    set(OPEN1722_LIBRARIES open1722)
endif()
```

#### Step 1.3: Create AVTP Wrapper Layer
Create new wrapper in `lib/avtp_pipeline/avtp_open1722/`:
```
lib/avtp_pipeline/avtp_open1722/
├── openavb_avtp_open1722.h      # Wrapper interface
├── openavb_avtp_open1722.c      # Implementation
├── openavb_avtp_adapter.h       # Adaptation layer
└── CMakeLists.txt               # Build configuration
```

### Phase 2: API Adaptation Layer

#### Key Mapping Requirements:
1. **Stream Management**: Map OpenAvnu's stream lifecycle to Open1722 packet handling
2. **Buffer Management**: Integrate with OpenAvnu's media queue system
3. **Timestamp Handling**: Preserve existing timing mechanisms
4. **Error Handling**: Maintain OpenAvnu error reporting conventions

#### Core Adapter Functions:
```c
// Core AVTP stream operations (maintaining existing API)
openavbRC openavbAvtpOpen1722StreamCreate(avtp_stream_t *pStream);
openavbRC openavbAvtpOpen1722StreamDestroy(avtp_stream_t *pStream);
openavbRC openavbAvtpOpen1722TxBuffer(void *pv, avtp_info_t *pInfo);
openavbRC openavbAvtpOpen1722RxBuffer(void *pv, avtp_info_t *pInfo);

// Format-specific wrappers
openavbRC openavbAvtpOpen1722CreateAAF(/* parameters */);
openavbRC openavbAvtpOpen1722CreateCVF(/* parameters */);
openavbRC openavbAvtpOpen1722CreateACF(/* automotive formats */);
```

### Phase 3: Format Migration

#### 3.1: Audio Formats (Priority 1)
- **AAF (Audio AVTP Format)**: Direct mapping from current audio implementation
- **61883-6**: Migrate existing 61883 support to Open1722

#### 3.2: Video Formats (Priority 2)
- **CVF (Compressed Video Format)**: H.264 support
- **RVF (Raw Video Format)**: Uncompressed video
- **MJPEG/JPEG2000**: New formats available in Open1722

#### 3.3: Automotive Formats (Priority 3)
- **ACF-CAN**: CAN message transport
- **ACF-LIN**: LIN bus data
- **ACF-FlexRay**: FlexRay network data

### Phase 4: Mapping Module Updates

Update existing mapping modules to use Open1722:
```
lib/avtp_pipeline/map_aaf/     # Audio mapping
lib/avtp_pipeline/map_mjpeg/   # MJPEG (new)
lib/avtp_pipeline/map_h264/    # H.264 video
lib/avtp_pipeline/map_can/     # CAN transport (new)
```

### Phase 5: Testing & Validation

#### 5.1: Unit Tests
- Create comprehensive tests for the adapter layer
- Validate packet format compliance
- Test interoperability with existing talker/listener

#### 5.2: Integration Tests
- End-to-end streaming tests
- Performance comparison with legacy implementation
- Automotive format validation

#### 5.3: Compliance Testing
- IEEE 1722-2016 compliance verification
- AVDECC compatibility testing
- Automotive standard compliance (if applicable)

## Implementation Timeline

### Week 1-2: Foundation
- [ ] Add Open1722 submodule
- [ ] CMake integration
- [ ] Basic wrapper skeleton

### Week 3-4: Core Adaptation
- [ ] Stream lifecycle management
- [ ] Buffer integration
- [ ] Basic AAF support

### Week 5-6: Format Support
- [ ] Complete audio formats
- [ ] Basic video format support
- [ ] Error handling integration

### Week 7-8: Testing & Refinement
- [ ] Unit tests
- [ ] Integration testing
- [ ] Performance optimization

### Week 9-10: Documentation & Deployment
- [ ] Documentation updates
- [ ] Migration guide
- [ ] Backward compatibility verification

## Backward Compatibility Strategy

### Dual Implementation Support
Initially maintain both implementations:
```c
#ifdef OPENAVNU_USE_OPEN1722
    // Use Open1722 implementation
    return openavbAvtpOpen1722TxBuffer(pv, pInfo);
#else
    // Use legacy implementation
    return openavbAvtpTxBuffer(pv, pInfo);
#endif
```

### Configuration Options
```cmake
option(OPENAVNU_USE_OPEN1722 "Use Open1722 for AVTP" ON)
option(OPENAVNU_LEGACY_AVTP "Keep legacy AVTP support" OFF)
```

### Migration Path
1. **Phase 1**: Dual support (default: Open1722)
2. **Phase 2**: Deprecate legacy (warnings)
3. **Phase 3**: Remove legacy implementation

## Benefits of Modernization

### Technical Benefits
1. **Standards Compliance**: Full IEEE 1722-2016 support
2. **Reduced Maintenance**: Leverage community-maintained library
3. **Enhanced Formats**: Support for modern and automotive AVTP formats
4. **Better Testing**: Comprehensive test coverage from Open1722
5. **Performance**: Optimized packet handling

### Strategic Benefits
1. **Industry Alignment**: Using widely-adopted library
2. **Automotive Ready**: Built-in support for automotive protocols
3. **Future-Proof**: Active development ensures ongoing standards support
4. **Community Support**: Benefit from broader development community

## Risks & Mitigation

### Technical Risks
1. **API Changes**: Wrapper layer minimizes impact
2. **Performance Impact**: Benchmark and optimize critical paths
3. **Feature Gaps**: Identify and address missing features early

### Mitigation Strategies
1. **Comprehensive Testing**: Extensive validation before deployment
2. **Phased Rollout**: Gradual migration with fallback options
3. **Community Engagement**: Work with Open1722 maintainers on gaps

## Success Criteria

1. **Functional Parity**: All existing AVTP functionality preserved
2. **Performance**: No significant performance degradation
3. **Standards Compliance**: Pass IEEE 1722-2016 compliance tests
4. **Automotive Support**: Successfully implement automotive AVTP formats
5. **Maintainability**: Reduced complexity in AVTP-specific code

---

**Next Steps**: Begin Phase 1 implementation with submodule integration and basic CMake setup.
