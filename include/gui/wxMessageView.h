///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   wxMessageView.h: wxMessageView and wxMessageViewFrame
// Purpose:     wxMessageView adds missing GUI bits and pieces to MessageView:
//              it shows the popup menu when needed &c
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef WXMESSAGEVIEW_H
#define WXMESSAGEVIEW_H

#ifdef __GNUG__
#   pragma interface "wxMessageView.h"
#endif

class FolderView;
class MsgCmdProc;

#include "FolderType.h"
#include "MessageView.h"

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
   wxMessageView(wxWindow *parent, FolderView *folderView = NULL);

   /// Destructor
   ~wxMessageView();

   /// show message
   virtual void DoShowMessage(Message *msg);

   /// update the GUI to show the new viewer window
   virtual void OnViewerChange(const MessageViewer *viewerOld,
                               const MessageViewer *viewerNew);

   /// show the URL popup menu
   virtual void PopupURLMenu(wxWindow *window,
                             const String& url,
                             const wxPoint& pt,
                             URLKind urlkind);

   /// show the MIME popup menu for this message part
   virtual void PopupMIMEMenu(wxWindow *window,
                              const MimePart *part,
                              const wxPoint& pt);

protected:
   virtual MessageViewer *CreateDefaultViewer() const;

private:
   FolderView *m_FolderView;
};

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

class wxMessageViewFrame : public wxMFrame
{
public:
   wxMessageViewFrame(wxWindow *parent, ASMailFolder *asmf, UIdType uid);
   virtual ~wxMessageViewFrame();

   MessageView *GetMessageView() { return m_MessageView; }

   virtual void OnMenuCommand(int id);

private:
   MessageView *m_MessageView;

   MsgCmdProc *m_msgCmdProc;
};

#endif // WXMESSAGEVIEW_H

