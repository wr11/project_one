#!/bin/bash
# GearEngine MSYS2 构建脚本

set -e

echo "=========================================="
echo "GearEngine MSYS2 Build Script"
echo "=========================================="
echo ""

# 检查编译器
if ! command -v gcc &> /dev/null; then
    echo "ERROR: gcc not found!"
    echo "Please install: pacman -S mingw-w64-x86_64-gcc"
    exit 1
fi

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found!"
    echo "Please install: pacman -S mingw-w64-x86_64-cmake"
    exit 1
fi

echo "Found GCC: $(gcc --version | head -n1)"
echo "Found CMake: $(cmake --version | head -n1)"
echo ""

# 创建构建目录
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo "Configuring CMake..."
cmake .. -G "MinGW Makefiles"

if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

echo ""
echo "Building..."
cmake --build .

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "Build successful!"
    echo "Executable location: ../server/ge.exe"
    echo "  (ge is the short name for GearEngine)"
    echo "=========================================="
    echo ""
    echo "Copying executables and DLL files to server/ and client/ directories..."
    cd ..
    if [ -f "gearengine/copy-output.sh" ]; then
        bash gearengine/copy-output.sh
    else
        echo "Warning: copy-output.sh not found, DLL files may need to be copied manually"
    fi
else
    echo ""
    echo "ERROR: Build failed!"
    exit 1
fi
