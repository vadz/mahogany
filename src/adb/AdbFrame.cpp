///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   adb/AdbFrame.cpp - ADB edit frame interface
// Purpose:     implementation of wxAdbEditFrame
// Author:      Vadim Zeitlin
// Modified by:
// Created:     09.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// M
#include "Mpch.h"

#ifndef  USE_PCH
#   include "kbList.h"

#   include "guidef.h"
#   include "strutil.h"

#   include "Mcommon.h"
#   include "MFrame.h"
#   include "gui/wxMFrame.h"

#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "MHelp.h"
#   include <ctype.h>
#endif //USE_PCH

#include "MDialogs.h"
#include "Mdefaults.h"
#include "gui/wxMenuDefs.h"
#include "gui/wxIconManager.h"

#undef   CreateListBox

// wxWindows
#ifndef USE_PCH
#   include <wx/frame.h>
#   include <wx/log.h>
#   include <wx/confbase.h>
#   include <wx/dynarray.h>
#   include <wx/toolbar.h>
#   include <wx/menu.h>
#   include <wx/layout.h>
#   include <wx/statbox.h>
#   include <wx/choicdlg.h>
#   include <wx/stattext.h>
#endif // USE_PCH

#include <wx/notebook.h>
#include <wx/treectrl.h>
#include <wx/file.h>
#include <wx/imaglist.h>

#include "wx/persctrl.h"

#include "adb/AdbManager.h"
#include "adb/AdbEntry.h"
#include "adb/AdbBook.h"
#include "adb/AdbDataProvider.h"
#include "adb/AdbImport.h"
#include "adb/AdbExport.h"
#include "adb/AdbImpExp.h"
#include "adb/AdbDialogs.h"

// our public interface
#include "adb/AdbFrame.h"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_SHOWADBEDITOR;
extern const MOption MP_USERDIR;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ADB_DELETE_ENTRY;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// get the full name of the group we use in wxConfig
extern String GetAdbEditorConfigPath()
{
  String path;
  path << _T('/') << M_SETTINGS_CONFIG_SECTION << _T('/') << ADB_CONFIG_PATH;

  return path;
}

// enum must be in sync with the array below
enum
{
  ConfigName_LastNewEntry,
  ConfigName_LastNewWasGroup,
  ConfigName_AddressBooks,
  ConfigName_AddressBookProviders,
  ConfigName_ExpandedBranches,
  ConfigName_TreeSelection,
  ConfigName_LastLookup,
  ConfigName_FindOptions
};

static const wxChar *aszConfigNames[] =
{
  _T("LastNewEntry"),
  _T("LastNewWasGroup"),
  _T("AddressBooks"),
  _T("Providers"),
  _T("ExpandedBranches"),
  _T("TreeSelection"),
  _T("FindWhere"),
  _T("FindOptions")
};

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// the frame class

/*
TODO: (+ in first column means done)
  0. more fields (encryption)
+ 1. copy/cut/paste
+ 2. toolbar
  3. entry renaming (probably should be supported in wxConfig?)
  4. context-sensitive help
+ 5. lookup (reg exps?)
+ 6. import/export
  7. splitter?
+ 8. set the type of lookup (where to look for the match)
+ 9. listbox interface (add/remove items)
+ 10. goto given item
+ 11. find/find next
+ 12. save expanded tree branches
  13. d&d would be nice (if supported under wxGTK)
+ 14. case (in)sensitive match, take into account find options, make
      the toolbar button work like "Find Next" if "Find" was done,
  15. sort the find results from top to bottom
+ 16. prompts should have a real history and not only remember the last value
+ 17. send an event from ADB code when a new ADB is created and react to it
      from here

  @@PERS indicates the things that should be persistent (default values for
         the controls &c) but are not yet

  NOT_IMPLEMENTED macro is used for features which are completely not done.
 */

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// search for this to find all places it's called from
#define NOT_IMPLEMENTED   \
  wxLogError(_("Sorry, this feature is not yet implemented."))

// hide casts inside macros
#define TEXTCTRL(index)   ((wxTextCtrl *)m_aEntries[(index)])
#define CHECKBOX(index)   ((wxCheckBox *)m_aEntries[(index)])

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------
class wxAdbEditFrame;
class wxAdbNotebook;

class AdbTreeEntry;
class AdbTreeNode;
class AdbTreeBook;
class AdbTreeRoot;

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// helper class: just stores all data from an AdbEntry
// ----------------------------------------------------------------------------

class AdbData
{
public:
  // ctor copies data from the entry
  AdbData(const AdbEntry& entry);

  // copy the data back
  void Copy(AdbEntry *entry);

private:
  wxString m_fields[AdbField_Max];
};

// ----------------------------------------------------------------------------
// ADB classes
// ----------------------------------------------------------------------------

enum TreeElement
{
  TreeElement_Invalid,    // bad marker
  TreeElement_Entry,      // a tree leaf
  TreeElement_Group,      // a group, but not an address book
  TreeElement_Book,       // an address book (top level items)
  TreeElement_Root,       // the root of the tree
  TreeElement_Max,        // last from normal ids

  TreeElement_KindMask = 0x0F,   // mask for the kind - the rest are flags
  TreeElement_Clipboard = 0x80   // item on clipboard (not in the tree at all)
};

// common base class for both entries and groups of them
// NB: the size of this object should be as small as possible and there is one
//     AdbTreeElement for each (visible) entry in the address book. That's why
//     several functions which should be virtual are not: we do the dispatching
//     ourselves which allows us to save 4*sizeof(void *) bytes for each item
//     (the size a virtual table for this class would have had)
class AdbTreeElement : public wxTreeItemData
{
public:
  AdbTreeElement(TreeElement kind,
                 const wxString& name,
                 AdbTreeNode *parent,
                 bool onClipboard = FALSE);
  virtual ~AdbTreeElement()
  {
    if ( IsOnClipboard() )
      delete m_data;
    else
      wxASSERT_MSG( !m_data, _T("memory leak in AdbTreeElement") );
  }

  // accessors
  const wxString& GetName() const { return m_name; }
  AdbTreeNode *GetParent() const { return m_parent; }
  wxString GetFullName() const;
  wxString GetPath() const;

    // what kind of item are we?
  bool IsRoot() const { return GetKind() == TreeElement_Root; }
  bool IsBook() const { return GetKind() == TreeElement_Book; }

    // group is anything that can contain entries, not just TreeElement_Group
  bool IsGroup() const { return GetKind() != TreeElement_Entry; }

    // just get the kind - useful for switch()es
  TreeElement GetKind() const
    { return (TreeElement)(m_kind & TreeElement_KindMask); }

    // item is on clipboard if it has this type
  bool IsOnClipboard() const { return (m_kind & TreeElement_Clipboard) != 0; }

  // operations
    // insert the item into the given tree control (under it's parent)
  /* virtual */ void TreeInsert(wxTreeCtrl& tree);
    // copy data from another entry/group - we prefer to use this function
    // explicitly rather than copy ctor because sometimes we need to split the
    // creation of the object and it's initialization.
  /* virtual */ void CopyData(const AdbTreeElement& other);
    // recursively clean dirty flag. Again, this function might be recursive
    // but we decide to dispatch it ourselves as for CopyData
  /* virtual */ void ClearDirtyFlag();

protected:
  TreeElement   m_kind;   // what kind of item we are
  wxString      m_name;   // the name which is the same as label
  AdbTreeNode  *m_parent; // the group which contains us
  AdbData      *m_data;   // our AdbEntry if we don't have parent
};

// an array of pointers to the entries
WX_DEFINE_ARRAY(AdbTreeElement *, ArrayEntries);

// a group of ADB entries
class AdbTreeNode : public AdbTreeElement
{
public:
  // ctors
    // the usual ctor which creates a subgroup of the parent
  AdbTreeNode(const wxString& name,
              AdbTreeNode *parent,
              bool onClipboard = FALSE);
    // a special ctor used by derived classes
  AdbTreeNode() : AdbTreeElement(TreeElement_Invalid, _T(""), 0)
    { m_bWasExpanded = FALSE; m_pGroup = NULL; }

    // dtor deletes all children
  virtual ~AdbTreeNode();

  // accessors
    // access to children
  size_t GetChildrenCount() const { return m_children.Count(); }
  AdbTreeElement *GetChild(size_t n) const { return m_children[n]; }
    // returns TRUE after first call to ExpandFirstTime
  bool WasExpanded() const { return m_bWasExpanded; }

    // the id of the last subgroup (or wxTREE_INSERT_LAST)
    // we need it to be able to insert a new subgroup after the last one, not
    // after all entries (i.e. we want to sort subgroups first)
  void SetLastGroupId(const wxTreeItemId& id) { m_idLastGroup = id; }
  const wxTreeItemId& GetLastGroupId() const { return m_idLastGroup; }

    // return the string of the form "in the group <foo>" or "at the root
    // level of the addressbook <bar>" for diagnostic messages
  wxString GetWhere() const;

  // operations
    // child management
  AdbTreeElement *CreateChild(const wxString& strName, bool bGroup);
  void AddChild(AdbTreeElement *child) { m_children.Add(child); }
  void DeleteChild(AdbTreeElement *child);

    // find child by name (returns NULL if not found)
  virtual AdbTreeElement *FindChild(const wxChar *szName);

    // load all children from the config file (but don't add them to the tree).
    // This function loads only immediate subgroups and entries (and the
    // others will be loaded only when needed, thus minimizing the load time).
  virtual void LoadChildren();

    // loads all subgroups recursively and also loads all entry data (which are
    // also normally loaded only when needed) - this is used to copy the
    // current group on the clipboard (we must copy everything in this case)
  void LoadAllData();

    // return FALSE if has no children, TRUE otherwise
  bool ExpandFirstTime(wxTreeCtrl& tree);

    // delete all entries and reload them
  void Refresh(wxTreeCtrl& tree);

    // copy data recursively from another group
  void CopyData(const AdbTreeNode& other);
    // clear dirty flag on all subgroups and entries
  void ClearDirty();

    // get the corresponding AdbEntryGroup
    // NB: the pointer returned by this function shouldn't be DecRef()'d,
    //     that's why it's not called GetAdbGroup but just AdbGroup
  AdbEntryGroup *AdbGroup() const { return m_pGroup; }

protected:
  // this function will create our associated ADB group if it's not done yet
  void EnsureHasGroup()
  {
    // check that we have an associated ADB group, but notice that the book
    // itself is the same as the root group and the root doesn't have any
    // associated ADB data structure
    if ( (m_pGroup == NULL) && !IsRoot() && !IsBook() ) {
      wxASSERT( IsGroup() );

      // create our AdbEntryGroup
      m_pGroup = GetParent()->AdbGroup()->GetGroup(m_name);
    }
  }

  bool           m_bWasExpanded;
  ArrayEntries   m_children;

  wxTreeItemId   m_idLastGroup;

  AdbEntryGroup *m_pGroup;
};

// an address book corresponds to a disk file
class AdbTreeBook : public AdbTreeNode
{
public:
  AdbTreeBook(AdbTreeRoot *root,
              const wxString& filename,
              AdbDataProvider *pProvider = NULL,
              wxString *pstrProviderName = NULL);
  virtual ~AdbTreeBook();

  // accessors
    // return the file name (NB: not really always a file name...)
  const wxChar *GetName() const { return m_pBook->GetName(); }

  size_t GetNumberOfEntries() const { return m_pBook->GetNumberOfEntries(); }

  AdbBook *GetBook() { return m_pBook; }

  // operations
    // flush
  bool Flush() const { return m_pBook->Flush(); }

protected:
  AdbBook   *m_pBook;
};

// there is only one object of this class: it has all address books as children
class AdbTreeRoot : public AdbTreeNode // must derive from group!
{
public:
  AdbTreeRoot(wxArrayString& astrAdb, wxArrayString& astrProviders);
  virtual ~AdbTreeRoot();

  // accessors
  AdbManager *GetAdbManager() const { return m_pManager; }

  // overriden base class virtuals
  virtual AdbTreeElement *FindChild(const wxChar *szName);
  virtual void LoadChildren();

private:
  wxArrayString& m_astrAdb;       // names of ADBs to load
  wxArrayString& m_astrProviders; // and the names of providers for them

  AdbManager *m_pManager;   // we keep it during all our lifetime

  DECLARE_NO_COPY_CLASS(AdbTreeRoot)
};

// an ADB entry
class AdbTreeEntry : public AdbTreeElement
{
public:
  // type of the field
  enum FieldType
  {
    FieldText,    // a single line of text
    FieldMemo,    // multi-line text field
    FieldList,    // list of strings, as "Additional E-Mails"
    FieldBool,    // yes/no or unknown
    FieldNum,     // can contain only digits
    FieldDate,    // TODO this and the following types are the same as
    FieldURL,     //      text now
    FieldPhone,   //
    FieldMax
  };

  // a structure describing a field
  struct FieldInfo
  {
    const wxChar *label;  // or the caption
    FieldType     type;   // and the type from the enum FieldType abvoe
  };

  // array of field descriptions: should be kept in sync with AdbField enum!
  static FieldInfo ms_aFields[];

  // ctor
  AdbTreeEntry(const wxString& name,
               AdbTreeNode *parent,
               bool onClipboard = FALSE);

  // operations
    // TODO renaming not implemented
  void Rename(const wxString& /* name */) { NOT_IMPLEMENTED; }

    // copy data from another entry
  void CopyData(const AdbTreeEntry& other);
    // mark the data as being clean
  void ClearDirty() { }

    // get the AdbEntry which we represent
  AdbEntry *GetAdbEntry() const
    { return GetParent()->AdbGroup()->GetEntry(m_name); }
};

// this array contains labels for all fields and their types - should be kept
// in sync with AdbField enum!
AdbTreeEntry::FieldInfo AdbTreeEntry::ms_aFields[] =
{
  { gettext_noop("Nick &Name"),         FieldText },
  { gettext_noop("F&ull Name"),         FieldText },
  { gettext_noop("&First Name"),        FieldText },
  { gettext_noop("F&amily Name"),       FieldText },
  { gettext_noop("&Prefix"),            FieldText },
  { gettext_noop("&Title"),             FieldText },
  { gettext_noop("&Organization"),      FieldText },
  { gettext_noop("&Birthday date"),     FieldDate },
  { gettext_noop("&Comments"),          FieldMemo },

  { gettext_noop("&E-Mail"),            FieldText },
  { gettext_noop("&Home Page"),         FieldURL  },
  { gettext_noop("&ICQ"),               FieldNum  },
  { gettext_noop("Prefers &HTML mail"), FieldBool },
  { gettext_noop("&Additional e-mail"
                 " addresses"),         FieldList },

  { gettext_noop("Street &Number"),     FieldText },
  { gettext_noop("&Street"),            FieldText },
  { gettext_noop("&Locality"),          FieldText },
  { gettext_noop("C&ity"),              FieldText },
  { gettext_noop("&Postcode"),          FieldNum  },
  { gettext_noop("&Country"),           FieldText },
  { gettext_noop("PO &Box"),            FieldText },
  { gettext_noop("P&hone"),             FieldPhone },
  { gettext_noop("&Fax"),               FieldPhone },

  { gettext_noop("Street &Number"),     FieldText },
  { gettext_noop("&Street"),            FieldText },
  { gettext_noop("&Locality"),          FieldText },
  { gettext_noop("C&ity"),              FieldText },
  { gettext_noop("&Postcode"),          FieldNum  },
  { gettext_noop("&Country"),           FieldText },
  { gettext_noop("PO &Box"),            FieldText },
  { gettext_noop("P&hone"),             FieldPhone },
  { gettext_noop("&Fax"),               FieldPhone },
};

// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------

class wxAdbTree : public wxTreeCtrl
{
public:
  wxAdbTree(wxAdbEditFrame *frame, wxWindow *parent, long id);
  virtual ~wxAdbTree();

  // select and show the item
  void SelectAndShow(wxTreeItemId id)
  {
    SelectItem(id);
    EnsureVisible(id);
  }

  // event handlers
  void OnChar(wxKeyEvent& event);
  void OnRightDown(wxMouseEvent& event);

  enum ImageListImage
  {
    Library,
    Book,
    Address,
    Opened,
    Closed,
    PalmOSBook, // if changes from 5, must be changed in ProvPalm.h, too!
    BbdbBook // if changes from 5, must be changed in ProvBbdb.cpp, too!
  };

private:
  wxAdbEditFrame *m_frame;

  // the popup menu we use for right mouse click
  class AdbTreeMenu : public wxMenu
  {
  public:
    AdbTreeMenu()
    {
      // FIXME the strings are from wxMenuDefs.cpp - how could we get them
      //       directly from there instead of copying them?
      Append(WXMENU_ADBEDIT_NEW, _("&New entry..."));
      Append(WXMENU_ADBEDIT_DELETE, _("&Delete"));
      Append(WXMENU_ADBEDIT_RENAME, _("&Rename..."));
      AppendSeparator();
      Append(WXMENU_ADBEDIT_CUT, _("Cu&t"));
      Append(WXMENU_ADBEDIT_COPY, _("&Copy"));
      Append(WXMENU_ADBEDIT_PASTE, _("&Paste"));
      AppendSeparator();
      Append(WXMENU_ADBBOOK_PROP, _("Properties..."));
    }

