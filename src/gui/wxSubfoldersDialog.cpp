///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wxSubfoldersDialog.cpp - implementation of functions from
//              MFolderDialogs.h
// Purpose:
// Author:      Vadim Zeitlin
// Modified by: VZ at 21.07.00: now each tree level is retrieved as needed,
//                              not the whole tree at once (huge perf boost)
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
#include "Mpers.h"

#include "ASMailFolder.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// control ids
enum
{
   Button_SelectAll = 100,
   Button_UnselectAll
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static void RemoveTrailingDelimiters(wxString *s, char chDel = '/')
{
   // now remove trailing backslashes if any
   size_t len = s->length();
   while ( len > 0 && (*s)[len - 1] == chDel )
   {
      // 1 more char to remove
      len--;
   }

   s->Truncate(len);
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this is the tree which is created with a folder which is the tree node and
// which is capable of populating the tree itself by getting the list of
// subfolders from the ASMailFolder
class wxSubfoldersTree : public wxTreeCtrl,
                         public MEventReceiver
{
public:
   // ctor takes the root of the folder tree and the mail folder to use
   wxSubfoldersTree(wxWindow *parent,
                    MFolder *folderRoot,
                    ASMailFolder *mailFolder);

   // dtor
   virtual ~wxSubfoldersTree();

   char GetDelimiter() const { return m_chDelimiter; }

   // event handlers
   // --------------

   // we add the items to the tree here when the user tries to expand a branch
   void OnTreeExpanding(wxTreeEvent& event);

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // called when a new folder must be added
   wxTreeItemId OnNewFolder(String& name);

   // called when the last folder is received
   void OnNoMoreFolders();

   // insert a new item named "name" under parent if it doesn't exist yet in
   // alphabetical order; returns the id of the (new) item
   wxTreeItemId InsertInOrder(wxTreeItemId parent, const wxString& name);

   // get the path of the item in the tree excluding the root part
   wxString GetRelativePath(wxTreeItemId id) const;

   // MEventReceiver cookie for the event manager
   void *m_regCookie;

   // the progress meter
   MProgressInfo *m_progressInfo;

   // the type of the folders we're enumerating
   FolderType m_folderType;

   // the name of the root folder
   String m_folderPath;

   // the folder itself
   MFolder *m_folder;

   // and the corresponding (halfopened) mail folder
   ASMailFolder *m_mailFolder;

   // number of folders retrieved since the last call to ListFolders()
   size_t m_nFoldersRetrieved;

   // the folder name separator
   char m_chDelimiter;

   // the full spec of the folder whose children we're currently listing
   wxString m_reference;

   // the item which we're currently populating in the tree
   wxTreeItemId m_idParent;

   DECLARE_EVENT_TABLE()
};

class wxSubscriptionDialog : public wxManuallyLaidOutDialog
{
public:
   wxSubscriptionDialog(wxWindow *parent,
                        MFolder *folder,
                        ASMailFolder *mailFolder);
   virtual ~wxSubscriptionDialog();

   // called when [Ok] is pressed, may veto it
   virtual bool TransferDataFromWindow();

   // callbacks
   // ---------

   // item selected in the tree
   void OnTreeSelect(wxTreeEvent& event);

   // update the number of items
   void OnTreeExpanded(wxTreeEvent& event);

   // events from "quick search" text control
   void OnText(wxCommandEvent& event);

   // select/unselect all button handlers
   void OnSelectAll(wxCommandEvent& event);
   void OnUnselectAll(wxCommandEvent& event);

   // called by wxFolderNameTextCtrl (see below)
   bool OnComplete(wxTextCtrl *textctrl);

private:
   // helper functions for "quick search" text control: selects the folder
   // corresponding to the given path
   void SelectRecursively(const wxString& path);

   // the GUI controls
   // ----------------

   wxStaticBox *m_box;
   class wxSubfoldersTree *m_treectrl;
   wxTextCtrl *m_textFind;

   // the variables used for "quick search"
   bool m_settingFromProgram;    // notification comes from program, not user
   wxString m_completion;        // TAB completion for the current item

   // the type of the folders we're enumerating
   FolderType m_folderType;

   // the name of the root folder
   String m_folderPath;

   // the folder itself
   MFolder *m_folder;

   // the folder name separator
   char m_chDelimiter;

   // returns the separator of the folder name components
   char GetFolderNameSeparator() const
   {
      return m_treectrl->GetDelimiter();
   }

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

// event receiver for the list events
class ListFolderEventReceiver : public MEventReceiver
{
public:
   ListFolderEventReceiver()
   {
      m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   }

   virtual ~ListFolderEventReceiver()
   {
      MEventManager::Deregister(m_regCookie);
   }

   // do retrieve all folders and create them
   size_t AddAllFolders(MFolder *folder, ASMailFolder *mailFolder);

   // event processing function
   virtual bool OnMEvent(MEventData& event);

private:
   // MEventReceiver cookie for the event manager
   void *m_regCookie;

   // the progress meter
   MProgressInfo *m_progressInfo;

   // number of folders retrieved since the last call to ListFolders()
   size_t m_nFoldersRetrieved;

   // done?
   bool m_finished;

   // the folder whose children we enum
   MFolder *m_folder;

   // the full spec of the folder whose children we're currently listing
   wxString m_reference;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxSubfoldersTree, wxTreeCtrl)
   EVT_TREE_ITEM_EXPANDING(-1, wxSubfoldersTree::OnTreeExpanding)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderNameTextCtrl, wxTextCtrl)
   EVT_CHAR(wxFolderNameTextCtrl::OnChar)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxSubscriptionDialog, wxManuallyLaidOutDialog)
   EVT_TEXT(-1, wxSubscriptionDialog::OnText)

