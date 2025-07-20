# IEEE 1722-2016 Phase 4 Gap Closure Implementation
# CMake configuration for complete "tutti completo" build

# =============================
# Phase 4: Critical Subtype Corrections
# =============================

# Add IEEE 1722-2016 Standards Library with corrected subtypes
add_library(ieee_1722_2016_standards INTERFACE)
target_include_directories(ieee_1722_2016_standards INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards
)

# =============================
# Phase 5: Industrial Protocol Implementation
# =============================

# Industrial Protocols Library
add_library(industrial_protocols INTERFACE)
target_include_directories(industrial_protocols INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards
)

# Industrial Protocol Test Executables
if(OPENAVNU_BUILD_TESTING)
    add_executable(industrial_protocol_tests
        lib/Standards/test_industrial_protocols.cpp
    )
    target_link_libraries(industrial_protocol_tests
        industrial_protocols
        ieee_1722_2016_standards
        ${CMAKE_THREAD_LIBS_INIT}
    )
    target_compile_features(industrial_protocol_tests PRIVATE cxx_std_17)
    
    add_test(NAME IndustrialProtocolTests COMMAND industrial_protocol_tests)
endif()

# =============================
# Phase 6: Security & Cryptography
# =============================

# Security Library with AES-SIV and ECC support
add_library(avtp_security INTERFACE)
target_include_directories(avtp_security INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards
)

# Find OpenSSL for cryptographic operations
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    target_link_libraries(avtp_security INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(avtp_security INTERFACE OPENAVNU_OPENSSL_SUPPORT=1)
endif()

# Security Test Executables
if(OPENAVNU_BUILD_TESTING)
    add_executable(security_cryptography_tests
        lib/Standards/test_security_cryptography.cpp
    )
    target_link_libraries(security_cryptography_tests
        avtp_security
        ieee_1722_2016_standards
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CMAKE_THREAD_LIBS_INIT}
    )
    target_compile_features(security_cryptography_tests PRIVATE cxx_std_17)
    
    add_test(NAME SecurityCryptographyTests COMMAND security_cryptography_tests)
endif()

# =============================
# Phase 7: AVDECC Integration
# =============================

# AVDECC Protocol Stack Library
add_library(avdecc_integration INTERFACE)
target_include_directories(avdecc_integration INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards
)

# Link with existing AVDECC library if available
if(TARGET la_avdecc_cxx)
    target_link_libraries(avdecc_integration INTERFACE la_avdecc_cxx)
    target_compile_definitions(avdecc_integration INTERFACE OPENAVNU_LA_AVDECC_SUPPORT=1)
endif()

# AVDECC Test Executables
if(OPENAVNU_BUILD_TESTING)
    add_executable(avdecc_integration_tests
        lib/Standards/test_avdecc_integration.cpp
    )
    target_link_libraries(avdecc_integration_tests
        avdecc_integration
        ieee_1722_2016_standards
        ${CMAKE_THREAD_LIBS_INIT}
    )
    target_compile_features(avdecc_integration_tests PRIVATE cxx_std_17)
    
    add_test(NAME AVDECCIntegrationTests COMMAND avdecc_integration_tests)
endif()

# =============================
# Complete Phase 4 Test Suite
# =============================

if(OPENAVNU_BUILD_TESTING)
    add_executable(ieee_1722_2016_phase4_complete_tests
        lib/Standards/test_phase4_complete.cpp
    )
    target_link_libraries(ieee_1722_2016_phase4_complete_tests
        ieee_1722_2016_standards
        industrial_protocols
        avtp_security
        avdecc_integration
        ${CMAKE_THREAD_LIBS_INIT}
    )
    if(OpenSSL_FOUND)
        target_link_libraries(ieee_1722_2016_phase4_complete_tests OpenSSL::SSL OpenSSL::Crypto)
    endif()
    target_compile_features(ieee_1722_2016_phase4_complete_tests PRIVATE cxx_std_17)
    
    # This is the comprehensive test that validates all Phase 4-7 implementations
    add_test(NAME IEEE1722Phase4CompleteTests COMMAND ieee_1722_2016_phase4_complete_tests)
endif()

# =============================
# Installation Configuration
# =============================

# Install headers for IEEE 1722-2016 complete implementation
install(FILES
    lib/Standards/ieee_1722_2016_streaming.h
    lib/Standards/industrial_protocols.h
    lib/Standards/security_cryptography.h
    lib/Standards/avdecc_integration.h
    lib/Standards/ieee_1722_2016_phase4_tests.h
    DESTINATION include/openavnu/ieee_1722_2016
    COMPONENT Development
)