  private:
    DECLARE_NO_COPY_CLASS(AdbTreeMenu)
  } *m_menu;

  DECLARE_EVENT_TABLE()
  DECLARE_NO_COPY_CLASS(wxAdbTree)
};

// ----------------------------------------------------------------------------
// dialogs
// ----------------------------------------------------------------------------

// find dialog (TODO should be modeless...)
class wxADBFindDialog : public wxDialog
{
public:
  // if the dialog is not cancelled, it will store the new values in where and
  // how parameters and also update the text in text control
  wxADBFindDialog(wxWindow *parent, wxPTextEntry *text, int *where, int *how);

  // base class virtuals implemented
  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
  // data
  wxPTextEntry *m_text;
  int          *m_where,
               *m_how;

  // controls
  wxCheckBox *m_checkNick,
             *m_checkFull,
             *m_checkOrg,
             *m_checkEMail,
             *m_checkWWW,
             *m_checkCase,
             *m_checkSub;
  wxTextCtrl *m_textWhat;

  DECLARE_NO_COPY_CLASS(wxADBFindDialog)
};

// ask the user the kind (entry or group) and the name of the object to create
class wxADBCreateDialog : public wxDialog
{
public:
  wxADBCreateDialog(wxWindow *parent, const wxString& strName, bool bGroup);

  // accessors (to be used after call to ShowModal)
  const wxString& GetNewEntryName() const { return m_strName; }
  bool CreateGroup() const { return m_bGroup; }

  // base class virtuals implemented
  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
  wxString m_strName;
  bool     m_bGroup;

  wxTextCtrl *m_textName;
  wxCheckBox *m_checkGroup;

  DECLARE_NO_COPY_CLASS(wxADBCreateDialog)
};

// a simple dialog to show/let the user modify ADB properties
class wxADBPropertiesDialog : public wxDialog
{
public:
  wxADBPropertiesDialog(wxWindow *parent, AdbTreeBook *book);

  virtual bool TransferDataToWindow();
  virtual bool TransferDataFromWindow();

private:
  AdbTreeBook *m_book;

  wxStaticText *m_staticFileSize,
               *m_staticNumEntries;
  wxTextCtrl   *m_textName,
               *m_textFileName,
               *m_textDescription;

  DECLARE_NO_COPY_CLASS(wxADBPropertiesDialog)
};

// ----------------------------------------------------------------------------
// GUI classes
// ----------------------------------------------------------------------------

// ADB view frame
class wxAdbEditFrame : public wxMFrame, public MEventReceiver
{
public:
  wxAdbEditFrame(wxFrame *parent);

  // operations
  void Load();

  // queries
  static bool IsEditorShown() { return ms_nAdbFrames > 0; }

  // callbacks
  bool OnMEvent(MEventData& event);

  void OnMenuCommand(wxCommandEvent&);

  void OnHelp(wxCommandEvent&);

  void OnTreeExpand(wxTreeEvent&);
  void OnTreeCollapse(wxTreeEvent&);
  void OnTreeExpanding(wxTreeEvent&);
  void OnTreeSelect(wxTreeEvent&);

  void OnTextLookupEnter(wxCommandEvent& event);
  void OnTextLookupChange(wxCommandEvent& event);

  void OnUpdateCancel(wxUpdateUIEvent& event);
  void OnUpdateProp(wxUpdateUIEvent& event);
  void OnUpdateCopy(wxUpdateUIEvent& event);
  void OnUpdatePaste(wxUpdateUIEvent& event);
  void OnUpdateDelete(wxUpdateUIEvent& event);
  void OnUpdateExport(wxUpdateUIEvent& event);
  void OnUpdateExportVCard(wxUpdateUIEvent& event);
  void OnUpdateImportVCard(wxUpdateUIEvent& event);

  void OnActivate(wxActivateEvent&);

  // return TRUE if the user can perform the corresponding operation (the menu
  // items are disabled automatically, these functions are for the tree control
  // which has an independent keyboard interface only)
  bool AllowCreate() const { return TRUE;                 }
  bool AllowDelete() const { return !m_current->IsRoot(); }
  bool AllowShowProp() const { return m_current->IsBook(); }

  bool AllowPaste() const { return m_clipboard != NULL; }
  bool AllowCopy() const { return !(m_current->IsRoot() || m_current->IsBook()); }

  // create/delete/rename items (entries, groups, address books...)
  void DoCreateNode();
  void DoDeleteNode(bool bAskConfirmation = TRUE);
  void DoRenameNode();

  // copy/paste the entry or group to/from the "clipboard"
  void DoCopy();
  void DoPaste();

  // undo all changes to the current item
  void DoUndoChanges();

  // find the contents of "lookup" text zone under the root key
  void DoFind();
  // find all entries which match the string under "root" (or the current
  // entry if root == NULL). '*' and '?' are recognized, case sensitive.
  void DoFind(const wxChar *szFindWhat, AdbTreeNode *root = NULL);
  // moves selection to the next item (previously found by DoFind())
  void AdvanceToNextFound();

  // is the given address book already opened (i.e., do we have it in the
  // book tree)?
  bool IsAdbOpened(const wxString& strPath) const
    { return m_astrAdb.Index(strPath) != wxNOT_FOUND; }

  // load the data from file and add the ADB to the tree control using
  // the specified (or any for the default NULL value) provider
  bool OpenAdb(const wxString& strPath,
               AdbDataProvider *pProvider = NULL,
               const wxChar *szProvName = NULL);
  // ask the user for filename and create or open the address book
  bool CreateOrOpenAdb(bool bDoCreate);

  // import/export
    // export the selected group/book
  void ExportAdb();
    // ask the user for filename and import the ADB from this file
  bool ImportAdb();

    // create a vCard from the current entry
  void ExportVCardEntry();
    // create a new entry from a vCard
  bool ImportVCardEntry();

  // show the current ADB statistics
  void DoShowAdbProperties();

  // dtor
  ~wxAdbEditFrame();

private:
  // associate an ADB entry with the notebook panels
  void UpdateNotebook();

  // put the buttons in the lower right corner, one after another but from
  // right to left (so to have correct TAB order they must be created in
  // opposite order)
  void LayoutButtons(wxPanel *panel, size_t nButtons, wxButton *aButtons[]);

  // calculate the (approx.) minimal dimensions of the frame (returns TRUE if
  // done, may return FALSE if called too early, i.e. before the window is
  // fully created)
  bool SetMinSize();

  // expand the specified path returning the pointer to the last item
  // (or NULL if the path is invalid)
  AdbTreeElement *ExpandBranch(const wxString& strEntry);

  // moves the selection to this entry, expanding the tree branches if
  // necessary. returns FALSE if entry not found.
  bool MoveSelection(const wxString& strEntry);

  // set tree item's icon according to it's state
  void SetTreeItemIcon(const wxTreeItemId& id, wxAdbTree::ImageListImage icon);

  // add the new entry/subgroup at the current position to the tree
  void AddNewTreeElement(AdbTreeElement *element);

  // get the current node (the same as selection if the selected item is a
  // group or the parent of the current selection if it isn't)
  AdbTreeNode *GetCurNode() const
  {
    AdbTreeNode *group;
    if ( m_current->IsGroup() )
      group = (AdbTreeNode *)m_current;
    else
      group = m_current->GetParent();

    wxASSERT( group );  // we should never return NULL from here

    return group;
  }

  // get the current entry - only works if the selection is an entry
  AdbTreeEntry *GetEntry() const
  {
    wxASSERT( !m_current->IsGroup() );

    return (AdbTreeEntry *)m_current;
  }

  // save/restore persistent data
  void TransferSettings(bool bSave);
  void SaveSettings();
  void RestoreSettings1();  // 1st step: before the root is created
  void RestoreSettings2();  // 2nd step: after the root tree node was created

  // adds all expanded branches of group and it's children to m_astrBranches
  bool SaveExpandedBranches(AdbTreeNode *group);

  // child controls
  // --------------
  wxAdbNotebook *m_notebook;
  wxAdbTree     *m_treeAdb;
  wxPTextEntry  *m_textKey;

  wxButton *m_btnCancel,
           *m_btnDelete;

  wxImageList *m_pImageList;

  // tree data
  // ---------
  AdbTreeRoot        *m_root;    // the parent of all ADB entries
  AdbTreeElement     *m_current; // item currently edited (never NULL)

  // cut&paste data
  // --------------
  AdbTreeElement *m_clipboard;   // TODO currently only one element/group

  // find data
  // ---------
    // the array holding the result of last "Find..." (as the full names)
  wxArrayString m_aFindResults;
    // the index of current entry in this array for "Find next"
  int m_nFindIndex;
    // the last "Find" argument
  wxString m_strFind;
    // where and how to search (see AdbLookup_xxx constants)?
  int m_FindWhere,
      m_FindHow;
    // was any search already done?
  bool m_bFindDone;

  // persistent data
  // ---------------

  // @@PERS should save several last values for text dialog entries
  wxString m_strLastNewEntry,   // last value of "New..." prompt
           m_strSelection;      // the full name of the entry which is selected
  long  m_bLastNewWasGroup;     // kind of last created entry (entry or group)

  // address books to load on startup
  wxArrayString m_astrAdb;

  // provider names for each of books in m_astrAdb
  wxArrayString m_astrProviders;

  // tree branches to expand on startup
  wxArrayString m_astrBranches;

  // misc data
  // ---------

  void *m_eventNewADB;          // event manager cookie

  static size_t ms_nAdbFrames;  // number of the editor frames currently shown

  DECLARE_EVENT_TABLE()
  DECLARE_NO_COPY_CLASS(wxAdbEditFrame)
};

size_t wxAdbEditFrame::ms_nAdbFrames = 0;

// ----------------------------------------------------------------------------
// notebook pages
// ----------------------------------------------------------------------------

// the ADB notebook
class wxAdbNotebook : public wxNotebook
{
public:
  // ctor and dtor
  wxAdbNotebook(wxPanel *parent, wxWindowID id);
  ~wxAdbNotebook();

  // accessors
    // must be saved?
  bool IsDirty() const { return m_bDirty; }
    // should be called when our data becomes dirty
  void SetDirty() { m_bDirty = TRUE; }

  // operations
    // saves the changes and loads the new entry
  void ChangeData(AdbTreeEntry *pEntry);
    // associates with the new data discarding current changes
  void SetData(AdbTreeEntry *pEntry);
    // save data to associated ADB item
  void SaveChanges();

  // constants
  enum ImageListImage
  {
    General,
    EMail,
    Home,
    Work
  };

private:
  // when there is no selected item we show this message in the box instead
  wxStaticText *m_message;
  wxStaticBox  *m_box;

  AdbTreeEntry *m_pTreeEntry; // the entry we're editing
  AdbEntry     *m_pAdbEntry;  // associated data (may be NULL)
  bool          m_bDirty;     // has the currently selected entry been changed?
  bool          m_bReadOnly;  // is the current entry read-only?

  DECLARE_NO_COPY_CLASS(wxAdbNotebook)
};

WX_DEFINE_ARRAY(wxControl *, ArrayControls);

// the base class for all of them
class wxAdbPage : public wxPanel
{
public:
  wxAdbPage(wxNotebook *notebook, const wxChar *title, int idImage,
            size_t nFirstField, size_t nLastField);

  // notify the notebook that we became dirty
  void SetDirty() { ((wxAdbNotebook *)GetParent())->SetDirty(); }

  // helpers
    // this function will create nCount controls using the information from
    // fields[] array and layout them nicely from the top to the bottom
  void LayoutControls(size_t nCount, ArrayControls& entries,
                      AdbTreeEntry::FieldInfo fields[]);
    // put the label and the control between 'last' and the bottom of the page
  void LayoutBelow(wxControl *control, wxStaticText *label, wxControl *last);

  // overridables
    // transfer the data from ADB item to the page controls
    // (base class version updates the values of all text controls)
  virtual void SetData(const AdbEntry& data);
    // transfer the data from the page to ADB item
    // (base class version saves all text fields)
  virtual void SaveChanges(AdbEntry& data);

  // callbacks
  void OnTextChange(wxCommandEvent&);

protected:
  // helpers
  void SetTopConstraint(wxLayoutConstraints *c, wxControl *last);

  // all the functions should be given translated label!
  wxTextCtrl *CreateMultiLineText(const wxChar *label, wxControl *last);
  wxListBox  *CreateListBox(const wxChar *label, wxControl *last);
  wxCheckBox *CreateCheckBox(const wxChar *label, wxControl *last);
  wxTextCtrl *CreateTextWithLabel(const wxChar *label, long w, wxControl *last);

  ArrayControls  m_aEntries;

  // first and last fields of this page in AdbField enum
  size_t m_nFirstField, m_nLastField;

  DECLARE_EVENT_TABLE()
  DECLARE_NO_COPY_CLASS(wxAdbPage)
};

// name and general information page
class wxAdbNamePage : public wxAdbPage
{
public:
  wxAdbNamePage(wxNotebook *notebook)
    : wxAdbPage(notebook, _T("General"), wxAdbNotebook::General,
                AdbField_NamePageFirst, AdbField_NamePageLast) { }

private:
  DECLARE_NO_COPY_CLASS(wxAdbNamePage)
};

// e-mail address & related things page
class wxAdbEMailPage : public wxAdbPage
{
public:
  wxAdbEMailPage(wxNotebook *notebook)
    : wxAdbPage(notebook, _T("Email"), wxAdbNotebook::EMail,
                AdbField_EMailPageFirst, AdbField_EMailPageLast) { }

  virtual void SetData(const AdbEntry& data);
  virtual void SaveChanges(AdbEntry& data);

  // callbacks
  void OnCheckBox(wxCommandEvent&);
  void OnNewEMail(wxCommandEvent&);
  void OnModifyEMail(wxCommandEvent&);
  void OnDeleteEMail(wxCommandEvent&);

private:
  // just hide the cast
  wxListBox *GetListBox() const
  {
    return (wxListBox *)m_aEntries[AdbField_OtherEMails -
                                   AdbField_EMailPageFirst];
  }

  bool m_bListboxModified;  // any new/deleted/modified entries?

  DECLARE_EVENT_TABLE()
  DECLARE_NO_COPY_CLASS(wxAdbEMailPage)
};

// address page (common base for office address and home address pages)
class wxAdbAddrPage : public wxAdbPage
{
public:
  wxAdbAddrPage(wxNotebook *notebook, const wxChar *title,
                int idImage, bool bOffice);

private:
  DECLARE_NO_COPY_CLASS(wxAdbAddrPage)
};

// office address
class wxAdbOfficeAddrPage : public wxAdbAddrPage
{
public:
  wxAdbOfficeAddrPage(wxNotebook *notebook)
    : wxAdbAddrPage(notebook, _T("Office"), wxAdbNotebook::Work, TRUE) { }

private:
  DECLARE_NO_COPY_CLASS(wxAdbOfficeAddrPage)
};

// home address
class wxAdbHomeAddrPage : public wxAdbAddrPage
{
public:
  wxAdbHomeAddrPage(wxNotebook *notebook)
    : wxAdbAddrPage(notebook, _T("Home"), wxAdbNotebook::Home, FALSE) { }

private:
  DECLARE_NO_COPY_CLASS(wxAdbHomeAddrPage)
};

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

// ID for the menu commands and controls
enum
{
  // controls
  AdbView_Tree = 1000,
  AdbView_Notebook,
  AdbView_Lookup,
    // buttons
  AdbView_New,
  AdbView_Delete,
  AdbView_Cancel,

  // notebook pages' controls
    // e-mail page buttons (must be in order)
  AdbPage_BtnFirst,
  AdbPage_BtnNew = AdbPage_BtnFirst,
  AdbPage_BtnModify,
  AdbPage_BtnDelete,
  AdbPage_BtnLast
};

