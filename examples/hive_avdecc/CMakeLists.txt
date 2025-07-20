cmake_minimum_required(VERSION 3.16)
project(HiveCompatibleAVDECC)

# Create the executable
add_executable(hive_compatible_avdecc_entity hive_compatible_avdecc_entity.c)

# Link libraries for Windows
if(WIN32)
    target_link_libraries(hive_compatible_avdecc_entity ws2_32 wpcap)
else()
    target_link_libraries(hive_compatible_avdecc_entity pcap)
endif()

# Set C standard
set_property(TARGET hive_compatible_avdecc_entity PROPERTY C_STANDARD 99)
