#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build-clangcl"

# Default local xwin sysroot (can still be overridden by caller env).
export XWIN_SYSROOT="${XWIN_SYSROOT:-/home/austin/xwin}"

pick_tool() {
  local out_var="$1"
  shift
  local c
  for c in "$@"; do
    if command -v "$c" >/dev/null 2>&1; then
      printf -v "$out_var" '%s' "$(command -v "$c")"
      return 0
    fi
  done
  return 1
}

CLANGCL=""
LLD_LINK=""
LLVM_RC=""
LLVM_MT=""
NINJA=""

if ! pick_tool CLANGCL clang-cl-19 clang-cl; then
  echo "Missing clang-cl (clang-cl-19 preferred)." >&2
  exit 1
fi

if ! pick_tool LLD_LINK lld-link-19 lld-link; then
  echo "Missing lld-link (lld-link-19 preferred)." >&2
  exit 1
fi

if ! pick_tool LLVM_RC llvm-rc-19 llvm-rc; then
  echo "Missing llvm-rc (llvm-rc-19 preferred)." >&2
  exit 1
fi

if ! pick_tool LLVM_MT llvm-mt-19 llvm-mt; then
  echo "Missing llvm-mt (llvm-mt-19 preferred)." >&2
  exit 1
fi

if ! pick_tool NINJA ninja; then
  echo "Missing ninja." >&2
  exit 1
fi

if [ -z "${XWIN_SYSROOT:-}" ]; then
  echo "XWIN_SYSROOT is not set." >&2
  echo "Example: export XWIN_SYSROOT=/home/austin/xwin" >&2
  exit 1
fi

if [ ! -d "${XWIN_SYSROOT}/crt/include" ] || [ ! -d "${XWIN_SYSROOT}/sdk/include/um" ]; then
  echo "XWIN_SYSROOT appears invalid: ${XWIN_SYSROOT}" >&2
  echo "Expected at least: ${XWIN_SYSROOT}/crt/include and ${XWIN_SYSROOT}/sdk/include/um" >&2
  exit 1
fi

COMMONLIBSF_INCLUDE_DIR="${COMMONLIBSF_INCLUDE_DIR:-}"
COMMONLIBSF_SHARED_INCLUDE_DIR="${COMMONLIBSF_SHARED_INCLUDE_DIR:-}"
COMMONLIBSF_LIBRARY="${COMMONLIBSF_LIBRARY:-}"
COMMONLIBSF_SHARED_LIBRARY="${COMMONLIBSF_SHARED_LIBRARY:-}"
COMMONLIBSF_ROOT="${COMMONLIBSF_ROOT:-${ROOT_DIR}/third_party/commonlibsf_build_src}"

# Ignore placeholder/bad env overrides so local autodiscovery still works.
if [ -n "${COMMONLIBSF_LIBRARY}" ] && [ ! -f "${COMMONLIBSF_LIBRARY}" ]; then
  echo "Ignoring invalid COMMONLIBSF_LIBRARY env value: ${COMMONLIBSF_LIBRARY}" >&2
  COMMONLIBSF_LIBRARY=""
fi
if [ -n "${COMMONLIBSF_SHARED_LIBRARY}" ] && [ ! -f "${COMMONLIBSF_SHARED_LIBRARY}" ]; then
  echo "Ignoring invalid COMMONLIBSF_SHARED_LIBRARY env value: ${COMMONLIBSF_SHARED_LIBRARY}" >&2
  COMMONLIBSF_SHARED_LIBRARY=""
fi

if [ -z "${COMMONLIBSF_INCLUDE_DIR}" ]; then
  COMMONLIBSF_INCLUDE_DIR="${COMMONLIBSF_ROOT}/include"
fi
if [ -z "${COMMONLIBSF_SHARED_INCLUDE_DIR}" ]; then
  COMMONLIBSF_SHARED_INCLUDE_DIR="${COMMONLIBSF_ROOT}/lib/commonlib-shared/include"
