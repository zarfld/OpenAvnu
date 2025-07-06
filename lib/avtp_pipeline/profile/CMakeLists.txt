# OpenAvnu Profile Framework Build Configuration

cmake_minimum_required(VERSION 3.5)

project(openavb_profile_framework)

# Include directories
include_directories(
    ../include
    ../openavb_common
    ../util
    ../types
)

# Source files for profile framework
set(PROFILE_FRAMEWORK_SOURCES
    openavb_profile_builtin_v2.c
    openavb_profile_capability_logic.c
    openavb_profile_dynamic_switching.c
    openavb_profile_integration_examples.c
    openavb_profile_framework_core.c     # Main implementation
    openavb_profile_config_parser.c      # Configuration parsing
    openavb_profile_stream_integration.c # Stream integration
    openavb_profile_la_avdecc.c         # L-Acoustics AVDECC integration
)

# Create profile framework library
add_library(openavb_profile_framework STATIC ${PROFILE_FRAMEWORK_SOURCES})

# Link dependencies
target_link_libraries(openavb_profile_framework
    openavb_common
    openavb_util
)

# Install headers
install(FILES 
    ../include/openavb_profile_framework.h
    DESTINATION include/openavb
)

# Install library
install(TARGETS openavb_profile_framework
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Add to main OpenAvnu build
if(NOT TARGET openavb_avtp_pipeline)
    message(WARNING "Profile framework requires openavb_avtp_pipeline target")
endif()

target_link_libraries(openavb_avtp_pipeline openavb_profile_framework)