# =============================
# Build Options and Features
# =============================

# Enable IEEE 1722-2016 Phase 4 complete implementation
option(OPENAVNU_BUILD_IEEE_1722_2016_PHASE4 "Build IEEE 1722-2016 Phase 4 complete implementation" ON)

if(OPENAVNU_BUILD_IEEE_1722_2016_PHASE4)
    # Create master interface library that includes all Phase 4 components
    add_library(ieee_1722_2016_complete INTERFACE)
    target_link_libraries(ieee_1722_2016_complete INTERFACE
        ieee_1722_2016_standards
        industrial_protocols
        avtp_security
        avdecc_integration
    )
    
    # Set compile definitions to enable all features
    target_compile_definitions(ieee_1722_2016_complete INTERFACE
        OPENAVNU_IEEE_1722_2016_PHASE4=1
        OPENAVNU_INDUSTRIAL_PROTOCOLS=1
        OPENAVNU_SECURITY_CRYPTOGRAPHY=1
        OPENAVNU_AVDECC_INTEGRATION=1
    )
    
    message(STATUS "IEEE 1722-2016 Phase 4 'Tutti Completo' implementation enabled")
    message(STATUS "  - Critical Subtype Corrections: ON")
    message(STATUS "  - Industrial Protocols (CAN/LIN/MOST/FlexRay/Sensor): ON")
    message(STATUS "  - Security & Cryptography (AES-SIV/ECC): ON")
    message(STATUS "  - AVDECC Integration (ADP/AECP/ACMP): ON")
endif()

# Enable specific industrial protocols
option(OPENAVNU_ENABLE_CAN_PROTOCOL "Enable CAN (Controller Area Network) protocol support" ON)
option(OPENAVNU_ENABLE_LIN_PROTOCOL "Enable LIN (Local Interconnect Network) protocol support" ON)
option(OPENAVNU_ENABLE_MOST_PROTOCOL "Enable MOST (Media Oriented Systems Transport) protocol support" ON)
option(OPENAVNU_ENABLE_FLEXRAY_PROTOCOL "Enable FlexRay protocol support" ON)
option(OPENAVNU_ENABLE_SENSOR_PROTOCOLS "Enable generic sensor data protocol support" ON)

if(OPENAVNU_ENABLE_CAN_PROTOCOL)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_CAN_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_LIN_PROTOCOL)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_LIN_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_MOST_PROTOCOL)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_MOST_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_FLEXRAY_PROTOCOL)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_FLEXRAY_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_SENSOR_PROTOCOLS)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_SENSOR_SUPPORT=1)
endif()

# Security and cryptography options
option(OPENAVNU_ENABLE_AES_SIV "Enable AES-SIV encryption support" ON)
option(OPENAVNU_ENABLE_ECC_CRYPTO "Enable ECC cryptography and digital signatures" ON)
option(OPENAVNU_ENABLE_REPLAY_PROTECTION "Enable replay protection for secure streams" ON)

if(OPENAVNU_ENABLE_AES_SIV)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_AES_SIV_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_ECC_CRYPTO)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_ECC_CRYPTO_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_REPLAY_PROTECTION)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_REPLAY_PROTECTION=1)
endif()

# AVDECC integration options
option(OPENAVNU_ENABLE_MILAN_COMPATIBILITY "Enable Milan AVDECC compatibility mode" ON)
option(OPENAVNU_ENABLE_AEM_CHECKSUMS "Enable AEM descriptor checksums for integrity" ON)

if(OPENAVNU_ENABLE_MILAN_COMPATIBILITY)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_MILAN_SUPPORT=1)
endif()

if(OPENAVNU_ENABLE_AEM_CHECKSUMS)
    target_compile_definitions(ieee_1722_2016_complete INTERFACE OPENAVNU_AEM_CHECKSUMS=1)
endif()

# =============================
# Compiler and Platform Configuration
# =============================

# Set C++17 as minimum standard for Phase 4 implementation
if(OPENAVNU_BUILD_IEEE_1722_2016_PHASE4)
    set_property(TARGET ieee_1722_2016_complete PROPERTY CXX_STANDARD 17)
    set_property(TARGET ieee_1722_2016_complete PROPERTY CXX_STANDARD_REQUIRED ON)
endif()

# Platform-specific configurations
if(WIN32)
    # Windows-specific settings for Phase 4
    target_compile_definitions(ieee_1722_2016_complete INTERFACE
        OPENAVNU_WINDOWS_PLATFORM=1
        _CRT_SECURE_NO_WARNINGS=1
    )
    
    # Link with Windows-specific libraries for cryptography
    target_link_libraries(ieee_1722_2016_complete INTERFACE
        ws2_32
        crypt32
        bcrypt
    )