// notebook pages
enum
{
  Page_General,
  Page_EMail,
  Page_Office,
  Page_Home,
  Page_Max
};

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxAdbEditFrame, wxFrame)
  // menu commands (we take all of them and dispatch them later)
  EVT_MENU(-1, wxAdbEditFrame::OnMenuCommand)
  EVT_TOOL(-1, wxAdbEditFrame::OnMenuCommand)

  // tree events
  EVT_TREE_SEL_CHANGED(AdbView_Tree, wxAdbEditFrame::OnTreeSelect)
  EVT_TREE_ITEM_EXPANDED(AdbView_Tree, wxAdbEditFrame::OnTreeExpand)
  EVT_TREE_ITEM_COLLAPSED(AdbView_Tree, wxAdbEditFrame::OnTreeCollapse)
  EVT_TREE_ITEM_EXPANDING(AdbView_Tree, wxAdbEditFrame::OnTreeExpanding)

  // buttons: they do the same as the menu entries from ADB submenu
  EVT_BUTTON(AdbView_New, wxAdbEditFrame::OnMenuCommand)
  EVT_BUTTON(AdbView_Cancel, wxAdbEditFrame::OnMenuCommand)
  EVT_BUTTON(AdbView_Delete, wxAdbEditFrame::OnMenuCommand)

  // lookup combobox
  EVT_TEXT(AdbView_Lookup, wxAdbEditFrame::OnTextLookupChange)
  EVT_COMBOBOX(AdbView_Lookup, wxAdbEditFrame::OnTextLookupEnter)

  // update UI events
    // enable/disbale anything that can be done only when an entry is
    // selected
  EVT_UPDATE_UI(WXMENU_ADBEDIT_UNDO, wxAdbEditFrame::OnUpdateCancel)
    // enable only if an ADB is selected
  EVT_UPDATE_UI(WXMENU_ADBBOOK_PROP, wxAdbEditFrame::OnUpdateProp)
    // edit operations are only enabled when an item/group are selected
  EVT_UPDATE_UI(WXMENU_ADBEDIT_CUT, wxAdbEditFrame::OnUpdateCopy)
  EVT_UPDATE_UI(WXMENU_ADBEDIT_COPY, wxAdbEditFrame::OnUpdateCopy)
    // paste is only enabled if there is something to paste
  EVT_UPDATE_UI(WXMENU_ADBEDIT_PASTE, wxAdbEditFrame::OnUpdatePaste)
    // enable/disable anything that can be done only when something except
    // root is selected
  EVT_UPDATE_UI(WXMENU_ADBEDIT_DELETE, wxAdbEditFrame::OnUpdateDelete)
  EVT_UPDATE_UI(WXMENU_ADBEDIT_RENAME, wxAdbEditFrame::OnUpdateDelete)
    // exporting is only possible when a group or book is selected
  EVT_UPDATE_UI(WXMENU_ADBBOOK_EXPORT, wxAdbEditFrame::OnUpdateExport)
    // exporting vCard is only possible when an entry is selected
  EVT_UPDATE_UI(WXMENU_ADBBOOK_VCARD_EXPORT, wxAdbEditFrame::OnUpdateExportVCard)
    // and importing only if we have a current group to put it under
  EVT_UPDATE_UI(WXMENU_ADBBOOK_VCARD_IMPORT, wxAdbEditFrame::OnUpdateImportVCard)

  // other
    // need to intercept this to prevent default implementation to give the
    // focus to the toolbar (as it's the first control in our frame)
  EVT_ACTIVATE(wxAdbEditFrame::OnActivate)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAdbTree, wxTreeCtrl)
  EVT_CHAR(wxAdbTree::OnChar)
  EVT_RIGHT_DOWN(wxAdbTree::OnRightDown)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAdbPage, wxPanel)
  // mark the data as dirty when text is changed
  EVT_TEXT(-1, wxAdbPage::OnTextChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxAdbEMailPage, wxAdbPage)
  EVT_CHECKBOX(-1, wxAdbEMailPage::OnCheckBox)

  EVT_BUTTON(AdbPage_BtnNew, wxAdbEMailPage::OnNewEMail)
  EVT_BUTTON(AdbPage_BtnModify, wxAdbEMailPage::OnModifyEMail)
  EVT_BUTTON(AdbPage_BtnDelete, wxAdbEMailPage::OnDeleteEMail)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

// =============================================================================
// implementation
// =============================================================================

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

void ShowAdbFrame(wxFrame *parent)
{
  wxFrame *pAdbFrame = new wxAdbEditFrame(parent);
  pAdbFrame->Show(TRUE);

  mApplication->GetProfile()->writeEntry(MP_SHOWADBEDITOR, TRUE);
}

void AddBookToAdbEditor(const String& adbname, const String& provname)
{
  if ( wxAdbEditFrame::IsEditorShown() )
  {
    // let the ADB editor update itself
    MEventManager::Send(new MEventNewADBData(adbname, provname));
  }
  else
  {
    // write to the profile, the editor will load the books the next time it
    // shows up

    wxConfigBase *conf = mApplication->GetProfile()->GetConfig();
    conf->SetPath(GetAdbEditorConfigPath());

    wxArrayString books;
    RestoreArray(conf, books, aszConfigNames[ConfigName_AddressBooks]);
    books.Add(adbname);
    SaveArray(conf, books, aszConfigNames[ConfigName_AddressBooks]);

    wxArrayString providers;
    RestoreArray(conf, providers, aszConfigNames[ConfigName_AddressBookProviders]);
    providers.Add(provname);
    SaveArray(conf, providers, aszConfigNames[ConfigName_AddressBookProviders]);
  }
}

// -----------------------------------------------------------------------------
// main frame
// -----------------------------------------------------------------------------

