/*-*- c++ -*-********************************************************
 * Mc-client.h - c-client header inclusion for Mahogany             *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (Ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#ifndef   MCCLIENT_H
#define   MCCLIENT_H

// MSVS 2022 requires this.
#ifdef CC_MSC
   #define M_LOGICAL_OP_NAMES
#endif

extern "C"
{
#  define private cc__private
#  ifdef  M_LOGICAL_OP_NAMES
#     define or cc_or
#     define not cc_not
#  else  // !M_LOGICAL_OP_NAMES
#     define cc_not not
#  endif //M_LOGICAL_OP_NAMES

#  ifdef OS_WIN
      // if windows.h had been already included it defined ERROR which c-client
      // redefines, avoid warnings about it (we don't really care about Windows
      // constant, we don't use it)
   #undef ERROR
#endif // OS_WIN

#  include <stdio.h>
#  include <mail.h>
#  include <osdep.h>
#  include <rfc822.h>
#  include <smtp.h>
#  include <nntp.h>
#  include <misc.h>
#  undef private

#  ifdef    M_LOGICAL_OP_NAMES
#     undef or
#     undef not
#  endif  //M_LOGICAL_OP_NAMES

   // stupid c-client lib redefines utime() in an incompatible way
#  undef utime

   // and it also thinks it's ok to redefine a symbol as common as "write"!
#  undef write

   //  this is defined as a struct while we use it as a MimeType enum value
#  undef MESSAGE

   // windows.h included from osdep.h under Windows #defines many symbols which
   // conflict with our ones
#  ifdef OS_WIN
      // including this header will #undef most of the harm done
#     include <wx/msw/winundef.h>

      // but not all
#     undef   SendMessage
#  endif // OS_WIN

   // misc.h redefines min/max as Min/Max breaking standard headers compilation
#  undef max
#  undef min

   // finally it also defines the name commonly used as C++ template parameter!
#  undef T
}

/**
   @name Helper functions for working with c-client.

   All of these functions are defined in MessageCC.cpp for historical reasons.
 */
//@{

/**
   Parse the complete message text into internal envelope and body structures.

   @param msgText message text in Unix (LF-only) line ending convention
   @param ppEnv filled with message ENVELOPE
   @param ppBody filled with message BODY
   @param pHdrLen filled with the length of the header if non-NULL
   @return true if ok, false on error
 */
extern bool
CclientParseMessage(const char *msgText,
                    ENVELOPE **ppEnv,
                    BODY **ppBody,
                    size_t *pHdrLen = NULL);


/**
   A helper macro to cast away constness from strings passed to c-client.

   C-client functions are not const correct and using them with literal strings
   results in gcc warnings about "deprecated conversion from string constant to
   char*" so use this function to suppress it. It also has the advantage of
   working with wxCharBuffer, i.e. can be used as @code CONST_CCAST(s.mb_str())
   @endcode with Unicode build of wx in which mb_str() doesn't return a char*
   directly.

   Hopefully one day c-client will be updated to work with const char strings
   making all this stuff unnecessary, removing it will be then simple as you
   could just grep for CONST_CCAST instead of examining all casts in the
   sources.
 */
inline char *CONST_CCAST(const char *p)
{
   return const_cast<char *>(p);
}

/**
   Cast a signed char string to unsigned char.

   c-client wants unsigned char strings while we work with char pointers
   everywhere, provide this macro-like helper function to cast the strings
   to the correct type (this has the added benefit of forcing implicit
   conversion of wxString::char_str() to "char *", i.e. we can write
   UCHAR_CAST(s.char_str()) while (unsigned char *)s.char_str() wouldn't
   compile)
 */
inline unsigned char *UCHAR_CAST(char *s)
{
   return reinterpret_cast<unsigned char *>(s);
}

/**
   Cast an unsigned char string to a char one.

   This is the reverse of UCHAR_CAST() and is useful for the strings returned
   from c-client.
 */
inline char *CHAR_CAST(unsigned char *us)
{
   return reinterpret_cast<char *>(us);
}

//@}

#endif  //MCCLIENT_H