fi
if [ -z "${COMMONLIBSF_LIBRARY}" ]; then
  COMMONLIBSF_LIBRARY="${COMMONLIBSF_ROOT}/build/windows/x64/releasedbg/commonlibsf.lib"
fi
if [ -z "${COMMONLIBSF_SHARED_LIBRARY}" ]; then
  COMMONLIBSF_SHARED_LIBRARY="${COMMONLIBSF_ROOT}/build/windows/x64/releasedbg/commonlib-shared.lib"
fi

if [ -z "${COMMONLIBSF_INCLUDE_DIR}" ] || [ ! -f "${COMMONLIBSF_INCLUDE_DIR}/SFSE/SFSE.h" ]; then
  echo "COMMONLIBSF_INCLUDE_DIR is missing or invalid." >&2
  echo "Set COMMONLIBSF_INCLUDE_DIR to a directory containing SFSE/SFSE.h" >&2
  exit 1
fi

if [ -z "${COMMONLIBSF_SHARED_INCLUDE_DIR}" ] || [ ! -f "${COMMONLIBSF_SHARED_INCLUDE_DIR}/REL/REL.h" ]; then
  echo "COMMONLIBSF_SHARED_INCLUDE_DIR is missing or invalid." >&2
  echo "Set COMMONLIBSF_SHARED_INCLUDE_DIR to a directory containing REL/REL.h" >&2
  exit 1
fi

if [ -z "${COMMONLIBSF_LIBRARY}" ] || [ ! -f "${COMMONLIBSF_LIBRARY}" ]; then
  echo "COMMONLIBSF_LIBRARY is missing or invalid." >&2
  echo "Set COMMONLIBSF_LIBRARY to the full path to commonlibsf.lib" >&2
  exit 1
fi

if [ -z "${COMMONLIBSF_SHARED_LIBRARY}" ] || [ ! -f "${COMMONLIBSF_SHARED_LIBRARY}" ]; then
  echo "COMMONLIBSF_SHARED_LIBRARY is missing or invalid." >&2
  echo "Set COMMONLIBSF_SHARED_LIBRARY to the full path to commonlib-shared.lib" >&2
  exit 1
fi

echo "Using CommonLibSF include dir: ${COMMONLIBSF_INCLUDE_DIR}"
echo "Using commonlib-shared include dir: ${COMMONLIBSF_SHARED_INCLUDE_DIR}"
echo "Using commonlibsf lib: ${COMMONLIBSF_LIBRARY}"
echo "Using commonlib-shared lib: ${COMMONLIBSF_SHARED_LIBRARY}"

CMAKE_ARGS=(
  -S "${ROOT_DIR}"
  -B "${BUILD_DIR}"
  -G Ninja
  -DCMAKE_MAKE_PROGRAM="${NINJA}"
  -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/windows-clangcl.cmake"
  -DCMAKE_BUILD_TYPE=Release
  -DCLANGCL_EXECUTABLE="${CLANGCL}"
  -DLLD_LINK_EXECUTABLE="${LLD_LINK}"
  -DLLVM_RC_EXECUTABLE="${LLVM_RC}"
  -DLLVM_MT_EXECUTABLE="${LLVM_MT}"
  -DCOMMONLIBSF_INCLUDE_DIR="${COMMONLIBSF_INCLUDE_DIR}"
  -DCOMMONLIBSF_SHARED_INCLUDE_DIR="${COMMONLIBSF_SHARED_INCLUDE_DIR}"
  -DCOMMONLIBSF_LIBRARY="${COMMONLIBSF_LIBRARY}"
  -DCOMMONLIBSF_SHARED_LIBRARY="${COMMONLIBSF_SHARED_LIBRARY}"
)

mkdir -p "${BUILD_DIR}"
rm -f "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/build.ninja" "${BUILD_DIR}/cmake_install.cmake"
rm -rf "${BUILD_DIR}/CMakeFiles"

cmake "${CMAKE_ARGS[@]}"

cmake --build "${BUILD_DIR}" -j

echo
echo "Built DLL:"
ls -la "${BUILD_DIR}/AutoStartSFSE.dll"