// ADB frame constructor
wxAdbEditFrame::wxAdbEditFrame(wxFrame *parent)
              : wxMFrame(_T("adbedit"), parent)
{
  // inc the number of objects alive
  ms_nAdbFrames++;

  // NULL everything
  // ---------------
  m_treeAdb = NULL;
  m_notebook = NULL;
  m_textKey = NULL;
  m_current = NULL;
  m_clipboard = NULL;
  m_pImageList = NULL;
  m_bFindDone = FALSE;
  m_btnCancel =
  m_btnDelete = NULL;

  m_eventNewADB = MEventManager::Register(*this, MEventId_NewADB);
  ASSERT_MSG( m_eventNewADB, _T("ADB editor failed to register with event manager") );

  // create our menu
  // ---------------

  AddFileMenu();                               // file (global) operations
  WXADD_MENU(GetMenuBar(), ADBBOOK, _("&Book"));  // operations on address books
  WXADD_MENU(GetMenuBar(), ADBEDIT, _("&Edit"));  // commands to edit ADB entries
  WXADD_MENU(GetMenuBar(), ADBFIND, _("&Find"));  // searching and moving
  AddHelpMenu();                               // help

  // toolbar and status bar
  // ----------------------

  // standard M toolbar
  CreateMToolbar(this, WXFRAME_ADB);

  // create a status bar with 2 panes
  CreateStatusBar(2);

  // make a panel with some items
  // ----------------------------
  wxPanel *panel = new wxPanel(this, -1);
  wxStaticText *label = new wxStaticText(panel, -1, _("&Lookup:"));
  m_textKey = new wxPTextEntry(ADB_CONFIG_PATH _T("/FindKey"), panel, AdbView_Lookup);
  m_treeAdb = new wxAdbTree(this, panel, AdbView_Tree);
  m_notebook = new wxAdbNotebook(panel, AdbView_Notebook);

  wxButton *buttons[3];
  buttons[2] = new wxButton(panel, AdbView_New, _("&New..."));
  buttons[1] = new wxButton(panel, AdbView_Delete, _("&Delete"));
  buttons[0] = new wxButton(panel, AdbView_Cancel, _("Cancel"));
  m_btnCancel = buttons[0];   // save them for status updates
  m_btnDelete = buttons[1];

  // set up the constraints
  LayoutButtons(panel, WXSIZEOF(buttons), buttons);

  wxLayoutConstraints *c;

  c = new wxLayoutConstraints;
  c->centreY.SameAs(m_textKey, wxCentreY, -2);
  c->height.AsIs();
  c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
  c->width.AsIs();
  label->SetConstraints(c);

  c = new wxLayoutConstraints;
  c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
  c->left.RightOf(label);
  c->right.PercentOf(panel, wxWidth, 40);
  c->height.AsIs();
  m_textKey->SetConstraints(c);

  c = new wxLayoutConstraints;
  c->left.SameAs(panel, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(m_textKey, wxRight);
  c->top.Below(m_textKey,LAYOUT_Y_MARGIN);
  c->bottom.SameAs(panel, wxBottom, LAYOUT_Y_MARGIN);
  m_treeAdb->SetConstraints(c);

  c = new wxLayoutConstraints;
  c->left.RightOf(m_treeAdb, LAYOUT_X_MARGIN);
  c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
  c->top.SameAs(panel, wxTop, LAYOUT_Y_MARGIN);
  c->bottom.Above(buttons[0], -LAYOUT_Y_MARGIN);
  m_notebook->SetConstraints(c);
  m_notebook->Show(FALSE);

  c = new wxLayoutConstraints;
  c->top.SameAs(this, wxTop);
  c->left.SameAs(this, wxLeft);
  c->right.SameAs(this, wxRight);
  c->bottom.SameAs(this, wxBottom);
  panel->SetConstraints(c);

  panel->SetAutoLayout(TRUE);
  SetAutoLayout(TRUE);

  SetMinSize();

  // caption and icon
  // ----------------
  SetTitle(_("Address Book Editor"));
  SetIcon(ICON(_T("adbedit")));

  // final initializations
  // ---------------------

  RestoreSettings1();

  // create the root item
  m_root = new AdbTreeRoot(m_astrAdb, m_astrProviders);
  m_root->TreeInsert(*m_treeAdb);

  // m_current must always be !NULL
  m_current = m_root;

  RestoreSettings2();
}

void wxAdbEditFrame::TransferSettings(bool bSave)
{
  #define TRANSFER_STRING(var, i)           \
    if ( bSave )                            \
      conf->Write(aszConfigNames[i], var);  \
    else                                    \
      var = conf->Read(aszConfigNames[i], var)

  #define TRANSFER_INT(var, i)                    \
    if ( bSave )                                  \
      conf->Write(aszConfigNames[i], (long)var);  \
    else                                          \
      var = conf->Read(aszConfigNames[i], 0l)

  #define TRANSFER_BOOL(var, i)                   \
    if ( bSave )                                  \
      conf->Write(aszConfigNames[i], var);        \
    else                                          \
      var = conf->Read(aszConfigNames[i], 0l) != 0

  #define TRANSFER_ARRAY(var, i)                  \
    if ( bSave )                                  \
      SaveArray(conf, var, aszConfigNames[i]);    \
    else                                          \
      RestoreArray(conf, var, aszConfigNames[i])

  wxConfigBase *conf = mApplication->GetProfile()->GetConfig();
  conf->SetPath(GetAdbEditorConfigPath());

  TRANSFER_STRING(m_strLastNewEntry, ConfigName_LastNewEntry);
  TRANSFER_STRING(m_strSelection, ConfigName_TreeSelection);

  TRANSFER_INT(m_bLastNewWasGroup, ConfigName_LastNewWasGroup);
  TRANSFER_INT(m_FindWhere, ConfigName_LastLookup);
  TRANSFER_INT(m_FindHow, ConfigName_FindOptions);

  TRANSFER_ARRAY(m_astrAdb, ConfigName_AddressBooks);
  TRANSFER_ARRAY(m_astrProviders, ConfigName_AddressBookProviders);
  TRANSFER_ARRAY(m_astrBranches, ConfigName_ExpandedBranches);

  // keep your namespace clean
  #undef TRANSFER_STRING
  #undef TRANSFER_BOOL
  #undef TRANSFER_INT
  #undef TRANSFER_ARRAY
}

void wxAdbEditFrame::SaveSettings()
{
  // save all expanded branches
  m_astrBranches.Empty();
  SaveExpandedBranches(m_root);

  TransferSettings(TRUE /* save */);
}

// "undo" SaveSettings()
void wxAdbEditFrame::RestoreSettings1()
{
  TransferSettings(FALSE /* load */);

  if ( m_FindWhere == 0 ) {
    // look at least somewhere...
    m_FindWhere = AdbLookup_NickName;
  }

  // if there are no address books at all, load at least the autocollect book
  if ( !m_astrAdb.Count() ) {
    String bookAutoCollect = READ_APPCONFIG_TEXT(MP_AUTOCOLLECT_ADB);
    if ( !wxIsAbsolutePath(bookAutoCollect) ) {
      bookAutoCollect.Prepend(mApplication->GetLocalDir() + _T('/'));
    }
    m_astrAdb.Add(bookAutoCollect);

    // it should be empty anyhow, but just in case
    m_astrProviders.Empty();
    AdbDataProvider *provDef = AdbDataProvider::GetNativeProvider();
    String nameDef;
    if ( provDef ) {
      nameDef = provDef->GetProviderName();

      provDef->DecRef();
    }
    else {
      wxFAIL_MSG( _T("no default ADB provider?") );
    }

    m_astrProviders.Add(nameDef);

    // under Unix, use the ADB for the users from /etc/passwd
#ifdef OS_UNIX
    AdbDataProvider *provPasswd =
      AdbDataProvider::GetProviderByName(_T("PasswdDataProvider"));
    if ( provPasswd )
    {
      m_astrAdb.Add(_T(""));  // no name for this provider
      m_astrProviders.Add(provPasswd->GetProviderName());

      provPasswd->DecRef();
    }
#endif // OS_UNIX
  }

  // now, m_astrAdb contains all previously opened ADBs: then copy the ones
  // which really exist to a temporary array and assign it to m_astrAdb
  wxArrayString astrAdb, astrProviders;
  bool bAllAdbOk = TRUE;
  wxString strFile, strProv;
  size_t nCountAdb = m_astrAdb.Count();

  // there should be one provider name for each address book
  if ( nCountAdb != m_astrProviders.Count() ) {
    wxLogDebug(_T("different number of address books and providers!"));

    // try to correct it somehow (our method is not the best, but it's simple
    // and we really don't know what went wrong - this can only happen if the
    // config file was hand (and wrongly) edited)
    m_astrProviders.Empty();
    for ( size_t n = 0; n < nCountAdb; n++ ) {
      // empty string means that the provider is unknown
      m_astrProviders.Add(wxGetEmptyString());
    }
  }

  AdbDataProvider *pProvider;
  for ( size_t nAdb = 0; nAdb < nCountAdb; nAdb++ ) {
    strFile = m_astrAdb[nAdb];
    strProv = m_astrProviders[nAdb];
    // GetProviderByName would return NULL anyhow, but why call it in the
    // first place?
    if ( wxIsEmpty(strProv) )
      pProvider = NULL;
    else
      pProvider = AdbDataProvider::GetProviderByName(strProv);

    if ( pProvider ) {
      // we need to be able to open it at least in read only mode, this doesn't
      // mean that we actually open it in it, i.e. it will still be read/write
      // for the ADB providers which support it
      if ( pProvider->TestBookAccess(strFile,
                                     AdbDataProvider::Test_OpenReadOnly) ||
            pProvider->TestBookAccess(strFile,
                                      AdbDataProvider::Test_Create) ) {
        astrProviders.Add(strProv);
        astrAdb.Add(strFile);
      }
      else {
        wxLogWarning(_("Address book '%s' couldn't be opened:\n"
                       "this format is not supported."),
                     strFile.c_str());
        bAllAdbOk = FALSE;
      }
    }
    else {
      // no provider, so no way to check if the adb name is valid or not:
      // assume it is
      astrProviders.Add(strProv);
      astrAdb.Add(strFile);
    }

    SafeDecRef(pProvider);
  }

  m_astrAdb = astrAdb;
  m_astrProviders = astrProviders;

  if ( !bAllAdbOk )
    wxLogWarning(_("Not all address books could be reopened."));
}

void wxAdbEditFrame::RestoreSettings2()
{
  // expand all previously expanded tree branches
  bool bAllBranchesOk = TRUE;
  size_t nCountBranches = m_astrBranches.Count();

  AdbTreeElement *current;
  for ( size_t nBranch = 0; nBranch < nCountBranches; nBranch++ ) {
    current = ExpandBranch(m_astrBranches[nBranch]);
    if ( current == NULL ) {
      // it didn't find it
      bAllBranchesOk = FALSE;
    }
    else {
      // ExpandBranch won't expand the group itself
      wxASSERT( current->IsGroup() );

      m_treeAdb->Expand(current->GetId());
    }
  }

  // if we are run for the first time, expand at least the root level to let the
  // user see that there is something in the tree
  if ( !nCountBranches )
    m_treeAdb->Expand(m_treeAdb->GetRootItem());

  // don't give the message if some address books are missing
  if ( !bAllBranchesOk )
    wxLogWarning(_("Not all tree branches could be reopened."));

  // select the element which had the selection last time
  if ( !m_strSelection.IsEmpty() )
    MoveSelection(m_strSelection);
}

void wxAdbEditFrame::UpdateNotebook()
{
  m_notebook->ChangeData(m_current->IsGroup() ? NULL : GetEntry());
}

void wxAdbEditFrame::LayoutButtons(wxPanel *panel,
                                   size_t nButtons,
                                   wxButton *aButtons[])
{
  size_t n;

  // first determine the longest button caption
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long width, widthMax = 0;
  for ( n = 0; n < nButtons; n++ ) {
    dc.GetTextExtent(aButtons[n]->GetLabel(), &width, NULL);
    if ( width > widthMax )
      widthMax = width;
  }

  widthMax += 10; // looks better like this

  // now layout them
  wxLayoutConstraints *c;
  for ( n = 0; n < nButtons; n++ ) {
    c = new wxLayoutConstraints;
    if ( n == 0 )
      c->right.SameAs(panel, wxRight, LAYOUT_X_MARGIN);
    else
      c->right.SameAs(aButtons[n - 1], wxLeft, LAYOUT_X_MARGIN);
    c->bottom.SameAs(panel, wxBottom, 2*LAYOUT_Y_MARGIN);
    c->width.Absolute(widthMax);
    c->height.AsIs();

    aButtons[n]->SetConstraints(c);
  }
}

bool wxAdbEditFrame::OpenAdb(const wxString& strPath,
                             AdbDataProvider *pProvider,
                             const wxChar *szProvName)
{
  // check that we don't already have it
  if ( IsAdbOpened(strPath) ) {
    wxLogError(_("The address book '%s' is already opened."), strPath.c_str());

    return FALSE;
  }

  // loading of a big file might take quite some time
  wxBeginBusyCursor();

  wxString strProv = szProvName;
  AdbTreeBook *adb = new AdbTreeBook(m_root, strPath, pProvider, &strProv);

  m_astrAdb.Add(adb->GetBook()->GetFileName());
  m_astrProviders.Add(strProv);

  if ( m_root->WasExpanded() )
    adb->TreeInsert(*m_treeAdb);
  //else: will be inserted when expanded for the first time

  // make sure the root can be expanded
  if ( m_root->GetChildrenCount() == 1 ) {
    // had no children before and might have had m_children = 0
    m_treeAdb->SetItemHasChildren(m_root->GetId());
  }

  // and do expand it!
  m_treeAdb->Expand(m_root->GetId());

  m_treeAdb->SelectItem(adb->GetId());

  // currently, we always succeed because even if the file doesn't exist
  // we create it -- should it be changed?

  wxEndBusyCursor();

  return TRUE;
}

void wxAdbEditFrame::AddNewTreeElement(AdbTreeElement *element)
{
  wxCHECK_RET( element && element->GetParent(),
               _T("bad parameter in AddNewTreeElement") );

  AdbTreeNode *parent = element->GetParent();

  // refresh the tree control
  if ( parent->WasExpanded() )
    element->TreeInsert(*m_treeAdb);
  //else: will be added when it's expanded for the first time

  // ensure that it may be expanded
  m_treeAdb->SetItemHasChildren(parent->GetId());
}

void wxAdbEditFrame::DoCreateNode()
{
  // only the address books can be created at top level
  if ( m_current->IsRoot() ) {
    CreateOrOpenAdb(TRUE /* bDoCreate */);
    return;
  }

  // creating an entry or a group
  AdbTreeNode *group = GetCurNode();

ask_name:
  wxLog *log = wxLog::GetActiveTarget();
  if ( log != NULL )
    log->Flush();

  // block outside which wxADBCreateDialog doesn't exist
  {
    wxADBCreateDialog dlg(this, m_strLastNewEntry, m_bLastNewWasGroup != 0);
    if ( dlg.ShowModal() != wxID_OK ) {
      wxLogStatus(this, _("New entry creation cancelled."));

      return;
    }

    // remember the type of new entry chosen to reuse it the next time
    m_strLastNewEntry = dlg.GetNewEntryName();
    m_bLastNewWasGroup = dlg.CreateGroup();
  }

  // prepare strings for diagnostic messages
  wxString strWhere, strGroup = group->GetWhere();
  wxString strWhat = wxGetTranslation(m_bLastNewWasGroup ? _T("group") : _T("entry"));

  // first check that it doesn't already exist
  wxASSERT( !m_strLastNewEntry.IsEmpty() ); // don't add empty entries
  if ( group->FindChild(m_strLastNewEntry) ) {
    wxLogError(_("%s '%s' already exists %s."),
               strWhat.c_str(), m_strLastNewEntry.c_str(), strWhere.c_str());
    goto ask_name;
  }

  // do create it
  AdbTreeElement *element = group->CreateChild(m_strLastNewEntry,
                                               m_bLastNewWasGroup != 0);
  if ( element == NULL ) {
    wxLogError(_("Can't create %s named '%s' %s."),
               strWhat.c_str(), m_strLastNewEntry.c_str(), strWhere.c_str());
    goto ask_name;
  }

  // add to the tree
  AddNewTreeElement(element);

  // and focus it
  m_treeAdb->SelectItem(element->GetId());

  wxLogStatus(this, _("Created new %s '%s' %s."),
              strWhat.c_str(), m_strLastNewEntry.c_str(), strWhere.c_str());
}

void wxAdbEditFrame::DoDeleteNode(bool bAskConfirmation)
{
  wxCHECK_RET( !m_current->IsRoot(), _T("command should be disabled") );

  AdbTreeNode *parent = m_current->GetParent();
  wxASSERT( parent != NULL ); // impossible if !m_current->IsRoot()

  wxString strName, strWhat;
  if ( m_current->IsBook() ) {
    // FIXME should deleting the address book also delete the file??

    strWhat = _("Address book");

    AdbTreeBook *adbBook = (AdbTreeBook *)m_current;
    strName = adbBook->GetBook()->GetFileName();

    // find the book index in m_astrAdb array
    int nIndex;

    // FIXME should keep the name format for this ADB somewhere instead of
    //       just assuming that anything is a file name!
    if ( strName.empty() ) {
      // FIXME and what if we have several books without name??
      nIndex = m_astrAdb.Index(strName);
    }
    else {
      // for file based books we have to take account of the fact that either
      // string may be relative or absolute filename
      wxASSERT_MSG( wxIsAbsolutePath(strName), _T("book name should be absolute") );

#ifdef __WXMSW__
      strName.Replace("\\", "/");
#endif

      nIndex = wxNOT_FOUND;
      size_t count = m_astrAdb.Count();
      for ( size_t n = 0; n < count; n++ ) {
        wxString bookname = m_astrAdb[n];
        if ( !wxIsAbsolutePath(bookname) ) {
          bookname = mApplication->GetLocalDir() + _T('/') + bookname;
        }
#ifdef __WXMSW__
        bookname.Replace("\\", "/");
#endif
        if ( strName == bookname ) {
          nIndex = n;
          break;
        }
      }
    }

    if ( nIndex == wxNOT_FOUND )
    {
      // this is never supposed to happen
      wxFAIL_MSG( _T("deleting book which isn't opened??") );

      return;
    }

    m_astrAdb.Remove((size_t)nIndex);
    m_astrProviders.Remove((size_t)nIndex);
  }
  else {
    // it's a normal entry or group
    strWhat = wxGetTranslation(m_current->IsGroup() ? _T("group") : _T("entry"));
    strName = m_current->GetName();

    if ( bAskConfirmation ) {
      // ask confirmation

      // if configPath is not empty, the user can disable the message box
      // (suppressing it completely for the next time). As it's dangerous to
      // delete groups without confirmation, we only enable this for simple
      // entries, not groups.
      const MPersMsgBox *msgbox = m_current->IsGroup()
                                    ? NULL
                                    : M_MSGBOX_ADB_DELETE_ENTRY;

      // construct the message
      wxString msg;
      msg.Printf(_("Really delete the %s '%s'?"),
                 strWhat.c_str(), strName.c_str());
      if ( !MDialog_YesNoDialog(msg, this,
                                _("Address book editor"),
                                M_DLG_NO_DEFAULT,
                                msgbox) ) {
        wxLogStatus(this, _("Cancelled: '%s' not deleted."),
                    m_current->GetName().c_str());
        return;
      }
    }
  }

  // remove from the parent group
  wxTreeItemId id = m_current->GetId();
  parent->DeleteChild(m_current);
  m_current = parent;

  // after the selected item is deleted the selection should move to the next
  // one
  wxTreeItemId idNewSel;
  if ( id == m_treeAdb->GetSelection() ) {
    // if there is no next sibling after this item, we go to the sibling of the
    // parent and further upwards the tree if needed
    wxTreeItemId id2 = id;
    while ( id2.IsOk() ) {
      idNewSel = m_treeAdb->GetNextSibling(id2);
      if ( idNewSel.IsOk() ) {
        // ok, found new selection
        break;
      }

      // move upwards
      id2 = m_treeAdb->GetItemParent(id2);
    }
  }

  // remove from the tree
  m_treeAdb->Delete(id);

  if ( idNewSel.IsOk() ) {
    m_treeAdb->SelectAndShow(idNewSel);
  }

  strWhat[0u] = toupper(strWhat[0u]);
  wxLogStatus(this, _("%s '%s' deleted."), strWhat.c_str(), strName.c_str());
}

void wxAdbEditFrame::DoRenameNode()
{
  NOT_IMPLEMENTED;  // TODO rename: probably must delete and recreate it
}

void wxAdbEditFrame::AdvanceToNextFound()
{
  size_t nCount = m_aFindResults.Count();
  if ( nCount == 0 )
    wxLogWarning(_("Cannot find any matches for '%s'."), m_strFind.c_str());
  else {
    if ( m_nFindIndex == -1 ) {
      // called for the first time (for this search)
      m_nFindIndex = 0;
      wxLogStatus(this, _("Search for '%s' found %d entries."),
                  m_strFind.c_str(), nCount);
    }
    else if ( (size_t)++m_nFindIndex == nCount ) {
      wxLogStatus(this, _("Search wrapped to the beginning."));
      m_nFindIndex = 0;
    }
    else {
      // remove the string which might have been left since the last time we
      // were called
      SetStatusText(_T(""));
    }

    MoveSelection(m_aFindResults[m_nFindIndex]);
  }
}

// finds all matches for m_textKey control contents
void wxAdbEditFrame::DoFind()
{
  m_aFindResults.Empty();
  m_nFindIndex = -1;
  m_strFind = m_textKey->GetValue();
  DoFind(m_strFind, m_root);
  m_bFindDone = TRUE;
  AdvanceToNextFound();
}

// find all entries which match the given string
void wxAdbEditFrame::DoFind(const wxChar *szFindWhat, AdbTreeNode *root)
{
  // start from the current group if root is not specified
  if ( root == NULL )
    root = GetCurNode();

  if ( !root->WasExpanded() )
    root->LoadChildren();

  // depth first search the tree recursively calling ourselves
  size_t nCount = root->GetChildrenCount();
  for ( size_t n = 0; n < nCount; n++ ) {
    AdbTreeElement *current = root->GetChild(n);
    if ( current->IsGroup() )
      DoFind(szFindWhat, (AdbTreeNode *)current);
    else {
      AdbEntry *pEntry = ((AdbTreeEntry *)current)->GetAdbEntry();

      if ( pEntry->Matches(szFindWhat, m_FindWhere, m_FindHow) ) {
        m_aFindResults.Add(current->GetFullName());
      }

      pEntry->DecRef();
    }
  }
}

// pressing cancel button undoes all changes to the current entry
// FIXME perhaps it should undo all changes to the current page only?
void wxAdbEditFrame::DoUndoChanges()
{
  wxCHECK_RET( !m_current->IsGroup(), _T("command should be disabled") );

  // the IncRef() done by GetData() compensated with DecRef() in SetData()
  m_notebook->SetData(GetEntry());

  wxLogStatus(this, _("Changes to '%s' undone"), m_current->GetName().c_str());
}

bool wxAdbEditFrame::OnMEvent(MEventData& d)
{
   if ( d.GetId() == MEventId_NewADB )
   {
      MEventNewADBData& data = (MEventNewADBData &)d;

      // we need to get the full address book name from the "user name"
      wxString adbname = data.GetAdbName(),
               provname = data.GetProviderName();

      AdbBook *adbBook = NULL;
      AdbDataProvider *provider = AdbDataProvider::GetProviderByName(provname);

      {
        AdbManager_obj adbManager;
        if ( adbManager )
        {
          adbBook = adbManager->CreateBook(adbname, provider);
        }
      }

      if ( adbBook )
      {
        adbname = adbBook->GetFileName();
      }

      SafeDecRef(adbBook);

      // add the newly created ADB to the tree
      if ( !IsAdbOpened(adbname) )
      {
        OpenAdb(adbname, provider, provider->GetProviderName());
      }

      SafeDecRef(provider);
   }

   return true;
}

// dispatch menu or toolbar command
void wxAdbEditFrame::OnMenuCommand(wxCommandEvent& event)
{
  switch ( event.GetId() ) {
    case WXMENU_ADBBOOK_NEW:
      CreateOrOpenAdb(TRUE /* create */);
      break;

    case WXMENU_ADBBOOK_OPEN:
      CreateOrOpenAdb(FALSE /* open */);
      break;

    case WXMENU_ADBBOOK_IMPORT:
      ImportAdb();
      break;

    case WXMENU_ADBBOOK_EXPORT:
      ExportAdb();
      break;

    case WXMENU_ADBBOOK_VCARD_IMPORT:
      ImportVCardEntry();
      break;

    case WXMENU_ADBBOOK_VCARD_EXPORT:
      ExportVCardEntry();
      break;

    case WXMENU_ADBBOOK_PROP:
      DoShowAdbProperties();
      break;

    case WXMENU_ADBEDIT_NEW:
    case AdbView_New:
      DoCreateNode();
      break;

    case WXMENU_ADBEDIT_DELETE:
    case AdbView_Delete:
      DoDeleteNode();
      break;

    case WXMENU_ADBEDIT_RENAME:
      DoRenameNode();
      break;

    case WXMENU_ADBEDIT_UNDO:
    case AdbView_Cancel:
      DoUndoChanges();
      break;

    case WXMENU_HELP_CONTEXT:
      OnHelp(event);
      break;

    case WXMENU_ADBFIND_GOTO:
      {
        wxString strGoto;
        if ( MInputBox(&strGoto, _("Go to entry/group"),
                       _("Full entry &name:"), this, _T("GoTo")) ) {
          MoveSelection(strGoto);
        }
      }
      break;

    case WXMENU_ADBEDIT_CUT:
      DoCopy();
      DoDeleteNode(FALSE /* don't ask for confirmation */);
      break;

    case WXMENU_ADBEDIT_COPY:
      DoCopy();
      break;

    case WXMENU_ADBEDIT_PASTE:
      DoPaste();
      break;

    case WXMENU_ADBFIND_NEXT:
      // if a search was already done, advance to the next match, otherwise
      // do the same thing as "Find" would have done
      if ( m_bFindDone ) {
        AdvanceToNextFound();
        break;
      }
      //else: fall through

    case WXMENU_ADBFIND_FIND:
      {
        wxADBFindDialog dlg(this, m_textKey, &m_FindWhere, &m_FindHow);
        if ( dlg.ShowModal() == wxID_OK )
          DoFind();
      }
      break;

#ifdef DEBUG
    case WXMENU_ADBBOOK_FLUSH:
      if ( m_current->IsBook() ) {
         AdbTreeBook *book = (AdbTreeBook *)m_current;
         wxString name = book->GetName();
         if ( !book->Flush() )
            wxLogError(_T("Couldn't flush book '%s'!"), name.c_str());
         else
            wxLogStatus(this, _T("Book '%s' flushed."), name.c_str());
      }
      else {
         wxLogError(_T("Select a book to flush"));
      }
      break;
#endif // debug

    default:
      wxMFrame::OnMenuCommand(event.GetId());
  }
}

bool wxAdbEditFrame::CreateOrOpenAdb(bool bDoCreate)
{
  // what kind of ADB to create, i.e. which provider to use?

  size_t nProvCount = 0;
  AdbDataProvider::AdbProviderInfo *info = AdbDataProvider::ms_listProviders;
  while ( info ) {
    if ( info->bSupportsCreation ) {
      nProvCount++;
    }

    info = info->pNext;
  }

  wxASSERT( nProvCount > 0 ); // no providers supporting creation??

  // now we now how many of them we have, so we can allocate memory
  wxString *aPrompts = new wxString[nProvCount];

  info = AdbDataProvider::ms_listProviders;
  size_t nProv = 0;
  while ( info ) {
    if ( info->bSupportsCreation ) {
      aPrompts[nProv++] = info->szFmtName;
    }

    info = info->pNext;
  }

  int nChoice;
  if ( nProvCount == 1 ) {
    // don't propose the user to make a choice between 1 possibility :-)
    nChoice = 0;
  }
  else {
   nChoice = wxGetSingleChoiceIndex
              (
                 bDoCreate ?
                 _("Which kind of address book do you want to create?")
                 : _("Which kind of address book do you want to open?"),
                 bDoCreate ?
                 _("Creating new address book")
                 :_("Opening an address book"),
                nProvCount, aPrompts,
                this
              );
  }

  delete [] aPrompts;

  if ( nChoice == -1 ) {
    // cancelled
    return FALSE;
  }

  // find the adb name format for this provider
  wxASSERT( (size_t)nChoice < nProvCount );
  info = AdbDataProvider::ms_listProviders;
  for ( nProv = 0; nProv != (size_t)nChoice; nProv++ ) {
    while ( !info->bSupportsCreation ) {
      // skip the providers we didn't put into aPrompts array
      info = info->pNext;
    }

    info = info->pNext;
  }

  wxString strAdbName;
  wxString strTitle;
  if(bDoCreate)
     strTitle = _("Create an address book");
  else
     strTitle = _("Open an address book");
  switch ( info->nameFormat ) {
    case AdbDataProvider::Name_No:
      // the ADB name is not significant, so empty will do as well as any other
      break;

    case AdbDataProvider::Name_File:
      {
        // ask for the file name
        strAdbName = wxPFileSelector
                     (
                      ADB_CONFIG_PATH _T("/AdbFile"),
                      strTitle,
                      READ_APPCONFIG_TEXT(MP_USERDIR),
                      _T("M.adb"),
                      _T("adb"),
                      _("Address books (*.adb)|*.adb|All files (*.*)|*.*"),
                      wxHIDE_READONLY | (bDoCreate ? 0 : wxFILE_MUST_EXIST),
                      this
                     );

        if ( strAdbName.IsEmpty() ) {
           // cancelled by user
           return FALSE;
        }
      }
      break;

    case AdbDataProvider::Name_String:
      {
        wxString strMsg = _("Enter the address book name");
        if ( !MInputBox(&strAdbName, strTitle, strMsg, this, _T("LastAdbName")) )
          return FALSE;
      }
      break;

    default:
      wxFAIL_MSG(_T("unknown ADB name format"));
  }

  AdbDataProvider *pProvider = info->CreateProvider();

  bool ok = OpenAdb(strAdbName, pProvider, info->szName);

  if ( ok ) {
     // the book is in the cache, it won't be really recreated
     AdbManager_obj adbManager;

     AdbBook *book = adbManager->CreateBook(strAdbName, pProvider);

     if ( book ) {
        if ( !book->Flush() ) {
           wxLogWarning(_("Address book '%s' was created, but could not "
                          "be flushed. It might be unaccessible until "
                          "the program is restarted."), strAdbName.c_str());
        }

        book->DecRef();
     }
     else {
        wxFAIL_MSG(_T("book should be in the cache if it was created"));
     }
  }
  SafeDecRef(pProvider);

  return ok;
}

void wxAdbEditFrame::ExportAdb()
{
  wxCHECK_RET( m_current->IsGroup() && !m_current->IsRoot(),
               _T("command should be disabled") );

  if ( AdbShowExportDialog(*(GetCurNode()->AdbGroup())) )
  {
    wxLogStatus(this, _("Successfully exported address book data."));
  }
  else
  {
    wxLogStatus(this, _("Address book export abandoned."));
  }
}

bool wxAdbEditFrame::ImportAdb()
{
  wxString adbname;
  bool ok = AdbShowImportDialog(this, &adbname);

  if ( ok )
  {
    wxLogStatus(this, _("Address book successfully imported into book '%s'."),
                adbname.c_str());
  }
  else
  {
    wxLogStatus(this, _("Address book import abandoned."));
  }

  return ok;
}

void wxAdbEditFrame::ExportVCardEntry()
{
  wxCHECK_RET( !m_current->IsGroup(), _T("command should be disabled") );

  AdbExporter *exporter = AdbExporter::GetExporterByName(_T("AdbVCardExporter"));
  if ( !exporter )
  {
    wxLogError(_("Sorry, vCard exporting is unavailable."));

    return;
  }

  // ask the user for the file name
  wxString filename = wxPFileSelector
                      (
                        ADB_CONFIG_PATH _T("/AdbVCardFile"),
                        _("Choose the name of vCard file"),
                        READ_APPCONFIG_TEXT(MP_USERDIR),
                        _T("vcard.vcf"),
                        _T("vcf"),
                        _("vCard files (*.vcf)|*.vcf|All files (*.*)|*.*"),
                        wxHIDE_READONLY,
                        this
                      );
  if ( !!filename )
  {
    AdbEntry *entry = ((AdbTreeEntry *)m_current)->GetAdbEntry();
    if ( exporter->Export(*entry, filename) )
    {
      wxLogStatus(this, _("Successfully exported address book data to the file '%s'."),
                  filename.c_str());
    }
    else
    {
      wxLogStatus(this, _("Address book export failed."));
    }

    entry->DecRef();
  }
  //else: cancelled by user

  exporter->DecRef();
}

bool wxAdbEditFrame::ImportVCardEntry()
{
  // check that we have a group to import it under
  wxCHECK_MSG( GetCurNode() && GetCurNode()->AdbGroup(), FALSE,
               _T("should be disabled as there is no current group") );

  // check that we have the importer for vCards
  AdbImporter *importer = AdbImporter::GetImporterByName(_T("AdbVCardImporter"));
  if ( !importer )
  {
    wxLogError(_("Sorry, importing vCards is unavailable."));

    return FALSE;
  }

  // choose the file
  wxString filename = wxPFileSelector
                      (
                        ADB_CONFIG_PATH _T("/AdbVCardFile"),
                        _("Choose the name of vCard file"),
                        READ_APPCONFIG_TEXT(MP_USERDIR),
                        _T("vcard.vcf"),
                        _T("vcf"),
                        _("vCard files (*.vcf)|*.vcf|All files (*.*)|*.*"),
                        wxHIDE_READONLY | wxFILE_MUST_EXIST,
                        this
                      );

  if ( !!filename )
  {
    AdbTreeNode *node = GetCurNode();
    if ( AdbImport(filename, node->AdbGroup(), importer) )
    {
      // refresh the group to show the new entry
      node->Refresh(*m_treeAdb);

      return TRUE;
    }
  }
  //else: cancelled by user

  return FALSE;
}

void wxAdbEditFrame::DoShowAdbProperties()
{
  wxCHECK_RET( m_current->IsBook(), _T("command should be disabled") );

  AdbTreeBook *book = (AdbTreeBook *)m_current;
  wxADBPropertiesDialog dlg(this, book);
  if ( dlg.ShowModal() == wxID_OK ) {
    // update the tree label (it may have been changed)
    m_treeAdb->SetItemText(book->GetId(), book->GetName());
  }
}

void wxAdbEditFrame::DoCopy()
{
  AdbTreeNode *parent = m_current->GetParent();
  wxCHECK_RET( parent && parent != m_root, _T("command should be disabled") );

  // first of all, delete the previous clipboard's contents
  delete m_clipboard;

  if ( m_current->IsGroup() ) {
    AdbTreeNode *group = GetCurNode();
    // it should be the same if m_current is a group
    wxASSERT( (AdbTreeElement *)group == m_current );

    // the problem is that if we're copying a group, it's children might not
    // yet be loaded - so we must do it now
    group->LoadAllData();

    m_clipboard = new AdbTreeNode(group->GetName(),
                                  NULL,    // no parent
                                  TRUE);   // on clipboard
  }
  else {
    // just one entry to copy
    m_clipboard = new AdbTreeEntry(m_current->GetName(),
                                   NULL,    // no parent
                                   TRUE);   // on clipboard
  }

  // save data to clipboard
  m_clipboard->CopyData(*m_current);
  m_clipboard->ClearDirtyFlag();  // clipboard shouldn't be saved!
}

void wxAdbEditFrame::DoPaste()
{
  // we must have something to paste and we can't paste address books (yet?)
  wxCHECK_RET( m_clipboard != NULL && !m_current->IsRoot(),
               _T("command should be disabled") );

  AdbTreeNode *group = GetCurNode();

  // can't paste if an item with the same name already exists
  if ( group->FindChild(m_clipboard->GetName()) ) {
    wxLogError(_("Cannot paste entry '%s' %s: an entry with\n"
                 "the same name already exists."),
               m_clipboard->GetName().c_str(), group->GetWhere().c_str());
    return;
  }

  // first add it to our internal data
  AdbTreeElement *copy = group->CreateChild(m_clipboard->GetName(),
                                            m_clipboard->IsGroup());
  if ( copy == NULL ) {
    wxLogError(_("Cannot paste the clipboard contents here."));
    return;
  }

  copy->CopyData(*m_clipboard);

  // add it to the tree
  AddNewTreeElement(copy);
}

void wxAdbEditFrame::OnHelp(wxCommandEvent&)
{
   mApplication->Help(MH_ADB_FRAME, this);
}

void wxAdbEditFrame::OnTreeSelect(wxTreeEvent& event)
{
  wxTreeItemId id = event.GetItem();

  // FIXME: I don't know how is this possible but under Windows when we create
  //        a new item in a group which had never been expanded yet the data
  //        may be NULL
  wxTreeItemData *data = m_treeAdb->GetItemData(id);
  if ( !data ) {
    event.Skip();
    return;
  }

  m_current = (AdbTreeNode *)data;

  if ( m_current->IsGroup() ) {
    if ( m_current->IsBook() ) {
      // show the book description in the status line
      SetStatusText(((AdbTreeBook *)m_current)->GetName(), 0);
    }
    else {
      SetStatusText(_T(""), 0);
    }

    // clear the text set previously
    SetStatusText(_T(""), 1);
  }
  else {
    wxString str;
    str.Printf(_("Editing entry '%s' %s"), m_current->GetName().c_str(), m_current->GetParent()->GetWhere().c_str());
    SetStatusText(str, 1);
  }

  UpdateNotebook();
}

void wxAdbEditFrame::SetTreeItemIcon(const wxTreeItemId& id,
                                     wxAdbTree::ImageListImage icon)
{
  AdbTreeNode *item = (AdbTreeNode *)m_treeAdb->GetItemData(id);
  wxASSERT( item->IsGroup() ); // items can't be expanded/collapsed

  // don't change icons for the root entry and address books
  if ( item->IsRoot() || item->GetParent()->IsRoot() )
    return;

  m_treeAdb->SetItemImage(id, icon);
  m_treeAdb->SetItemSelectedImage(id, icon);
}

void wxAdbEditFrame::OnTreeCollapse(wxTreeEvent& event)
{
  SetTreeItemIcon(event.GetItem(), wxAdbTree::Closed);
}

void wxAdbEditFrame::OnTreeExpand(wxTreeEvent& event)
{
  SetTreeItemIcon(event.GetItem(), wxAdbTree::Opened);
}

void wxAdbEditFrame::OnTreeExpanding(wxTreeEvent& event)
{
  wxTreeItemId id = event.GetItem();
  AdbTreeNode *parent = (AdbTreeNode *)m_treeAdb->GetItemData(id);

  wxASSERT( parent->IsGroup() ); // items can't be expanded

  if ( parent->WasExpanded() )
    return;

  if ( !parent->ExpandFirstTime(*m_treeAdb) ) {
    // if this group has no entries don't put [+] near it
    m_treeAdb->SetItemHasChildren(parent->GetId(), FALSE);

    wxLogStatus(this, _("This group has no entries"));
  }
}

void wxAdbEditFrame::OnTextLookupEnter(wxCommandEvent&)
{
  // these events are never received this early under MSW
#ifdef __WXGTK__
  if ( !m_textKey )
    return;
#endif // GTK

  DoFind();
}

// FIXME should do it in UpdateUI handler
void wxAdbEditFrame::OnTextLookupChange(wxCommandEvent&)
{
  // these events are never received this early under MSW
#ifdef __WXGTK__
  if ( !m_textKey )
    return;
#endif // GTK

  GetToolBar()->EnableTool(WXMENU_ADBFIND_NEXT, !m_textKey->GetValue().IsEmpty());
}

// UpdateUI event handlers are called during the idle time and set the state
// of misc GUI elements to correspond to the properties of current selection.
//
// NB: due to some wxGTK wierdness the idle events may be generated *before*
//     the frame construction is finished, so we must test for it.

#ifdef __WXGTK__
  #define CHECK_INIT_DONE if ( !m_current ) return;
#else  // !GTK
  // don't waste time to check for it, it can never happen
  #define CHECK_INIT_DONE
#endif // GTK

void wxAdbEditFrame::OnUpdateCancel(wxUpdateUIEvent& event)
{
  CHECK_INIT_DONE

  // 'undo' can only be performed when something is actually in process
  // of being edited
  bool bDoEnable = !m_current->IsGroup() && m_notebook->IsDirty();

  event.Enable(bDoEnable);
  m_btnCancel->Enable(bDoEnable);
  GetToolBar()->EnableTool(WXMENU_ADBEDIT_UNDO, bDoEnable);
}

void wxAdbEditFrame::OnUpdateProp(wxUpdateUIEvent& event)
{
  CHECK_INIT_DONE

  // only if the selected item is an ADB
  event.Enable(AllowShowProp());
}

void wxAdbEditFrame::OnUpdateDelete(wxUpdateUIEvent& event)
{
  CHECK_INIT_DONE

  // can't delete root
  bool bDoEnable = AllowDelete();

  event.Enable(bDoEnable);
  m_btnDelete->Enable(bDoEnable);
  GetToolBar()->EnableTool(WXMENU_ADBEDIT_DELETE, bDoEnable);
}

void wxAdbEditFrame::OnUpdatePaste(wxUpdateUIEvent& event)
{
  CHECK_INIT_DONE

  // only when there is something on the clipboard
  event.Enable(AllowPaste());
}

void wxAdbEditFrame::OnUpdateCopy(wxUpdateUIEvent& event)
{
  CHECK_INIT_DONE

  // only when not root or ADB (i.e. a normal entry or group)
  event.Enable(AllowCopy());
}

// keep your namespace clean
#undef CHECK_INIT_DONE

// can only export books or groups, not entries or root
void wxAdbEditFrame::OnUpdateExport(wxUpdateUIEvent& event)
{
  event.Enable( m_current->IsGroup() && !m_current->IsRoot() );
}

// can only export a single entry to vCard
void wxAdbEditFrame::OnUpdateExportVCard(wxUpdateUIEvent& event)
{
  event.Enable( !m_current->IsGroup() );
}

void wxAdbEditFrame::OnUpdateImportVCard(wxUpdateUIEvent& event)
{
  event.Enable( m_current->IsGroup() );
}

void wxAdbEditFrame::OnActivate(wxActivateEvent& event)
{
  // give the focus to the tree control (can't do it in ctor because
  // the default EVT_ACTIVATE processing will move it to the first
  // control in our frame, i.e. the toolbar - definitely not what we want)
  if ( event.GetActive() )
    m_treeAdb->SetFocus();
}

// expand all branches leading to the specified item
// (full and relative paths accepted), returns NULL if path is invalid
AdbTreeElement *wxAdbEditFrame::ExpandBranch(const wxString& strEntry)
{
  AdbTreeElement *current;
  if ( strEntry.empty() || wxIsPathSeparator(strEntry[0]) )
    current = m_root;
  else
    current = GetCurNode();

  wxArrayString aComponents;
  wxSplitPath(aComponents, strEntry);
  size_t n, nCount = aComponents.Count();
  for ( n = 0; n < nCount; n++ ) {
    if ( current->IsGroup() ) {
      // we can safely cast it to AdbTreeNode
      AdbTreeNode *curGroup = (AdbTreeNode *)current;
      if ( !curGroup->WasExpanded() )
        m_treeAdb->Expand(current->GetId());
      current = curGroup->FindChild(aComponents[n]);
      if ( current == NULL ) {
        wxLogError(_("No entry '%s':\n'%s' has no entry/subgroup '%s'."),
                   strEntry.c_str(),
                   curGroup->GetName().c_str(),
                   aComponents[n].c_str());
        return NULL;
      }
    }
    else { // current item is an entry
      wxLogError(_("Entry '%s' cannot have subgroup/entry '%s'!"),
                 current->GetName().c_str(), aComponents[n].c_str());

      return NULL;
    }
  }

  return current;
}

// move selection to the specified item (full and relative paths accepted)
// NB: if the path given doesn't exist it will expand all branches that can
//     be expanded - but not doing so will be much more complicated
bool wxAdbEditFrame::MoveSelection(const wxString& strEntry)
{
  AdbTreeElement *current = ExpandBranch(strEntry);
  if ( !current )
    return FALSE;

  m_treeAdb->SetFocus();
  m_treeAdb->SelectAndShow(current->GetId());

  return TRUE;
}

// calculate the minimal size of the frame and set it
bool wxAdbEditFrame::SetMinSize()
{
  if ( !m_btnCancel )
    return FALSE;

  // all buttons have the same size and we use them as length unit
  int widthBtn, heightBtn;
  m_btnCancel->GetClientSize(&widthBtn, &heightBtn);

  // FIXME the numbers are completely arbitrary
  SetSizeHints(7*widthBtn, 22*heightBtn);

  return TRUE;
}

bool wxAdbEditFrame::SaveExpandedBranches(AdbTreeNode *group)
{
  if ( m_treeAdb->IsExpanded(group->GetId()) ) {
    // recursively save the expanded branches of our children
    bool bHasExpandedChild = FALSE;
    size_t nChildren = group->GetChildrenCount();
    for ( size_t n = 0; n < nChildren; n++ ) {
      AdbTreeElement *child = group->GetChild(n);
      if ( child->IsGroup() && SaveExpandedBranches((AdbTreeNode *)child) )
        bHasExpandedChild = TRUE;
    }

    // if we have an expanded child we'll be expanded anyhow
    if ( !bHasExpandedChild )
      m_astrBranches.Add(group->GetFullName());

    return TRUE;
  }

  // not expanded
  return FALSE;
}

wxAdbEditFrame::~wxAdbEditFrame()
{
  // don't want to get events any more
  MEventManager::Deregister(m_eventNewADB);

  // don't show the frame at startup the next time unless the application is
  // closing too
  if ( mApplication->IsRunning() )
  {
    // app continues to run => only this frame is being closed
    mApplication->GetProfile()->writeEntry(MP_SHOWADBEDITOR, FALSE);
  }
  //else: we had already written TRUE there when we showed the ADB editor

  // save current selction
  m_strSelection = m_current->GetFullName();

  // if the notebook had unsaved data they will be saved now
  m_notebook->ChangeData(NULL);

  // save all settings
  SaveSettings();

  // clear the clipboard (may be NULL, it's ok)
  if(m_clipboard) delete m_clipboard;

  // and now delete all ADB entries
#if DELETE_TREE_CHILDREN
  delete m_root;
#endif

  // and the imagelist we were using
  delete m_pImageList;

  // dec the number of objects alive
  ms_nAdbFrames--;
}

// ----------------------------------------------------------------------------
// wxADBFindDialog dialog
// ----------------------------------------------------------------------------
wxADBFindDialog::wxADBFindDialog(wxWindow *parent,
                                 wxPTextEntry *text,
                                 int *where,
                                 int *how)
               : wxDialog(parent, -1, _("Find address book entry"),
                          wxDefaultPosition,
                          wxDefaultSize,
                          wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL)
{
  // init member vars
  // ----------------

  m_text = text;

  m_where = where;
  if ( *m_where == 0 ) {
    // must look at least _somewhere_
    *m_where = AdbLookup_NickName;
  }

  m_how = how;

  // determine dialog size
  // ---------------------

  const wxChar *label = _("Find &what:");
  long widthLabel, heightLabel;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  dc.GetTextExtent(label, &widthLabel, &heightLabel);

  size_t heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  size_t heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  size_t widthDlg = 5*widthBtn,
       heightDlg = heightText + 8*heightLabel + 8*LAYOUT_Y_MARGIN + heightBtn;
  size_t widthText = widthDlg - 5*LAYOUT_X_MARGIN - widthLabel;
  SetClientSize(widthDlg, heightDlg);

  // create child controls
  // ---------------------

  size_t x = 2*LAYOUT_X_MARGIN,
       y = 2*LAYOUT_Y_MARGIN,
       dy = (heightText - heightLabel)/2;

  // static box around everything except buttons
  (void)new wxStaticBox(this, -1, _T(""),  wxPoint(LAYOUT_X_MARGIN, 0),
                        wxSize(widthDlg - 2*LAYOUT_X_MARGIN,
                               heightDlg - 2*LAYOUT_Y_MARGIN - heightBtn));

  // label and text control
  (void)new wxStaticText(this, -1, label, wxPoint(x, y + dy),
            wxSize(widthLabel, heightLabel));
  m_textWhat = new wxTextCtrl(this, -1, _T(""),
                              wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                              wxSize(widthText, heightText));

  // settings box and checkboxes
  y += heightText;
  size_t widthBox = widthDlg - 4*LAYOUT_X_MARGIN,
       widthChk = widthBox/2 - 4*LAYOUT_X_MARGIN;

  (void)new wxStaticBox(this, -1, _("&How"), wxPoint(x, y),
                        wxSize(widthBox, 3*heightLabel));
  y += heightLabel + LAYOUT_Y_MARGIN;
  m_checkCase = new wxCheckBox(this, -1, _("&Case sensitive"),
                               wxPoint(x + LAYOUT_X_MARGIN, y),
                               wxSize(widthChk, heightLabel));
  m_checkSub = new wxCheckBox(this, -1, _("&Substring search"),
                               wxPoint(widthChk + 3*LAYOUT_X_MARGIN, y),
                               wxSize(widthChk, heightLabel));

  // where to look for it
  y += 2*heightLabel;

  (void)new wxStaticBox(this, -1, _("&Where"), wxPoint(x, y),
                       wxSize(widthBox, 4*(heightLabel + LAYOUT_Y_MARGIN)));

  y = y + heightLabel + LAYOUT_Y_MARGIN;
  m_checkNick = new wxCheckBox(this, -1, _("&Nick name"),
                              wxPoint(x + LAYOUT_X_MARGIN, y),
                              wxSize(widthChk, heightLabel));
  m_checkEMail = new wxCheckBox(this, -1, _("&E-mail address"),
                                wxPoint(widthChk + 3*LAYOUT_X_MARGIN, y),
                                wxSize(widthChk, heightLabel));

  y += heightLabel + LAYOUT_Y_MARGIN;
  m_checkFull = new wxCheckBox(this, -1, _("&Full name"),
                               wxPoint(x + LAYOUT_X_MARGIN, y),
                               wxSize(widthChk, heightLabel));
  m_checkWWW = new wxCheckBox(this, -1, _("&Home page"),
                              wxPoint(widthChk + 3*LAYOUT_X_MARGIN, y),
                              wxSize(widthChk, heightLabel));

  y = y + heightLabel + LAYOUT_Y_MARGIN;
  m_checkOrg = new wxCheckBox(this, -1, _("&Organization"),
                              wxPoint(x + LAYOUT_X_MARGIN, y),
                              wxSize(widthChk, heightLabel));

  // and the buttons
  wxButton *btnOk = new
    wxButton(this, wxID_OK, _("OK"),
             wxPoint(widthDlg - 2*LAYOUT_X_MARGIN - 2*widthBtn,
                     heightDlg - LAYOUT_Y_MARGIN - heightBtn),
             wxSize(widthBtn, heightBtn));
  (void)new wxButton(this, wxID_CANCEL, _("Cancel"),
                     wxPoint(widthDlg - LAYOUT_X_MARGIN - widthBtn,
                             heightDlg - LAYOUT_Y_MARGIN - heightBtn),
                     wxSize(widthBtn, heightBtn));

  btnOk->SetDefault();
	Centre(wxCENTER_FRAME | wxBOTH);
}

bool wxADBFindDialog::TransferDataToWindow()
{
  // select all text in the edit, so that pressing any alphanumeric key
  // will clear it
  m_textWhat->SetValue(m_text->GetValue());
  m_textWhat->SetSelection(-1, -1);

  // set the checkboxes
  m_checkNick->SetValue((*m_where & AdbLookup_NickName) != 0);
  m_checkFull->SetValue((*m_where & AdbLookup_FullName) != 0);
  m_checkOrg->SetValue((*m_where & AdbLookup_Organization) != 0);
  m_checkEMail->SetValue((*m_where & AdbLookup_EMail) != 0);
  m_checkWWW->SetValue((*m_where & AdbLookup_HomePage) != 0);

  m_checkCase->SetValue((*m_how & AdbLookup_CaseSensitive) != 0);
  m_checkSub->SetValue((*m_how & AdbLookup_Substring) != 0);

  return TRUE;
}

bool wxADBFindDialog::TransferDataFromWindow()
{
  *m_how = 0;
  if ( m_checkCase->GetValue() )
    *m_how |= AdbLookup_CaseSensitive;
  if ( m_checkSub->GetValue() )
    *m_how |= AdbLookup_Substring;

  *m_where = 0;
  if ( m_checkNick->GetValue() )
    *m_where |= AdbLookup_NickName;
  if ( m_checkFull->GetValue() )
    *m_where |= AdbLookup_FullName ;
  if ( m_checkOrg->GetValue() )
    *m_where |= AdbLookup_Organization;
  if ( m_checkEMail->GetValue() )
    *m_where |= AdbLookup_EMail;
  if ( m_checkWWW->GetValue() )
    *m_where |= AdbLookup_HomePage;

  if ( *m_where == 0 ) {
    wxLogError(_("Please specify where to search!"));
    return FALSE;
  }

  m_text->SetValue(m_textWhat->GetValue());

  return TRUE;
}

// ----------------------------------------------------------------------------
// wxADBCreateDialog: allows the user to enter the new entry or group name
// ----------------------------------------------------------------------------

wxADBCreateDialog::wxADBCreateDialog(wxWindow *parent,
                                     const wxString& strName,
                                     bool bGroup)
   : wxDialog(parent, -1, _("Create a new entry/group"),
              wxDefaultPosition,
              wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL),
     m_strName(strName), m_bGroup(bGroup)
{
  const wxChar *label = _("&New entry/group name:");

  // layout
  long widthLabel, heightLabel;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  dc.GetTextExtent(label, &widthLabel, &heightLabel);

  size_t widthText = widthLabel,
       heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  size_t heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  size_t widthDlg = widthLabel + widthText + 5*LAYOUT_X_MARGIN,
       heightDlg = 2*heightText + 6*LAYOUT_Y_MARGIN + heightBtn;

  SetClientSize(widthDlg, heightDlg);

  size_t x = 2*LAYOUT_X_MARGIN,
       y = 2*LAYOUT_Y_MARGIN,
       dy = (heightText - heightLabel) / 2;

  // a box around all entries
  (void)new wxStaticBox(this, -1, _T(""),
                        wxPoint(LAYOUT_X_MARGIN, 0),
                        wxSize(widthDlg - 2*LAYOUT_X_MARGIN,
                               heightDlg - 2*LAYOUT_Y_MARGIN - heightBtn));

  // label and the text
  (void)new wxStaticText(this, -1, label, wxPoint(x, y + dy),
                         wxSize(widthLabel, heightLabel));
  m_textName = new wxTextCtrl(this, -1, _T(""),
                              wxPoint(x + widthLabel + LAYOUT_X_MARGIN, y),
                              wxSize(widthText, heightText));

  // checkbox
  m_checkGroup = new wxCheckBox(this, -1, _("Create a &group"),
                                wxPoint(x, y + heightText),
                                wxSize(widthLabel + widthText, heightText));

  // buttons
  wxButton *btnOk = new
    wxButton(this, wxID_OK, _("OK"),
             wxPoint(widthDlg - 2*LAYOUT_X_MARGIN - 2*widthBtn,
                     heightDlg - LAYOUT_Y_MARGIN - heightBtn),
             wxSize(widthBtn, heightBtn));
  (void)new wxButton(this, wxID_CANCEL, _("Cancel"),
                     wxPoint(widthDlg - LAYOUT_X_MARGIN - widthBtn,
                             heightDlg - LAYOUT_Y_MARGIN - heightBtn),
                     wxSize(widthBtn, heightBtn));
  btnOk->SetDefault();

  // set position
  Centre(wxCENTER_FRAME | wxBOTH);
}

bool wxADBCreateDialog::TransferDataToWindow()
{
  // select all text in the edit, so that pressing any alphanumeric key
  // will clear it
  m_textName->SetValue(m_strName);
  m_textName->SetSelection(-1, -1);
  m_textName->SetFocus();

  m_checkGroup->SetValue(m_bGroup);

  return TRUE;
}

bool wxADBCreateDialog::TransferDataFromWindow()
{
  m_strName = m_textName->GetValue();
  m_bGroup = m_checkGroup->GetValue();
  if ( m_strName.IsEmpty() ) {
    wxLogError(_("Please specify a name for the new %s!"),
               wxGetTranslation(m_bGroup ? _T("group") : _T("entry")));
    return FALSE;
  }

  return TRUE;
}

// ----------------------------------------------------------------------------
// wxADBPropertiesDialog dialog
// ----------------------------------------------------------------------------
wxADBPropertiesDialog::wxADBPropertiesDialog(wxWindow *parent, AdbTreeBook *book)
                     : wxDialog(parent, -1, _T(""),
                                wxDefaultPosition,
                                wxDefaultSize,
                                wxDEFAULT_DIALOG_STYLE)
{
  m_book = book;

  // layout
  // ------

  static const wxChar *labels[] = {
    _("&Name:"),
    _("File name:"),
    _("File size:"),
    _("&Description: "),
    _("Number of entries: ")
  };

  // translated lables
  const wxChar *labelsT[WXSIZEOF(labels)];

  size_t n, x, y, dy;

  // first determine the longest label
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long widthLabel, heightLabel = 0, widthLabelMax = 0;
  for ( n = 0; n < WXSIZEOF(labels); n++ ) {
    labelsT[n] = wxGetTranslation(labels[n]);
    dc.GetTextExtent(labelsT[n], &widthLabel, &heightLabel);
    if ( widthLabel > widthLabelMax )
      widthLabelMax = widthLabel;
  }

  // TODO just don't look at this mess, ok? should be done using
  //      wxManuallyLaidOutDialog (but it didn't exist yet when I wrote this)
  size_t widthText = 2*widthLabelMax,
       heightText = TEXT_HEIGHT_FROM_LABEL(heightLabel);
  size_t heightBtn = TEXT_HEIGHT_FROM_LABEL(heightLabel),
       widthBtn = BUTTON_WIDTH_FROM_HEIGHT(heightBtn);
  size_t widthDlg = widthLabelMax + widthText + 5*LAYOUT_X_MARGIN,
       heightDlg = WXSIZEOF(labels)*heightText + 5*LAYOUT_Y_MARGIN + heightBtn;

  SetClientSize(widthDlg, heightDlg);

  // and a box around all entries
  (void)new wxStaticBox(this, -1, _T(""),
                        wxPoint(LAYOUT_X_MARGIN, 0 /*LAYOUT_Y_MARGIN*/),
                        wxSize(widthDlg - 2*LAYOUT_X_MARGIN,
                               heightDlg - 3*LAYOUT_Y_MARGIN - heightBtn));

  // create labels
  wxSize sizeLabel(widthLabelMax, heightLabel);
  x = 2*LAYOUT_X_MARGIN;
  dy = (heightText - heightLabel) / 2;
  y = 2*LAYOUT_Y_MARGIN + dy;
  for ( n = 0; n < WXSIZEOF(labels); n++ ) {
    (void)new wxStaticText(this, -1, labelsT[n], wxPoint(x, y), sizeLabel);
    y += heightText;
  }

  // create text fields
  wxSize sizeText(widthText, heightText);
  x = 3*LAYOUT_X_MARGIN + widthLabelMax;
  y = 2*LAYOUT_Y_MARGIN;
  m_textName = new wxTextCtrl(this, -1, _T(""), wxPoint(x, y), sizeText);
  y += heightText;
  m_textFileName = new wxTextCtrl(this, -1, _T(""),
                                  wxPoint(x, y + dy), sizeText, wxTE_READONLY);
  y += heightText;
  m_staticFileSize = new wxStaticText(this, -1, _T(""), wxPoint(x, y + dy), sizeLabel);
  y += heightText;
  m_textDescription = new wxTextCtrl(this, -1, _T(""), wxPoint(x, y), sizeText);
  y += heightText;
  m_staticNumEntries = new wxStaticText(this, -1, _T(""), wxPoint(x, y + dy), sizeLabel);

  // finally add buttons
  wxButton *btnOk = new
    wxButton(this, wxID_OK, _("OK"),
             wxPoint(widthDlg - 2*LAYOUT_X_MARGIN - 2*widthBtn,
                     heightDlg - LAYOUT_Y_MARGIN - heightBtn),
             wxSize(widthBtn, heightBtn));
  (void)new wxButton(this, wxID_CANCEL, _("Cancel"),
                     wxPoint(widthDlg - LAYOUT_X_MARGIN - widthBtn,
                             heightDlg - LAYOUT_Y_MARGIN - heightBtn),
                     wxSize(widthBtn, heightBtn));
  btnOk->SetDefault();

  // set label and position
  // ----------------------
  wxString strTitle;
  strTitle.Printf(_("Properties for '%s'"), book->GetName());
  SetTitle(strTitle);

  Centre(wxCENTER_FRAME | wxBOTH);
}

bool wxADBPropertiesDialog::TransferDataToWindow()
{
  wxString filename = m_book->GetBook()->GetFileName();

  wxString str;

  // get the file size
  if ( !filename.empty() )
  {
    // suppress log messages because the file might not yet exist and it's
    // perfectly normal
    wxLogNull nolog;
    wxFile file(filename);
    if ( file.IsOpened() ) {
      str.Printf(_T("%lu"), (unsigned long)file.Length());
    }
    else {
      str = _("unknown");
    }
  }
  else {
    // it is not a file based book
    str = _("not applicable");
  }

  m_staticFileSize->SetLabel(str);
  m_textFileName->SetValue(filename);

  str.Printf(_T("%ld"), (unsigned long)m_book->GetNumberOfEntries());
  m_staticNumEntries->SetLabel(str);

  m_textName->SetValue(m_book->GetBook()->GetName());
  m_textDescription->SetValue(m_book->GetBook()->GetDescription());

  return TRUE;
}

bool wxADBPropertiesDialog::TransferDataFromWindow()
{
  AdbBook *adbbook = m_book->GetBook();

  adbbook->SetName(m_textName->GetValue());
  adbbook->SetDescription(m_textDescription->GetValue());

  return TRUE;
}

// ----------------------------------------------------------------------------
// wxAdbTree control
// ----------------------------------------------------------------------------

wxAdbTree::wxAdbTree(wxAdbEditFrame *frame, wxWindow *parent, long id)
         : wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
                      wxTR_HAS_BUTTONS | wxSUNKEN_BORDER)
{
  m_frame = frame;
  m_menu = NULL;

  // add images to our image list
  static const wxChar *aszImages[] =
  {
    // should be in sync with the corresponding enum in wxAdbTree
    _T("adb_library"),
    _T("adb_book"),
    _T("adb_address"),
    _T("adb_opened"),
    _T("adb_closed"),
    _T("adb_palmos"),
    _T("adb_bbdb")
  };

  wxImageList *imageList = new wxImageList(16, 16, FALSE, WXSIZEOF(aszImages));

  for ( size_t n = 0; n < WXSIZEOF(aszImages); n++ ) {
    imageList->Add(mApplication->GetIconManager()->GetBitmap(aszImages[n]));
  }

  SetImageList(imageList);
}

