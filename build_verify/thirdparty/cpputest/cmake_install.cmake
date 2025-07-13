# Install script for directory: D:/Repos/OpenAvnu/thirdparty/cpputest

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/cpputest.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/CppUTest" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/generated/CppUTestGeneratedConfig.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES
    "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/install/CppUTestConfig.cmake"
    "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/install/CppUTestConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake/CppUTestTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake/CppUTestTargets.cmake"
         "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake/CppUTestTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake/CppUTestTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/CppUTest/cmake" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/CMakeFiles/Export/4306f34d0d0a58e19d5744ed1e2a0d78/CppUTestTargets-release.cmake")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/src/CppUTest/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/src/CppUTestExt/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/tests/CppUTest/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/tests/CppUTestExt/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
