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

#ifndef USE_PCH
#  include "gui/wxMFrame.h"
#  include "FolderType.h"

#  include <wx/dynarray.h>
#endif // USE_PCH

#include <wx/process.h>

#include "MessageView.h"

class FolderView;
class MsgCmdProc;

// ----------------------------------------------------------------------------
// wxMessageView
// ----------------------------------------------------------------------------

class wxMessageView : public MessageView
{
public:
   /**
      Constructor

      @param parent parent window
      @param folderView associated folder view (may be NULL if standalone)
      @param profile our profile (may be NULL if not standalone)
   */
   wxMessageView(wxWindow *parent,
                 FolderView *folderView = NULL,
                 Profile *profile = NULL);

   /// Destructor
   ~wxMessageView();

   /// show message
   virtual void DoShowMessage(Message *msg);

   /// update the GUI to show the new viewer window
   virtual void OnViewerChange(const MessageViewer *viewerOld,
                               const MessageViewer *viewerNew);

   /// create the "View" menu for our parent frame
   virtual void CreateViewMenu();
;
   virtual void OnToggleViewFilter(int id);
   virtual void OnSelectViewer(int id);

protected:
   virtual MessageViewer *CreateDefaultViewer() const;
   virtual void OnShowHeadersChange();

private:
   /// the associated folder view, if any
   FolderView *m_FolderView;

   /// the array containing the names of all the existing filters
   wxArrayString m_namesFilters;

   /// and another one containing their state (on/off)
   wxArrayInt m_statesFilters;

   /// the array containing the names of all the existing viewers
   wxArrayString m_namesViewers;
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

   DECLARE_NO_COPY_CLASS(wxMessageViewFrame)
};

#endif // WXMESSAGEVIEW_H

