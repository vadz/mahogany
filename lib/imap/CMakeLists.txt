#################################################################################
# CMakeLists.txt for IMAP Library
#################################################################################

# Build IMAP library for Windows or Linux (other platforms supported by
# c-client are not currently supported).

add_library(mahogany_imap STATIC)

set(DRIVERS
  mbox
  imap
  nntp
  pop3
  mix
  mx
  mbx
  tenex
  mtx
  mh
  mmdf
  unix
  news
  phile
  dummy
)

set(AUTHENTICATORS
  ext
  md5
  pla
  log
)

# Library sources from c-client (core functionality)
set(IMAP_C_CLIENT_SOURCES
    src/c-client/flstring.c
    src/c-client/misc.c
    src/c-client/netmsg.c
    src/c-client/newsrc.c
    src/c-client/rfc822.c
    src/c-client/utf8.c
    src/c-client/utf8aux.c
    src/c-client/mail.c
    src/c-client/smanager.c
    src/c-client/smtp.c
)

# Platform-specific OS sources and generated files.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(IMAP_OS "unix")

  set(IMAP_OSDEP_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/osdep.h
    ${CMAKE_CURRENT_BINARY_DIR}/osdep.c
    src/osdep/unix/fdstring.c
    src/osdep/unix/pseudo.c
    src/osdep/unix/sig_psx.c
  )

  file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/unix/os_slx.h
    ${CMAKE_CURRENT_BINARY_DIR}/osdep.h
    SYMBOLIC
  )

  set(OSDEP_C_CONTENTS "/* Generated osdep.c for ${CMAKE_SYSTEM_NAME} builds */\n")
  string(APPEND OSDEP_C_CONTENTS "#include \"os_slx.c\"\n")
  string(APPEND OSDEP_C_CONTENTS "#include \"log_std.c\"\n")
  string(APPEND OSDEP_C_CONTENTS "#include \"ckp_psx.c\"\n")
  if(USE_SSL)
    string(APPEND OSDEP_C_CONTENTS "#include \"ssl_unix.c\"\n")

    # Linking with OpenSSL::SSL is not enough because c-client code includes
    # OpenSSL headers without "openssl/" prefix, so we need to explicitly add
    # this directory to the include path to make it compile.
    set_property(
      SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/osdep.c
      APPEND PROPERTY
        COMPILE_DEFINITIONS
          SSL_CERT_DIRECTORY="/etc/ssl/certs"
          SSL_KEY_DIRECTORY="/etc/ssl/private"
    )
    set_property(
      SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/osdep.c
      APPEND PROPERTY
        INCLUDE_DIRECTORIES
          ${OPENSSL_INCLUDE_DIR}/openssl
    )
  else()
    string(APPEND OSDEP_C_CONTENTS "#include \"ssl_none.c\"\n")
  endif()

  file(GENERATE
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/osdep.c
    CONTENT
      ${OSDEP_C_CONTENTS}
  )

  file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/unix/crx_nfs.c
    ${CMAKE_CURRENT_BINARY_DIR}/crexcl.c
    SYMBOLIC
  )

  file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/unix/ip4_unix.c
    ${CMAKE_CURRENT_BINARY_DIR}/ip_unix.c
    SYMBOLIC
  )

  foreach(driver IN LISTS DRIVERS)
    # Most drivers live in files with the same name under c-client directory,
    # but there are a couple of exceptions.
    if(driver STREQUAL "imap")
      set(driver_file "c-client/imap4r1.c")
    elseif(driver STREQUAL "mbox")
      set(driver_file "osdep/unix/unix.c")
    else()
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/c-client/${driver}.c")
        set(driver_file "c-client/${driver}.c")
      else()
        set(driver_file "osdep/unix/${driver}.c")
      endif()
    endif()

    list(APPEND IMAP_C_CLIENT_SOURCES src/${driver_file})
  endforeach()

  # All drivers are enabled on Unix.
  set(ENABLED_DRIVERS ${DRIVERS})

  # Additional Unix-specific definitions.
  target_compile_definitions(mahogany_imap PRIVATE
    ACTIVEFILE="/var/lib/news/active"
    ANONYMOUSHOME="/var/spool/mail/anonymous"
    CREATEPROTO=unixproto
    EMPTYPROTO=unixproto
    LOCKPGM=""
    LOCKPGM1="/usr/libexec/mlock"
    LOCKPGM2="/usr/sbin/mlock"
    LOCKPGM3="/etc/mlock"
    MAILSPOOL="/var/spool/mail"
    MD5ENABLE="/etc/cram-md5.pwd"
    NEWSSPOOL="/var/spool/news"
    RSHPATH="/usr/bin/rsh"
    SPOOLDIR="/var/spool"
  )

  # Disable some warnings specific to c-client code, there are just too many of
  # them to fix (but ideally they should be fixed, of course).
  target_compile_options(mahogany_imap PRIVATE
    -Wno-deprecated-declarations
    -Wno-format
    -Wno-format-overflow
    -Wno-unused-result
  )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(IMAP_OS "nt")

  set(IMAP_OSDEP_SOURCES
    src/osdep/nt/fdstring.c
    src/osdep/nt/os_nt.c
    src/osdep/nt/pseudo.c
  )

  file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/nt/os_nt.h
    ${CMAKE_CURRENT_BINARY_DIR}/osdep.h
    SYMBOLIC
  )

  file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/nt/ip4_nt.c
    ${CMAKE_CURRENT_BINARY_DIR}/ip_nt.c
    SYMBOLIC
  )

  foreach(driver IN LISTS DRIVERS)
    if(driver STREQUAL "imap")
      set(driver_file "c-client/imap4r1.c")
    else()
      if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/c-client/${driver}.c")
        set(driver_file "c-client/${driver}.c")
      elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/nt/${driver}nt.c")
        set(driver_file "osdep/nt/${driver}nt.c")
      else()
        # Some drivers are not available on Windows. Skip them.
        continue()
      endif()
    endif()

    list(APPEND ENABLED_DRIVERS ${driver})
    list(APPEND IMAP_C_CLIENT_SOURCES src/${driver_file})
  endforeach()

  if(MSVC)
    target_compile_options(mahogany_imap PRIVATE
      /wd4311   # pointer truncation from 'type1' to 'type2'
      /wd4312   # conversion from 'type1' to 'type2' of greater size
    )
  endif()
