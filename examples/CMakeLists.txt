# OpenAvnu Examples CMakeLists.txt
# Modern CMake configuration for examples that use PCAP
# This allows building examples on Windows without modifying individual Makefiles

cmake_minimum_required(VERSION 3.10)

# Only build examples if PCAP is available
if(NOT OPENAVNU_PCAP_FOUND)
    message(STATUS "⚠️  Skipping PCAP-dependent examples - PCAP library not found")
    return()
endif()

message(STATUS "🔧 Building OpenAvnu examples with ${OPENAVNU_PCAP_TYPE} support")

# Common settings for all examples
if(WIN32)
    # Windows-specific definitions needed for examples
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DHAVE_REMOTE)
endif()

# ==========================================
# Common Objects (shared between examples)  
# ==========================================

# Note: Some examples use POSIX/Linux-specific headers and are not compatible with Windows
# We build only compatible examples on Windows

# async_pcap_storing - Used by multiple examples (POSIX-dependent)
if(UNIX)
    add_library(async_pcap_storing STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/common/async_pcap_storing.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/async_pcap_storing.h
    )
    target_include_directories(async_pcap_storing PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${OPENAVNU_PCAP_INCLUDE_DIRS}
    )
    target_link_libraries(async_pcap_storing OpenAvnu::PCAP)
endif()

# MRP clients - Used by multiple examples (POSIX-dependent)
if(UNIX)
    add_library(mrp_clients STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/common/talker_mrp_client.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/listener_mrp_client.c
        ${CMAKE_CURRENT_SOURCE_DIR}/common/talker_mrp_client.h
        ${CMAKE_CURRENT_SOURCE_DIR}/common/listener_mrp_client.h
    )
    target_include_directories(mrp_clients PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/../daemons/common
    )
endif()

# ==========================================
# Individual Examples (Linux/POSIX only for now)
# ==========================================
# Note: Current examples use POSIX-specific headers (pthread.h, netinet/in.h)
# and are not directly compatible with Windows. They are built on Linux/macOS only.

if(UNIX)
    message(STATUS "🔧 Building POSIX-compatible examples")

    # simple_listener example
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/simple_listener/simple_listener.c")
        add_executable(simple_listener
            ${CMAKE_CURRENT_SOURCE_DIR}/simple_listener/simple_listener.c
        )
        target_include_directories(simple_listener PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${OPENAVNU_PCAP_INCLUDE_DIRS}
        )
        target_link_libraries(simple_listener 
            OpenAvnu::PCAP
            mrp_clients
        )
        
        # Additional libraries for Linux
        target_link_libraries(simple_listener pthread sndfile)
        
        message(STATUS "✅ Configured simple_listener example")
    endif()

    # simple_rx example
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/simple_rx/simple_rx.c")
        add_executable(simple_rx
            ${CMAKE_CURRENT_SOURCE_DIR}/simple_rx/simple_rx.c
        )
        target_include_directories(simple_rx PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${OPENAVNU_PCAP_INCLUDE_DIRS}
        )
        target_link_libraries(simple_rx 
            OpenAvnu::PCAP
            mrp_clients
        )
        
        # Additional libraries for Linux
        target_link_libraries(simple_rx pthread sndfile pci rt)
        
        message(STATUS "✅ Configured simple_rx example")
    endif()

    # send_packet_precisely example
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/send_packet_precisely/send_packet_precisely.c")
        add_executable(send_packet_precisely
            ${CMAKE_CURRENT_SOURCE_DIR}/send_packet_precisely/send_packet_precisely.c
        )
        target_include_directories(send_packet_precisely PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/common
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/atl_avb
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/common
            ${CMAKE_CURRENT_SOURCE_DIR}/../daemons/common
            ${OPENAVNU_PCAP_INCLUDE_DIRS}
        )
        target_link_libraries(send_packet_precisely 
            OpenAvnu::PCAP
            async_pcap_storing
            mrp_clients
        )
        
        # Additional libraries for Linux
        target_link_libraries(send_packet_precisely pthread pci rt m)
        
        message(STATUS "✅ Configured send_packet_precisely example")
    endif()

    # jackd-listener example (Linux only, requires JACK)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/jackd-listener/jack_listener.c")
        find_library(JACK_LIBRARIES jack)
        find_library(SNDFILE_LIBRARIES sndfile)
        
        if(JACK_LIBRARIES AND SNDFILE_LIBRARIES)
            add_executable(jackd_listener
                ${CMAKE_CURRENT_SOURCE_DIR}/jackd-listener/jack_listener.c
            )
            target_include_directories(jackd_listener PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/common
                ${OPENAVNU_PCAP_INCLUDE_DIRS}
            )
            target_link_libraries(jackd_listener 
                OpenAvnu::PCAP
                ${JACK_LIBRARIES}
                ${SNDFILE_LIBRARIES}
                pthread
            )
            message(STATUS "✅ Configured jackd_listener example")
        else()
            message(STATUS "⚠️  Skipping jackd_listener - JACK or sndfile not found")
        endif()
    endif()

