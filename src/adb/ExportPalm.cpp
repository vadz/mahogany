///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ExportPalm.cpp - export ADB data to PalmOS
//              addressbook format
// Author:      Karsten Ballüder (based on ExportText.cpp by Vadim Zeitlin)
// Modified by:
// Created:     09.10.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Karsten Ballüder <ballueder@gmx.net>
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
#   include "Mcommon.h"

#   include "MApplication.h"
#   include "guidef.h"
#   include "Profile.h"

#   include <wx/log.h>                  // for wxLogMessage
#   include <wx/layout.h>
#endif // USE_PCH

#include <wx/textfile.h>
#include <wx/ffile.h>

#include "gui/wxDialogLayout.h"

#include "adb/AdbEntry.h"
#include "adb/AdbExport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the exporter itself
class AdbPalmExporter : public AdbExporter
{
public:
   virtual bool Export(AdbEntryGroup& group, const String& dest);
   virtual bool Export(const AdbEntry& entry, const String& dest);

   DECLARE_ADB_EXPORTER();

protected:
   // the real workers
   static bool DoExportEntry(const AdbEntry& entry,
                             wxFFile& file, wxString const &category,
                             bool includeEmpty, bool includeComments);

   static bool DoExportGroup(AdbEntryGroup& group,
                             wxFFile& file, wxString const &category,
                             bool includeEmpty, bool includeComments);

   // helper to escape quote characters:
   static wxString EscapeQuotes(const wxString &str)
      {
         wxString s;
         for(size_t i = 0; i < str.Length(); i++)
         {
            if( str[i] == '"') s << '\\';
            s << str[i];
         }
         return s;
      }
};

// a helper dialog used to ask the user for the name of the file
class wxAdbPalmExporterConfigDialog : public wxManuallyLaidOutDialog
{
public:
   // ctor
   wxAdbPalmExporterConfigDialog(const String& filename);

   // accessors which may be used after successful ShowModal()
   // get the filename (always not empty)
   const wxString& GetFileName() const { return m_filename; }

   // get the category name (always not empty)
   const wxString& GetCategoryName() const { return m_CategoryName; }

   // get the include empty flag
   bool GetIncludeEmpty() const { return m_IncludeEmpty; }

   // get the include empty flag
   bool GetIncludeComments() const { return m_IncludeComments; }

   // transfer the data from window
   virtual bool TransferDataFromWindow();

private:
   // the profile path for saving the last file used
   static const wxChar *ms_profilePathLastFile;
   // the profile path for saving the last category used
   static const wxChar *ms_profilePathLastCategory;
   // the profile path for saving the include empty flag
   static const wxChar *ms_profileIncludeEmpty;
   // the profile path for saving the include comments flag
   static const wxChar *ms_profileIncludeComments;

   // the data
   wxString
      m_filename,
      m_CategoryName;
   int m_selection;
   bool m_IncludeEmpty, m_IncludeComments;

