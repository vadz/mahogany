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
#   define private cc__private
#   define or cc_or
#   define not cc_not
#     include <stdio.h>
#     include <mail.h>
#     include <osdep.h>
#     include <rfc822.h>
#     include <smtp.h>
#     include <nntp.h>
#     include <misc.h>
#   undef private   
#   undef or
#   undef not
// stupid c-client lib redefines utime() in an incompatible way
#   undef utime
}

#endif  //MCCLIENT_H
