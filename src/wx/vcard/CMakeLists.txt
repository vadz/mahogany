#################################################################################
# CMakeLists.txt for Versit Library
#################################################################################
# This file builds the versit library which provides vCard and vCalendar support
# for contact and calendar data handling.

# Library sources
set(VERSIT_SOURCES
    vobject.c
)

# Check if we have vcc.c or need to use pre-compiled version
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcc.c")
    list(APPEND VERSIT_SOURCES vcc.c)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcc.c-yacc")
    # Copy pre-compiled yacc file
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/vcc.c-yacc"
        "${CMAKE_CURRENT_BINARY_DIR}/vcc.c"
        COPYONLY
    )
    list(APPEND VERSIT_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/vcc.c")
else()
    # Try to generate from yacc if available
    find_program(YACC_EXECUTABLE NAMES yacc bison)
    if(YACC_EXECUTABLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcc.y")
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/vcc.c"
            COMMAND ${YACC_EXECUTABLE} -d "${CMAKE_CURRENT_SOURCE_DIR}/vcc.y"
            COMMAND ${CMAKE_COMMAND} -E copy y.tab.c "${CMAKE_CURRENT_BINARY_DIR}/vcc.c"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vcc.y"
            COMMENT "Generating vcc.c from vcc.y"
        )
        list(APPEND VERSIT_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/vcc.c")
    else()
        message(FATAL_ERROR "Cannot find vcc.c source file. Please ensure vcc.c, vcc.c-yacc, or yacc/bison is available.")
    endif()
endif()

# Create the versit library
add_library(mahogany_versit STATIC ${VERSIT_SOURCES})

# Include directories
target_include_directories(mahogany_versit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Preprocessor definitions
target_compile_definitions(mahogany_versit PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
)

if(WIN32)
    target_compile_definitions(mahogany_versit PRIVATE WIN32)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(mahogany_versit PRIVATE _WINDLL)
    endif()
endif()

# Compiler-specific settings
if(MSVC)
    target_compile_options(mahogany_versit PRIVATE
        /wd4996  # Disable deprecated function warnings
        /wd4244  # Disable conversion warnings
    )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(mahogany_versit PRIVATE
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-parentheses
    )
endif()

# Force C compilation for this library  
set_target_properties(mahogany_versit PROPERTIES
    LINKER_LANGUAGE C
)