// tree control keyboard interface

// key          operation
// ----------------------------------------------------------------------
// INS          creates new item in the current node
// DEL          deletes the current item
// Alt-Enter    shows the properties of the current item
void wxAdbTree::OnChar(wxKeyEvent& event)
{
  switch ( event.GetKeyCode() ) {
    case WXK_DELETE:
      if ( m_frame->AllowDelete() )
        m_frame->DoDeleteNode();
      break;

    case WXK_INSERT:
      if ( m_frame->AllowCreate() )
        m_frame->DoCreateNode();
      break;

    case WXK_RETURN:
      if ( event.AltDown() && m_frame->AllowShowProp() )
        m_frame->DoShowAdbProperties();
      break;

    default:
      event.Skip();
  }
}

void wxAdbTree::OnRightDown(wxMouseEvent& event)
{
  wxPoint pt = event.GetPosition();
  wxTreeItemId item = HitTest(pt);
  if ( item.IsOk() )
  {
    SelectItem(item);
  }

  if ( !m_menu )
  {
    m_menu = new AdbTreeMenu;
  }

  m_menu->Enable(WXMENU_ADBEDIT_NEW, m_frame->AllowCreate());
  m_menu->Enable(WXMENU_ADBEDIT_DELETE, m_frame->AllowDelete());
  m_menu->Enable(WXMENU_ADBEDIT_CUT, m_frame->AllowCopy());
  m_menu->Enable(WXMENU_ADBEDIT_COPY, m_frame->AllowCopy());
  m_menu->Enable(WXMENU_ADBEDIT_PASTE, m_frame->AllowPaste());
  m_menu->Enable(WXMENU_ADBBOOK_PROP, m_frame->AllowShowProp());

  PopupMenu(m_menu, pt.x, pt.y);
}

