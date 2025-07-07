# LA_AVDECC CMake Wrapper
# This provides modern PCAP detection for L-Acoustics AVDECC without modifying the submodule

message(STATUS "🔧 Configuring L-Acoustics AVDECC with modern PCAP support")

# Check if PCAP is available from OpenAvnu's centralized detection
if(NOT OPENAVNU_PCAP_FOUND)
    message(WARNING "⚠️  PCAP not available - L-Acoustics AVDECC may not build correctly")
    message(STATUS "Attempting to build L-Acoustics AVDECC without PCAP interface")
    set(BUILD_AVDECC_INTERFACE_PCAP OFF CACHE BOOL "Disable PCAP interface - PCAP not available" FORCE)
else()
    message(STATUS "✅ Using ${OPENAVNU_PCAP_TYPE} for L-Acoustics AVDECC")
    
    if(WIN32)
        # Set up WinPcap compatibility layer for L-Acoustics AVDECC
        # Place compatibility layer outside the submodule to avoid git conflicts
        set(LA_AVDECC_WINPCAP_DIR "${CMAKE_CURRENT_BINARY_DIR}/la_avdecc_pcap_compat")
        
        # Create compatibility symlinks/copies if they don't exist
        if(NOT EXISTS "${LA_AVDECC_WINPCAP_DIR}/Include")
            message(STATUS "Creating PCAP compatibility layer for L-Acoustics AVDECC...")
            
            # Create directories
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Include")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Lib")
            file(MAKE_DIRECTORY "${LA_AVDECC_WINPCAP_DIR}/Lib/x64")
            
            # Copy/link include files
            if(EXISTS "${OPENAVNU_PCAP_INCLUDE_DIRS}")
                file(GLOB PCAP_HEADERS "${OPENAVNU_PCAP_INCLUDE_DIRS}/*.h")
                foreach(HEADER ${PCAP_HEADERS})
                    get_filename_component(HEADER_NAME ${HEADER} NAME)
                    file(COPY ${HEADER} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Include")
                endforeach()
                
                # Copy pcap subdirectory with real headers (some headers expect pcap/pcap.h)
                if(EXISTS "${OPENAVNU_PCAP_INCLUDE_DIRS}/pcap")
                    file(COPY "${OPENAVNU_PCAP_INCLUDE_DIRS}/pcap" DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Include")
                endif()
                
                message(STATUS "✅ Copied PCAP headers to L-Acoustics AVDECC compatibility layer")
            endif()
            
            # Copy/link library files
            if(EXISTS "${OPENAVNU_PCAP_LIBRARY_DIRS}")
                file(GLOB PCAP_LIBS "${OPENAVNU_PCAP_LIBRARY_DIRS}/*.lib" "${OPENAVNU_PCAP_LIBRARY_DIRS}/*.a")
                foreach(LIB ${PCAP_LIBS})
                    get_filename_component(LIB_NAME ${LIB} NAME)
                    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                        file(COPY ${LIB} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Lib/x64")
                    else()
                        file(COPY ${LIB} DESTINATION "${LA_AVDECC_WINPCAP_DIR}/Lib")
                    endif()
                endforeach()
                message(STATUS "✅ Copied PCAP libraries to L-Acoustics AVDECC compatibility layer")
            endif()
            
            # Create README explaining the compatibility layer
            file(WRITE "${LA_AVDECC_WINPCAP_DIR}/README.md"
"# PCAP Compatibility Layer for L-Acoustics AVDECC

This directory contains a compatibility layer that provides PCAP libraries
to the L-Acoustics AVDECC submodule using OpenAvnu's centralized PCAP detection.

**Source:** ${OPENAVNU_PCAP_TYPE} (${OPENAVNU_PCAP_INCLUDE_DIRS})
**Generated:** ${CMAKE_CURRENT_LIST_FILE}
**Architecture:** ${CMAKE_SIZEOF_VOID_P}-bit

This allows L-Acoustics AVDECC to build with modern Npcap libraries
without requiring modifications to the external submodule.
")
        endif()
        
        # Enable PCAP interface with compatibility layer
        set(BUILD_AVDECC_INTERFACE_PCAP ON CACHE BOOL "Enable PCAP interface with compatibility layer" FORCE)
        
        # Set the WinPcap base directory that L-Acoustics AVDECC expects
        set(LA_WINPCAP_BASE_DIR "${LA_AVDECC_WINPCAP_DIR}" CACHE PATH "WinPcap base directory for L-Acoustics AVDECC" FORCE)
        
        message(STATUS "✅ L-Acoustics AVDECC PCAP compatibility layer ready")
        message(STATUS "   WinPcap compatibility path: ${LA_WINPCAP_BASE_DIR}")
    else()
        # Linux: PCAP should work directly
        set(BUILD_AVDECC_INTERFACE_PCAP ON CACHE BOOL "Enable PCAP interface" FORCE)
    endif()
endif()

# Set other L-Acoustics AVDECC build options
set(AVDECC_BUILD_CONTROLLER ON CACHE BOOL "Build AVDECC Controller")
set(AVDECC_BUILD_VIRTUAL ON CACHE BOOL "Build AVDECC Virtual Entity") 
set(AVDECC_BUILD_EXAMPLES OFF CACHE BOOL "Disable AVDECC examples")
set(AVDECC_BUILD_TESTS OFF CACHE BOOL "Disable AVDECC tests for now")
set(AVDECC_BUILD_DOCUMENTATION OFF CACHE BOOL "Disable AVDECC documentation")

# Set C++17 requirement for L-Acoustics AVDECC
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.8")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

message(STATUS "📦 L-Acoustics AVDECC configuration:")
message(STATUS "   PCAP Interface: ${BUILD_AVDECC_INTERFACE_PCAP}")
message(STATUS "   Controller: ${AVDECC_BUILD_CONTROLLER}")
message(STATUS "   Virtual Entity: ${AVDECC_BUILD_VIRTUAL}")

# Include the actual submodule
add_subdirectory("lib/la_avdecc")

# Define flag for conditional compilation in OpenAvnu code
add_definitions(-DOPENAVNU_HAS_LA_AVDECC)
