/*-*- c++ -*-********************************************************
 * MMainFrame.h : application's main frame                          *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/06/05 16:56:41  VZ
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.2  1998/03/26 23:05:36  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
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

   CB_DECLARE_CLASS(MainFrameBase,CommonBase);
};

#ifdef	USE_WXWINDOWS
#	define	MainFrame	wxMainFrame
#	include	"gui/wxMainFrame.h"
#endif

#endif