wxAdbTree::~wxAdbTree()
{
  delete GetImageList();
  delete m_menu;
}

// -----------------------------------------------------------------------------
// ADB notebook
// -----------------------------------------------------------------------------

wxAdbNotebook::wxAdbNotebook(wxPanel *parent, wxWindowID id)
             : wxNotebook(parent, id)
{
  // create the message which is shown when there is nothing to edit
  m_box = new wxStaticBox(parent, -1, wxGetEmptyString());
  m_message = new wxStaticText(parent, -1, _("No selection"));

  wxLayoutConstraints *c;
  c = new wxLayoutConstraints;
  c->top.SameAs(this, wxTop);
  c->left.SameAs(this, wxLeft);
  c->width.SameAs(this, wxWidth);
  c->height.SameAs(this, wxHeight);
  m_box->SetConstraints(c);

  c = new wxLayoutConstraints;
  c->centreX.SameAs(m_box, wxCentreX);
  c->centreY.SameAs(m_box, wxCentreY);
  c->width.AsIs();
  c->height.AsIs();
  m_message->SetConstraints(c);

  // no data yet
  m_pTreeEntry = NULL;
  m_pAdbEntry = NULL;
  m_bDirty =
  m_bReadOnly = FALSE;

  // add images to our image list
  static const wxChar *aszImages[] =
  {
    // should be in sync with the corresponding enum
    _T("general"), _T("email"), _T("home"), _T("work")
  };

  wxImageList *imageList = new wxImageList(32, 32, TRUE, WXSIZEOF(aszImages));
  for ( size_t n = 0; n < WXSIZEOF(aszImages); n++ ) {
    imageList->Add(mApplication->GetIconManager()->GetBitmap(aszImages[n]));
  }

  SetImageList(imageList);

  // add pages to the notebook (in the order of creation)
  // NB: it must be done after the image list is assigned to the notebook
  (void)new wxAdbNamePage(this);
  (void)new wxAdbEMailPage(this);
  (void)new wxAdbOfficeAddrPage(this);
  (void)new wxAdbHomeAddrPage(this);
}