else()
    message(STATUS "⚠️  Skipping POSIX examples on Windows - not yet ported")
    message(STATUS "    Examples use pthread.h, netinet/in.h and other POSIX headers")
    message(STATUS "    Consider porting to Windows threading and networking APIs")
endif()

# ==========================================
# Clock Quality Demo (Cross-platform)
# ==========================================
# The clock quality demo is designed to work on both Windows and Linux

message(STATUS "🔧 Building Clock Quality Demo (cross-platform)")

# Add the clock quality demo subdirectory
add_subdirectory(clock_quality_demo)

# ==========================================
# Installation and Packaging
# ==========================================

# Install examples (optional)
option(OPENAVNU_INSTALL_EXAMPLES "Install OpenAvnu examples" OFF)

if(OPENAVNU_INSTALL_EXAMPLES)
    # Install binaries
    install(TARGETS 
        simple_listener
        simple_rx  
        send_packet_precisely
        DESTINATION bin/examples
        COMPONENT examples
    )
    
    # Install documentation and config files
    install(DIRECTORY examples/
        DESTINATION share/openavnu/examples
        COMPONENT examples
        FILES_MATCHING 
        PATTERN "*.md"
        PATTERN "*.txt" 
        PATTERN "*.ini"
        PATTERN "*.cfg"
    )
endif()

message(STATUS "📦 OpenAvnu examples configuration complete")

# ==========================================
# Intel Ethernet HAL Examples
# ==========================================

