#!/bin/bash

# TP-Link Device Controller Cross-Platform Build Script
# Supports Ubuntu 24 (x64) and Raspberry Pi (ARM)

set -e  # Exit on any error

echo "========================================"
echo "  TP-Link Device Controller Build Script"
echo "  Cross-Platform: Ubuntu 24 (x64) & Raspberry Pi (ARM)"
echo "========================================"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

print_arch() {
    echo -e "${BLUE}[ARCH]${NC} $1"
}

# Detect system information
detect_system() {
    local arch=$(uname -m)
    local os=$(uname -s)
    local distro=""
    
    if [ -f /etc/os-release ]; then
        distro=$(grep "^NAME=" /etc/os-release | cut -d'"' -f2)
    fi
    
    print_arch "Detected System:"
    print_arch "  Architecture: $arch"
    print_arch "  OS: $os"
    print_arch "  Distribution: $distro"
    
    # Set architecture-specific variables
    case "$arch" in
        "x86_64")
            ARCH_TYPE="x64"
            IS_RASPBERRY_PI=false
            ;;
        "aarch64"|"arm64")
            ARCH_TYPE="ARM64"
            IS_RASPBERRY_PI=true
            ;;
        "armv7l")
            ARCH_TYPE="ARM32"
            IS_RASPBERRY_PI=true
            ;;
        *)
            ARCH_TYPE="UNKNOWN"
            IS_RASPBERRY_PI=false
            print_warning "Unknown architecture: $arch"
            ;;
    esac
    
    print_arch "Architecture Type: $ARCH_TYPE"
    print_arch "Raspberry Pi: $IS_RASPBERRY_PI"
}

# Check if running on Raspberry Pi
check_raspberry_pi() {
    if [ "$IS_RASPBERRY_PI" = true ]; then
        if [ -f /proc/device-tree/model ]; then
            local model=$(cat /proc/device-tree/model 2>/dev/null || echo "Unknown")
            print_arch "Raspberry Pi Model: $model"
        else
            print_arch "ARM device detected (Raspberry Pi compatible)"
        fi
    else
        print_arch "x64 system detected (Ubuntu 24 compatible)"
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
    
    # Set architecture-specific optimization flags
    local cxxflags=""
    local cmake_flags=""
    
    case "$ARCH_TYPE" in
        "x64")
            cxxflags="-O2 -march=native -mtune=native"
            cmake_flags="-DCMAKE_BUILD_TYPE=Release"
            print_arch "Using x64 optimizations"
            ;;
        "ARM64")
            cxxflags="-O2 -march=native -mtune=native -mfpu=neon-fp-armv8 -mfloat-abi=hard"
            cmake_flags="-DCMAKE_BUILD_TYPE=Release"
            print_arch "Using ARM64 NEON optimizations"
            ;;
        "ARM32")
            cxxflags="-O2 -march=native -mtune=native -mfpu=neon -mfloat-abi=hard"
            cmake_flags="-DCMAKE_BUILD_TYPE=Release"
            print_arch "Using ARM32 NEON optimizations"
            ;;
        *)
            cxxflags="-O2"
            cmake_flags="-DCMAKE_BUILD_TYPE=Release"
            print_warning "Using generic optimizations for unknown architecture"
            ;;
    esac
    
    export CXXFLAGS="$cxxflags"
    
    cmake .. $cmake_flags \
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

# Create architecture-specific documentation
create_arch_docs() {
    print_status "Creating architecture-specific documentation..."
    
    cat > ARCHITECTURE.md << EOF
# Architecture Support

## Supported Platforms

### Ubuntu 24 (x64)
- **Architecture**: x86_64
- **Optimizations**: Native x64 instructions
- **Compiler Flags**: \`-O2 -march=native -mtune=native\`
- **Performance**: High performance on modern x64 processors

### Raspberry Pi 4 (ARM64)
- **Architecture**: aarch64/arm64
- **Optimizations**: ARM64 NEON SIMD instructions
- **Compiler Flags**: \`-O2 -march=native -mtune=native -mfpu=neon-fp-armv8 -mfloat-abi=hard\`
- **Performance**: Optimized for ARM Cortex-A72 cores

### Raspberry Pi 3 (ARM32)
- **Architecture**: armv7l
- **Optimizations**: ARM32 NEON SIMD instructions
- **Compiler Flags**: \`-O2 -march=native -mtune=native -mfpu=neon -mfloat-abi=hard\`
- **Performance**: Optimized for ARM Cortex-A53 cores

## Build Detection

The build system automatically detects your architecture and applies appropriate optimizations:

\`\`\`bash
# Automatic detection
./build_cross_platform.sh

# Manual build (if needed)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j\$(nproc)
\`\`\`

## Performance Notes

- **x64**: Best performance on development machines
- **ARM64**: Optimized for Raspberry Pi 4 with NEON instructions
- **ARM32**: Compatible with older Raspberry Pi models
EOF

    print_status "Architecture documentation created: ARCHITECTURE.md"
}

# Main build process
main() {
    echo "Starting cross-platform build process..."
    echo
    
    detect_system
    check_raspberry_pi
    check_dependencies
    check_cmake
    check_compiler
    setup_build
    configure_cmake
    build_project
    run_tests
    create_install_script
    create_arch_docs
    
    echo
    print_status "Cross-platform build process completed successfully!"
    echo
    echo "Architecture: $ARCH_TYPE"
    echo "Raspberry Pi: $IS_RASPBERRY_PI"
    echo
    echo "Next steps:"
    echo "1. Run the application: ./tplink_controller"
    echo "2. Or install as service: ./install.sh"
    echo "3. Check README.md for usage instructions"
    echo "4. Check ARCHITECTURE.md for platform details"
    echo
}

# Run main function
main "$@"
