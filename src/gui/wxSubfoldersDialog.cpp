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
#  include <wx/stattext.h>
#  include <wx/textctrl.h>
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

   // callbacks
   // ---------

   // event processing function
   virtual bool OnMEvent(MEventData& event);

   // item selected in the tree
   void OnTreeSelect(wxTreeEvent& event);

   // events from "quick search" text control
   void OnText(wxCommandEvent& event);

   // called by wxFolderNameTextCtrl (see below)
   bool OnComplete(wxTextCtrl *textctrl);

private:
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

   // helper functions for "quick search" text control
   // ------------------------------------------------

   // selects the folder corresponding to the given path
   void SelectRecursively(const wxString& path);

   // the GUI controls
   // ----------------

   wxStaticBox *m_box;
   wxTreeCtrl *m_treectrl;
   wxStaticText *m_msgBusy;
   wxTextCtrl *m_textFind;

   // the variables used for "quick search"
   bool m_settingFromProgram;    // notification comes from program, not user
   wxString m_completion;        // TAB completion for the current item

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

   // MEventReceiver cookie for the event manager
   void *m_regCookie;

   // total number of subfolders
   size_t m_nSubfolders;

   DECLARE_EVENT_TABLE()
};

// helper wxTextCtrl class which forwards to us TABs it gets
class wxFolderNameTextCtrl : public wxTextCtrl
{
public:
   // ctor
   wxFolderNameTextCtrl(wxSubscriptionDialog *dialog)
      : wxTextCtrl(dialog, -1, "",
                   wxDefaultPosition, wxDefaultSize,
                   wxTE_PROCESS_TAB)
   {
      m_dialog = dialog;
   }

   // callbacks
   void OnChar(wxKeyEvent& event);

private:
   wxSubscriptionDialog *m_dialog;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxFolderNameTextCtrl, wxTextCtrl)
   EVT_CHAR(wxFolderNameTextCtrl::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxSubscriptionDialog, wxManuallyLaidOutDialog)
   EVT_TEXT(-1, wxSubscriptionDialog::OnText)
   EVT_TREE_SEL_CHANGED(-1, wxSubscriptionDialog::OnTreeSelect)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFolderNameTextCtrl
// ----------------------------------------------------------------------------

void wxFolderNameTextCtrl::OnChar(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   // we're only interested in TABs and only it's not a second TAB in a row
   if ( event.KeyCode() == WXK_TAB )
   {
      if ( IsModified() &&
           !event.ControlDown() && !event.ShiftDown() && !event.AltDown() )
      {
         // mark control as being "not modified" - if the user presses TAB
         // the second time go to the next window immediately after having
         // expanded the entry
         DiscardEdits();

         if ( m_dialog->OnComplete(this) )
         {
            // text changed
            return;
         }
         //else: process normally
      }
      //else: nothing because we're not interested in Ctrl-TAB, Shift-TAB &c -
      //      and also in the TABs if the last one was already a TAB
   }

   // let the text control process it normally: if it's a TAB this will make
   // the focus go to the next window
   event.Skip();
}

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

   // a hack around old entries in config which were using '/inbox' instead of
   // just 'inbox' for #mh/inbox
   if ( m_folderType == MF_MH && !!m_folderPath && m_folderPath[0] == '/' )
   {
      m_folderPath.erase(0, 1);
   }

   m_nSubfolders = 0;
   m_settingFromProgram = false;

   wxString title;
   title.Printf(_("Subfolders of folder '%s'"), folder->GetFullName().c_str());
   SetTitle(title);

   // create controls
   wxLayoutConstraints *c;
   m_box = CreateStdButtonsAndBox(""); // label will be set later

   // first create the label, then the text control - we rely on it in
   // OnNoMoreFolders()
   wxStaticText *label = new wxStaticText(this, -1, _("&Find: "));
   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->height.AsIs();
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 2*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   m_textFind = new wxFolderNameTextCtrl(this);
   c = new wxLayoutConstraints;
   c->height.AsIs();
   c->left.RightOf(label, LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 2*LAYOUT_Y_MARGIN);
   m_textFind->SetConstraints(c);

   m_treectrl = new wxTreeCtrl(this, -1,
                               wxDefaultPosition, wxDefaultSize,
                               wxTR_HAS_BUTTONS |
                               wxBORDER |
                               wxTR_MULTIPLE);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_textFind, wxTop, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   // to avoid flicker, initially hide the tree ctrl and show some text
   // instead
   m_treectrl->Hide();

   // create a temp static text to occupy the place of the tree control
   m_msgBusy = new wxStaticText(this, -1, _("Retrieving the folder list..."));
   c = new wxLayoutConstraints;
   c->top.SameAs(m_treectrl, wxTop);
   c->left.SameAs(m_treectrl, wxLeft);
   c->right.SameAs(m_treectrl, wxRight);
   c->bottom.SameAs(m_treectrl, wxBottom);
   m_msgBusy->SetConstraints(c);

   // don't allow to do anything until all foders are retrieved (reenabled in
   // OnNoMoreFolders)
   Disable();

   SetDefaultSize(6*hBtn, 5*wBtn);
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   m_folder->DecRef();

   MEventManager::Deregister(m_regCookie);
}

