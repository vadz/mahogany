#################################################################################
# CMakeLists.txt for Mahogany Tests
#################################################################################
# This file builds test programs for the Mahogany libraries and components.

# Only build tests if testing is enabled
if(NOT BUILD_TESTING)
    return()
endif()

# Basic library tests
add_executable(test_imap_basic test_imap_basic.c)
target_link_libraries(test_imap_basic mahogany_imap)
target_include_directories(test_imap_basic PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/imap/src/c-client
)
add_test(NAME test_imap_basic COMMAND test_imap_basic)

# Check existing test directories
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mime")
    add_subdirectory(mime)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/layout")
    add_subdirectory(layout)
endif()