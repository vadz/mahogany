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
#  pragma interface "wxMLogFrame.h"
#endif

#ifndef  USE_PCH
#  include "MFrame.h"
#  include "gui/wxMFrame.h"
#  include "MLogFrame.h"
#endif

// fwd decl
class wxLog;

class wxMLogFrame : public wxMFrame, public MLogFrameBase
{
friend class MLog;
private:
   // text control where we redirect the log information
   wxTextWindow *tw;

#ifdef USE_WXWINDOWS2
   wxLog *m_pLogSink, *m_pOldLogSink;

public:
   wxLog *GetLog() const { return m_pLogSink; }
#endif // wxWin2

public:
   /// constructor
   wxMLogFrame();
   /// destructor
   ~wxMLogFrame();

   // implement base class's virtual functions
   
   /// show/hide log window
   virtual void ShowLog(bool bShow = true)
      { wxMFrame::Show(bShow); }
   /// clear the log contents
   virtual void Clear();
   /// save the log contents to a file
   virtual bool Save(const char *filename = NULL);

#ifndef USE_WXWINDOWS2
   /// output a line of text
   virtual void Write(const char *str);
#endif // wxWin1

   // callbacks
   void          OnMenuCommand(int id);
   ON_CLOSE_TYPE OnClose();

#ifdef  USE_WXWINDOWS2
   void OnLogClose(wxCommandEvent&) { OnMenuCommand(WXMENU_FILE_CLOSE); }

   DECLARE_EVENT_TABLE()
#endif //wxWin2
};

#endif
