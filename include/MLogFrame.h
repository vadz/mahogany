/*-*- c++ -*-********************************************************
 * MLogFrame.h : baseclass for window printing log output           *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:36  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MLOGFRAME_H
#define MLOGFRAME_H

/**
   MLogFrameBase virtual base class, defining the interface for a
   window printing log information.
*/

class MLogFrameBase
{   
public:
   /// virtual destructor
   virtual ~MLogFrameBase() {};
   /// output a line of text
   virtual void Write(String const &str) = 0;
   /// output a line of text
   virtual void Write(const char *str) = 0;
};

#if USE_WXWINDOWS
#	include	"gui/wxMLogFrame.h"
#	define	MLogFrame	wxMLogFrame
#else
#	error "No MLogFrame class defined!"
#endif

#endif
