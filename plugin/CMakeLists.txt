#
#  CMakeLists.txt
#  SynesthesiaVibeOBS
#
#  Created by lee5783 on 2026-11-23.
#

cmake_minimum_required(VERSION 3.28...3.30)

if(
  NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
  OR NOT CMAKE_GENERATOR STREQUAL "Xcode"
  OR NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64"
)
  message(
    FATAL_ERROR
    "SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for arm64 alone. "
    "Detected host '${CMAKE_HOST_SYSTEM_NAME}', generator '${CMAKE_GENERATOR}', "
    "CMAKE_OSX_ARCHITECTURES '${CMAKE_OSX_ARCHITECTURES}'. "
    "Configure with 'cmake --preset macos-arm64' and build with 'cmake --build --preset macos-arm64'."
  )
endif()

if(NOT DEFINED PLUGIN_BUILD_NUMBER AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber")
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber" "0")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)
include(ExternalProject)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

if(NOT DEFINED ENV{VCPKG_ROOT})
  message(FATAL_ERROR "VCPKG_ROOT must point at a vcpkg checkout to build the SynesthesiaVibe core.")
endif()

find_program(SYNESTHESIA_VIBE_NINJA ninja REQUIRED)

set(SYNESTHESIA_VIBE_CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/synesthesia-vibe")
set(SYNESTHESIA_VIBE_CORE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/synesthesia-vibe-core/build")
set(SYNESTHESIA_VIBE_CORE_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/synesthesia-vibe-core/install")
set(SYNESTHESIA_VIBE_CORE_LIBRARY "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/lib/libSynesthesiaVibeCore.a")

ExternalProject_Add(
  synesthesia-vibe-core
  SOURCE_DIR "${SYNESTHESIA_VIBE_CORE_SOURCE_DIR}"
  BINARY_DIR "${SYNESTHESIA_VIBE_CORE_BINARY_DIR}"
  INSTALL_DIR "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}"
  CMAKE_GENERATOR Ninja
  CMAKE_ARGS
    "-DCMAKE_MAKE_PROGRAM=${SYNESTHESIA_VIBE_NINJA}"
    "-DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    -DVCPKG_TARGET_TRIPLET=arm64-osx
    "-DVCPKG_OVERLAY_TRIPLETS=${SYNESTHESIA_VIBE_CORE_SOURCE_DIR}/triplets"
    -DCMAKE_OSX_ARCHITECTURES=arm64
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
    -DCMAKE_BUILD_TYPE=Release
    -DSYNESTHESIA_VIBE_GL_DEV=ON
    -DSYNESTHESIA_VIBE_BACKEND=gl
    -DSYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
  BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target SynesthesiaVibeCore
  INSTALL_COMMAND "${CMAKE_COMMAND}" --install <BINARY_DIR>
  BUILD_BYPRODUCTS "${SYNESTHESIA_VIBE_CORE_LIBRARY}"
  BUILD_ALWAYS TRUE
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
)

file(MAKE_DIRECTORY "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/include")

add_library(SynesthesiaVibe::Core STATIC IMPORTED)
set_target_properties(
  SynesthesiaVibe::Core
  PROPERTIES
    IMPORTED_LOCATION "${SYNESTHESIA_VIBE_CORE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/include"
)
target_link_libraries(
  SynesthesiaVibe::Core
  INTERFACE
    "-framework Foundation"
    "-framework AVFoundation"
    "-framework CoreAudio"
    "-framework OpenGL"
    "-framework AppKit"
    "-framework CoreVideo"
    "-framework IOSurface"
    "-framework QuartzCore"
)

add_dependencies(${CMAKE_PROJECT_NAME} synesthesia-vibe-core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SynesthesiaVibe::Core)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
set_target_properties(
  ${CMAKE_PROJECT_NAME}
  PROPERTIES XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/exported-symbols.txt"
)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/audio-ring.cpp src/audio-ring.hpp src/plugin-main.cpp src/visualizer-source.cpp src/visualizer-source.hpp
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

configure_file(packaging/macos/distribution.in "${CMAKE_CURRENT_BINARY_DIR}/distribution" @ONLY)