void wxSubscriptionDialog::SelectRecursively(const wxString& path)
{
   size_t len = m_folderPath.length();
   if ( wxStrncmp(path, m_folderPath, len) != 0 )
   {
      // invalid path anyhow
      return;
   }

   char sep = GetFolderNameSeparator();
   wxString name;
   if ( !m_folderPath )
   {
      // this happens when enumerating all MH subfolders
      name = path;
   }
   else // normal case of non empty m_folderPath
   {
      size_t ofs = len;
      if ( (path.length() > len) && (path[len] == sep) )
      {
         // +1 for trailing separator
         ofs++;
      }

      name = path.c_str() + ofs;
   }

   // newsgroup names are not case sensitive, MH folder names are
   bool withCase = sep == '.' ? false : true;

   m_completion = m_folderPath;
   if ( !!m_completion )
      m_completion += sep;

   wxTreeItemId item = m_treectrl->GetRootItem();
   wxStringTokenizer tk(name, sep);
   while ( item.IsOk() && tk.HasMoreTokens() )
   {
      // find the child with the given name
      wxString component = tk.GetNextToken();

      if ( !component )
      {
         // invalid folder name entered by user
         break;
      }

      // here is the idea: suppose we have the following children under the
      // current item: bar, foo, foobar, moo. Then:
      //
      // typing 'b'           should select 'bar'
      //        'c'                         nothing at all
      //        'f'                         'foo'
      //        'foo'                       'foo'
      //        'foob'                      'foobar'
      //
      // to implement this we first find an item which matches the first
      // letter of component, then one which matches 2 first letters, ...
      //
      // if, and only if, we can match the entire string, we should continue
      // with the rest of the path - otherwise we set item to be invalid and
      // bail out of the enclosing loop
      wxTreeItemId itemLastMatched;
      len = component.length();
      for ( size_t n = 0; n < len; n++ )
      {
         wxTreeItemId itemNew;
         wxString subStr = component.Left(n + 1);

         long cookie;
         wxTreeItemId child = m_treectrl->GetFirstChild(item, cookie);
         while ( child.IsOk() )
         {
            wxString label = m_treectrl->GetItemText(child);
            int rc = withCase ? subStr.Cmp(label) : subStr.CmpNoCase(label);

            if ( rc == 0 )
            {
               // we match this item, but may be we will match the next one
               // too: remember that we match and continue
               itemNew = child;
            }
            else if ( rc < 0 )
            {
               // we won't match anything any more later, so just check
               // whether we should continue or not
               if ( wxStrncmp(subStr, label, n + 1) == 0 )
               {
                  // we should select this item
                  itemNew = child;
               }

               break;
            }

            child = m_treectrl->GetNextChild(item, cookie);
         }

         if ( !itemNew )
         {
            // no need to continue with longer substrings if we couldn't match
            // a shorter one
            break;
         }

         itemLastMatched = itemNew;
      }

      // stop here if we couldn't match anything ...
      item = itemLastMatched;
      if ( !item.IsOk() )
      {
         break;
      }

      m_completion << m_treectrl->GetItemText(item) << sep;

      // ... or matched only a substring (and not the entire component)
      if ( m_treectrl->GetItemText(item) != component )
      {
         break;
      }
   }

   if ( item.IsOk() )
   {
      m_settingFromProgram = true;

      m_treectrl->SelectItem(item);

      // won't do any harm even if it doesn't have children
      m_treectrl->Expand(item);

      m_treectrl->EnsureVisible(item);

      m_settingFromProgram = false;
   }
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
      name = path.c_str() + len + 1;
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
   m_nSubfolders++;

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
   Enable();

   // expand the root or add it if it's not there
   wxTreeItemId root = m_treectrl->GetRootItem();
   if ( root.IsOk() )
   {
      m_treectrl->Expand(root);
      m_textFind->SetValue(m_folderPath);

      m_box->SetLabel(wxString::Format(_("%u subfolders"), m_nSubfolders));
   }
   else
   {
      m_treectrl->Disable();
      m_textFind->Disable();
      wxWindow *label = FindWindow(PrevControlId(m_textFind->GetId()));
      if ( label )
         label->Disable();

      m_box->SetLabel(_("No subfolders"));
   }

   m_msgBusy->Hide();
   m_treectrl->Show();
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

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

void wxSubscriptionDialog::OnTreeSelect(wxTreeEvent& event)
{
   if ( !m_settingFromProgram )
   {
      wxTreeItemId item = event.GetItem();
      if ( item.IsOk() )
      {
         m_settingFromProgram = true;
         m_textFind->SetValue(m_treectrl->GetItemText(item));
         m_settingFromProgram = false;
      }
   }
}

void wxSubscriptionDialog::OnText(wxCommandEvent& event)
{
   if ( event.GetEventObject() == m_textFind )
   {
      if ( !m_settingFromProgram )
      {
         SelectRecursively(event.GetString());
      }
   }
   else
   {
      // ignore
      event.Skip();
   }
}

bool wxSubscriptionDialog::OnComplete(wxTextCtrl *textctrl)
{
   if ( !!m_completion )
   {
      m_settingFromProgram = true;

      textctrl->SetValue(m_completion);
      m_completion.clear();

      m_settingFromProgram = false;

      return true;
   }
   else
   {
      return false;
   }
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
      // this is the tree item we're creating folder for
      wxTreeItemId id = selections[sel];

      // this will be used to set MF_FLAGS_GROUP flag later
      bool isGroup = m_treectrl->HasChildren(id);

      // construct the full folder name by going upwars the tree and
      // concatenating everything
      wxArrayString components;
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

         // the idea for this test is to avoid creating MH folders named
         // /inbox (m_folderPath is empty for the root MH folder)
         if ( !!fullpath )
         {
            fullpath += GetFolderNameSeparator();
         }
         fullpath += name;

         // to create a a/b/c, we must first create a, then b and then c, so
         // we create a folder during each loop iteration - unless it already
         // exists
         MFolder *folderNew = parent->GetSubfolder(name);
         if ( !folderNew )
         {
            folderNew = parent->CreateSubfolder(name, m_folderType);
            if ( !folderNew )
            {
               wxLogError(_("Failed to create folder '%s'."), fullpath.c_str());

               // can't create children if parent creation failed...
               break;
            }

            // set up the just created folder
            Profile_obj profile(folderNew->GetFullName());
            profile->writeEntry(MP_FOLDER_PATH, fullpath);

            // copy folder flags from its parent hadling MF_FLAGS_GROUP
            // specially: for all the intermediate folders, it must be set (as
            // they have children, they obviously _are_ groups), but for the
            // last one it should only be set if it is a group as detected
            // above
            int flags = parent->GetFlags();
            if ( !level && !isGroup )
            {
               flags &= ~MF_FLAGS_GROUP;
            }
            folderNew->SetFlags(flags);

            // we created a new folder, set the flag to refresh the tree
            createdSomething = TRUE;
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
                                        MEventFolderTreeChangeData::CreateUnder)
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
   ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folder, NULL);

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

