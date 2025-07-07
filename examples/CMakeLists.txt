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
