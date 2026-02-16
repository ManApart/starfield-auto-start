set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Set the MinGW-w64 root path (where headers and libs are installed)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Try to detect sysroot from compiler if CMAKE_FIND_ROOT_PATH wasn't set externally
if(NOT DEFINED CMAKE_FIND_ROOT_PATH OR CMAKE_FIND_ROOT_PATH STREQUAL "")
  # Common installation paths
  set(POSSIBLE_ROOTS
    /usr/${TOOLCHAIN_PREFIX}
    /usr/${TOOLCHAIN_PREFIX}/sys-root
    /usr/lib/${TOOLCHAIN_PREFIX}
  )
  
  # Check which one has the headers
  foreach(ROOT ${POSSIBLE_ROOTS})
    if(EXISTS "${ROOT}/include/Windows.h" OR EXISTS "${ROOT}/include/windows.h")
      set(CMAKE_FIND_ROOT_PATH "${ROOT}")
      break()
    endif()
  endforeach()
  
  # Fallback: use the standard location
  if(NOT DEFINED CMAKE_FIND_ROOT_PATH OR CMAKE_FIND_ROOT_PATH STREQUAL "")
    set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})
  endif()
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Do NOT set CMAKE_SYSROOT - it makes the compiler look for includes under
# $sysroot/usr/... and breaks -isystem paths. Rely on explicit include paths instead.

