/*-*- c++ -*-********************************************************
 * wxMAppl class: all GUI specific application stuff                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMAPP_H
#define WXMAPP_H

#define		Uses_wxApp
#include	<wx/wx.h>

#include	<MApplication.h>


/**
  * A wxWindows Application class.
  */

class wxMApp : public wxApp
{
public:
   /// Constructor
   wxMApp(void);
   /// Destructor
   ~wxMApp();
   /// initialised the application
   wxFrame	*OnInit(void);
};

#endif
