#!/bin/bash

# TP-Link Device Controller Build Script for Raspberry Pi 4
# This script automates the build process

set -e  # Exit on any error

echo "========================================"
echo "  TP-Link Device Controller Build Script"
echo "  Raspberry Pi 4 Compatible"
echo "========================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on Raspberry Pi
check_raspberry_pi() {
    if [ -f /proc/device-tree/model ]; then
        if grep -q "Raspberry Pi" /proc/device-tree/model; then
            print_status "Running on Raspberry Pi: $(cat /proc/device-tree/model)"
        else
            print_warning "Not running on Raspberry Pi. Some optimizations may not apply."
        fi
    else
        print_warning "Cannot detect Raspberry Pi. Proceeding anyway."
    fi
}

# Check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required packages
    for pkg in build-essential cmake pkg-config libsqlite3-dev libssl-dev libjsoncpp-dev; do
        if ! dpkg -l | grep -q "^ii.*$pkg"; then
            missing_deps+=($pkg)
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo
        echo "Please install them with:"
        echo "sudo apt update"
        echo "sudo apt install -y ${missing_deps[*]}"
        exit 1
    fi
    
    print_status "All dependencies found"
}

# Check CMake version
check_cmake() {
    local cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    local required_version="3.10"
    
    if [ "$(printf '%s\n' "$required_version" "$cmake_version" | sort -V | head -n1)" != "$required_version" ]; then
        print_error "CMake version $cmake_version is too old. Required: $required_version or higher"
        exit 1
    fi
    
    print_status "CMake version $cmake_version is compatible"
}

# Check C++ compiler
check_compiler() {
    if ! command -v g++ &> /dev/null; then
        print_error "g++ compiler not found"
        exit 1
    fi
    
    local cpp_version=$(g++ -dumpversion | cut -d. -f1)
    if [ "$cpp_version" -lt 7 ]; then
        print_error "C++ compiler version $cpp_version is too old. Required: C++17 support (g++ 7+)"
        exit 1
    fi
    
    print_status "C++ compiler version $cpp_version supports C++17"
}

# Create build directory
setup_build() {
    print_status "Setting up build directory..."
    
    if [ -d "build" ]; then
        print_warning "Build directory exists. Cleaning..."
        rm -rf build
    fi
    
    mkdir build
    cd build
    print_status "Build directory created"
}

# Configure with CMake
configure_cmake() {
    print_status "Configuring with CMake..."
    
    # Detect architecture and set appropriate flags
    local arch=$(uname -m)
    local cxxflags=""
    
    if [[ "$arch" == "aarch64" || "$arch" == "arm64" ]]; then
        # ARM64 (Raspberry Pi 4)
        cxxflags="-O2 -march=native -mtune=native -mfpu=neon-fp-armv8 -mfloat-abi=hard"
        print_status "Detected ARM64 architecture - using Raspberry Pi optimizations"
    elif [[ "$arch" == "armv7l" ]]; then
        # ARM32 (older Raspberry Pi)
        cxxflags="-O2 -march=native -mtune=native -mfpu=neon -mfloat-abi=hard"
        print_status "Detected ARM32 architecture - using ARM optimizations"
    else
        # x86_64 or other architectures
        cxxflags="-O2 -march=native -mtune=native"
        print_status "Detected $arch architecture - using generic optimizations"
    fi
    
    export CXXFLAGS="$cxxflags"
    
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
             -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    print_status "CMake configuration complete"
}

# Build the project
build_project() {
    print_status "Building project..."
    
    # Use all available CPU cores
    local cores=$(nproc)
    print_status "Using $cores CPU cores for compilation"
    
    make -j$cores
    
    if [ $? -eq 0 ]; then
        print_status "Build successful!"
    else
        print_error "Build failed!"
        exit 1
    fi
}

# Run tests (if any)
run_tests() {
    print_status "Checking for tests..."
    
    if [ -f "test_tplink_controller" ]; then
        print_status "Running tests..."
        ./test_tplink_controller
    else
        print_warning "No tests found"
    fi
}

# Create installation script
create_install_script() {
    print_status "Creating installation script..."
    
    cat > install.sh << 'EOF'
#!/bin/bash

# Installation script for TP-Link Device Controller

set -e

echo "Installing TP-Link Device Controller..."

# Create application directory
sudo mkdir -p /opt/tplink-controller
sudo cp tplink_controller /opt/tplink-controller/

# Create systemd service file
sudo tee /etc/systemd/system/tplink-controller.service > /dev/null << 'SERVICE_EOF'
[Unit]
Description=TP-Link Device Controller
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/opt/tplink-controller
ExecStart=/opt/tplink-controller/tplink_controller
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable tplink-controller

echo "Installation complete!"
echo "To start the service: sudo systemctl start tplink-controller"
echo "To check status: sudo systemctl status tplink-controller"
echo "To view logs: journalctl -u tplink-controller -f"
EOF

    chmod +x install.sh
    print_status "Installation script created: install.sh"
}

# Main build process
main() {
    echo "Starting build process..."
    echo
    
    check_raspberry_pi
    check_dependencies
    check_cmake
    check_compiler
    setup_build
    configure_cmake
    build_project
    run_tests
    create_install_script
    
    echo
    print_status "Build process completed successfully!"
    echo
    echo "Next steps:"
    echo "1. Run the application: ./tplink_controller"
    echo "2. Or install as service: ./install.sh"
    echo "3. Check README.md for usage instructions"
    echo
}

# Run main function
main "$@"
