# Top level CMakeLists.txt for Mahogany
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
# 2. Configure:
#
#    cmake -S . -B build-dir
#
#    The following Mahogany-specific options can be set:
#
#    - WITH_PYTHON: Enable Python scripting support (default: ON if Python found)
#    - WITH_SSL: Enable SSL/TLS support (default: ON if OpenSSL found)
#
# 3. Build:
#
#    cmake --build build-dir --config Release
#
# 4. Install:
#
#    cmake --install build-dir
#
# TODO:
# -----
#
# - Add support for running SWIG.
# - Add WITH_MODULES=static|dynamic option.

cmake_minimum_required(VERSION 3.16)

# Extract version from Mversion.h which is the source of truth for it.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/Mversion.h" MVERSION_H_CONTENT)
string(REGEX MATCH "#define +M_VERSION_MAJOR +([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(M_VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define +M_VERSION_MINOR +([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(M_VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "#define +M_VERSION_RELEASE +([0-9]+)" _ "${MVERSION_H_CONTENT}")
set(M_VERSION_RELEASE "${CMAKE_MATCH_1}")

project(Mahogany
    VERSION "${M_VERSION_MAJOR}.${M_VERSION_MINOR}.${M_VERSION_RELEASE}"
    DESCRIPTION "Mahogany Cross-Platform Email Client"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(WITH_PYTHON "Enable Python scripting support" ON)
option(WITH_SSL "Enable SSL/TLS support" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE
        STRING "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel"
        FORCE
    )
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()


# Find required dependencies
find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base net html qa)
if(NOT wxWidgets_FOUND)
    message(FATAL_ERROR "wxWidgets not found. Mahogany requires wxWidgets 3.2 or later.

To install wxWidgets:
- Ubuntu/Debian: sudo apt-get install libwxgtk3.2-gtk3-dev
- CentOS/RHEL: sudo yum install wxGTK3-devel
- Windows: Download from https://www.wxwidgets.org/")
endif()

include(${wxWidgets_USE_FILE})

# Find optional dependencies
if(WITH_PYTHON)
    find_package(Python3 COMPONENTS Development)
    if(Python3_FOUND)
        message(STATUS "Found Python: ${Python3_VERSION}")
        add_definitions(-DHAVE_PYTHON)
    else()
        message(WARNING "Python development headers not found. Python support will be disabled.")
        set(WITH_PYTHON OFF)
    endif()
endif()

if(WITH_SSL)
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
        add_definitions(-DHAVE_SSL)
    else()
        message(WARNING "OpenSSL not found. SSL/TLS support will be disabled.")
        set(WITH_SSL OFF)
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
message(STATUS "Configured Mahogany ${PROJECT_VERSION}:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  wxWidgets: ${wxWidgets_VERSION}")
message(STATUS "  Python support: ${WITH_PYTHON}")
message(STATUS "  SSL support: ${WITH_SSL}")
