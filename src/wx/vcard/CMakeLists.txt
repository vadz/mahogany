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

# Use common C library settings
target_link_libraries(mahogany_versit PRIVATE mahogany_c_library_common)

# Include directories
target_include_directories(mahogany_versit
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Force C compilation for this library  
set_target_properties(mahogany_versit PROPERTIES
    LINKER_LANGUAGE C
)