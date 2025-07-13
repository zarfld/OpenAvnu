#!/bin/bash
#
# Package OpenAvnu testing files for DAW02 deployment
#

PACKAGE_NAME="OpenAvnu_DAW02_Testing_Package_$(date +%Y%m%d_%H%M%S)"
PACKAGE_DIR="$PACKAGE_NAME"

echo "📦 Creating OpenAvnu DAW02 Testing Package..."
echo "Package: $PACKAGE_NAME"
echo ""

# Create package directory
mkdir -p "$PACKAGE_DIR"

# Copy core testing files
echo "Copying testing scripts..."
cp run_daw02_tests.sh "$PACKAGE_DIR/"
cp test_01_build_verification.sh "$PACKAGE_DIR/"
cp test_02_intel_hal_detection.sh "$PACKAGE_DIR/"

# Copy documentation
echo "Copying documentation..."
cp DAW02_TESTING_PLAN.md "$PACKAGE_DIR/"
cp DAW02_TESTING_README.md "$PACKAGE_DIR/"
cp DAEMON_MODERNIZATION_BUILD_VERIFICATION_REPORT.md "$PACKAGE_DIR/"

# Make scripts executable
chmod +x "$PACKAGE_DIR"/*.sh

# Create quick start script
cat > "$PACKAGE_DIR/QUICK_START.sh" << 'EOF'
#!/bin/bash
echo "🧪 OpenAvnu DAW02 Quick Start"
echo "============================"
echo ""
echo "This package contains comprehensive testing for OpenAvnu daemon modernization."
echo ""
echo "Quick Commands:"
echo "  ./run_daw02_tests.sh          # Run complete test suite"
echo "  ./test_01_build_verification.sh  # Test build only"
echo "  ./test_02_intel_hal_detection.sh # Test Intel HAL only"
echo ""
echo "Documentation:"
echo "  DAW02_TESTING_README.md       # Quick start guide"
echo "  DAW02_TESTING_PLAN.md         # Comprehensive testing plan"
echo ""
echo "Prerequisites:"
echo "  - CMake 3.16+"
echo "  - GCC/Clang or Visual Studio"
echo "  - libpcap (Linux) or Npcap (Windows)"
echo ""
echo "Ready to start testing! Run: ./run_daw02_tests.sh"
EOF

chmod +x "$PACKAGE_DIR/QUICK_START.sh"

# Create archive
echo "Creating archive..."
tar -czf "${PACKAGE_NAME}.tar.gz" "$PACKAGE_DIR"
zip -r "${PACKAGE_NAME}.zip" "$PACKAGE_DIR" >/dev/null 2>&1

# Generate transfer instructions
cat > "${PACKAGE_NAME}_TRANSFER_INSTRUCTIONS.txt" << EOF
OpenAvnu DAW02 Testing Package Transfer Instructions
===================================================

Package Created: $(date)
Package Name: $PACKAGE_NAME

Files Generated:
- ${PACKAGE_NAME}.tar.gz    (Linux/macOS)
- ${PACKAGE_NAME}.zip       (Windows)
- ${PACKAGE_NAME}/          (Directory)

Transfer to DAW02:
==================

Option 1: SCP (Linux/macOS)
scp ${PACKAGE_NAME}.tar.gz user@daw02:/tmp/
ssh user@daw02
cd /tmp
tar -xzf ${PACKAGE_NAME}.tar.gz
cd $PACKAGE_NAME
./QUICK_START.sh

Option 2: Shared Network Drive
Copy ${PACKAGE_NAME}.zip to shared drive
Extract on DAW02
Run QUICK_START.sh or run_daw02_tests.sh

Option 3: USB/Physical Media
Copy either archive to USB drive
Transfer to DAW02
Extract and run

On DAW02:
=========
1. Extract the package
2. Make scripts executable: chmod +x *.sh
3. Run: ./run_daw02_tests.sh
4. Review results in test_results_* directory
5. Archive results: tar -czf daw02_results.tar.gz test_results_*
6. Transfer results back for analysis

Expected Runtime: 15-30 minutes
Expected Output: Comprehensive test report with pass/fail status

Support:
========
If tests fail, the generated reports will contain detailed
error information for debugging and resolution.
EOF

# Summary
echo "✅ Package created successfully!"
echo ""
echo "📁 Package Directory: $PACKAGE_DIR/"
echo "📦 Linux Archive: ${PACKAGE_NAME}.tar.gz"
echo "📦 Windows Archive: ${PACKAGE_NAME}.zip"
echo "📋 Transfer Instructions: ${PACKAGE_NAME}_TRANSFER_INSTRUCTIONS.txt"
echo ""
echo "🚀 Ready for deployment to DAW02!"
echo ""
echo "Next steps:"
echo "1. Transfer package to DAW02 using preferred method"
echo "2. Extract and run: ./run_daw02_tests.sh"
echo "3. Review generated test reports"
echo "4. Transfer results back for analysis"
