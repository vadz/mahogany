/*-*- c++ -*-********************************************************
 * MMainFrame.h : application's main frame                          *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MMAINFRAME_H
#define MMAINFRAME_H

/**
   MFrameBase virtual base class, defining the interface for a window

   Every window should have a unique name associated with it for use
   in the configuration file. E.g. "FolderView" or "ComposeWindow".
*/

class MainFrameBase : public MFrameBase
{
public:
   // while this ctor is not needed, strictly speaking, let's put it
   // to silent gcc warnings
   MainFrameBase(const String& s) : MFrameBase(s) { }
   
   /// virtual destructor
   virtual ~MainFrameBase() {};

};

#ifdef	USE_WXWINDOWS
#	define	MainFrame	wxMainFrame
#	include	"gui/wxMainFrame.h"
#endif

#endif
