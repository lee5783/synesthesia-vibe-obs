#
#  CMakeLists.txt
#  SynesthesiaVibeOBS
#
#  Created by lee5783 on 2026-11-23.
#

cmake_minimum_required(VERSION 3.28...3.30)

cmake_host_system_information(RESULT SYNESTHESIA_VIBE_HOST_PROCESSOR QUERY OS_PLATFORM)

if(
  NOT (
    CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin"
    AND CMAKE_GENERATOR STREQUAL "Xcode"
    AND "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64"
  )
  AND NOT (
    CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux"
    AND CMAKE_GENERATOR STREQUAL "Ninja"
    AND SYNESTHESIA_VIBE_HOST_PROCESSOR STREQUAL "x86_64"
  )
)
  message(
    FATAL_ERROR
    "SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for arm64 alone, "
    "or on Linux, with the Ninja generator, for x86_64. "
    "Detected host '${CMAKE_HOST_SYSTEM_NAME}' (${SYNESTHESIA_VIBE_HOST_PROCESSOR}), generator '${CMAKE_GENERATOR}', "
    "CMAKE_OSX_ARCHITECTURES '${CMAKE_OSX_ARCHITECTURES}'. "
    "On macOS configure with 'cmake --preset macos-arm64' and build with 'cmake --build --preset macos-arm64'. "
    "On Linux configure with 'cmake --preset ubuntu-x86_64' and build with 'cmake --build --preset ubuntu-x86_64'."
  )
endif()

if(
  NOT DEFINED PLUGIN_BUILD_NUMBER
  AND NOT "$ENV{CI}"
  AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber"
)
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber" "0")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

if(OS_LINUX)
  set(CPACK_PACKAGE_CONTACT "lee5783 <lee5783@users.noreply.github.com>")
endif()

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

if(OS_MACOS)
  set(
    SYNESTHESIA_VIBE_CORE_PLATFORM_ARGS
    -DVCPKG_TARGET_TRIPLET=arm64-osx
    "-DVCPKG_OVERLAY_TRIPLETS=${SYNESTHESIA_VIBE_CORE_SOURCE_DIR}/triplets"
    -DCMAKE_OSX_ARCHITECTURES=arm64
    "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
  )
  set(SYNESTHESIA_VIBE_CORE_DEPENDENCY_LIBRARIES "")
else()
  set(
    SYNESTHESIA_VIBE_CORE_PLATFORM_ARGS
    -DVCPKG_TARGET_TRIPLET=x64-linux
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DCMAKE_INSTALL_LIBDIR=lib
  )
  set(
    SYNESTHESIA_VIBE_CORE_DEPENDENCY_LIBRARIES
    "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/lib/libprojectM_ns-eel2.a"
    "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/lib/libprojectM_eval.a"
  )
endif()

ExternalProject_Add(
  synesthesia-vibe-core
  SOURCE_DIR "${SYNESTHESIA_VIBE_CORE_SOURCE_DIR}"
  BINARY_DIR "${SYNESTHESIA_VIBE_CORE_BINARY_DIR}"
  INSTALL_DIR "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}"
  CMAKE_GENERATOR Ninja
  CMAKE_ARGS
    "-DCMAKE_MAKE_PROGRAM=${SYNESTHESIA_VIBE_NINJA}"
    "-DCMAKE_TOOLCHAIN_FILE=$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    ${SYNESTHESIA_VIBE_CORE_PLATFORM_ARGS}
    -DCMAKE_BUILD_TYPE=Release
    -DSYNESTHESIA_VIBE_GL_DEV=ON
    -DSYNESTHESIA_VIBE_BACKEND=gl
    -DSYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF
    "-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
  BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target SynesthesiaVibeCore
  INSTALL_COMMAND "${CMAKE_COMMAND}" --install <BINARY_DIR>
  BUILD_BYPRODUCTS "${SYNESTHESIA_VIBE_CORE_LIBRARY}" ${SYNESTHESIA_VIBE_CORE_DEPENDENCY_LIBRARIES}
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

if(OS_MACOS)
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
else()
  find_package(Threads REQUIRED)
  add_library(SynesthesiaVibe::NsEel2 STATIC IMPORTED)
  set_target_properties(
    SynesthesiaVibe::NsEel2
    PROPERTIES IMPORTED_LOCATION "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/lib/libprojectM_ns-eel2.a"
  )
  add_library(SynesthesiaVibe::ProjectMEval STATIC IMPORTED)
  set_target_properties(
    SynesthesiaVibe::ProjectMEval
    PROPERTIES IMPORTED_LOCATION "${SYNESTHESIA_VIBE_CORE_INSTALL_DIR}/lib/libprojectM_eval.a"
  )
  target_link_libraries(SynesthesiaVibe::Core INTERFACE Threads::Threads ${CMAKE_DL_LIBS})
endif()

add_dependencies(${CMAKE_PROJECT_NAME} synesthesia-vibe-core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE SynesthesiaVibe::Core)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/audio-ring.cpp
    src/audio-ring.hpp
    src/opengl-loader.hpp
    src/plugin-main.cpp
    src/visualizer-source.cpp
    src/visualizer-source.hpp
)

if(OS_MACOS)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/opengl-loader-macos.cpp)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/exported-symbols.txt"
  )
else()
  find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/opengl-loader-linux.cpp)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GL_GLEXT_PROTOTYPES EGL_NO_X11)
  target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      "$<LINK_GROUP:RESCAN,SynesthesiaVibe::Core,SynesthesiaVibe::NsEel2,SynesthesiaVibe::ProjectMEval>"
      OpenGL::OpenGL
      OpenGL::EGL
  )
  target_link_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exported-symbols.map"
      "-Wl,--exclude-libs,ALL"
      "-Wl,--no-undefined"
  )
  set_property(
    TARGET ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/exported-symbols.map"
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

if(OS_MACOS)
  configure_file(packaging/macos/distribution.in "${CMAKE_CURRENT_BINARY_DIR}/distribution" @ONLY)
endif()
