///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ExportText.cpp - export ADB data to text (CSV/TAB) files
// Purpose:     this exporter creates file in a generic "separated by
//              something" format - usually, "something" is a comma or TAB, but
//              may be anything at all
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.07.99
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

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/log.h>                  // for wxLogMessage

   #include "MApplication.h"
   #include "guidef.h"
   #include "Profile.h"

   #include <wx/layout.h>
   #include <wx/stattext.h>             // for wxStaticText
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
class AdbTextExporter : public AdbExporter
{
public:
   virtual bool Export(const AdbEntryGroup& group, const String& dest);
   virtual bool Export(const AdbEntry& entry, const String& dest);

   DECLARE_ADB_EXPORTER();

protected:
   // helper: is the character 'ch' valid in 'delimiter' separated string?
   static bool IsValidChar(char ch, const wxString& delimiter)
   {
      return delimiter.length() != 1 || delimiter[0u] != ch;
   }

   // the real workers
   static bool DoExportEntry(const AdbEntry& entry,
                             wxFFile& file,
                             const wxString& delimiter);

   static bool DoExportGroup(const AdbEntryGroup& group,
                             wxFFile& file,
                             const wxString& delimiter);
};

// a helper dialog used to ask the user for the name of the file and the
// delimiter to use
class wxAdbTextExporterConfigDialog : public wxManuallyLaidOutDialog
{
public:
   enum DelimiterRadioboxSelection
   {
      Delimiter_Comma,
      Delimiter_Tab,
      Delimiter_Custom,
      Delimiter_Max
   };

   // ctor
   wxAdbTextExporterConfigDialog(const String& filename);

   // accessors which may be used after successful ShowModal()
      // get the filename (always not empty)
   const wxString& GetFilename() const { return m_filename; }
      // get the delimiter string
   const wxString& GetDelimiter() const { return m_delim; }

   // transfer the data from window
   virtual bool TransferDataFromWindow();

   // event handlers
   void OnRadiobox(wxCommandEvent& event) { Update(event.GetInt()); }

   // enable/disable "field separator" text control and also remember the
   // current radio selection
   void Update(int selection);

private:
   // the profile path for saving the last file used
   static const wxChar *ms_profilePathLastFile;

   // the data
   wxString m_filename,
            m_delim;
   int m_selection;

   // GUI controls
   wxComboBox *m_textCustomSep;
   wxTextCtrl *m_textFileName;
   wxStaticText *m_labelCustomSep;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxAdbTextExporterConfigDialog)
};

BEGIN_EVENT_TABLE(wxAdbTextExporterConfigDialog, wxManuallyLaidOutDialog)
   EVT_RADIOBOX(-1, wxAdbTextExporterConfigDialog::OnRadiobox)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic exporter creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_EXPORTER(AdbTextExporter,
                       gettext_noop("Text format address book exporter"),
                       gettext_noop("Simple text format"),
                       _T("Vadim Zeitlin <vadim@wxwindows.org>"));

// ----------------------------------------------------------------------------
// AdbTextExporter
// ----------------------------------------------------------------------------

bool AdbTextExporter::DoExportEntry(const AdbEntry& entry,
                                    wxFFile& file,
                                    const wxString& delimiter)
{
   // find a character which we may safely use
   char pathSeparator = '/';
   while ( !IsValidChar(pathSeparator, delimiter) )
   {
      pathSeparator++;
   }

   // start with finding out the full path to the entry
   wxString s;
   s.Alloc(4096);       // be generous and speed up the export
   for ( AdbEntryGroup *group = entry.GetGroup();
         group;
         group = ((AdbElement *)group)->GetGroup() )
   {
      if ( !!s && s.Last() != pathSeparator )
      {
         s += pathSeparator;
      }

      s += group->GetName();
   }

   if ( !!s && s.Last() != pathSeparator )
   {
      // separate it from the entry name
      s += pathSeparator;
   }

   // and then dump all the fields
   wxString val;
   for ( size_t nField = 0; nField < AdbField_Max; nField++ )
   {
      entry.GetField(nField, &val);
      for ( const wxChar *pc = val.c_str(); *pc; pc++ )
      {
         if ( !IsValidChar(*pc, delimiter) )
         {
            // escape an invalid character
            s += '\\';
         }

         s += *pc;
      }

      s += delimiter;
   }

   s += wxTextFile::GetEOL();

   return file.Write(s);
}

bool AdbTextExporter::DoExportGroup(const AdbEntryGroup& group,
                                    wxFFile& file,
                                    const wxString& delimiter)
{
   // first, export all subgroups
   wxArrayString names;
   size_t nGroupCount = group.GetGroupNames(names);
   for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ )
   {
      AdbEntryGroup *subgroup = group.GetGroup(names[nGroup]);

      bool ok = DoExportGroup(*subgroup, file, delimiter);
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

      bool ok = DoExportEntry(*entry, file, delimiter);
      entry->DecRef();

      if ( !ok )
      {
         return FALSE;
      }
   }


   return TRUE;
}

