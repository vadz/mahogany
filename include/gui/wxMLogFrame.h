/*-*- c++ -*-********************************************************
 * wxMLogFrame.h : class for window printing log output             *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$            *
 *******************************************************************/

#ifndef WXMLOGFRAME_H
#define WXMLOGFRAME_H

#ifdef __GNUG__
#	pragma interface "wxMLogFrame.h"
#endif

#ifndef	USE_PCH
#	include	"MFrame.h"
#	include "gui/wxMFrame.h"
#	include	"MLogFrame.h"
#endif

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

   ON_CLOSE_TYPE OnClose(void);
};

#endif
