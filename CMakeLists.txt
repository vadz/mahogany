# Top level CMakeLists.txt for Mahogany
#
# USAGE:
# ------
# 1. Prerequisites:
#    - CMake 3.27 or later
#    - C++ compiler with C++11 support
#    - wxWidgets 3.2 or later
#    - Python development headers (optional, for Python support)
#    - OpenSSL (optional, for SSL/TLS support under Unix systems only)
#
# 2. Configure:
#
#    cmake -S . -B build-dir
#
#    The following Mahogany-specific options can be set:
#
#    - USE_PYTHON: Enable Python scripting support (default: ON if Python found)
#    - USE_SSL: Enable SSL/TLS support (default: ON if OpenSSL found)
#
# 3. Build:
#
#    cmake --build build-dir
#
# TODO:
# -----
#
# - Add installation targets.
# - Add USE_MODULES=static|dynamic option.

cmake_minimum_required(VERSION 3.27)

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
option(USE_PYTHON "Enable Python scripting support" ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  # This is always enabled under Windows because there is really no reason not
  # to do it, as system TLS support is used.
  set(USE_SSL ON)
else()
  option(USE_SSL "Enable SSL/TLS support" ON)
endif()
option(USE_DSPAM "Enable built-in support for spam filtering" OFF)
set(WX_CONFIG "" CACHE PATH "Path to wxWidgets wx-config script")

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

# Define helpers for testing for the compiler in generator expressions.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(IS_GCC_LIKE TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(IS_GCC_LIKE TRUE)
endif()

# Find required dependencies

# We need to use our own version of FindwxWidgets.cmake because the one
# bundled with CMake 3.27 does not support wxWidgets 3.3 yet (webp library is
# not linked under Windows).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")

if(WX_CONFIG)
  execute_process(
    COMMAND
      ${WX_CONFIG} --version
    OUTPUT_VARIABLE
      wxWidgets_VERSION
    ERROR_VARIABLE
      WX_CONFIG_ERROR
    RESULT_VARIABLE
      WX_CONFIG_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(NOT WX_CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run \"${WX_CONFIG}\": ${WX_CONFIG_ERROR}")
  endif()

  execute_process(
    COMMAND
      ${WX_CONFIG} --cxxflags
    OUTPUT_VARIABLE
      wxWidgets_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND
      ${WX_CONFIG} --libs net,html,qa,xml,core,base
    OUTPUT_VARIABLE
      wxWidgets_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  separate_arguments(wxWidgets_CXXFLAGS)
  separate_arguments(wxWidgets_LIBS)

  add_library(mahogany_wx INTERFACE)
  target_compile_options(mahogany_wx INTERFACE ${wxWidgets_CXXFLAGS})
  target_link_libraries(mahogany_wx INTERFACE ${wxWidgets_LIBS})
  set(wxWidgets_LIBRARIES mahogany_wx)
else()
  find_package(wxWidgets 3.2 COMPONENTS core base net html qa xml)
  if(NOT wxWidgets_FOUND)
    message(FATAL_ERROR "wxWidgets not found. Mahogany requires wxWidgets 3.2 or later.

  To install wxWidgets:
  - Ubuntu/Debian: sudo apt-get install libwxgtk3.2-dev
  - CentOS/RHEL: sudo yum install wxGTK3-devel
  - Windows: Download from https://www.wxwidgets.org/")
  endif()

  # This variable is not defined in CONFIG search mode.
  if(wxWidgets_USE_FILE)
    include(${wxWidgets_USE_FILE})
  endif()
endif()

# Find optional dependencies
if(USE_PYTHON)
  find_package(Python3 COMPONENTS Development)
  if(Python3_FOUND)
    list(APPEND extra_features "Python: ${Python3_VERSION}")

    # We use SWIG to generate Python bindings, but don't complain if it's
    # not found because we have pre-generated files in the repository.
    find_package(SWIG COMPONENTS python QUIET)
    if(SWIG_FOUND)
      # Enable new behaviour of UseSWIG module before including it.
      set(UseSWIG_MODULE_VERSION 2)
      include(UseSWIG)
      list(APPEND extra_features "SWIG: ${SWIG_VERSION}")
    endif()
  else()
    message(WARNING "Python development headers not found. Python support will be disabled.")
    set(USE_PYTHON OFF)
  endif()
endif()

if(USE_SSL AND NOT WIN32)
  find_package(OpenSSL COMPONENTS Crypto SSL)
  if(OPENSSL_FOUND)
    list(APPEND extra_features "OpenSSL: ${OPENSSL_VERSION}")
    set(USE_OPENSSL ON)
  else()
    message(WARNING "OpenSSL not found. SSL/TLS support will be disabled.")
    set(USE_SSL OFF)
  endif()
endif()

if(USE_DSPAM)
  list(APPEND extra_features "DSPAM")
  message(FATAL_ERROR "DSPAM support is not yet implemented in CMake.")
endif()

# Generate headers and related files now that we have all USE_XXX values.
add_subdirectory(include)

# Create interface target for C library common settings
add_library(mahogany_c_library_common INTERFACE)

if(WIN32)
  target_compile_definitions(mahogany_c_library_common INTERFACE WIN32)
endif()

# Common compiler-specific settings for C libraries
if(MSVC)
  target_compile_definitions(mahogany_c_library_common INTERFACE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
  )

  target_compile_options(mahogany_c_library_common INTERFACE
    /wd4100  # Disable unused parameter warnings
    /wd4244  # Disable conversion warnings
    /wd4267  # Disable size_t conversion warnings
    /wd4273  # Disable inconsistent dll linkage warnings
    /wd4996  # Disable deprecated function warnings
  )
elseif(IS_GCC_LIKE)
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
if(USE_DSPAM)
  add_subdirectory(lib/dspam)
endif()
add_subdirectory(src/wx/vcard)

# Add main application directory.
add_subdirectory(src)

# Summary
message(STATUS "Configured Mahogany ${PROJECT_VERSION}:")
if(CMAKE_BUILD_TYPE)
  message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "  wxWidgets: ${wxWidgets_VERSION}")
foreach(feature IN LISTS extra_features)
  message(STATUS "  ${feature}")
endforeach()
