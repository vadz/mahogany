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
#  include <wx/statbox.h>
#  include <wx/layout.h>
#endif

#include <wx/treectrl.h>

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

   // called when [Ok] is pressed, may veto it
   virtual bool TransferDataFromWindow();

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // the GUI controls
   wxTreeCtrl *m_treectrl;

   // the name of the folder whose subfolders we're enumerating
   String m_folderPath;

   // the type of the folders we're enumerating
   FolderType m_folderType;

   // and the folder itself
   MFolder *m_folder;

   // returns the separator of the folder name components
   char GetFolderNameSeparator() const
   {
      return (m_folderType == MF_NNTP) || (m_folderType == MF_NEWS) ? '.'
                                                                    : '/';
   }

   // to find out under which item to add a new folder we maintain a stack
   // which contains the current branch. This stack is implemented using 2
   // arrays (not terribly clever or fast, but simple)
   // --------------------------------------------------------------------

   void PushFolder(const String& name, wxTreeItemId id)
   {
      m_folderNames.Add(name);
      m_folderIds.Add(id);
   }

   bool IsStackEmpty() const { return m_folderNames.GetCount() == 0; }

   void EmptyStack() { m_folderNames.Clear(); m_folderIds.Clear(); }

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

   // helper function used to populate the tree with folders
   // ------------------------------------------------------

   // get the tree item under which we should insert this folder (modifies the
   // folder name to contain only the last path component)
   wxTreeItemId GetParentForFolder(String *name);

   // called when a new folder must be added
   void OnNewFolder(String& name);

   // called when the last folder is received - will expand the top branches
   // of the tree
   void OnNoMoreFolders();

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

   m_folder = folder;
   m_folder->IncRef();

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
   m_folder->DecRef();

   MEventManager::Deregister(m_regCookie);
}

