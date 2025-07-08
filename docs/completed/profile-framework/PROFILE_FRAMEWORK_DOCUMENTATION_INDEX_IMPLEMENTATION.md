# OpenAvnu Profile Framework Documentation Index

## 📋 **SINGLE AUTHORITATIVE SPECIFICATION**

**All developers should refer to one document only:**

### **OpenAvnu Profile Framework Specification**
- **File**: `OPENAVNU_PROFILE_FRAMEWORK_SPECIFICATION.md`
- **Status**: ✅ **CURRENT** - Complete specification
- **Description**: The **single, comprehensive specification** for OpenAvnu's capability-based profile framework
- **Contains**:
  - Complete architecture design with pure capability-based logic
  - Implementation guidelines and API reference
  - Built-in profile definitions (IEEE 1722, MILAN, Automotive)
  - Configuration examples and integration points
  - Testing and validation procedures
  - Evolution rationale and design decisions

### **Supporting Documentation**
- **File**: `IEEE_1722_COMPLIANCE_ASSESSMENT.md`
- **Status**: ✅ **CURRENT** - Compliance analysis
- **Description**: Assessment of OpenAvnu's current IEEE 1722 compliance and modernization recommendations
- **Purpose**: Provides context for why the new profile framework was needed

## 🗂️ **Implementation Files**

### Core Framework
- **`lib/avtp_pipeline/include/openavb_profile_framework.h`** - Core interface with capability matrices
- **`lib/avtp_pipeline/profile/openavb_profile_builtin_v2.c`** - Built-in profiles using capability abstraction
- **`lib/avtp_pipeline/profile/openavb_profile_capability_logic.c`** - Pure capability-based implementation examples
- **`lib/avtp_pipeline/profile/openavb_profile_dynamic_switching.c`** - Dynamic profile switching demonstration

### Configuration Examples
- **`examples/profile_framework_examples.ini`** - Configuration examples for different profiles

## ✅ **Documentation Consolidation Complete**

All profile framework documentation has been consolidated into the single, comprehensive specification file. Previous separate documents have been removed to eliminate confusion and ensure developers work from one authoritative source.

### Removed Files (Content Merged into Main Specification)
- ~~`PROFILE_FRAMEWORK_DESIGN.md`~~ - Original design (contained hardcoded boolean approach)
- ~~`PROFILE_FRAMEWORK_DESIGN_V2.md`~~ - V2 design (merged into unified specification)
- ~~`PROFILE_FRAMEWORK_EVOLUTION.md`~~ - Evolution summary (merged into unified specification)

## 📖 **How to Use This Documentation**

1. **Start here**: Read `OPENAVNU_PROFILE_FRAMEWORK_SPECIFICATION.md` - it contains everything you need
2. **For context**: Refer to `IEEE_1722_COMPLIANCE_ASSESSMENT.md` to understand why this framework was created
3. **For implementation**: Follow the code examples in the specification and refer to the implementation files listed above

**Key principle**: All decisions are made based on capability matrix values - no hardcoded profile names, version numbers, or boolean flags anywhere in the decision logic.

---

**Last Updated**: July 6, 2025  
**Status**: All documents synchronized with capability-based approach  
**Next Review**: When new IEEE 1722 specifications are released
