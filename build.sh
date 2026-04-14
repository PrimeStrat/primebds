#!/usr/bin/env bash
set -euo pipefail

# PrimeBDS Build Script for Linux
# Requires: CMake 3.15+, Clang with libc++ (C++20 support)
# Use --docker for glibc-compatible builds (targets Ubuntu 22.04)

BUILD_DIR="build/linux"
BUILD_TYPE="Release"
USE_DOCKER=0

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug)   BUILD_TYPE="Debug";   shift ;;
        --release) BUILD_TYPE="Release"; shift ;;
        --docker)  USE_DOCKER=1;         shift ;;
        --clean)
            echo "Cleaning build directory..."
            rm -rf "$BUILD_DIR"
            shift
            ;;
        --setup)
            echo "Installing Clang 18 + libc++..."
            sudo apt-get update -y -q
            sudo apt-get install -y -q lsb-release wget software-properties-common gnupg cmake ninja-build
            wget https://apt.llvm.org/llvm.sh
            chmod +x llvm.sh
            sudo ./llvm.sh 18
            sudo apt-get install -y -q libc++-18-dev libc++abi-18-dev
            rm -f llvm.sh
            echo "Setup complete."
            shift
            ;;
        --help)
            echo "Usage: ./build.sh [options]"
            echo "  --debug     Build in Debug mode"
            echo "  --release   Build in Release mode (default)"
            echo "  --clean     Remove build directory before building"
            echo "  --docker    Build inside Ubuntu 22.04 container (glibc compatible)"
            echo "  --setup     Install Clang 18 + libc++ (run once, native only)"
            echo "  --help      Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Docker build path
if [[ "$USE_DOCKER" -eq 1 ]]; then
    if ! command -v docker &>/dev/null; then
        echo "[ERROR] Docker not found. Install Docker to use --docker."
        exit 1
    fi

    echo "============================================"
    echo " PrimeBDS Docker Build - Linux ($BUILD_TYPE)"
    echo " Target: Ubuntu 22.04 (glibc 2.35)"
    echo "============================================"

    docker build -t primebds-builder -f - . <<'DOCKERFILE'
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y -q && \
    apt-get install -y -q lsb-release wget software-properties-common gnupg cmake ninja-build && \
    wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh && ./llvm.sh 18 && \
    apt-get install -y -q libc++-18-dev libc++abi-18-dev && \
    rm -f llvm.sh && apt-get clean
DOCKERFILE

    docker run --rm \
        -v "$(pwd):/src" \
        -w /src \
        primebds-builder \
        bash -c "
            rm -rf $BUILD_DIR
            CC=clang-18 CXX=clang++-18 cmake -S . -B $BUILD_DIR \
                -G Ninja \
                -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && \
            cmake --build $BUILD_DIR --parallel \$(nproc)
        "

    # Verify GLIBC requirements of built binary
    MAX_GLIBC=$(objdump -T "$BUILD_DIR/output/"*.so 2>/dev/null \
        | grep -oP 'GLIBC_\K[0-9.]+' | sort -V -u | tail -1)
    echo ""
    echo "Max GLIBC requirement: GLIBC_${MAX_GLIBC:-unknown}"
    if [[ -n "$MAX_GLIBC" ]] && [[ "$(printf '%s\n' "2.35" "$MAX_GLIBC" | sort -V | tail -1)" != "2.35" ]]; then
        echo "[WARNING] Binary requires GLIBC_${MAX_GLIBC} (> 2.35). May fail on older hosts."
    fi

    echo ""
    echo "============================================"
    echo " Docker build succeeded!"
    echo " Output: $BUILD_DIR/output/"
    echo "============================================"
    exit 0
fi

# Check for CMake
if ! command -v cmake &>/dev/null; then
    echo "[ERROR] CMake not found. Run: ./build.sh --setup"
    exit 1
fi

# Find Clang
CLANG_CC=""
CLANG_CXX=""
for ver in 18 17 16 15; do
    if command -v "clang-$ver" &>/dev/null; then
        CLANG_CC="clang-$ver"
        CLANG_CXX="clang++-$ver"
        break
    fi
done
if [[ -z "$CLANG_CC" ]]; then
    if command -v clang &>/dev/null; then
        CLANG_CC="clang"
        CLANG_CXX="clang++"
    else
        echo "[ERROR] Clang not found. Run: ./build.sh --setup"
        exit 1
    fi
fi
echo "Compiler: $CLANG_CXX"

# Pick generator
GENERATOR="Unix Makefiles"
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
fi

echo "============================================"
echo " PrimeBDS Build - Linux ($BUILD_TYPE)"
echo " Generator: $GENERATOR"
echo "============================================"

# Configure
echo "[1/2] Configuring..."
CC="$CLANG_CC" CXX="$CLANG_CXX" cmake -S . -B "$BUILD_DIR" \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
NPROC=$(nproc 2>/dev/null || echo 4)
echo "[2/2] Building with $NPROC jobs..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --parallel "$NPROC"

# Verify GLIBC requirements of built binary
MAX_GLIBC=$(objdump -T "$BUILD_DIR/output/"*.so 2>/dev/null \
    | grep -oP 'GLIBC_\K[0-9.]+' | sort -V -u | tail -1)
echo ""
echo "Max GLIBC requirement: GLIBC_${MAX_GLIBC:-unknown}"
if [[ -n "$MAX_GLIBC" ]] && [[ "$(printf '%s\n' "2.35" "$MAX_GLIBC" | sort -V | tail -1)" != "2.35" ]]; then
    echo "[WARNING] Binary requires GLIBC_${MAX_GLIBC} (> 2.35). May fail on older hosting environments."
    echo "          Use './build.sh --docker' to build with glibc 2.35 (Ubuntu 22.04)."
fi

echo ""
echo "============================================"
echo " Build succeeded!"
echo " Output: $BUILD_DIR/output/"
echo "============================================"