wxTreeItemId wxSubscriptionDialog::GetParentForFolder(String *name)
{
   // currently we suppose that the enumeration is depth-first, but a better
   // solution would be to be prepared for folders coming in any order - but
   // then we'd need to maintain some complicated data structure instead of a
   // simple stack we have currently...

   wxASSERT_MSG( *name != m_folderPath, "shouldn't be called for the root" );

   // if there is no root, add it now
   wxTreeItemId root = m_treectrl->GetRootItem();
   if ( !root )
   {
      // add the root item to the tree - this allows us to forget about the
      // difference between listing all folders and all folders under some
      // folder (in the first case, there is no "root" returned by cclient, in
      // the second case there would be)
      String label = m_folderPath.AfterLast(GetFolderNameSeparator());
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
         // to change name (this happens only when enumerating the subfolders
         // of the root MH pseufo-folder)
         return folderId;
      }

      // this should never happen, but check for it nevertheless
      if ( *name == folderName )
      {
         wxFAIL_MSG( "same folder listed twice in subfolders list" );

         return folderId;
      }

      // to be the child of the folderName, name must be longer than it and
      // start by folderName followed by '/' (or '.')
      size_t len = folderName.length() + 1;
      folderName += GetFolderNameSeparator();
      if ( strncmp(*name, folderName, len) == 0 )
      {
         // we found the parent, so now leave only the child part of the
         // folder name
         (*name).erase(0, len);

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

void wxSubscriptionDialog::OnNewFolder(String& name)
{
   String nameFull(name);

   // remove trailing backslashes if any
   size_t len = name.length();
   while ( len > 0 && name[len - 1] == '/' )
   {
      // 1 more char to remove
      len--;
   }

   name.Truncate(len);

   if ( name != m_folderPath )
   {
      wxTreeItemId parent = GetParentForFolder(&name);
      wxTreeItemId itemId = m_treectrl->PrependItem(parent, name);

      PushFolder(nameFull, itemId);
   }
}

void wxSubscriptionDialog::OnNoMoreFolders()
{
   // expand the root or add it if it's not there
   wxTreeItemId root = m_treectrl->GetRootItem();
   if ( root.IsOk() )
   {
      m_treectrl->Expand(root);
   }
   else
   {
      m_treectrl->AddRoot(_("No subfolders"));
      m_treectrl->Disable();
   }

   EmptyStack();
}

// needed to be able to use DECLARE_AUTOREF() macro
typedef ASMailFolder::ResultFolderExists ASFolderExistsResult;
DECLARE_AUTOPTR(ASFolderExistsResult);

bool wxSubscriptionDialog::OnMEvent(MEventData& event)
{
   // we're only subscribed to the ASFolder events
   CHECK( event.GetId() == MEventId_ASFolderResult, FALSE,
          "unexpected event type" );

   MEventASFolderResultData &data = (MEventASFolderResultData &)event;

   ASFolderExistsResult_obj result((ASFolderExistsResult *)data.GetResult());

   // is this message really for us?
   if ( result->GetUserData() != this )
   {
      // no: continue with other event handlers
      return TRUE;
   }

   if ( result->GetOperation() != ASMailFolder::Op_ListFolders )
   {
      FAIL_MSG( "unexpected operation notification" );

      // eat the event - it was for us but we didn't process it...
      return FALSE;
   }

   // is it the special event which signals that there will be no more of
   // folders?
   if ( !result->GetDelimiter() )
   {
      OnNoMoreFolders();
   }
   else
   {
      // we're passed a folder specification - extract the folder name from it
      // (it's better to show this to the user rather than cryptic cclient
      // string)
      wxString name,
               spec = result->GetName();
      if ( MailFolderCC::SpecToFolderName(spec, m_folderType, &name) )
      {
         OnNewFolder(name);
      }
      else
      {
         wxLogDebug("Folder specification '%s' unexpected.", spec.c_str());
      }
   }

   wxYield(); // FIXME: is this safe?

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

bool wxSubscriptionDialog::TransferDataFromWindow()
{
   // will be set to TRUE if we need to refresh the tree
   bool createdSomething = FALSE;

   wxTreeItemId idRoot = m_treectrl->GetRootItem();

   wxArrayTreeItemIds selections;
   size_t countSel = m_treectrl->GetSelections(selections);
   for ( size_t sel = 0; sel < countSel; sel++ )
   {
      // construct the full folder name by going upwars the tree and
      // concatenating everything
      wxArrayString components;
      wxTreeItemId id = selections[sel];
      while ( id != idRoot )
      {
         components.Add(m_treectrl->GetItemText(id));
         id = m_treectrl->GetParent(id);
      }

      size_t levelMax = components.GetCount();
      if ( !levelMax )
      {
         // doh... it was root...
         continue;
      }

      MFolder *parent = m_folder;
      parent->IncRef();

      wxString fullpath = m_folderPath;
      for ( int level = levelMax - 1; level >= 0; level-- )
      {
         wxString name = components[level];
         fullpath << GetFolderNameSeparator() << name;

         // to create a a/b/c, we must first create a, then b and then c, so
         // we create a folder during each loop iteration - unless it already
         // exists
         MFolder *folderNew = parent->GetSubfolder(name);
         if ( !folderNew )
         {
            folderNew = parent->CreateSubfolder(name, m_folderType);
            if ( !folderNew )
            {
               wxLogError(_("Failed to subscribe to '%s'."), fullpath.c_str());

               // can't create children if parent creation failed...
               break;
            }
            else
            {
               // set up the just created folder
               Profile_obj profile(folderNew->GetFullName());
               profile->writeEntry(MP_FOLDER_PATH, fullpath);

               // copy folder flags from its parent
               folderNew->SetFlags(parent->GetFlags());

               // we created a new folder, set the flag to refresh the tree
               createdSomething = TRUE;
            }
         }

         parent->DecRef();
         parent = folderNew;
      }

      parent->DecRef();
   }

   if ( createdSomething )
   {
      // generate an event notifying everybody that a new folder has been
      // created
      MEventManager::Send(
         new MEventFolderTreeChangeData(m_folder->GetFullName(),
                                        MEventFolderTreeChangeData::Create)
         );
      MEventManager::DispatchPending();
   }

   // show all errors which could have been accumulated
   wxLog::FlushActive();

   return TRUE;
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool ShowFolderSubfoldersDialog(MFolder *folder, wxWindow *parent)
{
   // The folder must be half opened because we don't really want to read any
   // messages in it, just enum subfolders
   ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folder->GetName());

   if ( !asmf )
   {
      if ( mApplication->GetLastError() != M_ERROR_CANCEL )
      {
         wxLogError(_("Impossible to browse subfolders of folder '%s' because "
                      "the folder cannot be opened."),
                    folder->GetPath().c_str());
      }
      //else: the user didn't want to open the folder (for example because it
      //      requires going online and he didn't want it)

      return FALSE;
   }

   wxSubscriptionDialog dlg(GetFrame(parent), folder);

   UserData ud = &dlg;
   (void)asmf->ListFolders
                (
                 "*",      // everything recursively
                 FALSE,    // subscribed only?
                 "",       // reference (what's this?)
                 ud        // data to pass to the callback
                );

   asmf->DecRef();

   dlg.ShowModal();

   return FALSE;
}

