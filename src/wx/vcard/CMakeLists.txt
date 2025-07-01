#################################################################################
# CMakeLists.txt for Versit Library
#################################################################################
# This file builds the versit library which provides vCard and vCalendar support
# for contact and calendar data handling.

# Library sources
set(VERSIT_SOURCES
    vcc.c
    vobject.c
)

# Create the versit library
# This library can be built as either static or shared
add_library(mahogany_versit ${VERSIT_SOURCES})

# Set library properties
set_target_properties(mahogany_versit PROPERTIES
    OUTPUT_NAME versit
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(mahogany_versit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/mahogany/versit>
    PRIVATE
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

# Install the library
install(TARGETS mahogany_versit
    EXPORT MahoganyTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
file(GLOB VERSIT_HEADERS "*.h")
install(FILES ${VERSIT_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mahogany/versit
)