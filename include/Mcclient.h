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

   // windows.h included from osdep.h under Windows #defines all these
#  undef   CreateDialog
#  undef   DrawText
#  undef   FindWindow
#  undef   GetCharWidth
#  undef   GetClassInfo
#  undef   GetMessage
#  undef   LoadAccelerators
#  undef   SendMessage
#  undef   StartDoc

   // finally it also defines the name commonly used as C++ template parameter!
#  undef T
}

#endif  //MCCLIENT_H
