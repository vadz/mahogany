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

// -----------------------------------------------------------------------------
// a page which performs the data transfer between the associated profile
// section values and the controls in the page (only text fields and
// checkboxes are currently supported)
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
      Field_Type   = 0x0fff  // bit mask selecting the type
   };

   enum FieldFlags
   {
      Field_Vital   = 0x1000, // vital setting, test after change
      Field_Restart = 0x2000, // will only take effect during next run
      Field_Flags   = 0xf000  // bit mask selecting the flags
   };

   struct FieldInfo
   {
      const char *label;   // which is shown in the dialog
      int         flags;   // containts the type and the flags (see above)
      int         enable;  // disable this field if "enable" field is unchecked
   };

   // array of all field descriptions
   static FieldInfo ms_aFields[];

   // get the type and the flags of the field
   static FieldType GetFieldType(size_t n)
      { return (FieldType)(ms_aFields[n].flags & Field_Type); }

   static FieldFlags GetFieldFlags(size_t n)
      { return (FieldFlags)(ms_aFields[n].flags & Field_Flags); }

   // ctor will add this page to the notebook
   wxOptionsPage(wxNotebook *parent,
                 const char *title,
                 ProfileBase *profile,
                 size_t nFirst,
                 size_t nLast,
                 int helpID = -1);
   ~wxOptionsPage() { SafeDecRef(m_Profile); }

   // transfer data to/from the controls
   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   // to change the profile associated with the page:
   void SetProfile(ProfileBase *profile)
   {
      SafeDecRef(m_Profile);
      m_Profile = profile;
      SafeIncRef(m_Profile);
   }

   // callbacks
      // called when text zone content changes
   void OnChange(wxEvent&);
      // called when a {radio/combo/check}box value changes
   void OnControlChange(wxEvent& event);

   // enable/disable controls (better than OnUpdateUI here)
   void UpdateUI();

   /// Returns the numeric help id.
   int HelpId(void) const { return m_HelpId; }

protected:
   void CreateControls();

   // range of our controls in ms_aFields
   size_t m_nFirst, m_nLast;

   // we need a pointer to the profile to write to
   ProfileBase *m_Profile;

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

   /// numeric help id
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

// settings concerning the compose window
class wxOptionsPageCompose : public wxOptionsPage
{
public:
   wxOptionsPageCompose(wxNotebook *parent, ProfileBase *profile);

   void OnButton(wxCommandEvent&);
   virtual bool TransferDataFromWindow();

private:
   DECLARE_EVENT_TABLE()
};

// settings concerning the message view window
class wxOptionsPageMessageView : public wxOptionsPage
{
public:
   wxOptionsPageMessageView(wxNotebook *parent, ProfileBase *profile);

   void OnButton(wxCommandEvent&);

private:
   DECLARE_EVENT_TABLE()
};

// user identity
class wxOptionsPageIdent : public wxOptionsPage
{
public:
   wxOptionsPageIdent(wxNotebook *parent, ProfileBase *profile);
};

// network configuration page
class wxOptionsPageNetwork : public wxOptionsPage
{
public:
   wxOptionsPageNetwork(wxNotebook *parent, ProfileBase *profile);
};

// global folder settings (each folder has its own settings which are changed
// from a separate dialog)
class wxOptionsPageFolders : public wxOptionsPage
{
public:
   wxOptionsPageFolders(wxNotebook *parent, ProfileBase *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

   void OnIdle(wxIdleEvent&);

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
class wxOptionsPagePython : public wxOptionsPage
{
public:
   wxOptionsPagePython(wxNotebook *parent, ProfileBase *profile);
};
#endif // USE_PYTHON

// all bbdb-related settings
class wxOptionsPageAdb : public wxOptionsPage
{
public:
   wxOptionsPageAdb(wxNotebook *parent, ProfileBase *profile);
};


// helper apps settings
class wxOptionsPageHelpers : public wxOptionsPage
{
public:
   wxOptionsPageHelpers(wxNotebook *parent, ProfileBase *profile);
};

// miscellaneous settings
class wxOptionsPageOthers : public wxOptionsPage
{
public:
   wxOptionsPageOthers(wxNotebook *parent, ProfileBase *profile);

   virtual bool TransferDataToWindow();
   virtual bool TransferDataFromWindow();

protected:
   long m_nAutosaveDelay;
};

#endif // _GUI_WXOPTIONSPAGE_H
