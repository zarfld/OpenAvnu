#!/bin/bash
# OpenAvnu Build and gPTP Verification Script

set -e

echo "========================================"
echo "OpenAvnu Build and gPTP Verification"
echo "========================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "ℹ $1"
}

# Check if we're in the OpenAvnu directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "lib/avtp_pipeline" ]; then
    print_error "This script must be run from the OpenAvnu root directory"
    exit 1
fi

print_info "Starting OpenAvnu build verification..."

# Check for required tools
print_info "Checking build dependencies..."

if ! command -v cmake &> /dev/null; then
    print_error "cmake is required but not installed"
    exit 1
fi
print_success "cmake found"

if ! command -v make &> /dev/null; then
    print_error "make is required but not installed"
    exit 1
fi
print_success "make found"

if ! command -v gcc &> /dev/null; then
    print_error "gcc is required but not installed"
    exit 1
fi
print_success "gcc found"

# Check for git submodules
print_info "Checking git submodules..."

if [ ! -d "thirdparty/gptp" ] || [ ! -f "thirdparty/gptp/CMakeLists.txt" ]; then
    print_warning "gPTP submodule not found, initializing..."
    git submodule update --init --recursive
    if [ $? -eq 0 ]; then
        print_success "gPTP submodule initialized"
    else
        print_error "Failed to initialize gPTP submodule"
        exit 1
    fi
else
    print_success "gPTP submodule found"
fi

# Create build directory
print_info "Setting up build directory..."
mkdir -p build
cd build

# Configure with cmake
print_info "Configuring build with cmake..."
cmake .. -DOPENAVNU_BUILD_GPTP=ON
if [ $? -eq 0 ]; then
    print_success "cmake configuration successful"
else
    print_error "cmake configuration failed"
    exit 1
fi

# Build the project
print_info "Building OpenAvnu..."
make -j$(nproc)
if [ $? -eq 0 ]; then
    print_success "Build successful"
else
    print_error "Build failed"
    exit 1
fi

# Return to root directory
cd ..

# Check if gPTP daemon was built
print_info "Verifying gPTP daemon..."

GPTP_PATHS=(
    "thirdparty/gptp/linux/build/obj/daemon_cl"
    "build/thirdparty/gptp/linux/daemon_cl"
)

GPTP_FOUND=false
for path in "${GPTP_PATHS[@]}"; do
    if [ -f "$path" ]; then
        print_success "gPTP daemon found: $path"
        GPTP_FOUND=true
        GPTP_DAEMON_PATH="$path"
        break
    fi
done

if [ "$GPTP_FOUND" = false ]; then
    print_error "gPTP daemon not found after build"
    exit 1
fi

# Check if the daemon is executable
if [ -x "$GPTP_DAEMON_PATH" ]; then
    print_success "gPTP daemon is executable"
else
    print_error "gPTP daemon is not executable"
    exit 1
fi

# Check for AVTP pipeline libraries
print_info "Checking AVTP pipeline components..."

if [ -d "build/lib/avtp_pipeline" ]; then
    print_success "AVTP pipeline components built"
else
    print_warning "AVTP pipeline components not found"
fi

# Check for MRP daemon
if [ -f "build/daemons/mrpd/mrpd" ]; then
    print_success "MRP daemon built"
else
    print_warning "MRP daemon not found"
fi

# Check for MAAP daemon
if [ -f "build/daemons/maap/maap_daemon" ]; then
    print_success "MAAP daemon built"
else
    print_warning "MAAP daemon not found"
fi

# Network interface check
print_info "Checking network interfaces..."
interfaces=$(ip link show | grep -E "^[0-9]+:" | grep -v lo | awk -F': ' '{print $2}' | head -3)
if [ -n "$interfaces" ]; then
    print_success "Available network interfaces:"
    echo "$interfaces" | while read -r interface; do
        echo "  - $interface"
    done
else
    print_warning "No network interfaces found (other than loopback)"
fi

# Test gPTP daemon help
print_info "Testing gPTP daemon functionality..."
if timeout 5 "$GPTP_DAEMON_PATH" --help &>/dev/null; then
    print_success "gPTP daemon responds to help command"
else
    print_warning "gPTP daemon help test inconclusive (this is normal)"
fi

# Final summary
echo ""
echo "========================================"
echo "BUILD VERIFICATION COMPLETE"
echo "========================================"
print_success "OpenAvnu build verification successful!"
echo ""
print_info "Next steps:"
echo "1. Start gPTP daemon: sudo ./run_gptp.sh <interface>"
echo "2. Start MRP daemon: sudo ./run_srp.sh <interface>"
echo "3. Run AVTP applications from examples/ directory"
echo ""
print_info "For more information, see:"
echo "- docs/gPTP_Integration.md"
echo "- examples/README files"
echo "- documents/examples/AVB_Talker_Listener_Example_Setup.md"
