#################################################################################
# CMakeLists.txt for DSPAM Library
#################################################################################
# This file builds the DSPAM spam filtering library which provides statistical
# spam detection and filtering capabilities.

# Library sources
set(DSPAM_SOURCES
    src/base64.c
    src/bnr.c
    src/buffer.c
    src/config_shared.c
    src/decode.c
    src/diction.c
    src/error.c
    src/hash.c
    src/hash_drv.c
    src/heap.c
    src/libdspam.c
    src/list.c
    src/nodetree.c
    src/pref.c
    src/read_config.c
    src/tokenizer.c
    src/util.c
)

# Create the DSPAM library
add_library(mahogany_dspam STATIC ${DSPAM_SOURCES})

# Use common C library settings
target_link_libraries(mahogany_dspam PRIVATE mahogany_c_library_common)

# Include directories
target_include_directories(mahogany_dspam
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Handle auto-config.h generation
if(WIN32)
    # On Windows, copy the existing Windows config file
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/auto-config.h.win32")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/auto-config.h.win32" 
                      "${CMAKE_CURRENT_BINARY_DIR}/auto-config.h" COPYONLY)
    else()
        # Fallback: create basic Windows config
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/auto-config.h" "
/* Basic Windows configuration for DSPAM library */
#ifndef AUTO_CONFIG_H
#define AUTO_CONFIG_H

#define HAVE_CONFIG_H
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

#endif /* AUTO_CONFIG_H */
")
    endif()
    target_include_directories(mahogany_dspam PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
elseif(UNIX)
    # On Unix, generate from template if available
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/auto-config.h.in")
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/auto-config.h.in" 
                      "${CMAKE_CURRENT_BINARY_DIR}/auto-config.h" @ONLY)
    else()
        # Fallback: create basic Unix config
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/auto-config.h" "
/* Basic Unix configuration for DSPAM library */
#ifndef AUTO_CONFIG_H
#define AUTO_CONFIG_H

#define HAVE_CONFIG_H
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_UNISTD_H 1

#endif /* AUTO_CONFIG_H */
")
    endif()
    target_include_directories(mahogany_dspam PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(WIN32)
    target_compile_definitions(mahogany_dspam PRIVATE 
        _WINDLL
    )
endif()

# Force C compilation for this library
set_target_properties(mahogany_dspam PROPERTIES
    LINKER_LANGUAGE C
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(mahogany_dspam PRIVATE ws2_32)
elseif(UNIX)
    target_link_libraries(mahogany_dspam PRIVATE m)
endif()