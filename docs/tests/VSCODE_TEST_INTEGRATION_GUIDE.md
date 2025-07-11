# VS Code Test Integration Guide for OpenAvnu

This guide explains different ways to integrate and run tests in VS Code for the OpenAvnu project.

## 1. VS Code Tasks (Added) ✅

### Overview
Added Intel HAL validation test to VS Code tasks for easy manual triggering.

### Usage
1. Open Command Palette (`Ctrl+Shift+P`)
2. Type "Tasks: Run Task"
3. Select one of:
   - "Run Intel HAL Validation Test (Windows)"
   - "Run Intel HAL Validation Test (Linux/WSL)"

### Benefits
- Quick access via Command Palette
- Integrated with build dependencies
- Output displayed in integrated terminal
- Problem matcher integration for error highlighting

## 2. VS Code Test Explorer Integration

### Overview
VS Code Test Explorer provides a dedicated test view with tree structure and individual test execution.

### Implementation Options

#### Option A: CMake Tools Extension
```json
// .vscode/settings.json
{
    "cmake.testExplorer": {
        "enabled": true
    },
    "cmake.ctest.args": [
        "--output-on-failure"
    ]
}
```

#### Option B: C++ TestMate Extension
For CppUTest integration:
```json
// .vscode/settings.json
{
    "testMate.cpp.test.executables": [
        {
            "name": "Intel HAL Tests",
            "pattern": "build/**/intel_hal_validation_test{.exe,}",
            "cwd": "${workspaceFolder}/build"
        }
    ]
}
```

### Benefits
- Dedicated test view in sidebar
- Individual test execution
- Test result visualization
- Test discovery and organization

## 3. Launch Configuration (Debugging)

### Implementation
Create `.vscode/launch.json` for debugging tests:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Intel HAL Test",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/tests/intel_hal/Debug/intel_hal_validation_test.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "console": "integratedTerminal",
            "preLaunchTask": "Run Intel HAL Validation Test (Windows)"
        }
    ]
}
```

### Benefits
- Integrated debugging support
- Breakpoint support
- Variable inspection
- Call stack analysis

## 4. Custom Commands

### Implementation
Add to `.vscode/settings.json`:

```json
{
    "commands": [
        {
            "command": "openavnu.runIntelHALTest",
            "title": "Run Intel HAL Test",
            "category": "OpenAvnu"
        }
    ]
}
```

With corresponding extension or keybinding:
```json
// .vscode/keybindings.json
[
    {
        "key": "ctrl+shift+t ctrl+h",
        "command": "workbench.action.tasks.runTask",
        "args": "Run Intel HAL Validation Test (Windows)"
    }
]
```

### Benefits
- Custom keyboard shortcuts
- Command palette integration
- Context menu integration

## 5. Status Bar Integration

### Implementation
Could add status bar items for quick test access using workspace settings or extensions.

### Benefits
- Always visible test controls
- Quick status indication
- One-click test execution

## Current Implementation Status

### ✅ Completed
- **VS Code Tasks**: Intel HAL validation test tasks added
- **Problem Matchers**: Configured for Windows (msCompile) and Linux (gcc)
- **Build Dependencies**: Automatic CMake configuration before test execution
- **Terminal Integration**: Output displayed in integrated terminal with focus

### Available Tasks
1. `Run Intel HAL Validation Test (Windows)` - Build and run on Windows
2. `Run Intel HAL Validation Test (Linux/WSL)` - Build and run on Linux/WSL

### Task Features
- **Auto-dependency**: Automatically runs CMake configuration
- **Problem Matching**: Compiler errors highlighted in Problems panel
- **Terminal Focus**: Automatically shows terminal output
- **Shared Panel**: Uses shared terminal for consistent output

## Recommended Next Steps

### Immediate (High Priority)
1. **Test VS Code Tasks** - Verify the added Intel HAL test tasks work correctly
2. **Document Usage** - Add task usage to main README
3. **Keyboard Shortcuts** - Add custom keybindings for frequent test execution

### Short Term (Medium Priority)
1. **Test Explorer Integration** - Implement CMake Tools or TestMate integration
2. **Launch Configuration** - Add debugging support for tests
3. **CI/CD Integration** - Ensure VS Code tasks align with CI/CD workflows

### Long Term (Low Priority)
1. **Custom Extension** - Consider developing OpenAvnu-specific VS Code extension
2. **Status Bar Integration** - Add test status indicators
3. **Test Discovery** - Automatic test discovery and organization

## Usage Examples

### Running Intel HAL Test
1. **Via Command Palette**:
   - `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Intel HAL Validation Test (Windows)"

2. **Via Terminal Menu**:
   - Terminal → Run Task → Select test task

3. **Via Keyboard** (if configured):
   - `Ctrl+Shift+T` → `Ctrl+H`

### Debugging Intel HAL Test
1. Set breakpoints in test code
2. Press `F5` or use Run and Debug view
3. Select "Debug Intel HAL Test" configuration

## Integration with CI/CD

The VS Code tasks are designed to mirror the CI/CD pipeline:
- Same CMake targets
- Same build configurations
- Same dependency chains
- Same problem detection

This ensures consistency between local development and automated builds.

## Troubleshooting

### Common Issues
1. **CMake Not Found**: Ensure CMake is in PATH
2. **Build Directory Missing**: Tasks automatically create and configure
3. **Permission Issues**: Run VS Code as administrator on Windows
4. **WSL Issues**: Ensure WSL is properly configured

### Debug Steps
1. Check VS Code Output panel for task execution logs
2. Verify CMake configuration in build directory
3. Check terminal output for detailed error messages
4. Validate environment variables and PATH settings

## Future Enhancements

1. **Multi-Platform Tasks**: Automatic platform detection
2. **Test Suites**: Grouped test execution
3. **Coverage Integration**: Code coverage reporting
4. **Performance Testing**: Benchmarking integration
5. **Hardware Detection**: Automatic Intel HAL capability detection
