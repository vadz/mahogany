// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
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
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef _GUI_WXOPTIONSPAGE_H
#define _GUI_WXOPTIONSPAGE_H

#include <wx/dynarray.h>

WX_DEFINE_ARRAY(wxControl *, ArrayControls);
WX_DEFINE_ARRAY(bool, ArrayBool);

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

// control ids
enum
{
   wxOptionsPage_BtnNew,
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
   ConfigValueDefault(const char *name_, long value)
      { bNumeric = TRUE; name = name_; lValue = value; }

   ConfigValueDefault(const char *name_, const char *value)
      { bNumeric = FALSE; name = name_; szValue = value; }

   long GetLong() const { wxASSERT( bNumeric ); return lValue; }
   const char *GetString() const { wxASSERT( !bNumeric ); return szValue; }

   bool IsNumeric() const { return bNumeric; }

   const char *name;
   union
   {
      long        lValue;
      const char *szValue;
   };
   bool bNumeric;
};

struct ConfigValueNone : public ConfigValueDefault
{
   ConfigValueNone() : ConfigValueDefault("none",0L) { }
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
      Field_File   = 0x0010, // a text entry with a "Browse..." button
      Field_Message= 0x0020, // just a bit of explaining text, no input
      Field_Action = 0x0040, // offering the 0,1,2 No,Ask,Yes radiobox
      Field_Combo  = 0x0080, // offering 0,1,2,..n, from a combobox
      Field_Color  = 0x0100, // a text entry with a "Browse for colour" button
      Field_SubDlg = 0x0200, // a button invoking another dialog
      Field_XFace  = 0x0400, // a wxXFaceButton invoking another dialog
      Field_Folder = 0x0800, // a text entry with a "Browse for folder" button
      Field_Passwd = 0x1000, // a masked text entry for the password
      Field_Dir    = 0x2000, // a text entry with a "Browse..." button
      Field_Type   = 0xffff  // bit mask selecting the type
   };

   enum FieldFlags
   {
      Field_Vital   = 0x10000000, // vital setting, test after change
      Field_Restart = 0x20000000, // will only take effect during next run
      Field_Advanced= 0x40000000, // don't show this field in novice mode
      Field_Global  = 0x80000000, // no identity override for this field
      Field_Flags   = 0xf0000000  // bit mask selecting the flags
   };

   struct FieldInfo
   {
      const char *label;   // which is shown in the dialog
      int         flags;   // containts the type and the flags (see above)
      int         enable;  // disable this field if "enable" field is unchecked
   };

   typedef const FieldInfo *FieldInfoArray;

   // get the type and the flags of the field
   FieldType GetFieldType(size_t n) const
      { return (FieldType)(m_aFields[n].flags & Field_Type); }

   FieldFlags GetFieldFlags(size_t n) const
      { return (FieldFlags)(m_aFields[n].flags & Field_Flags); }

   // ctor will add this page to the notebook (with the image refering to the
   // notebook's imagelist)
   wxOptionsPage(FieldInfoArray aFields,
                 ConfigValuesArray aDefaults,
                 size_t nFirst, size_t nLast,
                 wxNotebook *parent,
                 const char *title,
                 Profile *profile,
                 int helpID = -1,
                 int image = -1);
   virtual ~wxOptionsPage() { SafeDecRef(m_Profile); }

   // transfer data to/from the controls
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // to change the profile associated with the page:
   void SetProfile(Profile *profile)
   {
      SafeDecRef(m_Profile);
      m_Profile = profile;
      SafeIncRef(m_Profile);
   }

   // callbacks
      // called when text zone content changes
   void OnChange(wxEvent& event);
      // called when a {radio/combo/check}box value changes
   void OnControlChange(wxEvent& event);

   // enable/disable controls (better than OnUpdateUI here)
   void UpdateUI();

   // called when any control changes, returns TRUE if processed
   bool OnChangeCommon(wxControl *control);

   /// Returns the numeric help id.
   int HelpId(void) const { return m_HelpId; }

