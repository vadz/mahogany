/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/

#include	"Mpch.h"

#ifndef	USE_PCH
#	include	"Mcommon.h"
#	include "guidef.h"
#	include	"MFrame.h"
#	include	"gui/wxMFrame.h"
#	include	"MLogFrame.h"

#	include	"PathFinder.h"
#	include	"MimeList.h"
#	include	"MimeTypes.h"
#	include "appconf.h"
#	include	"Profile.h"

#	include  "MApplication.h"

#	include	"gui/wxMApp.h"
#endif

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

wxFrame *
wxMApp::OnInit(void)
{
   return mApplication.OnInit();
}

// This statement initialises the whole application
wxMApp mapp;

#endif  // wxWin ver

