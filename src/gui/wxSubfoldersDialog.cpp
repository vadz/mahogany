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
#  include "Mdefaults.h"
#  include "guidef.h"
#  include "MApplication.h"

#  include <wx/statbox.h>
#  include <wx/stattext.h>      // for wxStaticText
#  include <wx/layout.h>
#endif // USE_PCH

#include <wx/tokenzr.h>

#include "ASMailFolder.h"
#include "MFolder.h"
#include "ListReceiver.h"

#include "gui/wxDialogLayout.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDER_PATH;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ADD_ALL_SUBFOLDERS;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// how often to update the progress display?
static const size_t PROGRESS_THRESHOLD = 10;

// control ids
enum
{
   Button_AddAll = 100
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static void RemoveTrailingDelimiters(wxString *s, wxChar chDel = '/')
{
   // now remove trailing slashes if any
   size_t len = s->length();
   while ( len > 0 && (*s)[len - 1] == chDel )
   {
      // 1 more char to remove
      len--;
   }

   s->Truncate(len);
}

/// Values for StringStartsWith() last argument
enum CaseSensitivity
{
   Case_Exact,
   Case_Ignore
};

/**
   Compare two characaters either case sensitively or not.
 */
static inline bool IsSame(wxChar ch1, wxChar ch2, CaseSensitivity cs)
{
   return cs == Case_Ignore ? wxToupper(ch1) == wxToupper(ch2) : ch1 == ch2;
}

/**
   Checks whether the given string starts with the prefix and returns the
   remainder of the string in this case.

   @param str the string to examine
   @param prefix the prefix to detect and possibly remove
   @param cs case sensitivity: if Case_Ignore, prefix is matched ignoring case
   @param rest if non-NULL is filled with the remaining part of the string
               (after prefix) if the return value is true and becomes empty
               otherwise
   @return true if str starts with prefix
 */
static bool
StringStartsWith(const String& str,
                 const wxChar *prefix,
                 CaseSensitivity cs = Case_Exact,
                 String *rest = NULL)
{
   const wxChar *p = str.c_str();
   while ( *prefix )
   {
      if ( !IsSame(*prefix++, *p++, cs) )
      {
         // no match
         if ( rest )
            rest->clear();
         return false;
      }
   }

   if ( rest )
      *rest = p;

   return true;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this is the tree which is created with a folder which is the tree node and
// which is capable of populating the tree itself by getting the list of
// subfolders from the ASMailFolder
class wxSubfoldersTree : public wxTreeCtrl,
                         public ListEventReceiver
{
public:
   // ctor takes the root of the folder tree and the mail folder to use
   wxSubfoldersTree(wxWindow *parent,
                    MFolder *folderRoot,
                    ASMailFolder *mailFolder);

   // dtor
   virtual ~wxSubfoldersTree();

   wxChar GetDelimiter() const { return m_chDelimiter; }

   // event handlers
   // --------------

   // we add the items to the tree here when the user tries to expand a branch
   void OnTreeExpanding(wxTreeEvent& event);

   // list event processing functions
   virtual void OnListFolder(const String& path, wxChar delim, long flags);

   // called when the last folder is received
   virtual void OnNoMoreFolders();

private:
   // called when a new folder must be added
   wxTreeItemId OnNewFolder(String& name);

   // insert a new item named "name" under parent if it doesn't exist yet in
   // alphabetical order; returns the id of the (new) item
   wxTreeItemId InsertInOrder(wxTreeItemId parent, const wxString& name);

   // get the path of the item in the tree excluding the root part
   wxString GetRelativePath(wxTreeItemId id) const;


   // the progress meter
   MProgressInfo *m_progressInfo;

   // the type of the folders we're enumerating
   MFolderType m_folderType;

   // the name of the root folder
   String m_folderPath;

   // the root folder itself
   MFolder *m_folder;

   // the folder whose children we're currently enumerating, may be NULL
   MFolder *m_folderCur;

   // and the corresponding (halfopened) mail folder
   ASMailFolder *m_mailFolder;

   // number of folders retrieved since the last call to ListFolders()
   size_t m_nFoldersRetrieved;

   // the folder name separator
   wxChar m_chDelimiter;

   // the path of the folder whose children we're currently listing
   wxString m_reference;

   // the item which we're currently populating in the tree
   wxTreeItemId m_idParent;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxSubfoldersTree)
};

struct SubfoldersTreeItemData : public wxTreeItemData
{
   SubfoldersTreeItemData(long attr_, const String& name_)
      : name(name_),
        attr(attr_)
   {
   }

