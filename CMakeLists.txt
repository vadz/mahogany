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
#    cmake -DBUILD_SHARED_LIBS=ON -DENABLE_PYTHON=ON -DCMAKE_BUILD_TYPE=Release ..
#    make
#
# 4. Install:
#    make install
#
# BUILD OPTIONS:
# --------------
# - BUILD_SHARED_LIBS: Build shared libraries instead of static (default: OFF)
# - ENABLE_PYTHON: Enable Python scripting support (default: ON if Python found)
# - ENABLE_SSL: Enable SSL/TLS support (default: ON if OpenSSL found)
# - CMAKE_BUILD_TYPE: Debug, Release, RelWithDebInfo, MinSizeRel
# - CMAKE_INSTALL_PREFIX: Installation directory
#
# TARGETS:
# --------
# Libraries:
# - mahogany_imap: IMAP client library (static/shared)
# - mahogany_compface: X-Face image library (static/shared)
# - mahogany_dspam: Spam filtering library (static/shared)
# - mahogany_versit: vCard library (static/shared)
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

project(Mahogany 
    VERSION 0.68.0
    DESCRIPTION "Mahogany Cross-Platform Email Client"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_PYTHON "Enable Python scripting support" ON)
option(ENABLE_SSL "Enable SSL/TLS support" ON)
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_UTILITIES "Build utility programs" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING 
        "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    add_definitions(-DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE)
    if(BUILD_SHARED_LIBS)
        add_definitions(-DWXUSINGDLL)
    endif()
    
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
find_package(wxWidgets QUIET COMPONENTS core base net html aui adv)
if(wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
    message(STATUS "Found wxWidgets: ${wxWidgets_VERSION}")
    set(HAVE_WXWIDGETS ON)
else()
    message(WARNING "wxWidgets not found. GUI application will not be built.")
    message(STATUS "To build the complete application, please install wxWidgets 3.0 or later.")
    message(STATUS "On Ubuntu/Debian: sudo apt-get install libwxgtk3.0-gtk3-dev")
    message(STATUS "On CentOS/RHEL: sudo yum install wxGTK3-devel")
    message(STATUS "On Windows: Download from https://www.wxwidgets.org/")
    set(HAVE_WXWIDGETS OFF)
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

# Set global include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Configure version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Mversion.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/Mversion.h"
    @ONLY
)

# Add subdirectories for libraries
add_subdirectory(lib/imap)
add_subdirectory(lib/compface)
add_subdirectory(lib/dspam)
add_subdirectory(src/wx/vcard)

# Add main source directory (only if wxWidgets is available)
if(HAVE_WXWIDGETS)
    add_subdirectory(src)
else()
    message(STATUS "Skipping main application build (wxWidgets not found)")
endif()

# Add tests if enabled
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install configuration
include(GNUInstallDirs)

# Generate and install export configuration
install(EXPORT MahoganyTargets
    FILE MahoganyTargets.cmake
    NAMESPACE Mahogany::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mahogany
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MahoganyConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MahoganyConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mahogany
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MahoganyConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MahoganyConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MahoganyConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Mahogany
)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mahogany
    FILES_MATCHING PATTERN "*.h"
)

# Install configured version header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/Mversion.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mahogany
)

# Summary
message(STATUS "")
message(STATUS "Mahogany Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  wxWidgets found: ${HAVE_WXWIDGETS}")
message(STATUS "  Python support: ${ENABLE_PYTHON}")
message(STATUS "  SSL support: ${ENABLE_SSL}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build utilities: ${BUILD_UTILITIES}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(NOT HAVE_WXWIDGETS)
    message(STATUS "")
    message(STATUS "NOTE: Libraries will be built but GUI application requires wxWidgets.")
endif()
message(STATUS "")