   EVT_TREE_ITEM_EXPANDED(-1, wxSubscriptionDialog::OnTreeExpanded)
   EVT_TREE_SEL_CHANGED(-1, wxSubscriptionDialog::OnTreeSelect)

#ifdef USE_SELECT_BUTTONS
   EVT_BUTTON(Button_SelectAll, wxSubscriptionDialog::OnSelectAll)
   EVT_BUTTON(Button_UnselectAll, wxSubscriptionDialog::OnUnselectAll)
#endif // USE_SELECT_BUTTONS
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxSubfoldersTree
// ----------------------------------------------------------------------------

wxSubfoldersTree::wxSubfoldersTree(wxWindow *parent,
                                   MFolder *folderRoot,
                                   ASMailFolder *mailFolder)
                : wxTreeCtrl(parent, -1,
                             wxDefaultPosition, wxDefaultSize,
                             wxTR_HAS_BUTTONS | wxBORDER | wxTR_MULTIPLE)
{
   // init members
   m_folder = folderRoot;
   m_folder->IncRef();

   m_mailFolder = mailFolder;
   m_mailFolder->IncRef();

   m_regCookie = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookie, "can't register with event manager");

   m_progressInfo = (MProgressInfo *)NULL;
   m_chDelimiter = '\0';
   m_idParent = wxTreeItemId();

   m_folderType = m_folder->GetType();

   // a hack around old entries in config which were using '/inbox' instead of
   // just 'inbox' for #mh/inbox
   m_folderPath = m_folder->GetPath();
   if ( m_folderType == MF_MH )
   {
      if ( !!m_folderPath )
      {
         if ( m_folderPath[0] == '/' )
         {
            m_folderPath.erase(0, 1);
         }

         RemoveTrailingDelimiters(&m_folderPath);

         // andnow only leave the last component
         m_folderPath = m_folderPath.AfterLast('/');
      }
   }

   // show something...
   if ( !m_folderPath )
   {
      switch ( m_folderType )
      {
         case MF_MH:
            m_folderPath = _("MH root");
            break;

         case MF_IMAP:
            m_folderPath = _("IMAP server");
            break;

         case MF_NNTP:
         case MF_NEWS:
            m_folderPath = _("News server");
            break;

         default:
            FAIL_MSG( "unexpected older type" );
      }
   }

   // add the root item to the tree
   SetItemHasChildren(AddRoot(m_folderPath));
}

wxString wxSubfoldersTree::GetRelativePath(wxTreeItemId id) const
{
   wxString path;
   wxTreeItemId idRoot = GetRootItem();
   while ( id != idRoot )
   {
      ASSERT_MSG( m_chDelimiter, "should know folder name separator by now!" );

      path.Prepend(GetItemText(id) + m_chDelimiter);
      id = GetParent(id);
   }

   return path;
}

