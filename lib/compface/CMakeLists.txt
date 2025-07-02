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
add_library(mahogany_compface ${COMPFACE_SOURCES})

# Set library properties
set_target_properties(mahogany_compface PROPERTIES
    OUTPUT_NAME compface
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(mahogany_compface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Preprocessor definitions
target_compile_definitions(mahogany_compface PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
)

if(WIN32)
    target_compile_definitions(mahogany_compface PRIVATE WIN32)
endif()

# Compiler-specific settings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(mahogany_compface PRIVATE
        -Wno-unused-parameter
        -Wno-sign-compare
    )
endif()

# Force C compilation for this library
set_target_properties(mahogany_compface PROPERTIES
    LINKER_LANGUAGE C
)