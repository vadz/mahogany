///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wxSubfoldersDialog.cpp - implementation of functions from
//              MFolderDialogs.h
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "strutil.h"

#  include <wx/listbox.h>
#  include <wx/statbox.h>
#endif

#include "MFolder.h"

#include "ASMailFolder.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxSubscriptionDialog : public wxManuallyLaidOutDialog,
                             public MEventReceiver
{
public:
   wxSubscriptionDialog(wxWindow *parent, const wxString& title);
   virtual ~wxSubscriptionDialog();

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // the GUI controls
   wxListBox *m_listbox;

   // MEventReceiver cookie for the event manager
   void *m_regCookie;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSubscriptionDialog
// ----------------------------------------------------------------------------

wxSubscriptionDialog::wxSubscriptionDialog(wxWindow *parent,
                                           const wxString& title)
                    : wxManuallyLaidOutDialog(parent, title, "SubscribeDialog")
{
   m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookie, "can't register with event manager");

   // create controls
   wxLayoutConstraints *c;
   wxStaticBox *box = CreateStdButtonsAndBox(_("Subfolders"));

   m_listbox = new wxListBox(this, -1);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 2*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_listbox->SetConstraints(c);

   SetDefaultSize(6*hBtn, 4*wBtn);
   Centre();
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   MEventManager::Deregister(m_regCookie);
}

bool wxSubscriptionDialog::OnMEvent(MEventData& event)
{
   // we're only subscribed to the ASFolder events
   CHECK( event.GetId() == MEventId_ASFolderResult, FALSE,
          "unexpected event type" );

   ASMailFolder::Result *result = ((MEventASFolderResultData &)event).GetResult();

   // is this message really for us?
   if ( result->GetUserData() != this )
   {
      // no: conitnue with other event handlers
      return TRUE;
   }

   CHECK( result->GetOperation() == ASMailFolder::Op_ListFolders, FALSE,
          "unexpected operation notification" );

   m_listbox->Append(((ASMailFolder::ResultFolderExists *)result)->GetName());

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool ShowFolderSubfoldersDialog(MFolder *folder, wxWindow *parent)
{
   Profile_obj profile(folder->GetFullName());
   CHECK( profile, FALSE, "can't create profile" );

   String name = READ_CONFIG(profile, MP_FOLDER_PATH);
   CHECK( !!name, FALSE, "empty folder path?" );

   while ( name.Last() == '/' )
   {
      // shouldn't end with backslash because otherwise mail_list won't return
      // the root folder itself (but we want it)
      name.Truncate(name.Length() - 1);
   }

   int typeAndFlags = CombineFolderTypeAndFlags(folder->GetType(),
                                                folder->GetFlags());
   ASMailFolder *asmf = ASMailFolder::OpenFolder(typeAndFlags,
                                                 name,
                                                 profile);

   wxString title;
   title.Printf(_("Subfolders of folder '%s'"), folder->GetFullName().c_str()); 
   wxSubscriptionDialog dlg(parent, title);

   UserData ud = &dlg;
   Ticket ticket = asmf->ListFolders
                         (
                          "",       // host
                          MF_MH,    // folder type
                          name,     // start folder name
                          "*",      // everything recursively
                          FALSE,    // subscribed only?
                          "",       // reference (what's this?)
                          ud
                         );

   asmf->DecRef();

   dlg.ShowModal();

   return FALSE;
}

