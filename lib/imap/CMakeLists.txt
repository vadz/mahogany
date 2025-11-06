#################################################################################
# CMakeLists.txt for IMAP Library
#################################################################################
# This file builds the IMAP client library which provides email protocol support
# for IMAP4, POP3, SMTP, NNTP and other mail protocols.

# Library sources from c-client (core functionality)
set(IMAP_C_CLIENT_SOURCES
    src/c-client/flstring.c
    src/c-client/misc.c
    src/c-client/netmsg.c
    src/c-client/newsrc.c
    src/c-client/rfc822.c
    src/c-client/utf8.c
    src/c-client/utf8aux.c
    src/c-client/imap4r1.c
    src/c-client/mail.c
    src/c-client/nntp.c
    src/c-client/pop3.c
    src/c-client/smanager.c
    src/c-client/smtp.c
)

# Platform-specific OS sources
if(WIN32)
    set(IMAP_OSDEP_SOURCES
        src/osdep/nt/dummynt.c
        src/osdep/nt/fdstring.c
        src/osdep/nt/mbxnt.c
        src/osdep/nt/mhnt.c
        src/osdep/nt/mtxnt.c
        src/osdep/nt/os_nt.c
        src/osdep/nt/pseudo.c
        src/osdep/nt/tenexnt.c
        src/osdep/nt/unixnt.c
    )
    set(IMAP_OSDEP_INCLUDE src/osdep/nt)
elseif(UNIX)
    # For Unix systems - using generic Unix sources
    # Note: This is a simplified version - a complete implementation would
    # detect the specific Unix variant and use appropriate sources
    set(IMAP_OSDEP_SOURCES
        # Unix OSDEP sources would go here when properly configured
        # For now, we'll create minimal stubs
    )
    set(IMAP_OSDEP_INCLUDE src/osdep/unix)
    
    # Create basic headers for Unix builds
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/linkage.h" "
/* Minimal linkage.h for Unix builds */
extern DRIVER imapdriver;
extern DRIVER nntpdriver;
extern DRIVER pop3driver;
extern DRIVER dummydriver;
#define CREATEPROTO dummyproto
#define APPENDPROTO dummyproto
extern AUTHENTICATOR auth_log;
")
    
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/osdep.h" "
/* Minimal osdep.h for Unix builds */
#ifndef OSDEP_H
#define OSDEP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

/* Basic type definitions */
typedef unsigned long ulong;
typedef unsigned int uint;

/* Minimal function declarations */
#define NIL 0
#define T 1

/* Disable some features that need full OS support */
#define DISABLE_POP
#define DISABLE_IMAP
#define DISABLE_SMTP
#define DISABLE_NNTP

#endif /* OSDEP_H */
")
    
    # Add the binary directory to include path for generated headers
    list(APPEND IMAP_OSDEP_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# Create the IMAP library
add_library(mahogany_imap STATIC
    ${IMAP_C_CLIENT_SOURCES}
    ${IMAP_OSDEP_SOURCES}
)

# Use common C library settings
target_link_libraries(mahogany_imap PRIVATE mahogany_c_library_common)

# Include directories
target_include_directories(mahogany_imap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/c-client
        ${CMAKE_CURRENT_SOURCE_DIR}/src/charset
)

# Add platform-specific include directories
foreach(dir ${IMAP_OSDEP_INCLUDE})
    if(IS_ABSOLUTE "${dir}")
        target_include_directories(mahogany_imap PRIVATE "${dir}")
    else()
        target_include_directories(mahogany_imap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
    endif()
endforeach()

# Library-specific preprocessor definitions
target_compile_definitions(mahogany_imap PRIVATE
    CHUNKSIZE=65536
)

# Force C compilation for this library
set_target_properties(mahogany_imap PROPERTIES
    LINKER_LANGUAGE C
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(mahogany_imap PRIVATE ws2_32)
endif()

if(ENABLE_SSL AND OpenSSL_FOUND)
    target_link_libraries(mahogany_imap PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(mahogany_imap PRIVATE HAVE_SSL)
endif()