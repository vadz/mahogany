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
add_library(mahogany_dspam ${DSPAM_SOURCES})

# Set library properties
set_target_properties(mahogany_dspam PROPERTIES
    OUTPUT_NAME dspam
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(mahogany_dspam
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/mahogany/dspam>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Preprocessor definitions
target_compile_definitions(mahogany_dspam PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    HAVE_CONFIG_H
)

if(WIN32)
    target_compile_definitions(mahogany_dspam PRIVATE 
        WIN32
        _WINDLL
    )
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(mahogany_dspam PRIVATE
        /wd4996  # Disable deprecated function warnings
        /wd4244  # Disable conversion warnings
        /wd4267  # Disable size_t conversion warnings
    )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(mahogany_dspam PRIVATE
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-unused-variable
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

# Install the library
install(TARGETS mahogany_dspam
    EXPORT MahoganyTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mahogany/dspam
    FILES_MATCHING PATTERN "*.h"
)