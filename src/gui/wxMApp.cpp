/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#include	"Mpch.h"
#include	"Mcommon.h"

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

#include	"gui/wxMApp.h"
#include	"gui/wxMFrame.h"

wxMApp::wxMApp(void)
{
}

wxMApp::~wxMApp()
{
}

#ifdef  USE_WXWINDOWS2

bool wxMApp::OnInit()
{
  SetTopWindow(mApplication.OnInit());

  return true;
}

// This statement initialises the whole application
IMPLEMENT_APP(wxMApp);

#else   // wxWin1

wxFrame *wxMApp::OnInit(void)
{
   return mApplication.OnInit();
}

// This statement initialises the whole application
wxMApp mapp;

#endif  // wxWin ver