else()
  # See src/osdep/unix/Makefile for how to do it.
  message(FATAL_ERROR "Please add support for ${CMAKE_SYSTEM_NAME}.")
endif()

set(LINKAGE_H_CONTENTS "/* Generated linkage.h for ${CMAKE_SYSTEM_NAME} builds */\n")
set(LINKAGE_C_CONTENTS "/* Generated linkage.c for ${CMAKE_SYSTEM_NAME} builds */\n")

foreach(driver IN LISTS ENABLED_DRIVERS)
  string(APPEND LINKAGE_H_CONTENTS
    "extern DRIVER ${driver}driver\;\n"
  )
  string(APPEND LINKAGE_C_CONTENTS
    "  mail_link (&${driver}driver)\;\t\t/* link in the ${driver} driver */\n"
  )
endforeach()

set(AUTHS_C_CONTENTS "/* Generated auths.c for ${CMAKE_SYSTEM_NAME} builds */\n")
foreach(authenticator IN LISTS AUTHENTICATORS)
  string(APPEND AUTHS_C_CONTENTS
    "#include \"auth_${authenticator}.c\"\n"
  )

  string(APPEND LINKAGE_H_CONTENTS
    "extern AUTHENTICATOR auth_${authenticator}\;\n"
  )
  string(APPEND LINKAGE_C_CONTENTS
    "  auth_link (&auth_${authenticator})\;\t\t/* link in the ${authenticator} authenticator */\n"
  )
endforeach()

file(GENERATE
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/linkage.h"
  CONTENT
    ${LINKAGE_H_CONTENTS}
)
file(GENERATE
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/linkage.c"
  CONTENT
    ${LINKAGE_C_CONTENTS}
)
file(GENERATE
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/auths.c"
  CONTENT
    ${AUTHS_C_CONTENTS}
)

target_sources(mahogany_imap PRIVATE
  ${IMAP_C_CLIENT_SOURCES}
  ${IMAP_OSDEP_SOURCES}
)

# Use common C library settings
target_link_libraries(mahogany_imap PRIVATE mahogany_c_library_common)

# Include directories
target_include_directories(mahogany_imap
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/osdep/${IMAP_OS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/c-client
    ${CMAKE_CURRENT_SOURCE_DIR}/src/charset
)

# Library-specific preprocessor definitions
target_compile_definitions(mahogany_imap PRIVATE
    CHUNKSIZE=65536
)

# Platform-specific linking
if(WIN32)
  target_link_libraries(mahogany_imap INTERFACE ws2_32)
elseif(USE_SSL)
  target_link_libraries(mahogany_imap INTERFACE OpenSSL::SSL OpenSSL::Crypto crypt)
endif()
