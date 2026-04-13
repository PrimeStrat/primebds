#!/usr/bin/env bash
set -euo pipefail

# PrimeBDS Build Script for Linux
# Requires: CMake 3.15+, g++ or clang++ with C++20 support

BUILD_DIR="build/linux"
BUILD_TYPE="Release"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)   BUILD_TYPE="Debug";   shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --clean)
            echo "Cleaning build directory..."
            rm -rf "$BUILD_DIR"
            shift
            ;;
        --help)
            echo "Usage: ./build.sh [options]"
            echo "  --debug     Build in Debug mode"
            echo "  --release   Build in Release mode (default)"
            echo "  --clean     Remove build directory before building"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check for CMake
if ! command -v cmake &>/dev/null; then
    echo "[ERROR] CMake not found. Install CMake 3.15+."
    exit 1
fi

echo "============================================"
echo " PrimeBDS Build - Linux ($BUILD_TYPE)"
echo "============================================"

# Configure
echo "[1/2] Configuring..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
NPROC=$(nproc 2>/dev/null || echo 4)
echo "[2/2] Building with $NPROC jobs..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$NPROC"

echo ""
echo "============================================"
echo " Build succeeded!"
echo " Output: $BUILD_DIR/"
echo "============================================"
