cmake_minimum_required(VERSION 3.10)
project(phase3_production_test)

add_executable(phase3_production_test phase3_production_test.c)

if(WIN32)
    target_compile_definitions(phase3_production_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Add winmm library for timeGetTime if needed
    target_link_libraries(phase3_production_test winmm)
endif()
