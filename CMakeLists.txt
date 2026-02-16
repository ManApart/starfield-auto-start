cmake_minimum_required(VERSION 3.20)

project(AutoStartSFSE
  VERSION 0.1.0
  LANGUAGES C CXX
)

set(COMMONLIBSF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/commonlibsf/include" CACHE PATH
    "Path to CommonLibSF include directory containing SFSE/Interfaces.h")
set(COMMONLIBSF_SHARED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/commonlibsf/lib/commonlib-shared/include" CACHE PATH
    "Path to commonlib-shared include directory containing REL/REL.h")
set(COMMONLIBSF_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/commonlibsf/build/windows/x64/releasedbg/commonlibsf.lib" CACHE FILEPATH
    "Path to commonlibsf static library (commonlibsf.lib)")
set(COMMONLIBSF_SHARED_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/commonlibsf/build/windows/x64/releasedbg/commonlib-shared.lib" CACHE FILEPATH
    "Path to commonlib-shared static library (commonlib-shared.lib)")

if(NOT EXISTS "${COMMONLIBSF_INCLUDE_DIR}/SFSE/Interfaces.h")
  message(FATAL_ERROR
    "Missing CommonLibSF headers. Set COMMONLIBSF_INCLUDE_DIR to a directory containing SFSE/Interfaces.h")
endif()

if(NOT EXISTS "${COMMONLIBSF_SHARED_INCLUDE_DIR}/REL/REL.h")
  message(FATAL_ERROR
    "Missing commonlib-shared headers. Set COMMONLIBSF_SHARED_INCLUDE_DIR to a directory containing REL/REL.h")
endif()

if(NOT EXISTS "${COMMONLIBSF_LIBRARY}")
  message(FATAL_ERROR
    "Missing commonlibsf library. Set COMMONLIBSF_LIBRARY to commonlibsf.lib")
endif()

if(NOT EXISTS "${COMMONLIBSF_SHARED_LIBRARY}")
  message(FATAL_ERROR
    "Missing commonlib-shared library. Set COMMONLIBSF_SHARED_LIBRARY to commonlib-shared.lib")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  message(FATAL_ERROR
    "CommonLibSF headers require a C++ standard library with <format>. "
    "Current GNU compiler version is ${CMAKE_CXX_COMPILER_VERSION}; use GCC 13+ or clang-cl/MSVC.")
endif()

add_library(AutoStartSFSE SHARED
  src/main.cpp
)

target_compile_features(AutoStartSFSE PRIVATE cxx_std_23)
target_include_directories(AutoStartSFSE PRIVATE
  ${COMMONLIBSF_INCLUDE_DIR}
  ${COMMONLIBSF_SHARED_INCLUDE_DIR}
)

# CommonLibSF headers are warning-heavy on clang-cl; silence header-only noise.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(AutoStartSFSE PRIVATE
    -Wno-deprecated-declarations
    -Wno-deprecated-volatile
    -Wno-invalid-offsetof
    -Wno-switch
  )
endif()

target_link_libraries(AutoStartSFSE PRIVATE
  ${COMMONLIBSF_LIBRARY}
  ${COMMONLIBSF_SHARED_LIBRARY}
  # CommonLibSF/commonlib-shared expect these Win32 import libs.
  advapi32
  bcrypt
  d3d11
  d3dcompiler
  dbghelp
  dxgi
  ole32
  shell32
  user32
  version
  ws2_32
)

# Exported symbol names must match exactly what SFSE looks for.
target_compile_definitions(AutoStartSFSE PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(AutoStartSFSE PROPERTIES
  PREFIX ""
  OUTPUT_NAME "AutoStartSFSE"
)