elseif(UNIX)
    # Unix/Linux-specific settings for Phase 4
    target_compile_definitions(ieee_1722_2016_complete INTERFACE
        OPENAVNU_UNIX_PLATFORM=1
    )
    
    # Link with pthread for threading support
    find_package(Threads REQUIRED)
    target_link_libraries(ieee_1722_2016_complete INTERFACE Threads::Threads)
endif()

# =============================
# Documentation Generation
# =============================

# Generate documentation for Phase 4 implementation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND AND OPENAVNU_BUILD_IEEE_1722_2016_PHASE4)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs/ieee_1722_2016_phase4)
    set(DOXYGEN_INPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/Standards)
    
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.phase4.in
                   ${CMAKE_BINARY_DIR}/Doxyfile.phase4 @ONLY)
    
    add_custom_target(ieee_1722_2016_phase4_docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile.phase4
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating IEEE 1722-2016 Phase 4 documentation with Doxygen"
        VERBATIM
    )
endif()

# =============================
# Summary Report
# =============================

if(OPENAVNU_BUILD_IEEE_1722_2016_PHASE4)
    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "IEEE 1722-2016 Phase 4 'Tutti Completo'")
    message(STATUS "Complete Implementation Summary")
    message(STATUS "========================================")
    message(STATUS "Phase 4: Critical Subtype Corrections - ENABLED")
    message(STATUS "  ✓ Official IEEE 1722-2016 Table 6 subtype values")
    message(STATUS "  ✓ AVDECC protocol subtypes (ADP/AECP/ACMP/MAAP)")
    message(STATUS "  ✓ Backward compatibility aliases")
    message(STATUS "")
    message(STATUS "Phase 5: Industrial Protocol Implementation - ENABLED")
    message(STATUS "  ✓ CAN (Controller Area Network) - ${OPENAVNU_ENABLE_CAN_PROTOCOL}")
    message(STATUS "  ✓ LIN (Local Interconnect Network) - ${OPENAVNU_ENABLE_LIN_PROTOCOL}")
    message(STATUS "  ✓ MOST (Media Oriented Systems Transport) - ${OPENAVNU_ENABLE_MOST_PROTOCOL}")
    message(STATUS "  ✓ FlexRay automotive protocol - ${OPENAVNU_ENABLE_FLEXRAY_PROTOCOL}")
    message(STATUS "  ✓ Generic sensor data protocols - ${OPENAVNU_ENABLE_SENSOR_PROTOCOLS}")
    message(STATUS "")
    message(STATUS "Phase 6: Security & Cryptography - ENABLED")
    message(STATUS "  ✓ AES-SIV authenticated encryption - ${OPENAVNU_ENABLE_AES_SIV}")
    message(STATUS "  ✓ ECC cryptography and ECDSA signatures - ${OPENAVNU_ENABLE_ECC_CRYPTO}")
    message(STATUS "  ✓ Replay protection - ${OPENAVNU_ENABLE_REPLAY_PROTECTION}")
    message(STATUS "  ✓ Key management and security associations")
    message(STATUS "")
    message(STATUS "Phase 7: AVDECC Integration - ENABLED")
    message(STATUS "  ✓ ADP (AVDECC Discovery Protocol)")
    message(STATUS "  ✓ AECP (AVDECC Enumeration and Control Protocol)")
    message(STATUS "  ✓ ACMP (AVDECC Connection Management Protocol)")
    message(STATUS "  ✓ Milan compatibility mode - ${OPENAVNU_ENABLE_MILAN_COMPATIBILITY}")
    message(STATUS "  ✓ AEM descriptor checksums - ${OPENAVNU_ENABLE_AEM_CHECKSUMS}")
    message(STATUS "")
    if(OPENAVNU_BUILD_TESTING)
        message(STATUS "Testing Framework: ENABLED")
        message(STATUS "  ✓ Comprehensive test suites for all phases")
        message(STATUS "  ✓ Industrial protocol validation tests")
        message(STATUS "  ✓ Security and cryptography validation tests")
        message(STATUS "  ✓ AVDECC integration validation tests")
    else()
        message(STATUS "Testing Framework: DISABLED")
        message(STATUS "  Note: Enable OPENAVNU_BUILD_TESTING to run validation tests")
    endif()
    message(STATUS "========================================")
    message(STATUS "")
endif()
