/*-*- c++ -*-********************************************************
 * wxMAppl class: all GUI specific application stuff                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 *******************************************************************/
#ifndef	WXMAPP_H
#define WXMAPP_H

#ifndef USE_PCH
#	define	Uses_wxApp
#	include	<wx/wx.h>
#	include	<MApplication.h>
#endif  //USE_PCH

/**
  * A wxWindows Application class.
  */

class wxMApp : public wxApp
{
public:
   /// Constructor
   wxMApp(void);
   /// initialise the application
#ifdef  USE_WXWINDOWS2
   virtual bool OnInit();
#else   // wxWin1
   virtual wxFrame	*OnInit(void);
#endif  // wxWin ver   
   /// Destructor
   ~wxMApp();
};

#endif