   const String name;
   const long attr;


   DECLARE_NO_COPY_CLASS(SubfoldersTreeItemData)
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

   // add all folders and close the dialog
   void OnAddAll(wxCommandEvent& event);

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
   MFolderType m_folderType;

   // the name of the root folder
   String m_folderPath;

   // the folder itself
   MFolder *m_folder;

   // and the underlying mail folder
   ASMailFolder *m_asmf;

   // returns the separator of the folder name components
   wxChar GetFolderNameSeparator() const
   {
      return m_treectrl->GetDelimiter();
   }

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxSubscriptionDialog)
};

// helper wxTextCtrl class which forwards to us TABs it gets
class wxFolderNameTextCtrl : public wxTextCtrl
{
public:
   // ctor
   wxFolderNameTextCtrl(wxSubscriptionDialog *dialog)
      : wxTextCtrl(dialog, -1, wxEmptyString,
                   wxDefaultPosition, wxDefaultSize,
                   wxTE_PROCESS_TAB)
   {
      m_dialog = dialog;
   }

   // callbacks
   void OnKeyDown(wxKeyEvent& event);

private:
   wxSubscriptionDialog *m_dialog;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxFolderNameTextCtrl)
};

// event receiver for the list events
class ListFolderEventReceiver : public ListEventReceiver
{
public:
   ListFolderEventReceiver()
   {
      m_progressInfo = NULL;
   }

   // do retrieve all folders and create them
   size_t AddAllFolders(MFolder *folder, ASMailFolder *mailFolder);

   // list folder events processing function
   virtual void OnListFolder(const String& path, wxChar delim, long flags);
   virtual void OnNoMoreFolders();

private:
   // the progress meter
   MProgressInfo *m_progressInfo;

   // number of folders retrieved since the last call to ListFolders()
   size_t m_nFoldersRetrieved;

   // done?
   bool m_finished;

   // the folder whose children we enum
   MFolder *m_folder;

   // the (cached) flags of this folder
   int m_flagsParent;

   // the path of the folder whose children we're currently listing
   wxString m_reference;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxSubfoldersTree, wxTreeCtrl)
   EVT_TREE_ITEM_EXPANDING(-1, wxSubfoldersTree::OnTreeExpanding)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxFolderNameTextCtrl, wxTextCtrl)
   EVT_KEY_DOWN(wxFolderNameTextCtrl::OnKeyDown)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxSubscriptionDialog, wxManuallyLaidOutDialog)
   EVT_TEXT(-1, wxSubscriptionDialog::OnText)

   EVT_TREE_ITEM_EXPANDED(-1, wxSubscriptionDialog::OnTreeExpanded)
   EVT_TREE_SEL_CHANGED(-1, wxSubscriptionDialog::OnTreeSelect)

   EVT_BUTTON(Button_AddAll, wxSubscriptionDialog::OnAddAll)
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
   m_folderCur = folderRoot;
   m_folderCur->IncRef();

   m_mailFolder = mailFolder;
   m_mailFolder->IncRef();

   m_progressInfo = (MProgressInfo *)NULL;
   m_chDelimiter = '\0';
   m_idParent = wxTreeItemId();

   m_folderType = m_folder->GetType();

   // a hack around old entries in config which were using '/inbox' instead of
   // just 'inbox' for #mh/inbox
   m_folderPath = m_folder->GetPath();
   switch ( m_folderType )
   {
      case MF_MH:
         if ( !m_folderPath.empty() )
         {
            if ( m_folderPath[0u] == '/' )
            {
               m_folderPath.erase(0, 1);
            }

            RemoveTrailingDelimiters(&m_folderPath);

            // and now only leave the last component
            m_folderPath = m_folderPath.AfterLast('/');
         }
         break;

      case MF_FILE:
      case MF_IMAP:
         m_folderPath = MailFolder::GetLogicalMailboxName(m_folderPath);
         break;

      default:
         // needed just to suppress gcc warning
         ;
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
            FAIL_MSG( _T("unexpected older type") );
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
      ASSERT_MSG( m_chDelimiter, _T("should know folder name separator by now!") );

      path.Prepend(GetItemText(id) + m_chDelimiter);
      id = GetItemParent(id);
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

      // disable the tree right now to prevent it from getting other events
      // (possible as we call wxYield from MProgressInfo::SetValue)
      Disable();

      // construct the IMAP spec of the folder whose children we enum
      wxString refOld = m_reference;
      wxString reference = GetRelativePath(m_idParent);

      // and remember this folder itself, if we already have it in the tree: we
      // use it to check whether its children are already in the main tree
      if ( m_folderCur )
         m_folderCur->DecRef();
      m_folderCur = m_folder->GetSubfolder(reference);

      // we may need a separator
      if ( !m_reference.empty() )
      {
         if ( m_reference.Last() != m_chDelimiter )
         {
            ASSERT_MSG( m_chDelimiter, _T("should have folder name separator") );

            m_reference += m_chDelimiter;
         }
      }

      m_reference += reference;

      MBusyCursor bc;

      // now OnNewFolder() and OnNoMoreFolders() will be called
      List(m_mailFolder, _T("%"), reference);

      // process the events from ListFolders
      do
      {
         MEventManager::ForceDispatchPending();
      }
      while ( m_idParent.IsOk() );

      // restore the old value
      m_reference = refOld;
   }
   //else: this branch had already been expanded

   event.Skip();
}

