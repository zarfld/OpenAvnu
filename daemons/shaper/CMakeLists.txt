# OpenAvnu Daemon Windows Shaper Implementation
# Windows Traffic Shaper with Intel TSN Integration

cmake_minimum_required(VERSION 3.16)

set(DAEMON_SHAPER_WINDOWS_TARGET "openavnu_shaper_windows")

# Windows Shaper source files
set(SHAPER_WINDOWS_SOURCES
    windows/shaper_windows.c
)

# Windows Shaper header files
set(SHAPER_WINDOWS_HEADERS
    windows/shaper_windows.h
    common/shaper_common.h
)

# Only build on Windows
if(WIN32)
    # Create Windows Shaper daemon executable
    add_executable(${DAEMON_SHAPER_WINDOWS_TARGET}
        ${SHAPER_WINDOWS_SOURCES}
        ${SHAPER_WINDOWS_HEADERS}
    )
    
    # Include directories
    target_include_directories(${DAEMON_SHAPER_WINDOWS_TARGET} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/daemons/common
    )
    
    # Windows-specific system libraries
    target_link_libraries(${DAEMON_SHAPER_WINDOWS_TARGET}
        ws2_32
        iphlpapi
        winmm
        setupapi
        netapi32
    )
    
    # Intel HAL integration if enabled
    if(OPENAVNU_BUILD_INTEL_HAL)
        target_compile_definitions(${DAEMON_SHAPER_WINDOWS_TARGET} PRIVATE
            OPENAVNU_BUILD_INTEL_HAL=1
        )
        
        # Link Intel Ethernet HAL
        if(EXISTS "${PROJECT_SOURCE_DIR}/lib/intel_ethernet_hal")
            target_include_directories(${DAEMON_SHAPER_WINDOWS_TARGET} PRIVATE
                ${PROJECT_SOURCE_DIR}/lib/intel_ethernet_hal/include
            )
            target_link_libraries(${DAEMON_SHAPER_WINDOWS_TARGET}
                ${PROJECT_SOURCE_DIR}/lib/intel_ethernet_hal/lib/intel_ethernet_hal.lib
            )
        endif()
    endif()
    
    # Compiler-specific settings
    if(MSVC)
        target_compile_options(${DAEMON_SHAPER_WINDOWS_TARGET} PRIVATE
            /W4
            /WX
        )
        
        target_compile_definitions(${DAEMON_SHAPER_WINDOWS_TARGET} PRIVATE
            _WIN32_WINNT=0x0A00  # Windows 10
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
    endif()
    
    # Install target
    install(TARGETS ${DAEMON_SHAPER_WINDOWS_TARGET}
        RUNTIME DESTINATION bin
        COMPONENT Daemons
    )
    
    # Add to daemons group
    set_target_properties(${DAEMON_SHAPER_WINDOWS_TARGET} PROPERTIES
        FOLDER "Daemons"
        OUTPUT_NAME "openavnu_shaper_windows"
    )
    
    message(STATUS "Windows Shaper daemon target configured: ${DAEMON_SHAPER_WINDOWS_TARGET}")
else()
    message(STATUS "Windows Shaper daemon skipped (not building on Windows)")
endif()

# Configuration files
if(WIN32)
    # Copy Windows shaper configuration files
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config/shaper_windows.conf.example
        ${CMAKE_BINARY_DIR}/config/shaper_windows.conf.example
        COPYONLY
    )
endif()
