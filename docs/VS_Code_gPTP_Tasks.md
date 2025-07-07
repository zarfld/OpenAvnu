# VS Code Tasks for gPTP Integration

This document summarizes the VS Code tasks that have been added to enhance the developer experience with the gPTP integration in OpenAvnu.

## Available gPTP Tasks

The following tasks are now available in VS Code via **Terminal > Run Task...** or **Ctrl+Shift+P** > **Tasks: Run Task**:

### Build & Configuration Tasks

1. **Apply gPTP Windows Build Fix**
   - Fixes the config file path issue in gPTP CMakeLists.txt for Windows builds
   - Replaces `${CMAKE_SOURCE_DIR}` with `${CMAKE_CURRENT_SOURCE_DIR}`
   - Run this if you encounter config file copy errors during Windows builds

2. **Build gPTP Only (Windows)**
   - Builds only the gPTP target (`gptp2`) on Windows using Visual Studio
   - Faster than building the entire OpenAvnu project when only testing gPTP

3. **Build gPTP Only (Linux/WSL)**
   - Builds only the gPTP target (`gptp2`) on Linux/WSL
   - Faster than building the entire OpenAvnu project when only testing gPTP

4. **CMake Configure with gPTP (Windows)**
   - Configures the build system with gPTP explicitly enabled (`-DOPENAVNU_BUILD_GPTP=ON`)
   - Use this to ensure gPTP is built as part of the OpenAvnu project

5. **CMake Configure with gPTP (Linux/WSL)**
   - Configures the build system with gPTP explicitly enabled for Linux/WSL
   - Use this to ensure gPTP is built as part of the OpenAvnu project

### Runtime Tasks

6. **Run gPTP Daemon (Windows)**
   - Starts the gPTP daemon on Windows using `run_gptp.ps1`
   - Runs in background mode with dedicated terminal panel
   - Automatically builds the project first if needed

7. **Run gPTP Daemon (Linux/WSL)**
   - Starts the gPTP daemon on Linux/WSL using `run_gptp.sh`
   - Runs in background mode with dedicated terminal panel
   - Automatically builds the project first if needed

### Testing & Verification Tasks

8. **Verify gPTP Integration**
   - Runs the comprehensive verification script (`verify_gptp_integration.ps1`)
   - Checks that all components are properly integrated and functional
   - Provides detailed status report and quick start instructions

### Documentation Tasks

9. **Open gPTP Documentation**
   - Opens the main gPTP integration documentation (`docs/gPTP_Integration.md`)
   - Contains comprehensive setup, configuration, and troubleshooting information

10. **Open gPTP Windows Build Fix Guide**
    - Opens the Windows build fix documentation (`gptp_windows_build_fix.md`)
    - Contains specific instructions for resolving Windows build issues

## Task Dependencies

The tasks are designed with appropriate dependencies:

- Build tasks depend on CMake configuration tasks
- Run daemon tasks depend on build tasks
- All tasks automatically create the build directory if needed

## Usage Examples

### First-time Setup
1. Run **Initialize Git Submodules** (if not already done)
2. Run **Apply gPTP Windows Build Fix** (Windows only)
3. Run **CMake Configure with gPTP (Windows/Linux)**
4. Run **Build All (Windows/Linux)**

### Development Workflow
1. Run **Build gPTP Only** for faster gPTP-specific builds
2. Run **Verify gPTP Integration** to test changes
3. Run **Run gPTP Daemon** to test runtime functionality

### Troubleshooting
1. Run **Apply gPTP Windows Build Fix** for Windows config issues
2. Run **Open gPTP Documentation** for detailed troubleshooting
3. Run **Verify gPTP Integration** for comprehensive status check

## Integration with Existing Tasks

The new gPTP tasks complement the existing OpenAvnu build system:

- **Build All** tasks now include gPTP when `OPENAVNU_BUILD_GPTP=ON`
- **CMake Configure** tasks can be replaced with gPTP-specific versions
- **Test** tasks will include gPTP tests when available

## Cross-Platform Support

All tasks are designed to work on both Windows and Linux:

- Windows tasks use PowerShell and Visual Studio
- Linux/WSL tasks use bash and Unix Makefiles
- Platform-specific tasks are clearly labeled

## Next Steps

The gPTP integration is now complete with full VS Code task support. Developers can:

1. Use the tasks for streamlined development workflow
2. Reference the documentation for advanced configuration
3. Contribute improvements to the gPTP integration

For more information, see:
- `docs/gPTP_Integration.md` - Main integration documentation
- `gptp_windows_build_fix.md` - Windows-specific troubleshooting
- `.vscode/tasks.json` - Task definitions and comments