void
wxSubfoldersTree::OnListFolder(const String& path, wxChar delim, long attr)
{
   if ( m_chDelimiter == '\0' )
      m_chDelimiter = delim;

   // usually, all folders will have a non NUL delimiter ('.' for news, '/'
   // for everything else), but IMAP INBOX is special and can have a NUL one
   ASSERT_MSG( delim == m_chDelimiter || !delim,
               _T("unexpected delimiter returned by ListFolders") );

   if ( m_nFoldersRetrieved > PROGRESS_THRESHOLD )
   {
      // hide the tree to prevent flicker while it is being updated (it will
      // be shown back in OnNoMoreFolders)
      Hide();
   }

   // we're passed a folder path -- extract the folder name from it
   wxString name;
   if ( !StringStartsWith(path, m_reference, Case_Ignore, &name) )
   {
      wxLogDebug(_T("Folder specification '%s' unexpected."), path);
      return;
   }

   if ( m_chDelimiter )
   {
      if ( *name.c_str() == m_chDelimiter )
         name.erase(0, 1);
   }

   if ( name.empty() || (m_chDelimiter && wxStrchr(name, m_chDelimiter)) )
   {
      // ignore the folder itself and any grand children - we only want the
      // direct children here
      return;
   }

   // ignore "messages" part of the dual use mailboxes if we had already had
   // the "subfolders" one
   MailFolder_obj mf(m_mailFolder->GetMailFolder());
   if ( !mf )
      return;

   String namePhysical = name;
   name = MailFolder::GetLogicalMailboxName(name);


   // do add new folder to the tree
   wxTreeItemId id = OnNewFolder(name);
   if ( !id.IsOk() )
      return;

   if ( !(attr & ASMailFolder::ATT_NOINFERIORS) )
   {
      // this node can have children too
      SetItemHasChildren(id);
   }

   SetItemData(id, new SubfoldersTreeItemData(attr, namePhysical));

   // show the folders not already present in the tree in bold
   // so that new folders are immediately visible
   //
   // note that if the parent folder is not in the tree, its children
   // don't risk to be there neither
   MFolder_obj folder(m_folderCur ? m_folderCur->GetSubfolder(name) : NULL);
   if ( !folder )
   {
      SetItemBold(id);
   }
}

wxTreeItemId wxSubfoldersTree::OnNewFolder(String& name)
{
   CHECK( !!name, wxTreeItemId(), _T("folder name should not be empty") );

   // count the number of folders retrieved and show progress
   m_nFoldersRetrieved++;

   // create the progress indicator if it hadn't been yet created
   if ( !m_progressInfo )
   {
      m_progressInfo = new MProgressInfo(this, _("Retrieving the folder list: "));
   }

   // update the progress indicator from time to time
   if ( (m_nFoldersRetrieved < PROGRESS_THRESHOLD) ||
         !(m_nFoldersRetrieved % PROGRESS_THRESHOLD) )
   {
      m_progressInfo->SetValue(m_nFoldersRetrieved);
   }

   if ( m_chDelimiter != '\0' )
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

   Show();
   Enable();

   // we lose focus during expansion because we're disabled, restore it now
   SetFocus();

   if ( !m_nFoldersRetrieved && m_idParent.IsOk() )
   {
      // this item doesn't have any subfolders
      SetItemHasChildren(m_idParent, FALSE);
   }

   m_idParent.Unset();
}