bool AdbTextExporter::Export(const AdbEntryGroup& group, const String& dest)
{
   // try to guess a reasonable default name for the file to create
   wxString filename = dest;
   if ( !filename )
   {
      filename << group.GetDescription() << _T(".txt");
   }

   // get the name of the file to create
   wxAdbTextExporterConfigDialog dialog(filename);
   if ( dialog.ShowModal() != wxID_OK )
   {
      // cancelled...

      return FALSE;
   }

   // create the file
   filename = dialog.GetFilename();
   wxFFile file(filename, _T("w"));
   if ( file.IsOpened() )
   {
      // export everything recursively
      if ( DoExportGroup(group, file, dialog.GetDelimiter()) )
      {
         wxLogMessage(_("Successfully exported address book data to "
                        "file '%s'"), filename.c_str());

         return TRUE;
      }
   }

   wxLogError(_("Export failed."));

   return FALSE;
}

bool AdbTextExporter::Export(const AdbEntry& /* entry */,
                             const String& /* dest */)
{
   // TODO
   return FALSE;
}

// ----------------------------------------------------------------------------
// wxAdbTextExporterConfigDialog
// ----------------------------------------------------------------------------

const wxChar *wxAdbTextExporterConfigDialog::ms_profilePathLastFile
   = _T("Settings/AdbTextExportFile");

wxAdbTextExporterConfigDialog::wxAdbTextExporterConfigDialog
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
   wxWindow *parent = panel->GetCanvas();

   // a radio box allowing the user to choose the field delimiter
   wxString choices[Delimiter_Max];
   choices[Delimiter_Comma] = _("Use &commas");
   choices[Delimiter_Tab] = _("Use &tabs");
   choices[Delimiter_Custom] = _("&Use custom character");

   wxRadioBox *radiobox = new wxPRadioBox(_T("AdbTextExportDelim"),
                                          parent, -1, _("&Delimiter"),
                                          wxDefaultPosition, wxDefaultSize,
                                          WXSIZEOF(choices), choices,
                                          1, wxRA_SPECIFY_COLS);

   c = new wxLayoutConstraints;
   c->left.SameAs(parent, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(parent, wxRight, 2*LAYOUT_X_MARGIN);
   c->top.SameAs(parent, wxTop, 4*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   radiobox->SetConstraints(c);

   // a text with a label for entering the custom delimiter
   m_labelCustomSep = new wxStaticText(parent, -1, _("Field &separator:"));
   m_textCustomSep = new wxPTextEntry(_T("AdbTextExportSep"), parent, -1, _T(""));

   c = new wxLayoutConstraints;
   c->width.Absolute(5*GetCharWidth());
   c->right.SameAs(radiobox, wxRight, 2*LAYOUT_X_MARGIN);
   c->bottom.SameAs(radiobox, wxBottom, 2*LAYOUT_Y_MARGIN);
   c->height.AsIs();
   m_textCustomSep->SetConstraints(c);

   c = new wxLayoutConstraints;
   c->width.AsIs();
   c->right.LeftOf(m_textCustomSep, 2*LAYOUT_X_MARGIN);
   c->centreY.SameAs(m_textCustomSep, wxCentreY);
   c->height.AsIs();
   m_labelCustomSep->SetConstraints(c);

   // a hack: create an invisible static to take some place
   wxStaticText *dummy = new wxStaticText(parent, -1, _T(""));
   c = new wxLayoutConstraints;
   c->top.Below(radiobox, 2*LAYOUT_Y_MARGIN);
   c->left.AsIs();
   c->right.AsIs();
   c->height.Absolute(2*LAYOUT_Y_MARGIN);
   dummy->SetConstraints(c);
   dummy->Hide();

   // the text entry (with label and browse button) for choosing the file to
   // export to
   wxString label = _("&Filename to export to: ");
   int width;
   GetTextExtent(label, &width, NULL);
   m_textFileName = panel->CreateFileEntry(label, width, dummy,
                                           NULL, FALSE /* save */);

   wxString filename = filenameOrig;
   if ( !filename )
   {
      Profile *appProfile = mApplication->GetProfile();
      filename = appProfile->readEntry(ms_profilePathLastFile, _T("mahogany.csv"));
   }

   m_textFileName->SetValue(filename);

   // set the initial and minimal dialog size
   SetDefaultSize(5*wBtn, 8*hBtn);
}

bool wxAdbTextExporterConfigDialog::TransferDataFromWindow()
{
   m_filename = m_textFileName->GetValue();
   if ( !m_filename )
   {
      wxLogError(_("Please specify the file name!"));

      return FALSE;
   }

   mApplication->GetProfile()->writeEntry(ms_profilePathLastFile,
                                          m_filename);

   switch ( m_selection )
   {
      case Delimiter_Comma:
         m_delim = _T(",");
         break;

      case Delimiter_Tab:
         m_delim = _T("\t");
         break;

      case Delimiter_Custom:
         m_delim = m_textCustomSep->GetValue();
         if ( !m_delim )
         {
            wxLogError(_("Please enter specify the delimiter character!"));
            wxLog::GetActiveTarget()->Flush();

            return FALSE;
         }
         break;
   }

   return TRUE;
}

void wxAdbTextExporterConfigDialog::Update(int selection)
{
   m_selection = selection;
   bool enable = m_selection == Delimiter_Custom;

   m_labelCustomSep->Enable(enable);
   m_textCustomSep->Enable(enable);
}
