#!/bin/bash

# Simple test script to verify the build works
set -e

echo "Testing TP-Link Device Controller build..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Not in the project root directory"
    exit 1
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the project
echo "Building project..."
make -j$(nproc)

# Check if executable was created
if [ -f "tplink_controller" ]; then
    echo "✓ Build successful! Executable created: tplink_controller"
    
    # Test basic functionality
    echo "Testing basic functionality..."
    ./tplink_controller --help > /dev/null 2>&1 && echo "✓ Help command works" || echo "✗ Help command failed"
    
    echo "Build test completed successfully!"
else
    echo "✗ Build failed - executable not found"
    exit 1
fi