void wxSubfoldersTree::OnTreeExpanding(wxTreeEvent& event)
{
   m_idParent = event.GetItem();
   if ( !GetChildrenCount(m_idParent) )
   {
      // reset the counter, we will show the progress info if there are many
      // folders
      m_nFoldersRetrieved = 0;

      // start from the folder being expanded
      wxString reference = GetRelativePath(m_idParent);

      // disable the tree (it will be reenabled in OnNoMoreFolders) to prevent
      // other events (possible as we will call wxYield())
      Disable();

      // this is the cclient spec of the folder whose children we enum
      m_reference = ((MailFolderCC *)m_mailFolder->GetMailFolder())->GetSpec();
      m_reference += reference;

      wxBusyCursor bc;

      // now OnNewFolder() and OnNoMoreFolders() will be called
      (void)m_mailFolder->ListFolders
                          (
                             "%",         // everything at this tree level
                             FALSE,       // subscribed only?
                             reference,   // path relative to the folder
                             this         // data to pass to the callback
                          );

      // wait until the expansion ends
      do
      {
         wxYield();
      }
      while ( m_idParent.IsOk() );
   }
   //else: this branch had already been expanded

   event.Skip();
}

// needed to be able to use DECLARE_AUTOREF() macro
typedef ASMailFolder::ResultFolderExists ASFolderExistsResult;
DECLARE_AUTOPTR(ASFolderExistsResult);

bool wxSubfoldersTree::OnMEvent(MEventData& event)
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

   // usually, all folders will have a non NUL delimiter ('.' for news, '/'
   // for everything else), but IMAP INBOX is special
   //
   // FIXME the much better way to do it is to issue 'LIST "" ""' command
   //       which will always return the delimiter (and nothing else), however
   //       I don't know if it is supported by all cclient drivers?
   char chDelimiter = result->GetDelimiter();
   if ( chDelimiter )
   {
      // avoid setting m_chDelimiter to NUL
      m_chDelimiter = chDelimiter;
   }

   // is it the special event which signals that there will be no more of
   // folders?
   wxString spec = result->GetName();
   if ( !spec )
   {
      OnNoMoreFolders();
   }
   else // normal folder event
   {
      // we're passed a folder specification - extract the folder name from it
      wxString name;
      if ( spec.StartsWith(m_reference, &name) )
      {
         if ( m_chDelimiter )
         {
            if ( !!name && name[0] == m_chDelimiter )
            {
               name = name.c_str() + 1;
            }
         }

         // ignore the folder itself and any grand children - we only want the
         // direct children here
         if ( !!name && (!m_chDelimiter || !strchr(name, m_chDelimiter)) )
         {
            wxTreeItemId id = OnNewFolder(name);
            if ( id.IsOk() )
            {
               long attr = result->GetAttributes();
               if ( !(attr & LATT_NOINFERIORS) )
               {
                  // this node can have children too
                  SetItemHasChildren(id);
               }

               if ( attr & LATT_NOSELECT )
               {
                  SetItemData(id, new wxTreeItemData());
               }

               if ( attr & LATT_MARKED )
               {
                  SetItemBold(id);
               }
            }
         }
      }
      else
      {
         wxLogDebug("Folder specification '%s' unexpected.", spec.c_str());
      }
   }

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

wxTreeItemId wxSubfoldersTree::OnNewFolder(String& name)
{
   CHECK( !!name, wxTreeItemId(), "folder name should not be empty" );

   // count the number of folders retrieved and show progress
   m_nFoldersRetrieved++;

   // create the progress indicator if there are many folders and it hadn't
   // been yet created
   if ( m_nFoldersRetrieved > 20 && !m_progressInfo )
   {
      m_progressInfo = new MProgressInfo(this, _("Retrieving the folder list: "));
   }

   // update the progress indicator from time to time
   if ( !(m_nFoldersRetrieved % 100) )
   {
      m_progressInfo->SetValue(m_nFoldersRetrieved);
   }

   if ( m_chDelimiter )
      RemoveTrailingDelimiters(&name, m_chDelimiter);

   // and add the new folder into the tree
   return InsertInOrder(m_idParent, name);
}

void wxSubfoldersTree::OnNoMoreFolders()
{
   if ( m_progressInfo )
   {
      delete m_progressInfo;
      m_progressInfo = NULL;
   }

   Enable();

   if ( !m_nFoldersRetrieved && m_idParent.IsOk() )
   {
      // this item doesn't have any subfolders
      SetItemHasChildren(m_idParent, FALSE);
   }

   m_idParent = wxTreeItemId(); // invalid
}

