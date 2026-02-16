cmake_minimum_required(VERSION 3.20)

project(HelloWorldSFSE
  VERSION 0.1.0
  LANGUAGES C CXX
)

add_library(HelloWorldSFSE SHARED
  src/main.cpp
)

target_compile_features(HelloWorldSFSE PRIVATE cxx_std_17)

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
    
    target_compile_options(HelloWorldSFSE PRIVATE ${MINGW_ISYSTEM_LIST})
    message(STATUS "Configured MinGW-w64 cross-compilation (GCC: ${GCC_VER_DIR})")
  else()
    message(WARNING "Could not find MinGW-w64 GCC include directories")
  endif()
endif()

# Prefer static libstdc++/libgcc to avoid shipping extra MinGW DLLs.
if(MINGW)
  target_link_options(HelloWorldSFSE PRIVATE -static-libgcc -static-libstdc++)
endif()

# Exported symbol names must match exactly what SFSE looks for.
target_compile_definitions(HelloWorldSFSE PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

set_target_properties(HelloWorldSFSE PROPERTIES
  PREFIX ""
  OUTPUT_NAME "HelloWorldSFSE"
)

