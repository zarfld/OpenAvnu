# OpenAvnu Submodule Migration Plan

## Overview
This document outlines the strategy for converting `intel_avb` and `Standards` folders into separate Git submodules to improve code organization, maintainability, and enable independent development cycles.

## 🎯 **Migration Benefits**

### **intel_avb → intel-avb-hal Submodule**
- **Hardware Independence**: Separate Intel hardware support from core OpenAvnu
- **Development Velocity**: Intel hardware updates independent from OpenAvnu releases
- **Testing Isolation**: Dedicated hardware testing without affecting main build
- **Collaboration**: Easier contribution from Intel and hardware teams

### **Standards → openavnu-ieee-standards Submodule**
- **Protocol Independence**: IEEE standards evolve separately from OpenAvnu core
- **Cross-Project Reuse**: Standards implementations usable in other AVB projects
- **Specification Tracking**: Version control aligns with IEEE standard releases
- **Size Management**: Large protocol implementations don't bloat main repository

## 🚀 **Phase 1: intel-avb-hal Submodule Migration**

### **Step 1: Create New Repository**
```bash
# Create new repository: openavnu/intel-avb-hal
# Initial structure:
intel-avb-hal/
├── README.md
├── CMakeLists.txt
├── include/
│   └── intel.h
├── lib/
│   ├── intel.c
│   ├── intel_i210.c
│   ├── intel_i219.c
│   ├── intel_i225.c
│   └── Makefile
├── kmod/
│   └── (kernel modules)
└── tests/
    └── test_intel.c
```

### **Step 2: Migrate Content**
```bash
# Copy from lib/intel_avb/ to new repository
cp -r lib/intel_avb/* ../intel-avb-hal/
cd ../intel-avb-hal
git init
git add .
git commit -m "Initial migration from OpenAvnu/lib/intel_avb"
```

### **Step 3: Add as Submodule**
```bash
# In OpenAvnu repository
git submodule add https://github.com/openavnu/intel-avb-hal.git thirdparty/intel-avb-hal
git add .gitmodules thirdparty/intel-avb-hal
git commit -m "Add intel-avb-hal as submodule"
```

### **Step 4: Update Build System**
```cmake
# CMakeLists.txt updates
if(OPENAVNU_BUILD_INTEL_AVB)
    add_subdirectory(thirdparty/intel-avb-hal)
    target_link_libraries(openavb_common intel_avb_hal)
endif()
```

## 🚀 **Phase 2: openavnu-ieee-standards Submodule Migration**

### **Step 1: Create Standards Repository**
```bash
# Create new repository: openavnu/openavnu-ieee-standards
# Structure:
openavnu-ieee-standards/
├── README.md
├── CMakeLists.txt
├── ieee-1722-2016/
│   ├── ieee_1722_2016_streaming.h
│   └── ieee_1722_2016_streaming.cpp
├── ieee-1722.1-2021/
│   ├── ieee_1722_1_2021_complete_entity.h
│   └── ieee_1722_1_2021_complete_entity.cpp
├── avnu-milan/
│   ├── AVnu_Milan_1_2-2023_complete.h
│   ├── AVnu_Milan_1_2-2023_complete.cpp
│   └── AVnu_Milan_2_0a-2023.h
└── tests/
    ├── test_1722_2016.cpp
    ├── test_1722_1_2021.cpp
    └── test_milan.cpp
```

### **Step 2: Migrate Standards Content**
```bash
# Migrate Standards folder content
cp -r lib/Standards/* ../openavnu-ieee-standards/
cd ../openavnu-ieee-standards
git init
git add .
git commit -m "Initial migration of IEEE standards implementations"
```

### **Step 3: Add Standards Submodule**
```bash
# In OpenAvnu repository
git submodule add https://github.com/openavnu/openavnu-ieee-standards.git thirdparty/ieee-standards
```

