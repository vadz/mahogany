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

extern "C"
{
#  define private cc__private
#  ifdef  M_LOGICAL_OP_NAMES
#     define or cc_or
#     define not cc_not
#  else  // !M_LOGICAL_OP_NAMES
#     define cc_not not
#  endif //M_LOGICAL_OP_NAMES

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

//@}

#endif  //MCCLIENT_H
