# Install script for directory: D:/Repos/OpenAvnu/thirdparty/cpputest/src/CppUTest

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "./Install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CppUTest" TYPE FILE FILES
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/CommandLineArguments.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/PlatformSpecificFunctions.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestMemoryAllocator.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/CommandLineTestRunner.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/PlatformSpecificFunctions_c.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestOutput.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/CppUTestConfig.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/SimpleString.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestPlugin.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/JUnitTestOutput.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TeamCityTestOutput.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/StandardCLibrary.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestRegistry.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/MemoryLeakDetector.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestFailure.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestResult.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/MemoryLeakDetectorMallocMacros.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestFilter.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestTestingFixture.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/MemoryLeakDetectorNewMacros.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestHarness.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/Utest.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/MemoryLeakWarningPlugin.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/TestHarness_c.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTest/UtestMacros.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-test/thirdparty/cpputest/src/CppUTest/Debug/CppUTest.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-test/thirdparty/cpputest/src/CppUTest/Release/CppUTest.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-test/thirdparty/cpputest/src/CppUTest/MinSizeRel/CppUTest.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-test/thirdparty/cpputest/src/CppUTest/RelWithDebInfo/CppUTest.lib")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build-test/thirdparty/cpputest/src/CppUTest/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
