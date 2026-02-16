#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build-mingw"

pick_tool() {
  local out_var="$1"
  shift
  local c
  for c in "$@"; do
    if command -v "$c" >/dev/null 2>&1; then
      printf -v "$out_var" '%s' "$c"
      return 0
    fi
  done
  return 1
}

CC=""
CXX=""
WINDRES=""

if ! pick_tool CC x86_64-w64-mingw32-gcc x86_64-w64-mingw32-gcc-posix; then
  echo "Missing MinGW-w64 C compiler (x86_64-w64-mingw32-gcc)." >&2
  echo "On Pop!_OS/Ubuntu try:" >&2
  echo "  sudo apt install -y mingw-w64 gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64" >&2
  exit 1
fi

if ! pick_tool CXX x86_64-w64-mingw32-g++ x86_64-w64-mingw32-g++-posix; then
  echo "Missing MinGW-w64 C++ compiler (x86_64-w64-mingw32-g++)." >&2
  echo "On Pop!_OS/Ubuntu try:" >&2
  echo "  sudo apt install -y mingw-w64 g++-mingw-w64-x86-64" >&2
  exit 1
fi

if ! pick_tool WINDRES x86_64-w64-mingw32-windres; then
  echo "Missing MinGW-w64 windres (x86_64-w64-mingw32-windres)." >&2
  echo "On Pop!_OS/Ubuntu try:" >&2
  echo "  sudo apt install -y mingw-w64" >&2
  exit 1
fi

# Detect MinGW-w64 sysroot (where headers/libs are)
MINGW_SYSROOT=""
for candidate in \
  "/usr/x86_64-w64-mingw32" \
  "/usr/lib/gcc/x86_64-w64-mingw32/*/../../../../x86_64-w64-mingw32" \
  "$(dirname "$(dirname "${CXX}")")/x86_64-w64-mingw32" \
  "$(dirname "$(dirname "${CXX}")")"; do
  if [ -d "${candidate}/include" ] && [ -f "${candidate}/include/Windows.h" ] || [ -f "${candidate}/include/windows.h" ]; then
    MINGW_SYSROOT="${candidate}"
    break
  fi
done

if [ -z "${MINGW_SYSROOT}" ]; then
  echo "Warning: Could not auto-detect MinGW-w64 sysroot." >&2
  echo "Trying to find Windows.h..." >&2
  "${CXX}" -xc++ -E -v - </dev/null 2>&1 | grep "^ " | head -5 || true
fi

CMAKE_ARGS=(
  -S "${ROOT_DIR}"
  -B "${BUILD_DIR}"
  -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/mingw-w64-x86_64.cmake"
  -DCMAKE_C_COMPILER="${CC}"
  -DCMAKE_CXX_COMPILER="${CXX}"
  -DCMAKE_RC_COMPILER="${WINDRES}"
  -DCMAKE_BUILD_TYPE=Release
)

if [ -n "${MINGW_SYSROOT}" ]; then
  CMAKE_ARGS+=(-DCMAKE_FIND_ROOT_PATH="${MINGW_SYSROOT}")
fi

cmake "${CMAKE_ARGS[@]}"

cmake --build "${BUILD_DIR}" -j

echo
echo "Built DLL:"
ls -la "${BUILD_DIR}/AutoStartSFSE.dll"

