cmake_minimum_required(VERSION 3.15)
project(phase3_production_test_consolidated)

add_executable(phase3_production_test_consolidated phase3_production_test.c)

if(WIN32)
    target_compile_definitions(phase3_production_test_consolidated PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Add winmm library for timeGetTime if needed
    target_link_libraries(phase3_production_test_consolidated winmm)
endif()
