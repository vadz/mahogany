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
#include <wx/tokenzr.h>

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

   // helper function used to populate the tree with folders
   // ------------------------------------------------------

   // insert all components of the path into the tree
   void InsertRecursively(const wxString& path);

   // insert a new item named "name" under parent if it doesn't exist yet in
   // alphabetical order; returns the id of the (new) item
   wxTreeItemId InsertInOrder(wxTreeItemId parent, const wxString& name);

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

   // don't allow to do anything with it until all foders are retrieved
   // (reenabled in OnNoMoreFolders)
   m_treectrl->Disable();

   SetDefaultSize(6*hBtn, 4*wBtn);
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   m_folder->DecRef();

   MEventManager::Deregister(m_regCookie);
}

void wxSubscriptionDialog::InsertRecursively(const String& path)
{
   // don't do anything for this case as we're not sure to get here (see
   // comment below)
   if ( path == m_folderPath )
   {
      return;
   }

   // remove the common prefix and the following it separator from name
   //
   // we suppose that all subofolder names start with the parent folder name
   // followed by the name separator, but don't blindly believe cclient -
   // check it!
   size_t len = m_folderPath.length();
   char sep = GetFolderNameSeparator();
   CHECK_RET( !wxStrncmp(path, m_folderPath, len),
              "all folder names should start with the same common prefix" );

   wxString name;
   if ( !m_folderPath )
   {
      // this happens when enumerating all MH subfolders
      name = path;
   }
   else // normal case of non empty m_folderPath
   {
      CHECK_RET( (path.length() > len) && (path[len] == sep),
                 "folder name separator expected in the folder name" );

      // +1 for trailing separator
      name = path.c_str() + m_folderPath.length() + 1;
   }

   // note: in some cases (NNTP) cclient will not return the root folder, in
   // others (MH) it will, so don't count on it here and instead add the root
   // item independently of this
   wxTreeItemId parent = m_treectrl->GetRootItem();
   if ( !parent.IsOk() )
   {
      wxString rootName = m_folderPath;
      if ( !rootName )
      {
         rootName = _("All subfolders");
      }

      parent = m_treectrl->AddRoot(rootName);
   }

   wxStringTokenizer tk(name, sep);
   while ( tk.HasMoreTokens() )
   {
      // find the child with the given name
      wxString component = tk.GetNextToken();

      ASSERT_MSG( !!component, "token can't be empty here" );

      parent = InsertInOrder(parent, component);
   }
}

wxTreeItemId wxSubscriptionDialog::InsertInOrder(wxTreeItemId parent,
                                                 const wxString& name)
{
   // insert in alphabetic order under the parent
   long cookie;
   wxTreeItemId childPrev,
                child = m_treectrl->GetFirstChild(parent, cookie);
   while ( child.IsOk() )
   {
      int rc = name.Cmp(m_treectrl->GetItemText(child));

      if ( rc == 0 )
      {
         // the item is already there
         return child;
      }
      else if ( rc < 0 )
      {
         // should insert before this item (it's the first one which is
         // greater than us)
         break;
      }
      else // if ( rc > 0 )
      {
         childPrev = child;

         child = m_treectrl->GetNextChild(parent, cookie);
      }
   }

   if ( childPrev.IsOk() )
   {
      // insert after child
      return m_treectrl->InsertItem(parent, childPrev, name);
   }
   else
   {
      // prepend if there is no previous child
      return m_treectrl->PrependItem(parent, name);
   }
}

void wxSubscriptionDialog::OnNewFolder(String& name)
{
   // remove trailing backslashes if any
   size_t len = name.length();
   while ( len > 0 && name[len - 1] == '/' )
   {
      // 1 more char to remove
      len--;
   }

   name.Truncate(len);

   // put it into the tree
   InsertRecursively(name);
}

void wxSubscriptionDialog::OnNoMoreFolders()
{
   // expand the root or add it if it's not there
   wxTreeItemId root = m_treectrl->GetRootItem();
   if ( root.IsOk() )
   {
      m_treectrl->Expand(root);
      m_treectrl->Enable();
   }
   else
   {
      m_treectrl->AddRoot(_("No subfolders"));
   }
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