// change = save + set
void wxAdbNotebook::ChangeData(AdbTreeEntry *pEntry)
{
  SaveChanges();
  SetData(pEntry);
  m_bDirty = FALSE;
}

// save changes i.e. transfer all data from our controls to AdbEntry
void wxAdbNotebook::SaveChanges()
{
  if ( m_pAdbEntry && !m_bReadOnly ) {
    for ( size_t n = 0; n < Page_Max; n++ ) {
      ((wxAdbPage *)GetPage(n))->SaveChanges(*m_pAdbEntry);
    }

    wxString str;
    m_pAdbEntry->GetField(AdbField_NickName, &str);
    if ( m_bDirty ) {
      wxLogStatus((wxFrame *)this->GetGrandParent(),
                  _("Entry '%s' saved."), str.c_str());
    }
  }
}

// start editing this entry (or don't edit anything if it's NULL)
void wxAdbNotebook::SetData(AdbTreeEntry *pEntry)
{
  // free old data if any
  if ( m_pAdbEntry )
    m_pAdbEntry->DecRef();

  m_pTreeEntry = pEntry;
  if ( m_pTreeEntry ) {
    m_pAdbEntry = m_pTreeEntry->GetAdbEntry();
  }
  else {
    m_pAdbEntry = NULL;
  }

  // if we have got some data - show it
  if ( m_pAdbEntry ) {
    // determine if an entry is read only by querying the book it is in
    AdbTreeNode *parent = m_pTreeEntry->GetParent();
    while ( parent && !parent->IsBook() )
      parent = parent->GetParent();

    if ( parent ) {
      m_bReadOnly = ((AdbTreeBook *)parent)->GetBook()->IsReadOnly();
    }
    else {
      wxFAIL_MSG(_T("entry outside of an address book?"));
    }

    for ( size_t n = 0; n < Page_Max; n++ ) {
      ((wxAdbPage *)GetPage(n))->SetData(*m_pAdbEntry);
      ((wxAdbPage *)GetPage(n))->Enable(!m_bReadOnly);
    }
  }

  // hide notebook if nothing to edit
  m_box->Show(pEntry == NULL);
  m_message->Show(pEntry == NULL);
  Show(pEntry != NULL);
  m_bDirty = FALSE;
}

wxAdbNotebook::~wxAdbNotebook()
{
  wxASSERT( m_pAdbEntry == NULL );

  delete GetImageList();
}

// -----------------------------------------------------------------------------
// base notebook page
// -----------------------------------------------------------------------------
wxAdbPage::wxAdbPage(wxNotebook *notebook, const wxChar *title, int idImage,
                     size_t nFirstField, size_t nLastField)
         : wxPanel(notebook, -1)
{
  m_nFirstField = nFirstField;
  m_nLastField  = nLastField;

  notebook->AddPage(this, wxGetTranslation(title),
                    FALSE /* don't select */, idImage);

  LayoutControls(m_nLastField - m_nFirstField, m_aEntries,
                 &AdbTreeEntry::ms_aFields[m_nFirstField]);
  SetAutoLayout(TRUE);
}

// we implement the data transfer for the text fields and check boxes
void wxAdbPage::SetData(const AdbEntry& data)
{
  wxString str;
  wxTextCtrl *text;
  for ( size_t n = m_nFirstField; n < m_nLastField; n++ ) {
    switch ( AdbTreeEntry::ms_aFields[n].type ) {
      case AdbTreeEntry::FieldDate:
      case AdbTreeEntry::FieldURL:
      case AdbTreeEntry::FieldPhone:
      case AdbTreeEntry::FieldNum:
        // fall through -- for now they're the same as text
      case AdbTreeEntry::FieldText:
      case AdbTreeEntry::FieldMemo:
        text = TEXTCTRL(n - m_nFirstField);
        data.GetField(n, &str);
        text->SetValue(str);
        text->DiscardEdits();
        break;

      case AdbTreeEntry::FieldBool:
        data.GetField(n, &str);
        CHECKBOX(n - m_nFirstField)->SetValue(str == _T("1"));
        break;

      default:
        // nothing
        ;
    }
  }
}

void wxAdbPage::SaveChanges(AdbEntry& data)
{
  wxTextCtrl *text;
  for ( size_t n = m_nFirstField; n < m_nLastField; n++ ) {
    switch ( AdbTreeEntry::ms_aFields[n].type ) {
      case AdbTreeEntry::FieldDate:
      case AdbTreeEntry::FieldURL:
      case AdbTreeEntry::FieldPhone:
      case AdbTreeEntry::FieldNum:
        // fall through -- for now they're the same as text
      case AdbTreeEntry::FieldText:
      case AdbTreeEntry::FieldMemo:
        text = TEXTCTRL(n - m_nFirstField);
        // don't write the unchanged fields
        if ( text->IsModified() )
          data.SetField(n, text->GetValue());
        break;

      case AdbTreeEntry::FieldBool:
        if ( CHECKBOX(n - m_nFirstField)->GetValue() )
          data.SetField(n, _T("1"));
        else
          data.SetField(n, _T("0"));
        break;

      default:
        // nothing
        ;
    }
  }
}

// something changed, allow undo
void wxAdbPage::OnTextChange(wxCommandEvent&)
{
  SetDirty();
}

// Layout controls helpers
// -----------------------

// the top item is positioned near the top of the page, the others are
// positioned from top to bottom, i.e. under the last one
void wxAdbPage::SetTopConstraint(wxLayoutConstraints *c, wxControl *last)
{
  if ( last == NULL )
    c->top.SameAs(this, wxTop, 2*LAYOUT_Y_MARGIN);
  else
    c->top.Below(last, LAYOUT_Y_MARGIN);
}

// create a single-line text control with a label
wxTextCtrl *wxAdbPage::CreateTextWithLabel(const wxChar *label,
                                           long widthMax,
                                           wxControl *last)
{
  wxLayoutConstraints *c;

  // for the label
  c = new wxLayoutConstraints;
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  SetTopConstraint(c, last);
  c->width.Absolute(widthMax);
  c->height.AsIs();
  wxStaticText *pLabel = new wxStaticText(this, -1, label,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxALIGN_RIGHT);
  pLabel->SetConstraints(c);

  // for the text control
  c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.RightOf(pLabel, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->height.AsIs();
  wxTextCtrl *pText = new wxTextCtrl(this, -1, _T(""));
  pText->SetConstraints(c);

  return pText;
}

// create a multi-line text control with a label
wxTextCtrl *wxAdbPage::CreateMultiLineText(const wxChar *label, wxControl *last)
{
  wxStaticText *pLabel = new wxStaticText(this, -1, label);
  wxTextCtrl *textComments = new wxTextCtrl(this, -1, _T(""),
                                            wxDefaultPosition, wxDefaultSize,
                                            wxTE_MULTILINE);
  LayoutBelow(textComments, pLabel, last);

  return textComments;
}

// create a listbox and the buttons to work with it
// NB: we consider that there is only one listbox (at most) per page, so
//     the button ids are always the same
wxListBox *wxAdbPage::CreateListBox(const wxChar *label, wxControl *last)
{
  // a box around all this stuff
  wxStaticBox *box = new wxStaticBox(this, -1, label);

  wxLayoutConstraints *c;
  c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
  box->SetConstraints(c);

  // the buttons vertically on the right of listbox
  wxButton *button = NULL;
  static const wxChar *aszLabels[] =
  {
    _("&Add"),
    _("&Modify"),
    _("&Delete"),
  };
  const wxChar *aszLabelsT[WXSIZEOF(aszLabels)];  // translated labels

  // should be in sync with enum!
  wxASSERT(
    AdbPage_BtnLast - AdbPage_BtnFirst == WXSIZEOF(aszLabels)
  );

  // determine the longest button label
  size_t nBtn;
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long width, widthMax = 0;
  for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
    aszLabelsT[nBtn] = wxGetTranslation(aszLabels[nBtn]);
    dc.GetTextExtent(aszLabelsT[nBtn], &width, NULL);
    if ( width > widthMax )
      widthMax = width;
  }

  widthMax += 15; // FIXME loks better like this, but why 15?
  for ( nBtn = 0; nBtn < WXSIZEOF(aszLabels); nBtn++ ) {
    c = new wxLayoutConstraints;
    if ( nBtn == 0 )
      c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
    else
      c->top.Below(button, LAYOUT_Y_MARGIN);
    c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
    c->width.Absolute(widthMax);
    c->height.AsIs();
    button = new wxButton(this, AdbPage_BtnFirst + nBtn, aszLabelsT[nBtn]);
    button->SetConstraints(c);
  }

  // and the listbox itself
  wxListBox *listbox = new wxListBox(this, -1);
  c = new wxLayoutConstraints;
  c->top.SameAs(box, wxTop, 3*LAYOUT_Y_MARGIN);
  c->left.SameAs(box, wxLeft, LAYOUT_X_MARGIN);
  c->right.LeftOf(button, LAYOUT_X_MARGIN);;
  c->bottom.SameAs(box, wxBottom, LAYOUT_Y_MARGIN);
  listbox->SetConstraints(c);

  return listbox;
}

// create a checkbox
wxCheckBox *wxAdbPage::CreateCheckBox(const wxChar *label, wxControl *last)
{
  wxCheckBox *checkbox = new wxCheckBox(this, -1, label);

  wxLayoutConstraints *c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->height.AsIs();

  checkbox->SetConstraints(c);

  return checkbox;
}

// create the controls and layout them
void wxAdbPage::LayoutControls(size_t nCount,
                               ArrayControls& entries,
                               AdbTreeEntry::FieldInfo fields[])
{
  wxASSERT( entries.IsEmpty() );  // we create the controls ourselves

  size_t n;

  // first determine the longest label
  wxClientDC dc(this);
  dc.SetFont(wxSystemSettings::GetSystemFont(wxSYS_DEFAULT_GUI_FONT));
  long width, widthMax = 0;
  for ( n = 0; n < nCount; n++ ) {
    // do it only for text control labels
    switch ( fields[n].type ) {
      case AdbTreeEntry::FieldDate:
      case AdbTreeEntry::FieldURL:
      case AdbTreeEntry::FieldPhone:
      case AdbTreeEntry::FieldNum:
        // fall through -- for now they're the same as text
      case AdbTreeEntry::FieldText:
        break;

      default:
        continue;
    }

    dc.GetTextExtent(wxGetTranslation(fields[n].label), &width, NULL);
    if ( width > widthMax )
      widthMax = width;
  }

  // now create the controls
  wxControl *last = NULL; // last control created
  for ( n = 0; n < nCount; n++ ) {
    switch ( fields[n].type ) {
      case AdbTreeEntry::FieldDate:
      case AdbTreeEntry::FieldURL:
      case AdbTreeEntry::FieldPhone:
      case AdbTreeEntry::FieldNum:
        // fall through -- for now they're the same as text
      case AdbTreeEntry::FieldText:
        last = CreateTextWithLabel(wxGetTranslation(fields[n].label), widthMax, last);
        break;

      case AdbTreeEntry::FieldMemo:
        last = CreateMultiLineText(wxGetTranslation(fields[n].label), last);
        break;

      case AdbTreeEntry::FieldList:
        last = CreateListBox(wxGetTranslation(fields[n].label), last);
        break;

      case AdbTreeEntry::FieldBool:
        last = CreateCheckBox(wxGetTranslation(fields[n].label), last);
        break;

      default:
        wxFAIL_MSG(_T("unknown field type in LayoutControls"));
    }

    wxCHECK_RET( last, _T("control creation failed") );

    entries.Add(last);
  }
}

void wxAdbPage::LayoutBelow(wxControl *control,
                            wxStaticText *label,
                            wxControl *last)
{
  wxLayoutConstraints *c;
  c = new wxLayoutConstraints;
  SetTopConstraint(c, last);
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  c->width.AsIs();
  c->height.AsIs();
  label->SetConstraints(c);

  c = new wxLayoutConstraints;
  c->top.Below(label, LAYOUT_Y_MARGIN);
  c->left.SameAs(this, wxLeft, LAYOUT_X_MARGIN);
  c->right.SameAs(this, wxRight, LAYOUT_X_MARGIN);
  c->bottom.SameAs(this, wxBottom, LAYOUT_Y_MARGIN);
  control->SetConstraints(c);
}

// -----------------------------------------------------------------------------
// notebook pages
// -----------------------------------------------------------------------------

void wxAdbEMailPage::SetData(const AdbEntry& data)
{
  wxAdbPage::SetData(data);

  wxListBox *listbox = GetListBox();
  wxString str;
  listbox->Clear();
  size_t nCount = data.GetEMailCount();
  for ( size_t n = 0; n < nCount; n++ ) {
    data.GetEMail(n, &str);
    listbox->Append(str);
  }

  m_bListboxModified = FALSE;
}

