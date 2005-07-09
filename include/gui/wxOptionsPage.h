///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxOptionsPage.h - declaration of pages of the options
//              notebook
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.12.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _GUI_WXOPTIONSPAGE_H
#define _GUI_WXOPTIONSPAGE_H

#ifndef USE_PCH
#       include "guidef.h"              // for GET_PARENT_OF_CLASS
#       include <wx/control.h>
#       include <wx/dynarray.h>
#endif // USE_PCH

#include "gui/wxDialogLayout.h"         // for wxNotebookPageBase

WX_DEFINE_ARRAY(wxControl *, ArrayControls);
WX_DEFINE_ARRAY_INT(bool, ArrayBool);

class MFolder;

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

// control ids
enum
{
   wxOptionsPage_BtnNew = 20000,
   wxOptionsPage_BtnModify,
   wxOptionsPage_BtnDelete
};

// ----------------------------------------------------------------------------
// ConfigValue is a default value for an entry of the options page
// ----------------------------------------------------------------------------

// a structure giving the name of config key and it's default value which may
// be either numeric or a string (no special type for boolean values right
// now)
struct ConfigValueDefault
{
   ConfigValueDefault(const wxChar *name_, long value)
      { bNumeric = TRUE; name = name_; lValue = value; }

   ConfigValueDefault(const wxChar *name_, const wxChar *value)
      { bNumeric = FALSE; name = name_; szValue = value; }

   long GetLong() const { wxASSERT( bNumeric ); return lValue; }
   const wxChar *GetString() const { wxASSERT( !bNumeric ); return szValue; }

   bool IsNumeric() const { return bNumeric; }

   const wxChar *name;
   union
   {
      long        lValue;
      const wxChar *szValue;
   };
   bool bNumeric;
};

struct ConfigValueNone : public ConfigValueDefault
{
   ConfigValueNone() : ConfigValueDefault(_T(""),0L) { }
};

typedef const ConfigValueDefault *ConfigValuesArray;

// -----------------------------------------------------------------------------
// a page which performs the data transfer between the associated profile
// section values and the controls in the page (text fields, checkboxes,
// listboxes and sub dialogs are currently supported)
// -----------------------------------------------------------------------------

class wxOptionsPage : public wxNotebookPageBase
{
public:
   // FieldType and FieldFlags are stored in one 'int', so the bits should be
   // shared...
   enum FieldType
   {
      Field_Text   = 0x0001, // one line text field
      Field_Number = 0x0002, // the same as text but accepts only digits
      Field_List   = 0x0004, // list of values - represented as a listbox
      Field_Bool   = 0x0008, // a checkbox
      Field_File   = 0x0010, // a text entry with a "Browse..." for file button
      Field_Message= 0x0020, // just a bit of explaining text, no input
      Field_Radio  = 0x0040, // offering the radiobox
      Field_Combo  = 0x0080, // offering 0,1,2,..n, from a combobox
      Field_Color  = 0x0100, // a text entry with a "Browse for colour" button
      Field_SubDlg = 0x0200, // a button invoking another dialog
      Field_XFace  = 0x0400, // a wxXFaceButton invoking another dialog
      Field_Folder = 0x0800, // a text entry with a "Browse for folder" button
      Field_Passwd = 0x1000, // a masked text entry for the password
      Field_Dir    = 0x2000, // a text entry with a "Browse..." for dir button
      Field_Font   = 0x4000, // a text entry with a "Browse..." for font button
      Field_Type   = 0xffff  // bit mask selecting the type
   };

   enum FieldFlags
   {
      Field_Vital    = 0x10000000, // vital setting, test after change
      Field_Restart  = 0x20000000, // will only take effect during next run
      Field_Advanced = 0x40000000, // don't show this field in novice mode
      Field_Global   = 0x80000000, // no identity override for this field
      Field_AppWide  = 0x01000000, // same setting for all folders
      Field_Inverse  = 0x02000000, // invert the value (Field_Bool only)
      Field_FileSave = Field_Inverse, // only for Field_File
      Field_NotApp   = 0x04000000, // per folder option, opposite of AppWide
      Field_Flags    = 0xff000000  // bit mask selecting the flags
   };

   struct FieldInfo
   {
      const wxChar *label;   // which is shown in the dialog
      int           flags;   // containts the type and the flags (see above)
      int           enable;  // disable this field if "enable" field is unchecked
   };

