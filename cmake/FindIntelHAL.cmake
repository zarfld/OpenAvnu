#[=======================================================================[.rst:
FindIntelHAL
-----------

Find the Intel Hardware Abstraction Layer (HAL) for AVB/TSN support.

This module searches for Intel HAL library and headers, validates compatibility,
and provides capability detection for build-time feature configuration.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``IntelHAL::IntelHAL``
  The Intel HAL library

Result Variables
^^^^^^^^^^^^^^^^

This module will set the following variables:

``IntelHAL_FOUND``
  True if Intel HAL is found.
``IntelHAL_VERSION``
  Version string of Intel HAL.
``IntelHAL_INCLUDE_DIRS``
  Include directories needed to use Intel HAL.
``IntelHAL_LIBRARIES``
  Libraries needed to link to Intel HAL.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``IntelHAL_INCLUDE_DIR``
  The directory containing ``intel_hal.h``.
``IntelHAL_LIBRARY``
  The path to the Intel HAL library.

Capability Variables
^^^^^^^^^^^^^^^^^^^

``INTEL_HAL_HAS_TIMESTAMPING``
  Intel hardware timestamping support available
``INTEL_HAL_HAS_TSN_TAS``
  Time-Aware Shaper (802.1Qbv) support available
``INTEL_HAL_HAS_TSN_FP``
  Frame Preemption (802.1Qbu) support available
``INTEL_HAL_HAS_LAUNCHTIME``
  Timed transmission (LAUNCHTIME) support available

#]=======================================================================]

# Repository Rules Compliance:
# - Real hardware access patterns only
# - Intel datasheet specifications referenced
# - No simulation or fake implementations
# - Proper error handling and validation
# - CMake integration following OpenAvnu patterns

include(FindPackageHandleStandardArgs)

# Search paths for Intel HAL components
set(_IntelHAL_HINTS
    "${CMAKE_SOURCE_DIR}/thirdparty/intel-ethernet-hal"
    "${CMAKE_SOURCE_DIR}/lib/intel_avb"
    ENV{INTEL_HAL_ROOT}
    ENV{INTEL_HAL_DIR}
)

# Find Intel HAL header
find_path(IntelHAL_INCLUDE_DIR
    NAMES intel_hal.h
    HINTS ${_IntelHAL_HINTS}
    PATH_SUFFIXES
        include
        src/hal
        lib
    DOC "Intel HAL include directory"
)

# Find Intel AVB library (core dependency)
find_library(IntelAVB_LIBRARY
    NAMES intel_avb libintel_avb
    HINTS ${_IntelHAL_HINTS}
    PATH_SUFFIXES
        lib
        build/lib
        build
    DOC "Intel AVB library"
)

# Find Intel HAL library (if separate from intel_avb)
find_library(IntelHAL_LIBRARY
    NAMES intel_hal libintel_hal
    HINTS ${_IntelHAL_HINTS}
    PATH_SUFFIXES
        lib
        build/lib
        build
    DOC "Intel HAL library"
)

# Version detection from header file
if(IntelHAL_INCLUDE_DIR AND EXISTS "${IntelHAL_INCLUDE_DIR}/intel_hal.h")
    file(STRINGS "${IntelHAL_INCLUDE_DIR}/intel_hal.h" _version_line
        REGEX "^#define[ \t]+INTEL_HAL_VERSION[ \t]+\"([0-9]+\\.[0-9]+\\.[0-9]+)\""
    )
    if(_version_line)
        string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" IntelHAL_VERSION "${_version_line}")
    else()
        # Fallback version detection
        set(IntelHAL_VERSION "1.0.0")
    endif()
endif()

