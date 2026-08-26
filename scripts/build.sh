#!/usr/bin/env bash
#=====================================================================
# build.sh - BSGL one-click build script (Linux / macOS)
#
# Usage:
#   scripts/build.sh                 # Release build
#   scripts/build.sh Debug           # Debug build
#   scripts/build.sh --clean         # Clean and rebuild (can combine with Debug)
#   scripts/build.sh clean           # Clean only, do not build
#=====================================================================
set -e

CONFIG="Release"
CLEAN=0
CLEAN_ONLY=0

for arg in "$@"; do
    case "$arg" in
        Debug|Release) CONFIG="$arg" ;;
        --clean|-c)    CLEAN=1 ;;
        clean)         CLEAN=1; CLEAN_ONLY=1 ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"

do_clean() {
    echo "==> Cleaning build artifacts ..."
    rm -rf "$BUILD"
}

if [ "$CLEAN" -eq 1 ]; then
    do_clean
    [ "$CLEAN_ONLY" -eq 1 ] && exit 0
fi

# Initialize git submodules (3rd/*): any entry still prefixed with '-'
# in `git submodule status` has not been checked out yet.
if [ -e "$ROOT/.git" ]; then
    if git -C "$ROOT" submodule status --recursive 2>/dev/null | grep -q '^-'; then
        echo "==> Initializing git submodules ..."
        git -C "$ROOT" submodule update --init --recursive
    fi
fi

# Deploy the DragonBonesCPP CMake script
# DragonBonesCPP ships no CMake build of its own; once the submodule is
# present, copy our script in as 3rd/DragonBonesCPP/CMakeLists.txt.
if [ -d "$ROOT/3rd/DragonBonesCPP/DragonBones/src" ]; then
    if ! cmp -s "$ROOT/scripts/DragonBonesCPP-CMakeLists.txt" "$ROOT/3rd/DragonBonesCPP/CMakeLists.txt" 2>/dev/null; then
        echo "==> Deploying 3rd/DragonBonesCPP/CMakeLists.txt ..."
        cp "$ROOT/scripts/DragonBonesCPP-CMakeLists.txt" "$ROOT/3rd/DragonBonesCPP/CMakeLists.txt"
    fi
fi

# Deploy the wren CMake script
# wren ships no CMake build of its own; once the submodule is present,
# copy our script in as 3rd/wren/CMakeLists.txt.
if [ -d "$ROOT/3rd/wren/src/vm" ]; then
    if ! cmp -s "$ROOT/scripts/wren-CMakeLists.txt" "$ROOT/3rd/wren/CMakeLists.txt" 2>/dev/null; then
        echo "==> Deploying 3rd/wren/CMakeLists.txt ..."
        cp "$ROOT/scripts/wren-CMakeLists.txt" "$ROOT/3rd/wren/CMakeLists.txt"
    fi
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake not found" >&2
    exit 1
fi

echo "==> Configuring ($CONFIG) ..."
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG"

echo "==> Building ..."
cmake --build "$BUILD" --parallel

echo ""
echo "==> Build complete"
echo "    Binaries:  $BUILD/bin/$CONFIG"
echo "    Libraries: $BUILD/lib/$CONFIG"