   typedef const FieldInfo *FieldInfoArray;

   // get the type and the flags of the field
   FieldType GetFieldType(size_t n) const
      { return (FieldType)(m_aFields[n].flags & Field_Type); }

   int GetFieldFlags(size_t n) const
      { return m_aFields[n].flags & Field_Flags; }

   // almost default ctor: this just takes the parent notebook
   //
   // you'll need to call Create() later
   wxOptionsPage(wxNotebook *parent) : wxNotebookPageBase(parent) { }

   // initialize this page and add it to the notebook (with the image refering
   // to the notebook's imagelist)
   bool Create(FieldInfoArray aFields,
               ConfigValuesArray aDefaults,
               size_t nFirst,
               size_t nLast,
               wxNotebook *notebook,
               const wxChar *title,
               Profile *profile,
               int helpId = -1,
               int image = -1);

   // this ctor is equivalent to the other one and Create()
   wxOptionsPage(FieldInfoArray aFields,
                 ConfigValuesArray aDefaults,
                 size_t nFirst, size_t nLast,
                 wxNotebook *parent,
                 const wxChar *title,
                 Profile *profile,
                 int helpID = -1,
                 int image = -1);

   virtual ~wxOptionsPage();


   // transfer data to/from the controls
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // to change the profile associated with the page:
   void SetProfile(Profile *profile);

   // callbacks
      // called when text zone content changes
   void OnChange(wxEvent& event);

      // called when a {radio/combo/check}box value changes
   void OnControlChange(wxCommandEvent& event);

      // called when a textctrl value changes
   void OnTextChange(wxCommandEvent& event);

   // enable/disable controls (better than OnUpdateUI here)
   void UpdateUI();

   // called when any control changes, returns TRUE if processed
   bool OnChangeCommon(wxControl *control);

   /// Returns the numeric help id.
   int HelpId(void) const { return m_HelpId; }

protected:
   /// get the name of the folder we're editing the options of
   String GetFolderName() const;

   // get the parent dialog
   wxOptionsEditDialog *GetOptionsDialog() const
   {
      wxOptionsEditDialog *
         dialog = GET_PARENT_OF_CLASS(this, wxOptionsEditDialog);
      ASSERT_MSG( dialog, _T("options page outside of options dialog?") );
      return dialog;
   }

   // range of our controls in m_aFields
   size_t m_nFirst, m_nLast;

   // the controls description
   FieldInfoArray m_aFields;

   // and their default values
   ConfigValuesArray m_aDefaults;

   // create controls corresponding to the entries from m_nFirst to m_nLast in
   // the array m_aFields
   void CreateControls();

   // we need a pointer to the profile to write to
   Profile *m_Profile;

   // get the control with "right" index
   wxControl *GetControl(size_t /* ConfigFields */ n) const
      { return m_aControls[n - m_nFirst]; }

   // check whether the given object is the field with this index
   bool IsControl(wxObject *obj, size_t n) const
   {
      return GetControl(n) == obj;
   }

   // get the dirty flag for the control with index n
   bool IsDirty(size_t n) const
      { return m_aDirtyFlags[n- m_nFirst]; }

   // reset the dirty flag for the control with index n
   void ClearDirty(size_t n) const
      { m_aDirtyFlags[n - m_nFirst] = false; }

   // type safe access to the control text
   wxString GetControlText(size_t /* ConfigFields */ n) const
   {
      wxASSERT( GetControl(n)->IsKindOf(CLASSINFO(wxTextCtrl)) );

      return ((wxTextCtrl *)GetControl(n))->GetValue();
   }

   // methods to deal with handling the events from the listbox buttons: a
   // listbox always has 3 (Add/Edit/Delete) buttons in an option page and
   // these methods handle the events from them in a standard way

   // the listbox data: m_lboxData below must be init by the class in order
   // for all this mess to work at all
   struct LboxData
   {
      LboxData() { m_next = NULL; }

      int m_idListbox;           // id
      wxString m_lboxDlgTitle,   // the title for Add/Modifydialogs
               m_lboxDlgPrompt,  //     prompt
               m_lboxDlgPers;    // the config location for dialog pers data

      LboxData *m_next;          // next listbox data in the linked list
   };

   // handle the event: call one of OnXXX() below
   void OnListBoxButton(wxCommandEvent& event);