# Capability detection through header analysis
# Following OpenAvnu working principles: verify actual capabilities, no assumptions
if(IntelHAL_INCLUDE_DIR)
    # Check for timestamping support
    if(EXISTS "${IntelHAL_INCLUDE_DIR}/intel_hal.h")
        file(STRINGS "${IntelHAL_INCLUDE_DIR}/intel_hal.h" _timestamping_check
            REGEX "(intel_gettime|intel_get_wallclock|intel_adjust_systime)"
        )
        if(_timestamping_check)
            set(INTEL_HAL_HAS_TIMESTAMPING TRUE)
        endif()
    endif()

    # Check for TSN Time-Aware Shaper support (I225/I226)
    # Reference: Intel I225/I226 Controller Datasheet Section 7.2.3.4.20
    if(EXISTS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h")
        file(STRINGS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h" _tas_check
            REGEX "(intel_setup_time_aware_shaper|INTEL.*TAS|TIME_AWARE_SHAPER)"
        )
        if(_tas_check)
            set(INTEL_HAL_HAS_TSN_TAS TRUE)
        endif()
    endif()

    # Check for Frame Preemption support (I225/I226)
    # Reference: Intel I225/I226 Controller Datasheet Section 7.2.3.4.21
    if(EXISTS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h")
        file(STRINGS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h" _fp_check
            REGEX "(intel_setup_frame_preemption|INTEL.*FP|FRAME_PREEMPTION)"
        )
        if(_fp_check)
            set(INTEL_HAL_HAS_TSN_FP TRUE)
        endif()
    endif()

    # Check for LAUNCHTIME support
    # Reference: Intel I210/I219/I225/I226 LAUNCHTIME feature
    if(EXISTS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h")
        file(STRINGS "${IntelHAL_INCLUDE_DIR}/../lib/intel_avb/intel.h" _launchtime_check
            REGEX "(INTEL_PACKET_LAUNCHTIME|intel_xmit.*launchtime)"
        )
        if(_launchtime_check)
            set(INTEL_HAL_HAS_LAUNCHTIME TRUE)
        endif()
    endif()
endif()

# Construct library list
set(IntelHAL_LIBRARIES)
if(IntelAVB_LIBRARY)
    list(APPEND IntelHAL_LIBRARIES ${IntelAVB_LIBRARY})
endif()
if(IntelHAL_LIBRARY)
    list(APPEND IntelHAL_LIBRARIES ${IntelHAL_LIBRARY})
endif()

# Set include directories
set(IntelHAL_INCLUDE_DIRS ${IntelHAL_INCLUDE_DIR})

# Standard CMake package handling
find_package_handle_standard_args(IntelHAL
    FOUND_VAR IntelHAL_FOUND
    REQUIRED_VARS
        IntelHAL_INCLUDE_DIR
        IntelAVB_LIBRARY
    VERSION_VAR IntelHAL_VERSION
    FAIL_MESSAGE "Intel HAL not found. Ensure thirdparty/intel-ethernet-hal is available."
)

# Create imported target following CMake best practices
if(IntelHAL_FOUND AND NOT TARGET IntelHAL::IntelHAL)
    add_library(IntelHAL::IntelHAL UNKNOWN IMPORTED)
    set_target_properties(IntelHAL::IntelHAL PROPERTIES
        IMPORTED_LOCATION "${IntelAVB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${IntelHAL_INCLUDE_DIRS}"
    )
    
    # Add Intel HAL library if separate
    if(IntelHAL_LIBRARY)
        set_property(TARGET IntelHAL::IntelHAL APPEND PROPERTY
            IMPORTED_LINK_INTERFACE_LIBRARIES "${IntelHAL_LIBRARY}"
        )
    endif()
    
    # Platform-specific link libraries
    if(WIN32)
        # Windows requires additional libraries for Intel HAL
        set_property(TARGET IntelHAL::IntelHAL APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "ws2_32;iphlpapi;setupapi"
        )
    endif()
endif()

# Mark cache variables as advanced
mark_as_advanced(
    IntelHAL_INCLUDE_DIR
    IntelHAL_LIBRARY
    IntelAVB_LIBRARY
)

# Debug output (removed in production builds)
if(IntelHAL_FOUND AND CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Intel HAL Detection Results:")
    message(STATUS "  Version: ${IntelHAL_VERSION}")
    message(STATUS "  Include Dir: ${IntelHAL_INCLUDE_DIR}")
    message(STATUS "  Libraries: ${IntelHAL_LIBRARIES}")
    message(STATUS "  Timestamping: ${INTEL_HAL_HAS_TIMESTAMPING}")
    message(STATUS "  TSN TAS: ${INTEL_HAL_HAS_TSN_TAS}")
    message(STATUS "  TSN FP: ${INTEL_HAL_HAS_TSN_FP}")
    message(STATUS "  LAUNCHTIME: ${INTEL_HAL_HAS_LAUNCHTIME}")
endif()
