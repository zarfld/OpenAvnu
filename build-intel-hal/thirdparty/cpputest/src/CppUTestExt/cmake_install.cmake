# Install script for directory: D:/Repos/OpenAvnu/thirdparty/cpputest/src/CppUTestExt

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CppUTestExt" TYPE FILE FILES
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/CodeMemoryReportFormatter.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/IEEE754ExceptionsPlugin.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MemoryReportAllocator.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockExpectedCall.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockCheckedExpectedCall.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockExpectedCallsList.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockSupportPlugin.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MemoryReportFormatter.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockFailure.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockSupport.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockSupport_c.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/GMock.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/GTest.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MemoryReporterPlugin.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/OrderedTest.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/GTestConvertor.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockActualCall.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockCheckedActualCall.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockNamedValue.h"
    "D:/Repos/OpenAvnu/thirdparty/cpputest/include/CppUTestExt/MockSupport.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/thirdparty/cpputest/src/CppUTestExt/Debug/CppUTestExt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/thirdparty/cpputest/src/CppUTestExt/Release/CppUTestExt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/thirdparty/cpputest/src/CppUTestExt/MinSizeRel/CppUTestExt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/thirdparty/cpputest/src/CppUTestExt/RelWithDebInfo/CppUTestExt.lib")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build-intel-hal/thirdparty/cpputest/src/CppUTestExt/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
