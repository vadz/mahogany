#################################################################################
# CMakeLists.txt for Mahogany Tests
#################################################################################
# This file builds test programs for the Mahogany libraries and components.

# Only build tests if testing is enabled
if(NOT BUILD_TESTING)
    return()
endif()

# Basic library tests
# (tests can be added here as needed)

# Check existing test directories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mime")
    add_subdirectory(mime)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/layout")
    add_subdirectory(layout)
endif()