### **Step 4: Update Standards Integration**
```cmake
# CMakeLists.txt for Standards
if(OPENAVNU_BUILD_MILAN_V1_2)
    add_subdirectory(thirdparty/ieee-standards)
    target_link_libraries(openavb_avtp_pipeline ieee_standards)
endif()
```

## 🔄 **Phase 3: Update Existing Code**

### **Update Include Paths**
```cpp
// Before:
#include "../lib/Standards/AVnu_Milan_1_2-2023_complete.h"
#include "../lib/intel_avb/intel.h"

// After:
#include "../thirdparty/ieee-standards/avnu-milan/AVnu_Milan_1_2-2023_complete.h"
#include "../thirdparty/intel-avb-hal/include/intel.h"
```

### **Update CMake References**
```cmake
# Update all references to lib/Standards and lib/intel_avb
find_package(IntelAVBHAL REQUIRED)
find_package(OpenAvnuIEEEStandards REQUIRED)
```

## 📋 **Validation and Testing**

### **Phase 1 Validation: Intel AVB HAL**
```bash
# Test intel-avb-hal submodule
cd thirdparty/intel-avb-hal
make test
./test_intel

# Test OpenAvnu integration
cmake --build . --target intel_hal_validation_test_consolidated
```

### **Phase 2 Validation: IEEE Standards**
```bash
# Test standards submodule
cd thirdparty/ieee-standards
cmake --build . --target test_milan
cmake --build . --target test_1722_2016

# Test examples integration
cmake --build . --target milan_avdecc_entity_example
```

## 🎯 **Migration Timeline**

### **Week 1: intel-avb-hal Submodule**
- [ ] Create intel-avb-hal repository (separate from intel-ethernet-hal)
- [ ] Migrate lib/intel_avb content
- [ ] Add as submodule to OpenAvnu
- [ ] Update build system integration
- [ ] Test hardware validation
- [ ] **Note**: Keep separate from intel-ethernet-hal - they serve different purposes (see INTEL_COMPONENTS_ANALYSIS.md)

### **Week 2: ieee-standards Submodule**
- [ ] Create openavnu-ieee-standards repository
- [ ] Migrate lib/Standards content
- [ ] Add as submodule to OpenAvnu
- [ ] Update examples and integration code
- [ ] Test Milan and IEEE implementations

### **Week 3: Cleanup and Documentation**
- [ ] Remove original lib/intel_avb and lib/Standards folders
- [ ] Update all documentation references
- [ ] Update build instructions
- [ ] Validate complete workflow

## 🔍 **Risks and Mitigation**

### **Risk 1: Build Complexity**
- **Mitigation**: Maintain backward compatibility during transition
- **Solution**: Keep CMake options consistent (OPENAVNU_BUILD_INTEL_AVB, etc.)

### **Risk 2: Submodule Management**
- **Mitigation**: Document submodule workflow clearly
- **Solution**: Provide automation scripts for common operations

### **Risk 3: Development Workflow Changes**
- **Mitigation**: Gradual transition with overlap period
- **Solution**: Update VS Code tasks and automation scripts

## ✅ **Success Criteria**

1. **intel-avb-hal submodule**: Independent Intel hardware support
2. **ieee-standards submodule**: Reusable IEEE protocol implementations
3. **Maintained Functionality**: All existing OpenAvnu features work
4. **Improved Organization**: Cleaner repository structure
5. **Independent Development**: Submodules can evolve independently

## 📚 **Documentation Updates Required**

- [ ] Update README.md with submodule instructions
- [ ] Update build documentation
- [ ] Update contributor guidelines
- [ ] Create submodule management guide
- [ ] Update VS Code tasks and settings

## 🎯 **Long-term Benefits**

1. **Cleaner Architecture**: Better separation of concerns
2. **Independent Versioning**: Components can evolve at their own pace
3. **Easier Collaboration**: Hardware teams can contribute to intel-avb-hal directly
4. **Reusability**: Standards implementations available for other projects
5. **Maintainability**: Smaller, focused repositories easier to maintain