   // virtual functions called by OnListBoxButton(), the base class implements
   // them by using a standard text input dialog
   virtual bool OnListBoxAdd(wxListBox *lbox, const LboxData& lboxData);
   virtual bool OnListBoxModify(wxListBox *lbox, const LboxData& lboxData);
   virtual bool OnListBoxDelete(wxListBox *lbox, const LboxData& lboxData);

   // update the listbox buttons state
   void OnUpdateUIListboxBtns(wxUpdateUIEvent& event);

   // find the listbox and the associated data by the event generated by one
   // of its buttons: this relies on the fact that listbox is set as client
   // data of all of its buttons
   //
   // returns false if something bad happened, true if everything was ok and
   // the pointers being returned are valid
   bool GetListboxFromButtonEvent(const wxEvent& event,
                                  wxListBox **pLbox,
                                  LboxData **pData = NULL) const;

   // array of LboxData or NULL if we have no listboxes
   LboxData *m_lboxData;

   // numeric help id
   int m_HelpId;

private:
   // the controls themselves (indexes in this array are shifted by m_nFirst
   // with respect to ConfigFields enum!) - use GetControl()
   ArrayControls m_aControls;

   // the controls which require restarting the program to take effect
   ArrayControls m_aRestartControls;

   // the controls which are so important for the proper program working that
   // we propose to the user to test the new settings before accepting them
   ArrayControls m_aVitalControls;

   // n-th element tells if the n-th control in m_aControls is dirty or not
   ArrayBool m_aDirtyFlags;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPage)
};

// ----------------------------------------------------------------------------
// a page which gets the information about its controls from the static array
// ms_aFields - this is the base class for all of the standard option pages
// while wxOptionsPage itself may also be used for run-time construction of the
// page
// ----------------------------------------------------------------------------

class wxOptionsPageStandard : public wxOptionsPage
{
public:
   // ctor will create the controls corresponding to the fields from nFirst to
   // nLast in ms_aFields
   wxOptionsPageStandard(wxNotebook *parent,
                         const wxChar *title,
                         Profile *profile,
                         int nFirst,
                         size_t nLast,
                         int helpID = -1);

   // get the type and the flags of one of the standard field
   static FieldType GetStandardFieldType(size_t n)
      { return (FieldType)(ms_aFields[n].flags & Field_Type); }

   static int GetStandardFieldFlags(size_t n)
      { return ms_aFields[n].flags & Field_Flags; }

   // array of all field descriptions
   static const FieldInfo ms_aFields[];

   // and of their default values
   static const ConfigValueDefault ms_aConfigDefaults[];

protected:
   // ctor for pages created via static New()
   wxOptionsPageStandard(wxNotebook *parent,
                         const wxChar *title,
                         Profile *profile,
                         FieldInfoArray aFields,
                         ConfigValuesArray aDefaults,
                         size_t nFields,
                         size_t nOffset = 0,
                         int helpID = -1,
                         int image = -1);

private:
   DECLARE_NO_COPY_CLASS(wxOptionsPageStandard)
};

// ----------------------------------------------------------------------------
// a dynamic options page - pages deriving from this class can be configured
// during run-time or used by external modules
// ----------------------------------------------------------------------------

class wxOptionsPageDynamic : public wxOptionsPage
{
public:
   // if you use this ctor, you must use Create() later
   wxOptionsPageDynamic(wxNotebook *parent) : wxOptionsPage(parent) { }

   // the aFields array contains the controls descriptions
   bool Create(wxNotebook *parent,
               const wxChar *title,
               Profile *profile,
               FieldInfoArray aFields,
               ConfigValuesArray aDefaults,
               size_t nFields,
               size_t nOffset = 0,
               int helpID = -1,
               int image = -1);

   // ctor doing all at once
   wxOptionsPageDynamic(wxNotebook *parent,
                        const wxChar *title,
                        Profile *profile,
                        FieldInfoArray aFields,
                        ConfigValuesArray aDefaults,
                        size_t nFields,
                        size_t nOffset = 0,
                        int helpID = -1,
                        int image = -1);

   // for wxOptionsPageDesc usage
   static wxOptionsPage *New(wxNotebook *parent,
                             const wxChar *title,
                             Profile *profile,
                             FieldInfoArray aFields,
                             ConfigValuesArray aDefaults,
                             size_t nFields,
                             size_t nOffset = 0,
                             int helpID = -1,
                             int image = -1)
   {
      return new wxOptionsPageDynamic(parent, title, profile,
                                      aFields, aDefaults, nFields, nOffset,
                                      helpID, image);
   }

private:
   DECLARE_NO_COPY_CLASS(wxOptionsPageDynamic)
};

