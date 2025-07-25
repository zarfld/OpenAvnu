# Install script for directory: D:/Repos/OpenAvnu

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/cpputest/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/thirdparty/gptp/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/examples/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/daemons/common/tests/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/daemons/mrpd/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/daemons/maap/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/daemons/shaper/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/lib/avtp_pipeline/tests/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/lib/avtp_pipeline/avtp_open1722/cmake_install.cmake")
  include("D:/Repos/OpenAvnu/build_verify/testing/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build_verify/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build_verify/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
