/*-*- c++ -*-********************************************************
 * wxMLogFrame.h : class for window printing log output             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef WXMLOGFRAME_H
#define WXMLOGFRAME_H

#ifdef __GNUG__
#	pragma interface "wxMLogFrame.h"
#endif

#include	<guidef.h>
#include	<MLogFrame.h>
#include	<wxMFrame.h>

class wxMLogFrame : public MLogFrameBase, public wxMFrame
{
private:
   wxTextWindow	*tw;
public:
   /// constructor
   wxMLogFrame(void);
   /// destructor
   ~wxMLogFrame();
   /// output a line of text
   void Write(String const &str);
   /// output a line of text
   void Write(const char *str);
   void OnMenuCommand(int id);
   int OnClose(void);
};

#endif
