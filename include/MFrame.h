/*-*- c++ -*-********************************************************
 * MFrame.h : GUI independent frame/window representation           *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
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

#ifndef MFRAME_H
#define MFRAME_H

//#ifdef __GNUG__
//#pragma interface "MFrame.h"
//#endif

/**
   MFrameBase virtual base class, defining the interface for a window.
   Every window should have a unique name associated with it for use
   in the configuration file. E.g. "FolderView" or "ComposeWindow".
*/

class MFrameBase
{   
private:
   /// used to set the name of the window class
   virtual void	SetName(String const & name = String("MFrame")) = 0;
public:
   /// virtual destructor
   virtual ~MFrameBase() {};

   /// used to set the title of the window class
   virtual void	SetTitle(String const & name = String("M")) = 0;

#ifndef USE_WXWINDOWS2    // wxMFrame shouldn't have 2 virtual Show()
   /// make it visible or invisible
   virtual void Show(bool visible = true) = 0;
#endif  // wxWin 2
};

#ifdef	USE_WXWINDOWS
#	define	MFrame	wxMFrame
#	include	"gui/wxMFrame.h"
#endif

#endif