// the data from which wxOptionsPageDynamic may be created by the notebook -
// using this structure is more convenient than passing all these parameters
// around separately
//
// this used to be a dummy struct, now it is a real class but we'd have to
// change all fwd declarations if we changed it here, so leave it alone
struct wxOptionsPageDesc
{
public:
   // a pointer to creation function may be specified to create a page of given
   // type instead of generic wxOptionsPageDynamic
   typedef wxOptionsPage *(*NewFunc_t)
                           (
                              wxNotebook *parent,
                              const wxChar *title,
                              Profile *profile,
                              wxOptionsPage::FieldInfoArray aFields,
                              ConfigValuesArray aDefaults,
                              size_t nFields,
                              size_t nOffset,
                              int helpID,
                              int image
                           );

   // default ctor needed to be able to create arrays of these objects, but it
   // really should never be used
   wxOptionsPageDesc()
   {
   }

   wxOptionsPageDesc(const wxChar *title,
                     const wxChar *image,
                     int helpId,
                     const wxOptionsPage::FieldInfo *aFields,
                     ConfigValuesArray aDefaults,
                     size_t nFields,
                     size_t nOffset = 0,
                     NewFunc_t createFunc = wxOptionsPageDynamic::New)
      : m_title(title),
        m_image(image),
        m_helpId(helpId),
        m_aFields(aFields),
        m_aDefaults(aDefaults),
        m_nFields(nFields),
        m_nOffset(nOffset),
        m_pfnNew(createFunc)
   {
   }

   // create a page from this description
   wxOptionsPage *New(wxNotebook *parent, Profile *profile, int image) const
   {
      return (*m_pfnNew)
             (
                parent,
                wxGetTranslation(m_title),
                profile,
                m_aFields,
                m_aDefaults,
                m_nFields,
                m_nOffset,
                m_helpId,
                image
             );
   }

   const String& GetImage() const { return m_image; }

private:
   String m_title;        // the page title in the notebook
   String m_image;        // image

   int m_helpId;

   // the fields description
   const wxOptionsPage::FieldInfo *m_aFields;
   ConfigValuesArray m_aDefaults;
   size_t m_nFields,
          m_nOffset;

   NewFunc_t m_pfnNew;
};

// ----------------------------------------------------------------------------
// standard pages
// ----------------------------------------------------------------------------

// user identity
class wxOptionsPageIdent : public wxOptionsPageStandard
{
public:
   wxOptionsPageIdent(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageIdent)
};

// network configuration page
class wxOptionsPageNetwork : public wxOptionsPageStandard
{
public:
   wxOptionsPageNetwork(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // for wxOptionsPageDesc
   static wxOptionsPage *New(wxNotebook *parent,
                             const wxChar *title,
                             Profile *profile,
                             FieldInfoArray aFields,
                             ConfigValuesArray aDefaults,
                             size_t nFields,
                             size_t nOffset = 0,
                             int helpID = -1,
                             int image = -1)
   {
      return new wxOptionsPageNetwork(parent, title, profile,
                                      aFields, aDefaults, nFields, nOffset,
                                      helpID, image);
   }

private:
   // ctor for New()
   wxOptionsPageNetwork(wxNotebook *parent,
                        const wxChar *title,
                        Profile *profile,
                        FieldInfoArray aFields,
                        ConfigValuesArray aDefaults,
                        size_t nFields,
                        size_t nOffset = 0,
                        int helpID = -1,
                        int image = -1)
      : wxOptionsPageStandard(parent, title, profile,
                              aFields, aDefaults, nFields, nOffset,
                              helpID, image)
   {
   }


   wxString m_oldAuthsDisabled;

   DECLARE_NO_COPY_CLASS(wxOptionsPageNetwork)
};

// new mail handling options
class wxOptionsPageNewMail : public wxOptionsPageStandard
{
public:
   wxOptionsPageNewMail(wxNotebook *parent, Profile *profile);
   virtual ~wxOptionsPageNewMail();

   void OnButton(wxCommandEvent&);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   // create m_folder for our m_Profile
   bool GetFolderFromProfile();

   // remember the old values for the settings in these variables
   long m_nIncomingDelay,
        m_nPingDelay;

   bool m_collectOld,
        m_monitorOld;

