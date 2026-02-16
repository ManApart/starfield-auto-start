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

# Configure MinGW-w64 cross-compilation to avoid host header contamination
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(COMPILER_PREFIX_DIR "${COMPILER_DIR}" DIRECTORY)
  file(GLOB GCC_VERSIONS "${COMPILER_PREFIX_DIR}/lib/gcc/x86_64-w64-mingw32/*")
  
  set(GCC_VER_DIR "")
  foreach(VER_DIR ${GCC_VERSIONS})
    if(IS_DIRECTORY "${VER_DIR}" AND EXISTS "${VER_DIR}/include")
      set(GCC_VER_DIR "${VER_DIR}")
      break()
    endif()
  endforeach()
  
  if(GCC_VER_DIR)
    # CMake de-duplicates compile options and breaks "-isystem path" pairs.
    # Use SHELL: prefix so each "-isystem /path" is kept as one unit (CMake 3.12+).
    set(MINGW_ISYSTEM_LIST
      -nostdinc
      -nostdinc++
    )
    
    if(EXISTS "${GCC_VER_DIR}/include/c++")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${GCC_VER_DIR}/include/c++")
      # Architecture-specific C++ headers (bits/c++config.h etc.); may be .../c++/10/... or .../c++/x86_64-w64-mingw32
      if(EXISTS "${GCC_VER_DIR}/include/c++/x86_64-w64-mingw32")
        list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${GCC_VER_DIR}/include/c++/x86_64-w64-mingw32")
      endif()
      file(GLOB ARCH_DIRS "${GCC_VER_DIR}/include/c++/*/x86_64-w64-mingw32")
      foreach(ARCH_DIR ${ARCH_DIRS})
        list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${ARCH_DIR}")
      endforeach()
    endif()
    if(EXISTS "${GCC_VER_DIR}/include/c++/backward")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${GCC_VER_DIR}/include/c++/backward")
    endif()
    if(EXISTS "${GCC_VER_DIR}/include")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${GCC_VER_DIR}/include")
    endif()
    if(EXISTS "${GCC_VER_DIR}/include-fixed")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${GCC_VER_DIR}/include-fixed")
    endif()
    
    get_filename_component(RESOLVED_MINGW_INCLUDE "${GCC_VER_DIR}/../../../../x86_64-w64-mingw32/include" ABSOLUTE)
    if(EXISTS "${RESOLVED_MINGW_INCLUDE}")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem ${RESOLVED_MINGW_INCLUDE}")
    endif()
    if(EXISTS "/usr/x86_64-w64-mingw32/include")
      list(APPEND MINGW_ISYSTEM_LIST "SHELL:-isystem /usr/x86_64-w64-mingw32/include")
    endif()
    
    target_compile_options(AutoStartSFSE PRIVATE ${MINGW_ISYSTEM_LIST})
    message(STATUS "Configured MinGW-w64 cross-compilation (GCC: ${GCC_VER_DIR})")
  else()
    message(WARNING "Could not find MinGW-w64 GCC include directories")
  endif()
endif()

# Prefer static libstdc++/libgcc to avoid shipping extra MinGW DLLs.
if(MINGW)
  target_link_options(AutoStartSFSE PRIVATE -static-libgcc -static-libstdc++)
endif()

# Exported symbol names must match exactly what SFSE looks for.
target_compile_definitions(AutoStartSFSE PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(AutoStartSFSE PROPERTIES
  PREFIX ""
  OUTPUT_NAME "AutoStartSFSE"
)
