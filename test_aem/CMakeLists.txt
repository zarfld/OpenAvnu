cmake_minimum_required(VERSION 3.14)
project(AEMChecksumTest)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_aem_checksum test_aem_checksum.cpp)

# Link libraries if needed (Winsock for Windows)
if(WIN32)
    target_link_libraries(test_aem_checksum ws2_32)
endif()