wxTreeItemId wxSubfoldersTree::InsertInOrder(wxTreeItemId parent,
                                             const wxString& name)
{
   // insert in alphabetic order under the parent
   long cookie;
   wxTreeItemId childPrev,
                child = GetFirstChild(parent, cookie);
   while ( child.IsOk() )
   {
      int rc = name.Cmp(GetItemText(child));

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

         child = GetNextChild(parent, cookie);
      }
   }

   if ( childPrev.IsOk() )
   {
      // insert after child
      return InsertItem(parent, childPrev, name);
   }
   else
   {
      // prepend if there is no previous child
      return PrependItem(parent, name);
   }
}

wxSubfoldersTree::~wxSubfoldersTree()
{
   m_folder->DecRef();
   m_mailFolder->DecRef();

   MEventManager::Deregister(m_regCookie);
}

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

wxSubscriptionDialog::wxSubscriptionDialog(wxWindow *parent,
                                          MFolder *folder,
                                          ASMailFolder *mailFolder)
                    : wxManuallyLaidOutDialog(parent,
                                              _("Select subfolders to add to the tree"),
                                              "SubscribeDialog")
{
   // init members
   m_folder = folder;
   m_folder->IncRef();
   m_folderType = folder->GetType();
   m_settingFromProgram = false;
   m_chDelimiter = '\0';

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

#ifdef USE_SELECT_BUTTONS
   m_btnSelectAll = new wxButton(this, , _("&Select all"));
#endif // USE_SELECT_BUTTONS

   m_treectrl = new wxSubfoldersTree(this, folder, mailFolder);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_box, wxTop, 4*LAYOUT_Y_MARGIN);
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_textFind, wxTop, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   SetDefaultSize(4*wBtn, 10*hBtn);

   wxTreeItemId idRoot = m_treectrl->GetRootItem();
   m_folderPath = m_treectrl->GetItemText(idRoot);
   m_treectrl->Expand(idRoot);
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   m_folder->DecRef();
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

#ifdef USE_SELECT_BUTTONS

void wxSubscriptionDialog::OnSelectAll(wxCommandEvent& event)
{
   wxTreeItemId root = m_treectrl->GetRootItem();
   SelectAllUnder(root);
}

void wxSubscriptionDialog::SelectAllUnder(const wxTreeItemId& item)
{
   wxTreeItemId child = m_treectrl->GetFirstChild(item, cookie);
   while ( child.IsOk() )
   {
      m_treectrl->Select(child);

      if ( m_treectrl->ItemHasChildren() )
      {
         SelectAllUnder(child);
      }

      child = m_treectrl->GetNextChild(item, cookie);
   }
}

void wxSubscriptionDialog::OnUnselectAll(wxCommandEvent& event)
{
   m_treectrl->UnselectAll();
}

#endif // USE_SELECT_BUTTONS