protected:
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

   // handle the event: call one of OnXXX() below
   void OnListBoxButton(wxCommandEvent& event);

   // virtual functions called by OnListBoxButton(), the base class implements
   // them by using a standard text input dialog
   virtual bool OnListBoxAdd();
   virtual bool OnListBoxModify();
   virtual bool OnListBoxDelete();

   // update the listbox buttons state
   void OnUpdateUIListboxBtns(wxUpdateUIEvent& event);

   // the listbox data
   int m_idListbox;           // id (no listbox if -1)
   wxString m_lboxDlgTitle,   // the title for Add/Modifydialogs
            m_lboxDlgPrompt,  //     prompt
            m_lboxDlgPers;    // the config location for dialog pers data

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
                         const char *title,
                         Profile *profile,
                         size_t nFirst,
                         size_t nLast,
                         int helpID = -1);

   // get the type and the flags of one of the standard field
   static FieldType GetStandardFieldType(size_t n)
      { return (FieldType)(ms_aFields[n].flags & Field_Type); }

   static FieldFlags GetStandardFieldFlags(size_t n)
      { return (FieldFlags)(ms_aFields[n].flags & Field_Flags); }

   // array of all field descriptions
   static const FieldInfo ms_aFields[];

   // and of their default values
   static const ConfigValueDefault ms_aConfigDefaults[];
};

// ----------------------------------------------------------------------------
// a dynamic options page - pages deriving from this class can be configured
// during run-time or used by external modules
// ----------------------------------------------------------------------------

class wxOptionsPageDynamic : public wxOptionsPage
{
public:
   // the aFields array contains the controls descriptions
   wxOptionsPageDynamic(wxNotebook *parent,
                        const char *title,
                        Profile *profile,
                        FieldInfoArray aFields,
                        ConfigValuesArray aDefaults,
                        size_t nFields,
                        size_t nOffset = 0,
                        int helpID = -1,
                        int image = -1);
};

// the data from which wxOptionsPageDynamic may be created by the notebook -
// using this structure is more convenient than passing all these parameters
// around separately
struct wxOptionsPageDesc
{
   wxOptionsPageDesc()
   {
      nFields = 0;
   }

   wxOptionsPageDesc(const char *title_,
                     const char *image_,
                     int helpId_,
                     const wxOptionsPage::FieldInfo *aFields_,
                     ConfigValuesArray aDefaults_,
                     size_t nFields_,
                     size_t nOffset_ = 0)
      : title(title_),
        image(image_),
        helpId(helpId_),
        aFields(aFields_),
        aDefaults(aDefaults_),
        nFields(nFields_),
        nOffset(nOffset_)
   {
   }

   String title;        // the page title in the notebook
   String image;        // image

   int helpId;

   // the fields description
   const wxOptionsPage::FieldInfo *aFields;
   ConfigValuesArray aDefaults;
   size_t nFields, nOffset;
};

// ----------------------------------------------------------------------------
// standard pages
// ----------------------------------------------------------------------------

// settings concerning the compose window
class wxOptionsPageCompose : public wxOptionsPageStandard
{
public:
   wxOptionsPageCompose(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent&);
   virtual bool TransferDataFromWindow();

private:
   DECLARE_EVENT_TABLE()
};

// settings concerning the message view window
class wxOptionsPageMessageView : public wxOptionsPageStandard
{
public:
   wxOptionsPageMessageView(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
};

// settings concerning the folder view window
class wxOptionsPageFolderView : public wxOptionsPageStandard
{
public:
   wxOptionsPageFolderView(wxNotebook *parent, Profile *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnButton(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
};

// user identity
class wxOptionsPageIdent : public wxOptionsPageStandard
{
public:
   wxOptionsPageIdent(wxNotebook *parent, Profile *profile);

   void OnButton(wxCommandEvent&);
private:
   DECLARE_EVENT_TABLE()
};

// network configuration page
class wxOptionsPageNetwork : public wxOptionsPageStandard
{
public:
   wxOptionsPageNetwork(wxNotebook *parent, Profile *profile);
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

   void OnButton(wxCommandEvent&);
   void OnNewFolder(wxCommandEvent&);
   void OnModifyFolder(wxCommandEvent&);
   void OnDeleteFolder(wxCommandEvent&);

protected:
   // remember the old values for the global timers settings in these vars
   long m_nIncomingDelay,
        m_nPingDelay;

private:
   DECLARE_EVENT_TABLE()
};

#ifdef USE_PYTHON
// all python-related settings
class wxOptionsPagePython : public wxOptionsPageStandard
{
public:
   wxOptionsPagePython(wxNotebook *parent, Profile *profile);
};
#endif // USE_PYTHON

// all bbdb-related settings
class wxOptionsPageAdb : public wxOptionsPageStandard
{
public:
   wxOptionsPageAdb(wxNotebook *parent, Profile *profile);
};


// helper apps settings
class wxOptionsPageHelpers : public wxOptionsPageStandard
{
public:
   wxOptionsPageHelpers(wxNotebook *parent, Profile *profile);
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
   long m_nAutosaveDelay;
   int  m_SyncRemote;
private:
   DECLARE_EVENT_TABLE()
};

#endif // _GUI_WXOPTIONSPAGE_H
