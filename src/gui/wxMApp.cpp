/*-*- c++ -*-********************************************************
 * wxMApp class: do all GUI specific  application stuff             *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#include	<gui/wxMApp.h>
#include	<gui/wxMFrame.h>

wxMApp::wxMApp(void)
{
}

wxMApp::~wxMApp()
{
}

wxFrame *
wxMApp::OnInit(void)
{
   return mApplication.OnInit();
}

// This statement initialises the whole application
wxMApp mapp;

