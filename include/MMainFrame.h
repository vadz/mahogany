/*-*- c++ -*-********************************************************
 * MMainFrame.h : application's main frame                          *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MMAINFRAME_H
#define MMAINFRAME_H

#include	<Mcommon.h>
#include	<CommonBase.h>
#include	<MFrame.h>

/**
   MFrameBase virtual base class, defining the interface for a window

   Every window should have a unique name associated with it for use
   in the configuration file. E.g. "FolderView" or "ComposeWindow".
*/

class MainFrameBase : public MFrameBase
{
public:
   /// virtual destructor
   virtual ~MainFrameBase() {};

   CB_DECLARE_CLASS(MainFrameBase,CommonBase);
};

#ifdef	USE_WXWINDOWS
#	define	MainFrame	wxMainFrame
#	include	<wxMainFrame.h>
#endif

#endif