void wxAdbEMailPage::SaveChanges(AdbEntry& data)
{
  // base version will save the text fields
  wxAdbPage::SaveChanges(data);

  // don't do unnecessary work
  if ( m_bListboxModified ) {
    data.ClearExtraEMails();
    wxListBox *listbox = GetListBox();
#if wxCHECK_VERSION(2, 3, 2)
    size_t nCount = listbox->GetCount();
#else
    size_t nCount = listbox->Number();
#endif
    for ( size_t n = 0; n < nCount; n++ ) {
      data.AddEMail(listbox->GetString(n));
    }
  }
}

void wxAdbEMailPage::OnCheckBox(wxCommandEvent&)
{
  SetDirty();
}

void wxAdbEMailPage::OnNewEMail(wxCommandEvent&)
{
  wxString str;
  if ( MInputBox(&str, _("Add another address"),
                 _("E-mail &address:"), this) ) {
    GetListBox()->Append(str);

    SetDirty();
    m_bListboxModified = TRUE;
  }
}

void wxAdbEMailPage::OnModifyEMail(wxCommandEvent&)
{
  wxListBox *listbox = GetListBox();
  int nSel = listbox->GetSelection();
  if ( nSel == -1 ) {
    wxLogError(_("Please select an entry to modify it."));

    return;
  }

  wxString str = listbox->GetString(nSel);
  if ( MInputBox(&str, _("Modify address"), _("E-mail &address:"), this) ) {
    listbox->SetString(nSel, str);

    SetDirty();
    m_bListboxModified = TRUE;
  }
}

void wxAdbEMailPage::OnDeleteEMail(wxCommandEvent&)
{
  wxListBox *listbox = GetListBox();
  int nSel = listbox->GetSelection();
  if ( nSel == -1 ) {
    wxLogError(_("Please select an entry to delete."));

    return;
  }

  SetDirty();
  listbox->Delete(nSel);
  m_bListboxModified = TRUE;
}

wxAdbAddrPage::wxAdbAddrPage(wxNotebook *notebook, const wxChar *title,
                             int idImage, bool bOffice)
             : wxAdbPage(notebook, title, idImage,
                         bOffice ? AdbField_O_AddrPageFirst
                                 : AdbField_H_AddrPageFirst,
                         bOffice ? AdbField_O_AddrPageLast
                                 : AdbField_H_AddrPageLast)
{
}

// -----------------------------------------------------------------------------
// Base class for ADB entries and groups
// -----------------------------------------------------------------------------

// this returns the fully qualified name (starts with the ADB name)
wxString AdbTreeElement::GetFullName() const
{
  // this should be a virtual function, but we prefer to write bad code here
  // (bad because AdbTreeElement shouldn't know about AdbTreeBook at all) than
  // to augment the size of AdbTreeElement which must be as small as possible.
  wxString strPath;
  if ( IsRoot() )
    strPath = _T("/");
  else if ( GetParent()->IsRoot() )
    strPath << _T("/") << ((AdbTreeBook *)this)->GetName();
  else {
    strPath << GetParent()->GetFullName() << _T("/") << m_name;
  }

  return strPath;
}

void AdbTreeElement::CopyData(const AdbTreeElement& other)
{
  // as explained near the declaration, this is morally a pure virtual function
  if ( IsGroup() ) {
    ((AdbTreeNode *)this)->CopyData((const AdbTreeNode&)other);
  }
  else {
    ((AdbTreeEntry *)this)->CopyData((const AdbTreeEntry&)other);
  }
}

void AdbTreeElement::ClearDirtyFlag()
{
  // another "pure virtual function"
  if ( IsGroup() ) {
    ((AdbTreeNode *)this)->ClearDirty();
  }
  else {
    ((AdbTreeEntry *)this)->ClearDirty();
  }
}

void AdbTreeElement::TreeInsert(wxTreeCtrl& tree)
{
  int image = -1;
  switch ( m_kind )
  {
  case TreeElement_Entry:
     image = wxAdbTree::Address;
     break;

  case TreeElement_Group:
  {
     AdbElement *group = ((AdbTreeNode *)this)->AdbGroup();
     if(group)
        image = group->GetIconId();
     if(image == -1)
        image = wxAdbTree::Closed;
     break;
  }
  case TreeElement_Book:
  {
     AdbBook *book = ((AdbTreeBook *)this)->GetBook();
     if(book)
        image = book->GetIconId();
     if(image == -1)
        image = wxAdbTree::Book;
     break;
  }
  case TreeElement_Root:
     image = wxAdbTree::Library;
     break;

  default:
     wxFAIL_MSG(_T("unknown tree element type"));
  }


  if ( IsRoot() ) {
    SetId(tree.AddRoot(wxString(_("Address Books")), image, image, this));
    tree.SetItemHasChildren(GetId());
  }
  else {
    // we want to have all the groups before all the items
    if ( IsGroup() ) {
      wxTreeItemId newItemId;
      const wxTreeItemId& lastGroupId = GetParent()->GetLastGroupId();
      if ( lastGroupId.IsOk() ) {
        newItemId = tree.InsertItem(GetParent()->GetId(), lastGroupId,
                                    GetName(), image, image, this);
      }
      else {
        // no last group, this is the first one
        newItemId = tree.AppendItem(GetParent()->GetId(), GetName(),
                                    image, image, this);
      }

      SetId(newItemId);
      GetParent()->SetLastGroupId(newItemId);
    }
    else {
      // it's an item, insert it in the end
      SetId(tree.AppendItem(GetParent()->GetId(), GetName(),
                            image, image, this));
    }

    tree.SetItemHasChildren(GetId(), IsGroup());
  }
}

// -----------------------------------------------------------------------------
// an ADB entry
// -----------------------------------------------------------------------------

AdbTreeEntry::AdbTreeEntry(const wxString& name,
                           AdbTreeNode *parent,
                           bool onClipboard)
            : AdbTreeElement(TreeElement_Entry, name, parent, onClipboard)
{
}

// this function either copies data to the clipboard or from it
void AdbTreeEntry::CopyData(const AdbTreeEntry& other)
{
  if ( IsOnClipboard() ) {
    // currently it's not possible - if it changes later, this assert will
    // remind that this case has never been tested
    wxCHECK_RET( !m_data, _T("copying to item which already has some data?") );

    // we're copying data to the clipboard
    AdbEntry *otherEntry = other.GetAdbEntry();
    wxCHECK_RET( otherEntry, _T("can't copy from entry without data") );

    m_data = new AdbData(*otherEntry);

    otherEntry->DecRef();
  }
  else {
    // we should be copying from clipboard to a normal entry
    AdbEntry *entry = GetAdbEntry();

    wxCHECK_RET( other.IsOnClipboard() && entry, _T("error copying data") );

    other.m_data->Copy(entry);
    entry->DecRef();
  }

  m_name = other.GetName();
}

// -----------------------------------------------------------------------------
// a temporary placeholder for ADB data
// -----------------------------------------------------------------------------

AdbData::AdbData(const AdbEntry& entry)
{
  for ( size_t i = 0; i < AdbField_Max; i++ )
  {
    entry.GetField(i, &m_fields[i]);
  }
}

void AdbData::Copy(AdbEntry *entry)
{
  for ( size_t i = 0; i < AdbField_Max; i++ )
  {
    entry->SetField(i, m_fields[i]);
  }
}

// -----------------------------------------------------------------------------
// a group of entries
// -----------------------------------------------------------------------------

// normal ctor
AdbTreeNode::AdbTreeNode(const wxString& name,
                         AdbTreeNode *parent,
                         bool onClipboard)
           : AdbTreeElement(TreeElement_Group, name, parent, onClipboard)
{
  m_bWasExpanded = FALSE;
  m_pGroup = NULL;
}

// recursively copy the data
void AdbTreeNode::CopyData(const AdbTreeNode& other)
{
  wxASSERT( m_children.Count() == 0 ); // we must be empty

  // we're either copying to the clipboard or from it, so if the other item is
  // on the clipboard we're not and ice versa
  bool onClipboard = !other.IsOnClipboard();

  if ( !onClipboard ) {
    // create the associated ADB group
    EnsureHasGroup();
  }

  AdbTreeElement *child, *current;
  size_t nCount = other.m_children.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    current = other.m_children[n];
    if ( current->IsGroup() )
      child = new AdbTreeNode(current->GetName(), this, onClipboard);
    else
      child = new AdbTreeEntry(current->GetName(), this, onClipboard);
    child->CopyData(*current);
  }
}

void AdbTreeNode::ClearDirty()
{
  size_t nCount = m_children.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    m_children[n]->ClearDirtyFlag();
  }
}

// this function doesn't (and shouldn't!) change the current config path
void AdbTreeNode::LoadChildren()
{
  // only load children once
  if ( !m_children.IsEmpty() )
    return;

  EnsureHasGroup();

  wxCHECK_RET( m_pGroup, _T("AdbTreeNode without associated AdbEntryGroup") );

  wxArrayString aNames;
  size_t n, nCount = m_pGroup->GetEntryNames(aNames);
  for ( n = 0; n < nCount; n++ ) {
    (void)new AdbTreeEntry(aNames[n], this);
  }

  nCount = m_pGroup->GetGroupNames(aNames);
  for ( n = 0; n < nCount; n++ ) {
    (void)new AdbTreeNode(aNames[n], this);
  }
}

void AdbTreeNode::LoadAllData()
{
  LoadChildren(); // NOP if it was already done

  AdbTreeElement *current;
  size_t nCount = m_children.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    current = m_children[n];
    if ( current->IsGroup() )
      ((AdbTreeNode *)current)->LoadAllData();
    /* @@@@
    else
      (void)((AdbTreeEntry *)current)->GetData();
    */
  }
}

AdbTreeElement *AdbTreeNode::CreateChild(const wxString& name, bool bGroup)
{
  wxCHECK_MSG( !IsRoot(), NULL, _T("only address books can be created at root") );

  wxString strWhat = bGroup ? _("Group") : _("Entry");

  EnsureHasGroup();

  // first check that it doesn't already exist
  if ( m_pGroup->Exists(name) ) {
    wxLogError(_("%s '%s' already exists in this group."),
               strWhat.c_str(), name.c_str());

    return NULL;
  }

  // if the entries which already exist in the config file now were not yet
  // loaded they will be never loaded because LoadChildren() checks if we
  // already have some children and does nothing if we do - so load them now.
  LoadChildren();

  // create the new entry in the ADB and if it succeeds also create the tree
  // element corresponding to it
  AdbTreeElement *pTreeEntry = NULL;
  AdbElement *pAdbEntry = NULL;
  if ( bGroup ) {
    pAdbEntry = m_pGroup->CreateGroup(name);
    if ( pAdbEntry )
      pTreeEntry = new AdbTreeNode(name, this);
  }
  else {
    pAdbEntry = m_pGroup->CreateEntry(name);
    if ( pAdbEntry )
      pTreeEntry = new AdbTreeEntry(name, this);
  }

  // we don't need it for now
  if ( pAdbEntry )
    pAdbEntry->DecRef();

  return pTreeEntry;
}

void AdbTreeNode::DeleteChild(AdbTreeElement *child)
{
  EnsureHasGroup();

  m_children.Remove(child);

  switch ( child->GetKind() ) {
    case TreeElement_Entry:
      m_pGroup->DeleteEntry(child->GetName());
      break;

    case TreeElement_Group:
      m_pGroup->DeleteGroup(child->GetName());

      // fall through - for m_idLastGroup processing

    case TreeElement_Book:
      // there is a big problem with deleting the last group: our m_idLastGroup
      // must be changed because it corresponds to a non-existing tree item.
      if ( child->GetId() == m_idLastGroup ) {
        // unfortunately, we don't have the old valu any more, so we must search
        // for it ourselves - TODO. For now we will just insert it in the
        // beginning...
        m_idLastGroup = 0l;
      }

      // do nothing for address books (could delete the file...)
      break;

    default:
      wxFAIL_MSG(_T("something weird in our ADB tree"));
  }

  // don't do this, the tree will delete the item itself
  //delete child;
}

void AdbTreeNode::Refresh(wxTreeCtrl& tree)
{
  m_children.Empty();
  tree.DeleteChildren(GetId());
  m_bWasExpanded = FALSE;
  ExpandFirstTime(tree);
}

AdbTreeElement *AdbTreeNode::FindChild(const wxChar *szName)
{
  // TODO we should sort the items in alphabetical order and use binary search
  //      instead of linear search
  size_t nCount = m_children.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    if ( m_children[n]->GetName() == szName )
      return m_children[n];
  }

  return NULL;
}

bool AdbTreeNode::ExpandFirstTime(wxTreeCtrl& tree)
{
  wxCHECK( !m_bWasExpanded, TRUE );  // must be only called once

  m_bWasExpanded = TRUE;
  m_idLastGroup = 0l;

  LoadChildren();

  size_t nCount = GetChildrenCount();
  for ( size_t n = 0; n < nCount; n++ ) {
    GetChild(n)->TreeInsert(tree);
  }

  return nCount != 0;
}

wxString AdbTreeNode::GetWhere() const
{
  wxString strWhere, strGroup = GetName();
  if ( strGroup.empty() ) {
    strWhere << _("at the root level of the addressbook '")
             << ((AdbTreeBook *)this)->GetName() << '\'';
  }
  else {
    strWhere << _("in group '") << strGroup << '\'';
  }

  return strWhere;
}

AdbTreeNode::~AdbTreeNode()
{
#if DELETE_TREE_CHILDREN
  size_t nCount = m_children.Count();
  for ( size_t n = 0; n < nCount; n++ ) {
    AdbTreeElement *child = m_children[n];
    delete child;
  }
#endif // 0

  SafeDecRef(m_pGroup);
}

// -----------------------------------------------------------------------------
// an address book
// -----------------------------------------------------------------------------
AdbTreeBook::AdbTreeBook(AdbTreeRoot *root,
                         const wxString& filename,
                         AdbDataProvider *pProvider,
                         wxString *pstrProviderName)
           : AdbTreeNode()
{
  m_kind = TreeElement_Book;

  m_parent = root;
  m_parent->AddChild(this);

  m_data = NULL;

  m_pGroup = m_pBook = root->GetAdbManager()->
    CreateBook(filename, pProvider, pstrProviderName);

  m_name = m_pBook->GetName();
}

AdbTreeBook::~AdbTreeBook()
{
  // must delete children first to give them the last chance to save changes
  size_t nCount = m_children.Count();
  for ( size_t n = 0; n < nCount; n++ )
    ; // delete m_children[n];
  m_children.Clear();

  SafeDecRef(m_pBook);
  m_pGroup = NULL;  // prevent it from being unlocked in ~AdbTreeNode
}

// -----------------------------------------------------------------------------
// the root of ADB hierarchy
// -----------------------------------------------------------------------------

AdbTreeRoot::AdbTreeRoot(wxArrayString& astrAdb, wxArrayString& astrProviders)
           : m_astrAdb(astrAdb), m_astrProviders(astrProviders)
{
  m_kind = TreeElement_Root;
  m_name = _T("root entry");

  m_bWasExpanded = FALSE;

  m_pManager = AdbManager::Get();
}

// find the ADB by name
AdbTreeElement *AdbTreeRoot::FindChild(const wxChar *szName)
{
  size_t nCount = m_children.Count();
  wxString strAdbName;
  for ( size_t n = 0; n < nCount; n++ ) {
    strAdbName = ((AdbTreeBook *)m_children[n])->GetName();

    if ( strAdbName.IsSameAs(szName, wxARE_FILENAMES_CASE_SENSITIVE) )
      return m_children[n];
  }

  return NULL;
}

// create all address books
void AdbTreeRoot::LoadChildren()
{
  // do it only once
  if ( !m_children.IsEmpty() )
    return;

  wxString strProv;
  AdbDataProvider *pProvider;
  size_t nAdbCount = m_astrAdb.Count();
  for ( size_t nAdb = 0; nAdb < nAdbCount; nAdb++ ) {
    strProv = m_astrProviders[nAdb];
    if ( strProv.empty() )
      pProvider = NULL;
    else
      pProvider = AdbDataProvider::GetProviderByName(strProv);

    (void)new AdbTreeBook(this, m_astrAdb[nAdb], pProvider, &strProv);

    if ( pProvider == NULL ) {
      // now we have the name of provider which was used for creation
      m_astrProviders[nAdb] = strProv;
    }
    else {
      pProvider->DecRef();
    }
  }
}

// release AdbManager
AdbTreeRoot::~AdbTreeRoot()
{
  // to match the Get() in ctor
  AdbManager::Unget();
}

// -----------------------------------------------------------------------------
// AdbTreeElement - the base class for all others
// -----------------------------------------------------------------------------

AdbTreeElement::AdbTreeElement(TreeElement kind,
                               const wxString& name,
                               AdbTreeNode *parent,
                               bool onClipboard)
              : m_name(name)
{
   m_parent = parent;
   if ( m_parent )
      m_parent->AddChild(this);

   m_kind = kind;
   if ( onClipboard )
      m_kind = (TreeElement)(m_kind | TreeElement_Clipboard);

   m_data = NULL;
}

/* vi: set cin ts=2 sw=2 et list tw=80: */