   // GUI controls
   wxComboBox *m_textCustomSep;
   wxTextCtrl *m_textFileName;
   wxTextCtrl *m_textCategoryName;
   wxCheckBox *m_checkIncludeEmpty, *m_checkIncludeComments;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic exporter creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_EXPORTER(AdbPalmExporter,
                       gettext_noop("PalmOS format address book exporter"),
                       gettext_noop("PalmOS addressbook"),
                       _T("Karsten Ballüder <ballueder@gmx.net>"));


// ----------------------------------------------------------------------------
// AdbPalmExporter
// ----------------------------------------------------------------------------

#undef ADD
#define ADD(n)       entry.GetField(n, &val); s << _T('"') << EscapeQuotes(val) << _T("\",")
#define ADDP(prefix,n)  entry.GetField(n, &val); \
if(wxStrlen(val)) s << prefix; s << _T('"') << EscapeQuotes(val) << _T("\",")

bool AdbPalmExporter::DoExportEntry(const AdbEntry& entry,
                                    wxFFile& file, const wxString &
                                    category, bool includeEmpty, bool includeComments)
{
   /*
     The format:
     "category name";["address type";]"lastname","firstname","title","company","",
     "phone type";"phone1",
     "phone type";"phone1",
     "phone type";"phone1",
     "phone type";"phone1",
     "phone type";"phone1",
     "street & number", "town", "county", "postcode","country",
     "custom1","custom2","custom3","custom4","Notes","0"

     Types are: "Home", "Work", "E-mail", "Fax", "Other", "Main",
     "Pager", "Mobile"

     "Last","First","Titie","Org","wobk","home","fax","other","email","Add","Town",
     "County","Pcode","Uk","C1","C2","C3","C4","Note","0"

   */

   wxString s;
   s.Alloc(4096);       // be generous and speed up the export

/*  It seems the category no longer gets written.
  s << '"' << category << "\";";
*/
   // dump all the fields
   wxString val, tmp;
   entry.GetField(AdbField_FamilyName, &val);
   if(val.Length())
   {
      ADD(AdbField_FamilyName);
      ADD(AdbField_FirstName);
   }
   else
   {
      entry.GetField(AdbField_FullName, &val);
      if(val.Length() == 0 && ! includeEmpty) // no name
         return TRUE; // ignore entry
      s << '"' << EscapeQuotes(val) << _T("\",\"\",");
   }
   entry.GetField(AdbField_Prefix, &val);
   entry.GetField(AdbField_Title, &tmp);
   if(!tmp.empty()) val << ' ' << tmp;
   s << '"' << EscapeQuotes(val) << _T("\",");
   ADD(AdbField_Organization);

   s << _T("\"\","); // unknown field after company
   ADDP(_T("\"E-mail\";"), AdbField_EMail);
   ADDP(_T("\"Home\";"), AdbField_H_Phone);
   ADDP(_T("\"Fax\";"), AdbField_H_Fax);
   ADDP(_T("\"Work\";"), AdbField_O_Phone);
   ADDP(_T("\"Fax\";"), AdbField_O_Fax);

   entry.GetField(AdbField_H_City, &val);
   if(val.Length()) // has home address?
   {
      entry.GetField(AdbField_H_Street, &val);
      if(val.Length())
      {
         entry.GetField(AdbField_H_StreetNo, &tmp);
         if(!tmp.empty()) val << ' ' << tmp;
         if(! val) entry.GetField(AdbField_H_POBox, &val);
         s << '"' << val << _T("\",");
      }
      ADD(AdbField_H_City);
      ADD(AdbField_H_Locality);
      ADD(AdbField_H_Postcode);
      ADD(AdbField_H_Country);
   }
   else // take work address
   {
      entry.GetField(AdbField_O_Street, &val);
      if(val.Length())
      {
         entry.GetField(AdbField_O_StreetNo, &tmp);
         if(!tmp.empty()) val << ' ' << tmp;
         if(! val) entry.GetField(AdbField_O_POBox, &val);
         s << '"' << val << _T("\",");
      }
      ADD(AdbField_O_City);
      ADD(AdbField_O_Locality);
      ADD(AdbField_O_Postcode);
      ADD(AdbField_O_Country);
   }

   // custom fields:
   ADD(AdbField_Birthday);
   ADD(AdbField_HomePage);
   ADD(AdbField_ICQ);
   ADD(AdbField_NickName);
   if(includeComments)
   {
      ADD(AdbField_Comments);
   }
   else
      s << _T("\"\","); // empty comment

   s << _T("\"0\"");
   s += wxTextFile::GetEOL();

   return file.Write(s);
}
#undef ADD
#undef ADDP

bool AdbPalmExporter::DoExportGroup(AdbEntryGroup& group,
                                    wxFFile& file, const wxString &
                                    category, bool includeEmpty,
                                    bool includeComments)
{
   // first, export all subgroups
   wxArrayString names;
   size_t nGroupCount = group.GetGroupNames(names);
   for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ )
   {
      AdbEntryGroup *subgroup = group.GetGroup(names[nGroup]);

      bool ok = DoExportGroup(*subgroup, file, category, includeEmpty,
                              includeComments);
      subgroup->DecRef();

      if ( !ok )
      {
         return FALSE;
      }
   }

   // and then all entries
   size_t nEntryCount = group.GetEntryNames(names);
   for ( size_t nEntry = 0; nEntry < nEntryCount; nEntry++ )
   {
      AdbEntry *entry = group.GetEntry(names[nEntry]);

      bool ok = DoExportEntry(*entry, file, category, includeEmpty,
                              includeComments);
      entry->DecRef();

      if ( !ok )
      {
         return FALSE;
      }
   }


   return TRUE;
}

bool AdbPalmExporter::Export(AdbEntryGroup& group, const String& dest)
{
   // try to guess a reasonable default name for the file to create
   wxString filename = dest;
   if ( !filename )
   {
      filename << group.GetDescription() << _T(".palm");
   }

   // get the name of the file to create
   wxAdbPalmExporterConfigDialog dialog(filename);
   if ( dialog.ShowModal() != wxID_OK )
   {
      // cancelled...

      return FALSE;
   }

   // create the file
   filename = dialog.GetFileName();
   wxFFile file(filename, _T("w"));
   if ( file.IsOpened() )
   {
      // export everything recursively
      if ( DoExportGroup(group, file, dialog.GetCategoryName(),
                         dialog.GetIncludeEmpty(), dialog.GetIncludeComments() ) )
      {
         wxLogMessage(_("Successfully exported address book data to "
                        "file '%s'"), filename.c_str());

         return TRUE;
      }
   }

   wxLogError(_("Export failed."));

   return FALSE;
}

bool AdbPalmExporter::Export(const AdbEntry& entry, const String& dest)
{
   // TODO
   return FALSE;
}

