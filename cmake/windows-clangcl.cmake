set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

find_program(CLANGCL_EXECUTABLE NAMES clang-cl-19 clang-cl REQUIRED)
find_program(LLD_LINK_EXECUTABLE NAMES lld-link-19 lld-link REQUIRED)
find_program(LLVM_RC_EXECUTABLE NAMES llvm-rc-19 llvm-rc REQUIRED)
find_program(LLVM_MT_EXECUTABLE NAMES llvm-mt-19 llvm-mt REQUIRED)

set(CMAKE_C_COMPILER "${CLANGCL_EXECUTABLE}" CACHE STRING "")
set(CMAKE_CXX_COMPILER "${CLANGCL_EXECUTABLE}" CACHE STRING "")
set(CMAKE_LINKER "${LLD_LINK_EXECUTABLE}" CACHE FILEPATH "")
set(CMAKE_RC_COMPILER "${LLVM_RC_EXECUTABLE}" CACHE FILEPATH "")
set(CMAKE_MT "${LLVM_MT_EXECUTABLE}" CACHE FILEPATH "")

set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)

if(NOT DEFINED ENV{XWIN_SYSROOT})
  message(FATAL_ERROR
    "Set XWIN_SYSROOT to your xwin root (containing crt/ and sdk/). "
    "Example: export XWIN_SYSROOT=/home/austin/xwin")
endif()

set(XWIN_SYSROOT "$ENV{XWIN_SYSROOT}")
set(XWIN_CRT_DIR "${XWIN_SYSROOT}/crt")
set(XWIN_SDK_DIR "${XWIN_SYSROOT}/sdk")

set(XWIN_LIB_ARCH "x64")
if(EXISTS "${XWIN_CRT_DIR}/lib/x86_64" OR EXISTS "${XWIN_SDK_DIR}/lib/um/x86_64")
  set(XWIN_LIB_ARCH "x86_64")
endif()

if(NOT EXISTS "${XWIN_CRT_DIR}/include")
  message(FATAL_ERROR "Missing ${XWIN_CRT_DIR}/include")
endif()
if(NOT EXISTS "${XWIN_SDK_DIR}/include/um")
  message(FATAL_ERROR "Missing ${XWIN_SDK_DIR}/include/um")
endif()
if(NOT EXISTS "${XWIN_SDK_DIR}/lib/um/${XWIN_LIB_ARCH}")
  message(FATAL_ERROR "Missing ${XWIN_SDK_DIR}/lib/um/${XWIN_LIB_ARCH}")
endif()

string(JOIN " " _XWIN_IMSVC_FLAGS
  "/imsvc${XWIN_CRT_DIR}/include"
  "/imsvc${XWIN_SDK_DIR}/include/ucrt"
  "/imsvc${XWIN_SDK_DIR}/include/um"
  "/imsvc${XWIN_SDK_DIR}/include/shared"
  "/imsvc${XWIN_SDK_DIR}/include/winrt"
)

string(JOIN " " _XWIN_LIBPATH_FLAGS
  "/libpath:${XWIN_CRT_DIR}/lib/${XWIN_LIB_ARCH}"
  "/libpath:${XWIN_SDK_DIR}/lib/ucrt/${XWIN_LIB_ARCH}"
  "/libpath:${XWIN_SDK_DIR}/lib/um/${XWIN_LIB_ARCH}"
)

set(CMAKE_C_FLAGS_INIT "${_XWIN_IMSVC_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${_XWIN_IMSVC_FLAGS} /EHsc /permissive-")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_XWIN_LIBPATH_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_XWIN_LIBPATH_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_XWIN_LIBPATH_FLAGS}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
