#!/bin/bash

# Build script for Linux

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
cmake --build . --config Release -j$(nproc)

echo "Build completed!"
echo "Server executable: ../server/ge (ge is the short name for GearEngine)"
echo "Client executable: ../client/gearchat_client"
echo ""
echo "Copying executables and DLL files to server/ and client/ directories..."
cd ..
if [ -f "gearengine/copy-output.sh" ]; then
    bash gearengine/copy-output.sh
else
    echo "Warning: copy-output.sh not found, DLL files may need to be copied manually"
fi