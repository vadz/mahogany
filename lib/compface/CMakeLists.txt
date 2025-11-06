#################################################################################
# CMakeLists.txt for Compface Library  
#################################################################################
# This file builds the compface library which provides X-Face image compression
# and decompression functionality for email headers.

# Library sources
set(COMPFACE_SOURCES
    arith.c
    compface.c  
    compress.c
    file.c
    gen.c
    uncompface.c
)

# Create the compface library
add_library(mahogany_compface STATIC ${COMPFACE_SOURCES})

# Use common C library settings
target_link_libraries(mahogany_compface PRIVATE mahogany_c_library_common)

# Include directories
target_include_directories(mahogany_compface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Force C compilation for this library
set_target_properties(mahogany_compface PROPERTIES
    LINKER_LANGUAGE C
)