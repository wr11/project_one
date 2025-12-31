#!/bin/bash
# Copy executables and dependencies to server/client directories

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if build directory exists
if [ ! -d "build/bin" ]; then
    echo "Error: build/bin directory not found. Please build the project first."
    exit 1
fi

# Create directories if they don't exist
mkdir -p ../server
mkdir -p ../client

echo "Copying executables..."

# Copy server executable (ge is the short name for GearEngine)
if [ -f "../server/ge.exe" ]; then
    echo "  ✓ Server executable already in ../server/ (ge.exe)"
elif [ -f "../server/ge" ]; then
    echo "  ✓ Server executable already in ../server/ (ge)"
elif [ -f "build/bin/ge.exe" ]; then
    cp build/bin/ge.exe ../server/
    echo "  ✓ Copied ge.exe to ../server/"
elif [ -f "build/bin/ge" ]; then
    cp build/bin/ge ../server/
    echo "  ✓ Copied ge to ../server/"
else
    echo "  ✗ Server executable not found"
fi

# Copy client executable
if [ -f "build/bin/gearchat_client.exe" ]; then
    cp build/bin/gearchat_client.exe ../client/
    echo "  ✓ Copied gearchat_client.exe to ../client/"
elif [ -f "build/bin/gearchat_client" ]; then
    cp build/bin/gearchat_client ../client/
    echo "  ✓ Copied gearchat_client to ../client/"
else
    echo "  ✗ Client executable not found"
fi

# Copy DLL files (Windows)
echo "Copying DLL files..."
DLL_COPIED=0

# Copy libuv.dll from build/libuv
if [ -f "build/libuv/libuv.dll" ]; then
    cp build/libuv/libuv.dll ../server/ 2>/dev/null || true
    cp build/libuv/libuv.dll ../client/ 2>/dev/null || true
    echo "  ✓ Copied libuv.dll to ../server/ and ../client/"
    DLL_COPIED=1
fi

# Copy libbson DLL from build/libbson (if exists)
if [ -f "build/libbson/lib1.dll" ]; then
    cp build/libbson/lib1.dll ../server/ 2>/dev/null || true
    cp build/libbson/lib1.dll ../client/ 2>/dev/null || true
    echo "  ✓ Copied libbson DLL to ../server/ and ../client/"
    DLL_COPIED=1
fi

# Also check build/bin for any DLL files
if [ -d "build/bin" ]; then
    DLL_COUNT=$(find build/bin -name "*.dll" 2>/dev/null | wc -l)
    if [ "$DLL_COUNT" -gt 0 ]; then
        cp build/bin/*.dll ../server/ 2>/dev/null || true
        cp build/bin/*.dll ../client/ 2>/dev/null || true
        echo "  ✓ Copied DLL files from build/bin/"
        DLL_COPIED=1
    fi
fi

if [ "$DLL_COPIED" -eq 0 ]; then
    echo "  ⚠ No DLL files found (may be using static linking)"
fi

echo ""
echo "Done! Executables are now in:"
echo "  - ../server/"
echo "  - ../client/"
