#!/bin/bash

# PQC Keychain Generator - Complete Setup Script
# Kubuntu 24.04 (X11/Wayland)
#
# Run: chmod +x setup.sh && ./setup.sh

set -e

echo "========================================="
echo "  PQC Keychain Generator - Setup"
echo "  Kubuntu 24.04 with liboqs 0.12.x"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required build tools
echo -e "${YELLOW}[1/6] Checking build tools...${NC}"
for tool in gcc make git cmake pkg-config; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}✗ $tool not found${NC}"
        echo "Install with: sudo apt install build-essential git cmake pkg-config"
        exit 1
    fi
done
echo -e "${GREEN}✓ All build tools present${NC}\n"

# Check for libraries
echo -e "${YELLOW}[2/6] Checking dependencies...${NC}"
for lib in openssl; do
    if ! pkg-config --exists $lib; then
        echo -e "${RED}✗ $lib not found${NC}"
        echo "Install with: sudo apt install libssl-dev"
        exit 1
    fi
done
echo -e "${GREEN}✓ Dependencies OK${NC}\n"

# Install jansson if needed
echo -e "${YELLOW}[3/6] Checking jansson library...${NC}"
if ! pkg-config --exists jansson; then
    echo "Installing jansson..."
    sudo apt install -y libjansson-dev 2>/dev/null || {
        echo -e "${YELLOW}⚠ Could not install jansson via apt${NC}"
        echo "Try manual install: sudo apt install libjansson-dev"
        exit 1
    }
fi
echo -e "${GREEN}✓ jansson OK${NC}\n"

# Build or link liboqs 0.12.x
echo -e "${YELLOW}[4/6] Checking liboqs 0.12.x...${NC}"
if pkg-config --modversion liboqs 2>/dev/null | grep -q "0.12"; then
    echo -e "${GREEN}✓ liboqs 0.12.x already installed${NC}"
else
    echo "liboqs 0.12.x not found. Building from source..."
    mkdir -p build
    cd build
    
    if [ ! -d "liboqs" ]; then
        echo "Cloning liboqs repository..."
        git clone --depth 1 --branch 0.12.0 https://github.com/open-quantum-safe/liboqs.git
    fi
    
    cd liboqs
    mkdir -p build
    cd build
    
    echo "Configuring liboqs..."
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=/usr/local .. > /dev/null
    echo "Building liboqs (this may take 2-3 minutes)..."
    make -j$(nproc) > /dev/null 2>&1
    echo "Installing liboqs..."
    sudo make install > /dev/null 2>&1
    
    # Update library cache
    sudo ldconfig
    
    cd ../../..
    echo -e "${GREEN}✓ liboqs built and installed${NC}"
fi
echo -e "${GREEN}✓ liboqs ready${NC}\n"

# Create svc-wallet directory
echo -e "${YELLOW}[5/6] Creating svc-wallet directory...${NC}"
mkdir -p svc-wallet
echo -e "${GREEN}✓ svc-wallet created${NC}\n"

# Compile main program
echo -e "${YELLOW}[6/6] Compiling pqc_keygen...${NC}"
COMPILE_FLAGS="-o pqc_keygen pqc_keygen.c -loqs -ljansson -lm -O3"

if gcc $COMPILE_FLAGS 2>/dev/null; then
    echo -e "${GREEN}✓ Compilation successful${NC}\n"
elif gcc -I/usr/local/include -L/usr/local/lib $COMPILE_FLAGS 2>/dev/null; then
    echo -e "${GREEN}✓ Compilation successful (with local paths)${NC}\n"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    echo "Try manual compile:"
    echo "  gcc -I/usr/local/include -L/usr/local/lib -o pqc_keygen pqc_keygen.c -loqs -ljansson -lm -O3 -v"
    exit 1
fi

# Also compile validator
echo -e "${YELLOW}Compiling validate_kchain...${NC}"
if gcc -o validate_kchain validate_kchain.c -loqs -ljansson -lm -O3 2>/dev/null || \
   gcc -I/usr/local/include -L/usr/local/lib -o validate_kchain validate_kchain.c -loqs -ljansson -lm -O3 2>/dev/null; then
    echo -e "${GREEN}✓ validate_kchain compiled${NC}\n"
else
    echo -e "${YELLOW}⚠ validate_kchain compilation skipped (optional)${NC}\n"
fi

# Set library path if needed
if [ ! -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=/usr/local/lib
fi

echo "========================================="
echo -e "${GREEN}✅ Setup complete!${NC}"
echo "========================================="
echo ""
echo "Usage:"
echo "  ./pqc_keygen"
echo ""
echo "Output:"
echo "  svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain"
echo ""
echo "Verify keychain (optional):"
echo "  ./validate_kchain svc-wallet/pqc_master_YYYYMMDD_HHMMSS.kchain"
echo ""
