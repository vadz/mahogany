/*-*- c++ -*-********************************************************
 * Mpch.h - precompiled header support for M                        *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$      *
 *                                                                  *
 * This reduces to Mconfig.h when no  precompiled headers are used. *
 *******************************************************************/

#ifndef   MPCH_H
#define   MPCH_H

#include        "Mconfig.h"

#ifdef	USE_PCH

// includes for c-client library

#include	<stdio.h>

extern "C"
{
#	include      <mail.h>
#	include      <osdep.h>
#	include      <rfc822.h>
#	include      <smtp.h>
#	include      <nntp.h>

  // windows.h included from osdep.h #defines all these
#	undef    GetMessage
#	undef    FindWindow
#	undef    GetCharWidth
#	undef    LoadAccelerators
}


#include        <guidef.h>
#include        <strutil.h>
#include        <appconf.h>
#include	<time.h>

#include	<Mcommon.h>
#include	<MimeList.h>
#include	<MimeTypes.h>
#include	<MFrame.h>
#include	<MLogFrame.h>
#include	<Profile.h>
#include	<MApplication.h>

#endif	// USE_PCH
#endif  //MPCH_H
