#################################################################################
# CMakeLists.txt for Mahogany Email Client
#################################################################################
# 
# This is the main CMake configuration file for the Mahogany email client project.
# It provides a cross-platform build system that mirrors the existing Visual Studio
# MSBuild configuration while adding support for other platforms.
#
# OVERVIEW:
# ---------
# Mahogany is a full-featured GUI email client built with wxWidgets that includes:
# - Main application (M/Mahogany executable)
# - IMAP library for mail handling
# - Compface library for X-Face image support
# - DSPAM library for spam filtering
# - Versit library for vCard handling
# - Python scripting interface
#
# USAGE:
# ------
# 1. Prerequisites:
#    - CMake 3.16 or later
#    - C++ compiler with C++11 support
#    - wxWidgets 3.0 or later
#    - Python development headers (optional, for Python support)
#    - OpenSSL (optional, for SSL/TLS support)
#
# 2. Basic build:
#    mkdir build && cd build
#    cmake ..
#    make
#
# 3. Build with options:
#    cmake -DENABLE_PYTHON=ON -DCMAKE_BUILD_TYPE=Release ..
#    make
#
# 4. Install:
#    make
#
# BUILD OPTIONS:
# --------------
# - ENABLE_PYTHON: Enable Python scripting support (default: ON if Python found)
# - ENABLE_SSL: Enable SSL/TLS support (default: ON if OpenSSL found)
# - CMAKE_BUILD_TYPE: Debug, Release, RelWithDebInfo, MinSizeRel
#
# TARGETS:
# --------
# Libraries (all built as static):
# - mahogany_imap: IMAP client library
# - mahogany_compface: X-Face image library
# - mahogany_dspam: Spam filtering library
# - mahogany_versit: vCard library
#
# Executables:
# - mahogany: Main GUI application
# - mtest: IMAP test utility (if built)
# - mailutil: Mail utility (if built)
#
# PLATFORM SUPPORT:
# ------------------
# - Windows (MSVC, MinGW)
# - Linux (GCC, Clang)
# - macOS (Clang, GCC)
# - Other Unix-like systems
#
# The CMake configuration automatically detects the platform and applies
# appropriate compiler flags and library dependencies.
#
#################################################################################

cmake_minimum_required(VERSION 3.16)

# Extract version from Mversion.h
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/Mversion.h" MVERSION_H_CONTENT)
string(REGEX MATCH "#define M_VERSION_MAJOR[ \t]+([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define M_VERSION_MINOR[ \t]+([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define M_VERSION_RELEASE[ \t]+([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(VERSION_RELEASE "${CMAKE_MATCH_1}")

project(Mahogany 
    VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_RELEASE}"
    DESCRIPTION "Mahogany Cross-Platform Email Client"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(ENABLE_PYTHON "Enable Python scripting support" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_definitions(-DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE)
    
    # Disable specific warnings to match MSBuild configuration
    if(MSVC)
        add_compile_options(/wd4100 /wd4244 /wd4267)
        # Set stack reserve size for debug builds
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000")
        endif()
    endif()
elseif(UNIX)
    # Unix-specific settings
    add_definitions(-D_GNU_SOURCE)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    endif()
endif()

# Find required dependencies
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base net html qa)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
    message(STATUS "Found wxWidgets: ${wxWidgets_VERSION}")
    set(HAVE_WXWIDGETS ON)
else()
    message(FATAL_ERROR "wxWidgets not found. Mahogany requires wxWidgets 3.2 or later.
To install wxWidgets:
- Ubuntu/Debian: sudo apt-get install libwxgtk3.2-gtk3-dev
- CentOS/RHEL: sudo yum install wxGTK3-devel  
- Windows: Download from https://www.wxwidgets.org/")
endif()

# Find optional dependencies
if(ENABLE_PYTHON)
    find_package(Python3 COMPONENTS Development)
    if(Python3_FOUND)
        message(STATUS "Found Python: ${Python3_VERSION}")
        add_definitions(-DHAVE_PYTHON)
    else()
        message(WARNING "Python development headers not found. Python support will be disabled.")
        set(ENABLE_PYTHON OFF)
    endif()
endif()

if(ENABLE_SSL)
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
        add_definitions(-DHAVE_SSL)
    else()
        message(WARNING "OpenSSL not found. SSL/TLS support will be disabled.")
        set(ENABLE_SSL OFF)
    endif()
endif()

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS winmm comctl32 rpcrt4 ws2_32)
elseif(UNIX)
    set(PLATFORM_LIBS dl pthread)
    if(NOT APPLE)
        list(APPEND PLATFORM_LIBS rt)
    endif()
endif()

# Create interface target for C library common settings
add_library(mahogany_c_library_common INTERFACE)

# Common preprocessor definitions for C libraries
target_compile_definitions(mahogany_c_library_common INTERFACE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
)

if(WIN32)
    target_compile_definitions(mahogany_c_library_common INTERFACE WIN32)
endif()

# Common compiler-specific settings for C libraries
if(MSVC)
    target_compile_options(mahogany_c_library_common INTERFACE
        /wd4996  # Disable deprecated function warnings
        /wd4244  # Disable conversion warnings
        /wd4267  # Disable size_t conversion warnings
        /wd4100  # Disable unused parameter warnings
        /wd4273  # Disable inconsistent dll linkage warnings
    )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(mahogany_c_library_common INTERFACE
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-unused-variable
        -Wno-pointer-sign
        -Wno-parentheses
    )
endif()

# Add subdirectories for libraries
add_subdirectory(lib/imap)
add_subdirectory(lib/compface)
add_subdirectory(lib/dspam)
add_subdirectory(src/wx/vcard)

# Add main application directory.
add_subdirectory(src)

# Summary
message(STATUS "")
message(STATUS "Mahogany Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  wxWidgets found: ${HAVE_WXWIDGETS}")
message(STATUS "  Python support: ${ENABLE_PYTHON}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "")