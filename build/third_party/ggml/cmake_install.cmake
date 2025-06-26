# Install script for directory: /Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
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

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src/libggml.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src"
      -delete_rpath "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src/ggml-blas"
      -delete_rpath "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src/ggml-metal"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-cpu.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-alloc.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-backend.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-blas.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-cann.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-cpp.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-cuda.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-kompute.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-opt.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-metal.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-rpc.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-sycl.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/ggml-vulkan.h"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/third_party/ggml/include/gguf.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/src/libggml-base.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml-base.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml-base.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libggml-base.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ggml" TYPE FILE FILES
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/ggml-config.cmake"
    "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/ggml-version.cmake"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/willb/Vaults/Personal/👨‍💻 DevLab/ggml/ggml-viz/build/third_party/ggml/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
