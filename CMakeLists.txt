cmake_minimum_required (VERSION 3.10)
project (open-avb)

# Set policy to handle older CMake versions in submodules
if(POLICY CMP0022)
    cmake_policy(SET CMP0022 NEW)
endif()
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

# Set minimum policy version to handle submodules with older CMake requirements
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

enable_testing()

include(CheckCXXCompilerFlag)

# Set C++11 standard for modern CMake
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.1")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    # Legacy C++11 detection for older CMake versions
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()

# Third-party libraries
add_subdirectory("thirdparty/cpputest")

# Optional gPTP daemon for time synchronization
option(OPENAVNU_BUILD_GPTP "Build gPTP daemon for time synchronization" ON)

if(OPENAVNU_BUILD_GPTP)
    message(STATUS "Building with gPTP daemon support")
    add_subdirectory("thirdparty/gptp")
endif()

# Optional Open1722 AVTP library for modernized AVTP implementation
option(OPENAVNU_USE_OPEN1722 "Use Open1722 for AVTP implementation" ON)

if(OPENAVNU_USE_OPEN1722)
    message(STATUS "Building with Open1722 AVTP library support")
    
    # Set Open1722 include directory
    set(OPEN1722_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/open1722/include")
    
    # Create Open1722 library manually for Windows compatibility
    if(WIN32)
        # For Windows, create the Open1722 target manually
        file(GLOB_RECURSE OPEN1722_SOURCES 
            "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/open1722/src/*.c"
        )
        
        add_library(open1722 STATIC ${OPEN1722_SOURCES})
        target_include_directories(open1722 PUBLIC ${OPEN1722_INCLUDE_DIRS})
        
        # MSVC compatibility definitions
        if(MSVC)
            # Use forced include to inject our compatibility header
            target_compile_options(open1722 PRIVATE
                /FI"${CMAKE_CURRENT_SOURCE_DIR}/lib/avtp_pipeline/avtp_open1722/open1722_msvc_compat.h"
                /wd4200  # nonstandard extension used: zero-sized array in struct/union
                /wd4244  # conversion warnings
                /wd4996  # deprecated function warnings
                /Zp1     # force structure packing to 1 byte for compatibility
            )
            target_compile_definitions(open1722 PRIVATE _CRT_SECURE_NO_WARNINGS)
        else()
            target_compile_definitions(open1722 PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        
        set(OPEN1722_LIBRARIES open1722)
        message(STATUS "Created Open1722 static library for Windows with MSVC compatibility")
    else()
        # For Linux/other platforms, use the standard Open1722 build
        add_subdirectory("thirdparty/open1722")
        set(OPEN1722_LIBRARIES open1722)
    endif()
    
    add_definitions(-DOPENAVNU_USE_OPEN1722)
endif()

# OpenAvnu components
add_subdirectory("daemons/common/tests")
add_subdirectory("daemons/mrpd")
add_subdirectory("daemons/maap")
add_subdirectory("lib/avtp_pipeline/tests")

# Add Open1722 wrapper if enabled
if(OPENAVNU_USE_OPEN1722)
    add_subdirectory("lib/avtp_pipeline/avtp_open1722")
endif()

message("
-------------------------------------------------------
OpenAvnu Build information

Current compiler options:
    CC:                                 ${CMAKE_C_COMPILER}
    CXX:                                ${CMAKE_CXX_COMPILER}
    OpenAvnu CFLAGS:                    ${CMAKE_C_FLAGS}
    OpenAvnu CXXFLAGS:                  ${CMAKE_CXX_FLAGS}
    OpenAvnu LDFLAGS:                   ${CMAKE_LD_FLAGS}

Features configured in OpenAvnu root CMakeFile.txt:
    Memory Leak Detection:              ${MEMORY_LEAK_DETECTION}
    Compiling Extensions:               ${EXTENSIONS}
    Support Long Long:                  ${LONGLONG}
    Use OpenAvnu flags:                 ${CMAKE_FLAGS}

    Using Standard C library:           ${STD_C}
    Using Standard C++ library:         ${STD_CPP}
    Using C++11 library:                ${C++11}

-------------------------------------------------------
")
