/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXMESSAGEVIEW_H
#define WXMESSAGEVIEW_H

#ifdef __GNUG__
#   pragma interface "wxMessageView.h"
#endif

#include "FolderType.h"
#include "MessageView.h"

#include "gui/wxlwindow.h"

#ifndef USE_PCH
#  include "gui/wxMFrame.h"

#  include <wx/dynarray.h>
#endif

#include <wx/process.h>

// ----------------------------------------------------------------------------
// wxMessageView
// ----------------------------------------------------------------------------

class wxMessageView : public MessageView
{
public:
   /** Constructor
       @param parent parent window
   */
   wxMessageView(wxWindow *parent);

   /// Destructor
   ~wxMessageView();

   /// show message
   virtual void DoShowMessage(Message *msg);

private:
   /// show the URL popup menu
   virtual void PopupURLMenu(const String& url, const wxPoint& pt);

   /// show the MIME popup menu for this message part
   virtual void PopupMIMEMenu(size_t nPart, const wxPoint& pt);
};

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

class wxMessageViewFrame : public wxMFrame
{
public:
   wxMessageViewFrame(wxWindow *parent = NULL);

   wxMessageView *GetMessageView() { return m_MessageView; }

   virtual void OnMenuCommand(int id);

   void ShowMessage(Message *msg)
      { m_MessageView->DoShowMessage(msg); }

private:
   wxMessageView *m_MessageView;

   DECLARE_DYNAMIC_CLASS(wxMessageViewFrame)
};

#endif // WXMESSAGEVIEW_H

