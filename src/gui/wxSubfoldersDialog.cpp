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

#  include <wx/app.h>
#  include <wx/treectrl.h>
#  include <wx/statbox.h>
#  include <wx/layout.h>
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
   wxSubscriptionDialog(wxWindow *parent, MFolder *folder);
   virtual ~wxSubscriptionDialog();

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // the GUI controls
   wxTreeCtrl *m_treectrl;

   // the name of the folder whose subfolders we're enumerating
   String m_folderPath;

   // the type of the folders we're enumerating
   FolderType m_folderType;

   // to find out under which item to add a new folder we maintain a stack
   // which contains the current branch. This stack is implemented using 2
   // arrays (not terribly clever or fast, but simple)
   void PushFolder(const String& name, wxTreeItemId id)
   {
      m_folderNames.Add(name);
      m_folderIds.Add(id);
   }

   bool IsStackEmpty() const { return m_folderNames.GetCount() == 0; }

   // just discards the top stack element
   void PopFolder()
   {
      ASSERT_MSG( !IsStackEmpty(), "stack is empty!" );

      m_folderNames.Remove(m_folderNames.GetCount() - 1);
      m_folderIds.Remove(m_folderIds.GetCount() - 1);
   }

   // get the top stack element
   void GetStackTop(String *name, wxTreeItemId *id) const
   {
      ASSERT_MSG( !IsStackEmpty(), "stack is empty!" );

      *name = m_folderNames.Last();
      *id = m_folderIds.Last();
   }

   wxArrayString m_folderNames;
   wxArrayTreeItemIds m_folderIds;

   // get the tree item under which we should insert this folder (modifies the
   // folder name to contain only the last path component)
   wxTreeItemId GetParentForFolder(String *name);

   // MEventReceiver cookie for the event manager
   void *m_regCookie;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSubscriptionDialog
// ----------------------------------------------------------------------------

wxSubscriptionDialog::wxSubscriptionDialog(wxWindow *parent, MFolder *folder)
                    : wxManuallyLaidOutDialog(parent, "", "SubscribeDialog")
{
   // init members
   m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookie, "can't register with event manager");

   m_folderPath = folder->GetPath();
   m_folderType = folder->GetType();

   wxString title;
   title.Printf(_("Subfolders of folder '%s'"), folder->GetFullName().c_str());
   SetTitle(title);

   // create controls
   wxLayoutConstraints *c;
   wxStaticBox *box = CreateStdButtonsAndBox(_("Subfolders"));

   m_treectrl = new wxTreeCtrl(this, -1,
                               wxDefaultPosition, wxDefaultSize,
                              wxTR_HAS_BUTTONS |
                              wxBORDER |
                              wxTR_MULTIPLE);
   c = new wxLayoutConstraints;
   c->top.SameAs(box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   SetDefaultSize(6*hBtn, 4*wBtn);
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   MEventManager::Deregister(m_regCookie);
}

wxTreeItemId wxSubscriptionDialog::GetParentForFolder(String *name)
{
   // currently we suppose that the enumeration is depth-first, but a better
   // solution would be to be prepared for folders coming in any order - but
   // then we'd need to maintain some complicated data structure instead of a
   // simple stack we have currently...

   // if there is no root, add it now
   wxTreeItemId root = m_treectrl->GetRootItem();
   if ( !root )
   {
      // add the root item to the tree - this allows us to forget about the
      // difference between listing all folders and all folders under some
      // folder (in the first case, there is no "root" returned bycclient, in
      // the second case there would be)
      String label = m_folderPath.AfterLast('/');
      if ( !label )
      {
         // anything better to say?
         label = _T("All subfolders");
      }

      root = m_treectrl->AddRoot(label);

      PushFolder(m_folderPath, root);
   }

   while ( !IsStackEmpty() )
   {
      String folderName;
      wxTreeItemId folderId;
      GetStackTop(&folderName, &folderId);

      if ( !folderName )
      {
         // special case: comparison below would match too, but we don't have
         // to change name (thishappens only when enumerating the subfolders
         // of the root MH pseufo-folder)
         return folderId;
      }
      if ( strncmp(*name, folderName, folderName.length()) == 0 )
      {
         // we found the parent (+1 is needed to skip the '/')
         *name = name->c_str() + folderName.length() + 1;

         return folderId;
      }
      else
      {
         // no, it doesn't live under this folder - go one step up
         PopFolder();
      }
   }

   return root;
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
      // no: continue with other event handlers
      result->DecRef();

      return TRUE;
   }

   CHECK( result->GetOperation() == ASMailFolder::Op_ListFolders, FALSE,
          "unexpected operation notification" );

   // we're passed a folder specification - extract the folder name from it
   // (it's better to show this to the user rather than cryptic cclient string)
   wxString name,
            spec = ((ASMailFolder::ResultFolderExists *)result)->GetName();
   if ( MailFolderCC::SpecToFolderName(spec, m_folderType, &name) )
   {
      String nameFull(name);
      wxTreeItemId parent = GetParentForFolder(&name);
      wxTreeItemId itemId = m_treectrl->AppendItem(parent, name);

      PushFolder(nameFull, itemId);
   }
   else
   {
      wxLogDebug("Folder specification '%s' unexpected.", spec.c_str());
   }

   result->DecRef();

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

   wxString name = folder->GetPath();
   if ( !name || name.Last() != '/' )
   {
      // we don't want to get this folder itself (because it isn't returned
      // anyhow if we enumerate _all_ MH folders, but is returned in the other
      // cases - this is too confusing), so construct the pattern of the form
      // "#mh/foldername/*" which will only retrieve strict subfolders.
      name += '/';
   }

   int typeAndFlags = CombineFolderTypeAndFlags(folder->GetType(),
                                                folder->GetFlags());
   ASMailFolder *asmf = ASMailFolder::OpenFolder(typeAndFlags,
                                                 name,
                                                 profile);

   wxSubscriptionDialog dlg(GetFrame(parent), folder);

   UserData ud = &dlg;
   (void)asmf->ListFolders
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

#ifdef __WXGTK__
   wxTheApp->ProcessIdle();
#endif

   dlg.ShowModal();

   return FALSE;
}

