# CMake Build System for Mahogany

This document describes the new CMake-based build system for the Mahogany email client.

## Overview

The CMake build system provides a cross-platform alternative to the existing MSBuild (Windows) and autotools (Unix) build systems. It supports building all major components:

- **mahogany_imap**: IMAP/POP3/SMTP client library
- **mahogany_compface**: X-Face image compression library  
- **mahogany_dspam**: Spam filtering library
- **mahogany_versit**: vCard/vCalendar library
- **mahogany**: Main GUI application (requires wxWidgets)

## Quick Start

### Prerequisites

- CMake 3.16 or later
- C++ compiler with C++11 support
- wxWidgets 3.0 or later (for GUI application)
- Python development headers (optional, for Python scripting)
- OpenSSL (optional, for SSL/TLS support)

### Ubuntu/Debian
```bash
sudo apt-get install build-essential cmake libwxgtk3.0-gtk3-dev libssl-dev python3-dev
```

### Windows
- Install Visual Studio 2019 or later
- Install CMake from https://cmake.org/
- Install wxWidgets from https://www.wxwidgets.org/

### Build

#### Linux/macOS
```bash
# Basic build
./build.sh

# Custom build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### Windows
```batch
# Basic build
build.bat

# Custom build with Visual Studio
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENABLE_PYTHON` | `ON` | Enable Python scripting support |
| `ENABLE_SSL` | `ON` | Enable SSL/TLS support |
| `CMAKE_BUILD_TYPE` | `Release` | Build type (Debug/Release/RelWithDebInfo/MinSizeRel) |

Example:
```bash
cmake .. -DENABLE_PYTHON=OFF -DCMAKE_BUILD_TYPE=Debug
```

## Application Build

Mahogany is built as a standalone application with all libraries statically linked.

## Platform-Specific Notes

### Windows
- The build uses the same compiler flags and dependencies as the existing MSBuild system
- All libraries are built statically
- Precompiled headers are used when available

### Unix/Linux
- Currently builds a subset of IMAP functionality due to missing Unix OSDEP implementation
- Full functionality requires porting the complete c-client OSDEP layer
- SSL support via OpenSSL instead of platform-specific implementations

### macOS
- Should work similarly to Linux but may need platform-specific adjustments

## Current Limitations

1. **IMAP Library**: Unix builds currently include only core functionality. Full protocol support requires complete OSDEP implementation.

2. **Utilities**: IMAP test utilities (mtest, mailutil) are disabled pending full OSDEP support.

3. **Python Interface**: SWIG-generated files are included as-is. Regeneration requires SWIG.

## Integration with Existing Build Systems

The CMake system is designed to coexist with existing build systems:

- MSBuild files (.vcxproj/.sln) remain unchanged
- Autotools files (configure.ac/Makefile) remain unchanged  
- Build artifacts are placed in separate directories

## Future Improvements

1. Complete Unix OSDEP implementation for full IMAP functionality
2. Cross-compilation support
3. Package generation (CPack)
4. Continuous integration configuration
5. Enhanced testing framework