# Install script for directory: D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/json

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/Install")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/json/include/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json" TYPE FILE FILES
    "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/nlohmann_jsonConfig.cmake"
    "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/nlohmann_jsonConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "D:/Repos/OpenAvnu/lib/la_avdecc/externals/3rdparty/json/nlohmann_json.natvis")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json/nlohmann_jsonTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json/nlohmann_jsonTargets.cmake"
         "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/CMakeFiles/Export/c5a95352faa8b09b394d8af6a01f43bc/nlohmann_jsonTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json/nlohmann_jsonTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json/nlohmann_jsonTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/nlohmann_json" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/CMakeFiles/Export/c5a95352faa8b09b394d8af6a01f43bc/nlohmann_jsonTargets.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "LA_avdecc" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pkgconfig" TYPE FILE FILES "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/nlohmann_json.pc")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Repos/OpenAvnu/build_verify/lib/la_avdecc/externals/3rdparty/json/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