// ----------------------------------------------------------------------------
// wxAdbPalmExporterConfigDialog
// ----------------------------------------------------------------------------

const wxChar *wxAdbPalmExporterConfigDialog::ms_profilePathLastFile
   = _T("Settings/AdbPalmExportFile");

const wxChar *wxAdbPalmExporterConfigDialog::ms_profilePathLastCategory
   = _T("Settings/AdbPalmExportCategory");

const wxChar *wxAdbPalmExporterConfigDialog::ms_profileIncludeEmpty
   = _T("Settings/AdbPalmExportIncludeEmpty");

const wxChar *wxAdbPalmExporterConfigDialog::ms_profileIncludeComments
   = _T("Settings/AdbPalmExportIncludeComments");

wxAdbPalmExporterConfigDialog::wxAdbPalmExporterConfigDialog
                               (
                                const String& filenameOrig
                               )
                             : wxManuallyLaidOutDialog
                               (
                                NULL,
                                _("Mahogany: Exporting address book"),
                                _T("AdbTextExport")
                               )
{
   wxLayoutConstraints *c;

   // don't create the box because we have another one already and boxes inside
   // boxes look ugly
   (void)CreateStdButtonsAndBox(_("Configure export"), TRUE /* no box */);

   // put the items into an enhanced panel - even if we don't use scrolling
   // here, we may use functions like CreateFileEntry() like this
   wxEnhancedPanel *panel = new wxEnhancedPanel(this, FALSE /* no scrolling */);
   c = new wxLayoutConstraints;
   c->left.SameAs(this, wxLeft);
   c->right.SameAs(this, wxRight);
   c->top.SameAs(this, wxTop);
   c->bottom.Above(FindWindow(wxID_OK), -2*LAYOUT_Y_MARGIN);
   panel->SetConstraints(c);

   // all controls must be created on the canvas, not the panel itself
//   wxWindow *parent = panel->GetCanvas();

   // the text entry (with label and browse button) for choosing the file to
   // export to
   wxString label = _("&Filename to export to: ");
   int width;
   GetTextExtent(label, &width, NULL);
   m_textFileName = panel->CreateFileEntry(label, width, NULL,
                                           NULL, FALSE /* save */);

   // the category name entry (with label) for choosing the file to
   // export to
   label = _("&Addressbook Category: ");
   GetTextExtent(label, &width, NULL);
   m_textCategoryName =   panel->CreateTextWithLabel(label, width, m_textFileName);

   label = _("&Include entries with no name: ");
   GetTextExtent(label, &width, NULL);
   m_checkIncludeEmpty =  panel->CreateCheckBox(label,width,m_textCategoryName);

   label = _("&Include comments: ");
   GetTextExtent(label, &width, NULL);
   m_checkIncludeComments =  panel->CreateCheckBox(label,width,m_checkIncludeEmpty);

   String filename = filenameOrig;
   if ( !filename )
   {
      Profile *appProfile = mApplication->GetProfile();
      filename = appProfile->readEntry(ms_profilePathLastFile, _T("mahogany.txt"));
   }

   m_textFileName->SetValue(
      mApplication->GetProfile()->readEntry(ms_profilePathLastFile,
                                            wxEmptyString));

   m_textCategoryName->SetValue(
      mApplication->GetProfile()->readEntry(ms_profilePathLastCategory,
                                            _T("Unfiled")));
   m_checkIncludeEmpty->SetValue(
      mApplication->GetProfile()->readEntry(ms_profileIncludeEmpty, 0) != 0);
   m_checkIncludeComments->SetValue(
      mApplication->GetProfile()->readEntry(ms_profileIncludeComments,1) != 0);

   // set the initial and minimal dialog size
   SetDefaultSize(5*wBtn, 8*hBtn);
}

bool wxAdbPalmExporterConfigDialog::TransferDataFromWindow()
{
   m_filename = m_textFileName->GetValue();
   if ( !m_filename )
   {
      wxLogError(_("Please specify the file name!"));

      return FALSE;
   }

   mApplication->GetProfile()->writeEntry(ms_profilePathLastFile,
                                          m_filename);

   m_CategoryName = m_textCategoryName->GetValue();
   if ( !m_CategoryName )
   {
      wxLogError(_("Please specify the category name!"));

      return FALSE;
   }

   m_IncludeEmpty = m_checkIncludeEmpty->GetValue();

   m_IncludeComments = m_checkIncludeComments->GetValue();

   mApplication->GetProfile()->writeEntry(ms_profilePathLastFile,
                                          m_filename);
   mApplication->GetProfile()->writeEntry(ms_profilePathLastCategory,
                                          m_CategoryName);
   mApplication->GetProfile()->writeEntry(ms_profileIncludeEmpty,
                                          m_IncludeEmpty);
   mApplication->GetProfile()->writeEntry(ms_profileIncludeComments,
                                          m_IncludeComments);
   return TRUE;
}