wxTreeItemId wxSubfoldersTree::InsertInOrder(wxTreeItemId parent,
                                             const wxString& name)
{
   // insert in alphabetic order under the parent
   wxTreeItemIdValue cookie;
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
   if ( m_folderCur )
      m_folderCur->DecRef();
   m_folder->DecRef();
   m_mailFolder->DecRef();
}

// ----------------------------------------------------------------------------
// wxFolderNameTextCtrl
// ----------------------------------------------------------------------------

void wxFolderNameTextCtrl::OnKeyDown(wxKeyEvent& event)
{
   ASSERT( event.GetEventObject() == this ); // how can we get anything else?

   // we're only interested in TABs and only if it's not a second TAB in a row
   if ( event.GetKeyCode() != WXK_TAB )
   {
      event.Skip();
      return;
   }

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

   // handle TAB as usually: navigate to the next control (or previous one
   // with Shift)
   Navigate(!event.ShiftDown());
}

// ----------------------------------------------------------------------------
// wxSubscriptionDialog
// ----------------------------------------------------------------------------

wxSubscriptionDialog::wxSubscriptionDialog(wxWindow *parent,
                                          MFolder *folder,
                                          ASMailFolder *mailFolder)
                    : wxManuallyLaidOutDialog(parent,
                                              _("Select subfolders to add to the tree"),
                                              _T("SubscribeDialog"))
{
   // init members
   m_folder = folder;
   m_folder->IncRef();
   m_folderType = folder->GetType();
   m_asmf = mailFolder;
   m_asmf->IncRef();
   m_settingFromProgram = false;

   // create controls
   wxLayoutConstraints *c;
   m_box = CreateStdButtonsAndBox(wxEmptyString); // label will be set later

   // first create the label, then the text control - we rely on it in
   // OnNoMoreFolders()
   wxStaticText *label = new wxStaticText(this, -1, _("&Find: "));
   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->height.AsIs();
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_box, wxBottom, 3*LAYOUT_Y_MARGIN);
   label->SetConstraints(c);

   m_textFind = new wxFolderNameTextCtrl(this);
   c = new wxLayoutConstraints;
   c->height.AsIs();
   c->left.RightOf(label, LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(label, wxCentreY);
   m_textFind->SetConstraints(c);

   wxButton *btnAddAll = new wxButton(this, Button_AddAll, _("&Add all"));
   wxWindow *btnOk = FindWindow(wxID_OK);
   c = new wxLayoutConstraints;
   c->top.SameAs(btnOk, wxTop);
   c->height.AsIs();
   c->width.AsIs();
   c->right.SameAs(btnOk, wxLeft, 2*LAYOUT_X_MARGIN);
   btnAddAll->SetConstraints(c);

   m_treectrl = new wxSubfoldersTree(this, folder, mailFolder);
   c = new wxLayoutConstraints;
   c->top.SameAs(m_box, wxTop, 5*LAYOUT_Y_MARGIN);
   c->left.SameAs(m_box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(m_box, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(m_textFind, wxTop, 2*LAYOUT_Y_MARGIN);
   m_treectrl->SetConstraints(c);

   wxTreeItemId idRoot = m_treectrl->GetRootItem();
   m_folderPath = m_treectrl->GetItemText(idRoot);
   m_treectrl->Expand(idRoot);

   SetDefaultSize(4*wBtn, 10*hBtn);
}

wxSubscriptionDialog::~wxSubscriptionDialog()
{
   m_folder->DecRef();
   m_asmf->DecRef();
}

void wxSubscriptionDialog::SelectRecursively(const wxString& path)
{
   size_t len = m_folderPath.length();
   if ( wxStrncmp(path, m_folderPath, len) != 0 )
   {
      // invalid path anyhow
      return;
   }

   wxChar sep = GetFolderNameSeparator();
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

         wxTreeItemIdValue cookie;
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


// update the number of items in the box
void wxSubscriptionDialog::OnTreeExpanded(wxTreeEvent& event)
{
   wxTreeItemId id = event.GetItem();
   size_t nFolders = m_treectrl->GetChildrenCount(id);
   m_box->SetLabel(wxString::Format(_("%u subfolders under %s"),
                   nFolders, m_treectrl->GetItemText(id)));

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
   if ( !m_completion.empty() )
      return false;

   m_settingFromProgram = true;

   textctrl->SetValue(m_completion);
   m_completion.clear();

   m_settingFromProgram = false;

   return true;
}

void wxSubscriptionDialog::OnAddAll(wxCommandEvent& WXUNUSED(event))
{
   if ( !AddAllSubfoldersToTree(m_folder, m_asmf) )
   {
      wxLogError(_("Failed to add all subfolders to the tree."));
   }
   else
   {
      EndModal(wxID_OK);
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
      bool isGroup = m_treectrl->ItemHasChildren(id);

      // construct the full folder name by going upwards the tree and
      // concatenating everything
      wxArrayString components;
      wxArrayTreeItemIds ids;
      while ( id != idRoot )
      {
         components.Add(m_treectrl->GetItemText(id));
         ids.Add(id);
         id = m_treectrl->GetItemParent(id);
      }

      size_t levelMax = components.GetCount();
      if ( !levelMax )
      {
         // doh... it was root...
         continue;
      }

      MFolder *parent = m_folder;
      parent->IncRef();

      int flagsParent = parent->GetFlags();

      wxString fullpath = m_folderPath;
      for ( int level = levelMax - 1; level >= 0; level-- )
      {
         const String name = components[level];

         // the idea for this test is to avoid creating MH folders named
         // /inbox (fullpath is empty for the root MH folder)
         if ( !fullpath.empty() )
         {
            fullpath += GetFolderNameSeparator();
         }

         // to create a/b/c, we must first create a, then b and then c, so we
         // create a folder during each loop iteration if it doesn't exist yet
         MFolder *folderNew = parent->GetSubfolder(name);
         if ( !folderNew )
         {
            // last parameter tell CreateSubfolder() to not try to create
            // this folder - we know that it already exists
            folderNew = parent->CreateSubfolder(name, m_folderType, false);
            if ( !folderNew )
            {
               wxLogError(_("Failed to create folder '%s'."), fullpath + name);

               // can't create children if parent creation failed...
               break;
            }

            SubfoldersTreeItemData *
               data = static_cast<SubfoldersTreeItemData *>(
                                          m_treectrl->GetItemData(ids[level]));

            // set up the just created folder
            Profile_obj profile(folderNew->GetProfile());

            // notice the use of data->name: this is important as for emulated
            // dual use mailboxes it may be different than name and here we
            // need the real path pointing to the mailbox containing the
            // messages, not the path in the folders hierarchy
            profile->writeEntry(MP_FOLDER_PATH, fullpath + data->name);

            // copy folder flags from its parent handling MF_FLAGS_GROUP
            // specially: for all the intermediate folders, it must be set (as
            // they have children, they obviously _are_ groups), but for the
            // last one it should only be set if it is a group as detected
            // above
            int flags = flagsParent;
            if ( !level && !isGroup )
            {
               flags &= ~MF_FLAGS_GROUP;
            }

            if ( data && (data->attr & ASMailFolder::ATT_NOSELECT) )
            {
               flags |= MF_FLAGS_NOSELECT;
            }
            else // this folder doesn't have NOSELECT flag
            {
               flags &= ~MF_FLAGS_NOSELECT;
            }

            folderNew->SetFlags(flags);

            // we created a new folder, set the flag to refresh the tree
            createdSomething = TRUE;
         }

         fullpath += name;

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
      MEventManager::ForceDispatchPending();
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
   m_flagsParent = m_folder->GetFlags();

   m_nFoldersRetrieved = 0u;
   m_finished = false;

   m_progressInfo = new MProgressInfo(NULL,
                                      _("Retrieving the folder list: "));
   wxYieldIfNeeded(); // to show the frame

   (void)mailFolder->ListFolders
                     (
                        _T("*"),     // everything
                        FALSE,       // subscribed only?
                        wxEmptyString,      // path relative to the folder
                        this         // data to pass to the callback
                     );

   // wait until the expansion ends
   do
   {
      MEventManager::ForceDispatchPending();
   }
   while ( !m_finished );

   // clean up
   delete m_progressInfo;

   m_folder->DecRef();

   return m_nFoldersRetrieved;
}

void
ListFolderEventReceiver::OnNoMoreFolders()
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
      MEventManager::ForceDispatchPending();
   }
}

void
ListFolderEventReceiver::OnListFolder(const String& path,
                                      wxChar chDelimiter,
                                      long attr)
{
   // count the number of folders retrieved and show progress
   m_nFoldersRetrieved++;

   // update the progress indicator from time to time (but often in the
   // beginning)
   if ( (m_nFoldersRetrieved < PROGRESS_THRESHOLD) ||
          !(m_nFoldersRetrieved % PROGRESS_THRESHOLD) )
   {
      m_progressInfo->SetValue(m_nFoldersRetrieved);
   }

   // we're passed a folder specification - extract the folder name from it
   wxString name;
   if ( StringStartsWith(path, m_reference, Case_Ignore, &name) )
   {
      // remove the leading delimiter to get a relative name
      if ( chDelimiter != '\0' && *name.c_str() == chDelimiter )
      {
         name.erase(0, 1);
      }
   }

   if ( name.empty() )
   {
      wxLogDebug(_T("Folder specification '%s' unexpected."), path);
      return;
   }

   wxString relpath = name;
   if ( chDelimiter != '\0' )
      name.Replace(wxString(chDelimiter), _T("/"));

   MFolder *folderNew = m_folder->GetSubfolder(name);
   if ( !folderNew )
   {
      // last parameter tell CreateSubfolder() to not try to create
      // this folder - we know that it already exists
      folderNew = m_folder->CreateSubfolder(name,
                                            m_folder->GetType(), false);
   }

   // note that we must set the folder flags/whatever even if it already
   // exists as we can get first the notification for folder.subfolder
   // and when we create it, we create the config group for folder as
   // well but it is empty and so we have to set the params for it later
   // when we get the notification for the folder itself
   if ( folderNew )
   {
      Profile_obj profile(folderNew->GetProfile());

      // check if the folder really already exists, if not - create it
      if ( !profile->HasEntry(MP_FOLDER_PATH) )
      {
         int flags = m_flagsParent;
         if ( attr & ASMailFolder::ATT_NOINFERIORS )
         {
            flags &= ~MF_FLAGS_GROUP;
         }
         else
         {
            flags |= MF_FLAGS_GROUP;
         }

         if ( attr & ASMailFolder::ATT_NOSELECT )
         {
            flags |= MF_FLAGS_NOSELECT;
         }
         else
         {
            flags &= ~MF_FLAGS_NOSELECT;
         }

         folderNew->SetFlags(flags);

         String fullpath = m_folder->GetPath();
         if ( !fullpath.empty() )
         {
            // we don't want the paths to start with '/', but we want to
            // have it if there is something before
            fullpath += chDelimiter;
         }

         fullpath += relpath;

         profile->writeEntry(MP_FOLDER_PATH, fullpath);
      }
      //else: folder already exists with the correct parameters

      folderNew->DecRef();
   }
   else
   {
      wxLogError(_("Failed to create the folder '%s'"), name);
   }
}

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

// show the dialog allowing the user to choose the subfolders to add to the
// tree
bool ShowFolderSubfoldersDialog(MFolder *folder, wxWindow *parent)
{
   if ( !CanHaveSubfolders(folder->GetType(), folder->GetFlags()) )
   {
      // how did we get here at all?
      wxLogMessage(_("The folder '%s' has no subfolders."), folder->GetPath());

      return FALSE;
   }

   // The folder must be half opened because we don't really want to read any
   // messages in it, just enum subfolders
   ASMailFolder *asmf = ASMailFolder::HalfOpenFolder(folder);

   if ( !asmf )
   {
      if ( mApplication->GetLastError() != M_ERROR_CANCEL )
      {
         wxString folderPath = folder->GetPath();
         if ( !folderPath )
         {
            // take its name or the error message will be unreadable
            folderPath = folder->GetFullName();
         }

         wxLogError(_("Impossible to browse subfolders of folder '%s' because "
                      "the folder cannot be opened."),
                    folderPath);
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
         wxString::Format(_("Subfolders of '%s'"), folder->GetPath()),
         M_DLG_YES_DEFAULT,
         M_MSGBOX_ADD_ALL_SUBFOLDERS
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