// update the number of items in the box
void wxSubscriptionDialog::OnTreeExpanded(wxTreeEvent& event)
{
   wxTreeItemId id = event.GetItem();
   size_t nFolders = m_treectrl->GetChildrenCount(id);
   m_box->SetLabel(wxString::Format(_("%u subfolders under %s"),
                   nFolders, m_treectrl->GetItemText(id).c_str()));

   event.Skip();
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

      // the item data is only set by the tree for non selectable folders
      bool canBeOpened = !m_treectrl->GetItemData(id);

      // this will be used to set MF_FLAGS_GROUP flag later
      bool isGroup = m_treectrl->ItemHasChildren(id);

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

      wxString fullpath = m_folder->GetPath();
      for ( int level = levelMax - 1; level >= 0; level-- )
      {
         wxString name = components[level];

         // the idea for this test is to avoid creating MH folders named
         // /inbox (fullpath is empty for the root MH folder)
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

            if ( canBeOpened )
            {
               flags &= ~MF_FLAGS_NOSELECT;
            }
            else
            {
               flags |= MF_FLAGS_NOSELECT;
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
// ListFolderEventReceiver
// ----------------------------------------------------------------------------

size_t ListFolderEventReceiver::AddAllFolders(MFolder *folder,
                                              ASMailFolder *mailFolder)
{
   m_folder = folder;
   m_folder->IncRef();

   m_reference = ((MailFolderCC *)mailFolder->GetMailFolder())->GetSpec();
   m_nFoldersRetrieved = 0u;
   m_finished = false;

   (void)mailFolder->ListFolders
                     (
                        "*",         // everything
                        FALSE,       // subscribed only?
                        "",          // path relative to the folder
                        this         // data to pass to the callback
                     );

   // wait until the expansion ends
   do
   {
      wxYield();
   }
   while ( !m_finished );

   m_folder->DecRef();

   return m_nFoldersRetrieved;
}

bool ListFolderEventReceiver::OnMEvent(MEventData& event)
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

   char chDelimiter = result->GetDelimiter();

   // is it the special event which signals that there will be no more of
   // folders?
   wxString spec = result->GetName();
   if ( !spec )
   {
      // no more folders
      m_finished = true;

      if ( m_nFoldersRetrieved )
      {
         // generate an event notifying everybody that a new folder has been
         // created
         MEventManager::Send(
            new MEventFolderTreeChangeData(m_folder->GetFullName(),
                                           MEventFolderTreeChangeData::CreateUnder)
            );
         MEventManager::DispatchPending();
      }
   }
   else // normal folder event
   {
      // count the number of folders retrieved and show progress
      m_nFoldersRetrieved++;

      // create the progress indicator if there are many folders and it hadn't
      // been yet created
      if ( m_nFoldersRetrieved > 20 && !m_progressInfo )
      {
         m_progressInfo = new MProgressInfo(NULL,
                                            _("Retrieving the folder list: "));
      }

      // update the progress indicator from time to time
      if ( !(m_nFoldersRetrieved % 100) )
      {
         m_progressInfo->SetValue(m_nFoldersRetrieved);
      }

      // we're passed a folder specification - extract the folder name from it
      wxString name;
      if ( spec.StartsWith(m_reference, &name) && !!name )
      {
         if ( name[0u] == chDelimiter && chDelimiter != '\0')
         {
            name = name.c_str() + 1;
         }

         wxString path = name;
         if(chDelimiter != '\0')
            name.Replace(wxString(chDelimiter), "/");
         MFolder *folderNew = m_folder->GetSubfolder(name);
         if ( !folderNew )
         {
            folderNew = m_folder->CreateSubfolder(name, m_folder->GetType());

            long flags = folderNew->GetFlags();
            long attr = result->GetAttributes();
            if ( attr & LATT_NOINFERIORS )
            {
               flags &= ~MF_FLAGS_GROUP;
            }
            else
            {
               flags |= MF_FLAGS_GROUP;
            }

            if ( attr & LATT_NOSELECT )
            {
               flags |= MF_FLAGS_NOSELECT;
            }
            else
            {
               flags &= ~MF_FLAGS_NOSELECT;
            }

            folderNew->SetFlags(flags);

            Profile_obj profile(folderNew->GetFullName());
            String fullpath;
            fullpath << m_folder->GetPath() << chDelimiter << path;
            profile->writeEntry(MP_FOLDER_PATH, path);
         }

         folderNew->DecRef();
      }
      else
      {
         wxLogDebug("Folder specification '%s' unexpected.", spec.c_str());
      }
   }

   // we don't want anyone else to receive this message - it was for us only
   return FALSE;
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

// show the dialog allowing the user to choose the subfoldersto add to the
// tree
bool ShowFolderSubfoldersDialog(MFolder *folder, wxWindow *parent)
{
   if ( !(folder->GetFlags() & MF_FLAGS_GROUP) )
   {
      // how did we get here at all?
      wxLogMessage(_("The folder '%s' has no subfolders."),
                   folder->GetPath().c_str());

      return FALSE;
   }

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

   if ( MDialog_YesNoDialog
        (
         _("Would you like to add all subfolders of this folder\n"
           "to the folder tree (or select the individual folders\n"
           "manually)?"),
         parent,
         wxString::Format(_("Subfolders of '%s'"), folder->GetPath().c_str()),
         true, // [Yes] default
         GetPersMsgBoxName(M_MSGBOX_ADD_ALL_SUBFOLDERS)
        )
      )
   {
      AddAllSubfoldersToTree(folder, asmf);

      asmf->DecRef();
   }
   else // select the folders manually
   {
      wxSubscriptionDialog dlg(GetFrame(parent), folder, asmf);

      asmf->DecRef();

      dlg.ShowModal();
   }

   return TRUE;
}

// add all subfolders to the tree
extern size_t AddAllSubfoldersToTree(MFolder *folder, ASMailFolder *mailFolder)
{
   ListFolderEventReceiver receiver;

   return receiver.AddAllFolders(folder, mailFolder);
}
