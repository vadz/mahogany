///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ExportVCard.cpp - export ADB data to vCard (.vcf) files
// Purpose:     this exporter creates one or more vCard files each one
//              containing exactly one ADB entry
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.05.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

   #include "guidef.h"

   #include <wx/dirdlg.h>
#endif // USE_PCH

#include "adb/AdbEntry.h"
#include "adb/AdbExport.h"

#include <wx/dir.h>
#include <wx/datetime.h>
#include <wx/persctrl.h>
#include <wx/vcard.h>

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the exporter itself
class AdbVCardExporter : public AdbExporter
{
public:
   virtual bool Export(AdbEntryGroup& group, const String& dest);
   virtual bool Export(const AdbEntry& entry, const String& dest);

protected:
   // the real workers
   static bool DoExportEntry(const AdbEntry& entry,
                             const wxString& filename);

   static bool DoExportGroup(AdbEntryGroup& group,
                             const wxString& dirname);

   DECLARE_ADB_EXPORTER();
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic exporter creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_EXPORTER(AdbVCardExporter,
                       gettext_noop("vCard format address book exporter"),
                       gettext_noop("vCard (.vcf) file"),
                       "Vadim Zeitlin <vadim@wxwindows.org>");

// ----------------------------------------------------------------------------
// AdbVCardExporter helpers
// ----------------------------------------------------------------------------

bool AdbVCardExporter::DoExportEntry(const AdbEntry& entry,
                                     const wxString& filename)
{
   // create and fill the vCard object
   wxVCard vcard;

   // first the simple things
   wxString val;

   #define COPY_ADB_TO_VCARD(field, prop)       \
      entry.GetField(AdbField_##field, &val);   \
      if ( !!val )                              \
         vcard.Set##prop(val)

   COPY_ADB_TO_VCARD(FullName, FullName);

   wxString familyName = entry.GetField(AdbField_FamilyName),
            firstName = entry.GetField(AdbField_FirstName),
            prefix = entry.GetField(AdbField_Prefix);
   if ( !!familyName || !!firstName || !!prefix )
   {
      vcard.SetName(familyName,
                    firstName,
                    wxEmptyString, // no middle name in the ADB
                    prefix);
   }
   else
   {
      // we must have a name
      vcard.SetName(entry.GetField(AdbField_NickName));
   }

   COPY_ADB_TO_VCARD(Title, Title);
   COPY_ADB_TO_VCARD(Organization, Organization);

   wxDateTime dt;
   entry.GetField(AdbField_Birthday, &val);
   if ( !!val && dt.ParseDate(val) )
   {
      vcard.SetBirthDay(dt);
   }

   COPY_ADB_TO_VCARD(Comments, Comment);
   COPY_ADB_TO_VCARD(HomePage, URL);

   #undef COPY_ADB_TO_VCARD

   // create two address entries: one for home, another for work
   // (assume that city must be specified and don't create address entry if it
   // is not)
   if ( !!entry.GetField(AdbField_H_City) )
   {
      vcard.AddAddress(
                        entry.GetField(AdbField_H_POBox),
                        entry.GetField(AdbField_H_StreetNo),
                        entry.GetField(AdbField_H_Street),
                        entry.GetField(AdbField_H_City),
                        entry.GetField(AdbField_H_Locality),
                        entry.GetField(AdbField_H_Postcode),
                        entry.GetField(AdbField_H_Country),
                        wxVCardAddress::Home |
                        wxVCardAddress::Intl |
                        wxVCardAddress::Parcel |
                        wxVCardAddress::Postal
                      );
   }

   if ( !!entry.GetField(AdbField_O_City) )
   {
      vcard.AddAddress(
                        entry.GetField(AdbField_O_POBox),
                        entry.GetField(AdbField_O_StreetNo),
                        entry.GetField(AdbField_O_Street),
                        entry.GetField(AdbField_O_City),
                        entry.GetField(AdbField_O_Locality),
                        entry.GetField(AdbField_O_Postcode),
                        entry.GetField(AdbField_O_Country),
                        wxVCardAddress::Work |
                        wxVCardAddress::Intl |
                        wxVCardAddress::Parcel |
                        wxVCardAddress::Postal
                      );
   }

   // and 4 phone number entries: voice/fax home/work
   val = entry.GetField(AdbField_H_Phone);
   if ( !!val )
   {
      vcard.AddPhoneNumber(
                           val,
                           wxVCardPhoneNumber::Home |
                           wxVCardPhoneNumber::Voice
                          );
   }
   val = entry.GetField(AdbField_H_Fax);
   if ( !!val )
   {
      vcard.AddPhoneNumber(
                           val,
                           wxVCardPhoneNumber::Home |
                           wxVCardPhoneNumber::Fax
                          );
   }

   val = entry.GetField(AdbField_O_Phone);
   if ( !!val )
   {
      vcard.AddPhoneNumber(
                           val,
                           wxVCardPhoneNumber::Work |
                           wxVCardPhoneNumber::Voice
                          );
   }
   val = entry.GetField(AdbField_O_Fax);
   if ( !!val )
   {
      vcard.AddPhoneNumber(
                           val,
                           wxVCardPhoneNumber::Home |
                           wxVCardPhoneNumber::Fax
                          );
   }

   // then all emails
   wxString email = entry.GetField(AdbField_EMail);
   if ( !!email )
      vcard.AddEMail(email);

   size_t countEmail = entry.GetEMailCount();
   for ( size_t nEmail = 0; nEmail < countEmail; nEmail++ )
   {
      entry.GetEMail(nEmail, &email);
      vcard.AddEMail(email);
   }

   // TODO: use X-properties to save additional fields (ICQ, Prefers HTML)

   // write vCard to the file
   if ( !vcard.Write(filename) )
   {
      wxLogError(_("Failed to write vCard to the file '%s'."), filename.c_str());

      return FALSE;
   }

   return TRUE;
}

bool AdbVCardExporter::DoExportGroup(AdbEntryGroup& group,
                                     const wxString& dirname)
{
   // before doing anything, ensure that the directory exists
   if ( !wxDir::Exists(dirname) )
   {
      if ( !wxMkdir(dirname, 0755) )
      {
         wxLogError(_("Failed to export address book to '%s'."),
                    dirname.c_str());

         return FALSE;
      }
   }

   // first, export all subgroups
   wxArrayString names;
   size_t nGroupCount = group.GetGroupNames(names);
   for ( size_t nGroup = 0; nGroup < nGroupCount; nGroup++ )
   {
      AdbEntryGroup *subgroup = group.GetGroup(names[nGroup]);

      bool ok = DoExportGroup(*subgroup, dirname + DIR_SEPARATOR + names[nGroup]);
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

      wxString filename;
      filename << dirname << DIR_SEPARATOR << names[nEntry] << _T(".vcf");
      bool ok = DoExportEntry(*entry, filename);
      entry->DecRef();

      if ( !ok )
      {
         return FALSE;
      }
   }


   return TRUE;
}

// ----------------------------------------------------------------------------
// AdbVCardExporter public API
// ----------------------------------------------------------------------------

bool AdbVCardExporter::Export(AdbEntryGroup& group, const String& dest)
{
   wxString dirname = dest;
   if ( !dirname )
   {
      // choose the initial directory for the vCard files to create
      wxDirDialog dlg(NULL, _("Choose the directory for vCard files"));
      if ( dlg.ShowModal() != wxID_OK )
      {
         // cancelled
         return FALSE;
      }

      dirname = dlg.GetPath();
   }

   // export everything recursively
   if ( DoExportGroup(group, dirname) )
   {
      wxLogMessage(_("Successfully exported address book data to "
                     "directory '%s'"), dirname.c_str());

      return TRUE;
   }

   wxLogError(_("Export failed."));

   return FALSE;
}

bool AdbVCardExporter::Export(const AdbEntry& entry, const String& dest)
{
   wxString filename = dest;
   if ( !filename )
   {
      filename = wxPSaveFileSelector
                 (
                     NULL, // no parent
                     "vcard",
                     _("Choose the name for vCard file"),
                     NULL, NULL, _T(".vcf"),
                     _("vCard files (*.vcf)|*.vcf|All files (*.*)|*.*")
                 );
      if ( !filename )
      {
         // cancelled
         return FALSE;
      }
   }

   return DoExportEntry(entry, filename);
}