   // the folder we're editing properties of or NULL if this is a global dialog
   MFolder *m_folder;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageNewMail)
};

// settings concerning the compose window
class wxOptionsPageCompose : public wxOptionsPageStandard
{
public:
   wxOptionsPageCompose(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent& event);

   // for wxOptionsPageDesc
   static wxOptionsPage *New(wxNotebook *parent,
                             const wxChar *title,
                             Profile *profile,
                             FieldInfoArray aFields,
                             ConfigValuesArray aDefaults,
                             size_t nFields,
                             size_t nOffset = 0,
                             int helpID = -1,
                             int image = -1)
   {
      return new wxOptionsPageCompose(parent, title, profile,
                                      aFields, aDefaults, nFields, nOffset,
                                      helpID, image);
   }

private:
   // ctor for New()
   wxOptionsPageCompose(wxNotebook *parent,
                        const wxChar *title,
                        Profile *profile,
                        FieldInfoArray aFields,
                        ConfigValuesArray aDefaults,
                        size_t nFields,
                        size_t nOffset = 0,
                        int helpID = -1,
                        int image = -1)
      : wxOptionsPageStandard(parent, title, profile,
                              aFields, aDefaults, nFields, nOffset,
                              helpID, image)
   {
   }

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageCompose)
};

// settings concerning the message view window
class wxOptionsPageMessageView : public wxOptionsPageStandard
{
public:
   wxOptionsPageMessageView(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent&);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   // the names of all available viewers
   wxArrayString m_nameViewers;

   // the index of the current viewer in the arryas above or -1
   int m_currentViewer;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageMessageView)
};

// settings concerning the folder view window
class wxOptionsPageFolderView : public wxOptionsPageStandard
{
public:
   wxOptionsPageFolderView(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   void OnButton(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageFolderView)
};

// settings concerning the folder tree
class wxOptionsPageFolderTree : public wxOptionsPageStandard
{
public:
   wxOptionsPageFolderTree(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   bool m_isHomeOrig;

   DECLARE_NO_COPY_CLASS(wxOptionsPageFolderTree)
};

// global folder settings (each folder has its own settings which are changed
// from a separate dialog)
class wxOptionsPageFolders : public wxOptionsPageStandard
{
public:
   wxOptionsPageFolders(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnUpdateUIBtns(wxUpdateUIEvent&);

   void OnAddFolder(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageFolders)
};

#ifdef USE_PYTHON

// all python-related settings
class wxOptionsPagePython : public wxOptionsPageStandard
{
public:
   wxOptionsPagePython(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataFromWindow();

private:
   DECLARE_NO_COPY_CLASS(wxOptionsPagePython)
};

#endif // USE_PYTHON

// all bbdb-related settings
class wxOptionsPageAdb : public wxOptionsPageStandard
{
public:
   wxOptionsPageAdb(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

private:
   DECLARE_NO_COPY_CLASS(wxOptionsPageAdb)
};


// helper apps settings
class wxOptionsPageHelpers : public wxOptionsPageStandard
{
public:
   wxOptionsPageHelpers(wxNotebook *parent, Profile *profile);

private:
   DECLARE_NO_COPY_CLASS(wxOptionsPageHelpers)
};

// sync settings page
class wxOptionsPageSync : public wxOptionsPageStandard
{
public:
   wxOptionsPageSync(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnButton(wxCommandEvent& event);

protected:
   // do we want to use settings synchronization?
   // (may be true, false or -1 if unknown)
   int m_activateSync;

#ifdef OS_WIN
   // do we use config file (or registry, which is default) now?
   int m_usingConfigFile;
#endif // OS_WIN

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageSync)
};

// miscellaneous settings
class wxOptionsPageOthers : public wxOptionsPageStandard
{
public:
   wxOptionsPageOthers(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnButton(wxCommandEvent&);

protected:
   // the old auto save timer interval
   long m_nAutosaveDelay;

   // the old away mode timer interval
   long m_nAutoAwayDelay;

private:
   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxOptionsPageOthers)
};

#ifdef USE_TEST_PAGE

// test page just for testing layout &c
class wxOptionsPageTest : public wxOptionsPageStandard
{
public:
   wxOptionsPageTest(wxNotebook *parent, Profile *profile);
};

#endif // USE_TEST_PAGE

#endif // _GUI_WXOPTIONSPAGE_H
