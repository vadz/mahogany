/*-*- c++ -*-********************************************************
 * wxMAppl class: all GUI specific application stuff                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMAPP_H
#define WXMAPP_H

#if !USE_PCH
  #define		Uses_wxApp
  #include	<wx/wx.h>

  #include	<MApplication.h>
#endif  //USE_PCH

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
   /// initialise the application
#ifdef  USE_WXWINDOWS2
   virtual bool OnInit();
#else   // wxWin1
   virtual wxFrame	*OnInit(void);
#endif  // wxWin ver
};

#endif