if(OPENAVNU_BUILD_INTEL_HAL)
    message(STATUS "🔧 Building Intel Ethernet HAL examples")
    
    # I219 VLAN/QoS Configuration Example
    add_executable(i219_vlan_qos_example
        i219_vlan_qos_example_corrected.c
    )
    
    target_include_directories(i219_vlan_qos_example PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/intel-ethernet-hal/include
    )
    
    target_link_libraries(i219_vlan_qos_example 
        PRIVATE 
        intel-ethernet-hal-static
    )
    
    if(WIN32)
        target_link_libraries(i219_vlan_qos_example 
            PRIVATE 
            iphlpapi 
            ws2_32
        )
    endif()
    
    # Set output directory
    set_target_properties(i219_vlan_qos_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    
    # Installation
    install(TARGETS i219_vlan_qos_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )
    
    # Install source as documentation
    install(FILES i219_vlan_qos_example.c
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
    )
endif()

# ==========================================
# ASIO Audio Listener for Windows
# ==========================================

if(WIN32 AND OPENAVNU_PCAP_FOUND)
    message(STATUS "🎵 Building ASIO Audio Listener for Windows")
    
    # Simple ASIO Audio Listener (without external ASIO SDK dependency)
    add_executable(simple_asio_listener
        asio-listener/simple_asio_listener.c
    )
    
    target_include_directories(simple_asio_listener PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/asio-listener
        ${OPENAVNU_PCAP_INCLUDE_DIRS}
    )
    
    target_link_libraries(simple_asio_listener 
        PRIVATE 
        ws2_32
        winmm
        OpenAvnu::PCAP
    )
    
    # Add Windows-specific definitions
    target_compile_definitions(simple_asio_listener PRIVATE
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
    
    # Set output directory
    set_target_properties(simple_asio_listener PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    
    # Full ASIO Audio Listener (requires ASIO SDK)
    if(DEFINED ENV{ASIO_SDK_DIR} AND EXISTS "$ENV{ASIO_SDK_DIR}")
        message(STATUS "🎹 ASIO SDK found - building full ASIO listener")
        
        add_executable(asio_listener
            asio-listener/asio_listener.c
        )
        
        target_include_directories(asio_listener PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/asio-listener
            $ENV{ASIO_SDK_DIR}/common
            $ENV{ASIO_SDK_DIR}/host
            $ENV{ASIO_SDK_DIR}/host/pc
            ${OPENAVNU_PCAP_INCLUDE_DIRS}
        )
        
        target_link_libraries(asio_listener 
            PRIVATE 
            ws2_32
            winmm
            OpenAvnu::PCAP
        )
        
        target_compile_definitions(asio_listener PRIVATE
            WIN32_LEAN_AND_MEAN
            _CRT_SECURE_NO_WARNINGS
        )
        
        set_target_properties(asio_listener PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
        )
    else()
        message(STATUS "ℹ️  ASIO SDK not found - only building simple listener")
        message(STATUS "   Set ASIO_SDK_DIR environment variable for full ASIO support")
    endif()
    
    # Installation
    install(TARGETS simple_asio_listener
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )
    
    if(TARGET asio_listener)
        install(TARGETS asio_listener
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
        )
    endif()
    
    # Install configuration files
    install(FILES 
        asio-listener/asio_config.ini
        asio-listener/asio_listener_config.ini
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples/asio-listener
    )
endif()

# ==========================================
# Simple AVDECC Entity (Windows Compatible)
# ==========================================

if(WIN32)
    # Simple AVDECC Entity for Hive-AVDECC discovery with L-Acoustics library
    add_executable(simple_avdecc_entity
        ${CMAKE_CURRENT_SOURCE_DIR}/simple_avdecc_entity.c
    )
    
    target_link_libraries(simple_avdecc_entity
        ws2_32
        iphlpapi
    )
    
    # Link with L-Acoustics AVDECC library if available
    if(TARGET la_avdecc_cxx)
        target_link_libraries(simple_avdecc_entity la_avdecc_cxx)
        target_include_directories(simple_avdecc_entity PRIVATE
            ${CMAKE_SOURCE_DIR}/lib/la_avdecc/include
        )
        message(STATUS "🎵 AVDECC entity linked with L-Acoustics library")
    else()
        message(STATUS "ℹ️  AVDECC entity using simulation mode (L-Acoustics not available)")
    endif()
    
    # Set output directory
    set_target_properties(simple_avdecc_entity PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/examples/RelWithDebInfo
    )
    
    message(STATUS "✅ Added simple AVDECC entity for Windows (UDP-based)")
endif()

# ==========================================
# L-Acoustics AVDECC Integration Example
# ==========================================

if(WIN32 AND TARGET la_avdecc_cxx)
    # L-Acoustics AVDECC integration example
    add_executable(la_avdecc_integration_example_simple
        ${CMAKE_CURRENT_SOURCE_DIR}/la_avdecc_test.c
    )
    
    # Link with the L-Acoustics AVDECC library
    target_link_libraries(la_avdecc_integration_example_simple
        la_avdecc_cxx
        ws2_32
    )
    
    # Include L-Acoustics AVDECC headers
    target_include_directories(la_avdecc_integration_example_simple PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/la_avdecc/include
    )
    
    # Set output directory
    set_target_properties(la_avdecc_integration_example_simple PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/examples/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/examples/Release
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/examples/RelWithDebInfo
    )
    
    message(STATUS "✅ Added L-Acoustics AVDECC integration example")
endif()
