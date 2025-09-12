# Install script for directory: D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/Install")
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

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/Debug/fmtd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/Release/fmt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/MinSizeRel/fmt.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/RelWithDebInfo/fmt.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fmt" TYPE FILE FILES
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/args.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/chrono.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/color.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/compile.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/core.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/format.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/format-inl.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/os.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/ostream.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/printf.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/ranges.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/std.h"
    "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/fmtlib/include/fmt/xchar.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES
    "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/fmt-config.cmake"
    "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/fmt-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake"
         "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/fmt.pc")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build-intel-hal/lib/la_avdecc/externals/3rdparty/fmtlib/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
