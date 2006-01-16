///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/ImportVCard.cpp - import ADB data from vCard (.vcf) files
// Purpose:     import .vcf files which can be produced by Netscape/Outlook
// Author:      Vadim Zeitlin
// Modified by:
// Created:     17.05.00
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

   #include <wx/log.h>                  // for wxLogNull
#endif // USE_PCH

#include <wx/vcard.h>

#include "adb/AdbEntry.h"
#include "adb/AdbImport.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// the class for importing ADB data in vCard format
class AdbVCardImporter : public AdbImporter
{
public:
   AdbVCardImporter() { }
   virtual ~AdbVCardImporter() { DeleteCards(); }

   // implement base class pure virtuals
   virtual String GetDefaultFilename() const { return wxEmptyString; }
   virtual bool CanImport(const String& filename);
   virtual bool StartImport(const String& filename);
   virtual size_t GetEntryNames(const String& path,
                                wxArrayString& entries) const;
   virtual size_t GetGroupNames(const String& path,
                                wxArrayString& groups) const;
   virtual bool ImportEntry(const String& path,
                            size_t index,
                            AdbEntry *entry);

protected:
   // delete all cards we have
   void DeleteCards();

private:
   // import address info
   void CopyAddress(wxVCardAddress *addr, int fieldFirst, AdbEntry *entry);

   // the filename which was tested by CanImport() the last time
   wxString m_filename;

   // and the vCards we loaded from it - if we did
   wxArrayCards m_cards;

   DECLARE_ADB_IMPORTER();
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// macros for dynamic importer creation
// ----------------------------------------------------------------------------

IMPLEMENT_ADB_IMPORTER(AdbVCardImporter,
                       gettext_noop("vCard address book import module"),
                       gettext_noop("vCard (.vcf) files"),
                       _T("Vadim Zeitlin <vadim@wxwindows.org>"));

// ----------------------------------------------------------------------------
// AdbVCardImporter
// ----------------------------------------------------------------------------

void AdbVCardImporter::DeleteCards()
{
   size_t count = m_cards.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      delete m_cards[n];
   }

   m_cards.Empty();
}

bool AdbVCardImporter::CanImport(const String& filename)
{
   if ( filename != m_filename )
   {
      m_filename = filename;
      DeleteCards(); // if we had any old ones, harmles otherwise

      // suppress possible error messages from CreateFromFile(), we only want
      // to check if it succeeded
      wxLogNull nolog;

      m_cards = wxVCard::CreateFromFile(filename);
   }
   // else: we already tried loading cards from this file

   return !m_cards.IsEmpty();
}

bool AdbVCardImporter::StartImport(const String& filename)
{
   return CanImport(filename);
}

size_t AdbVCardImporter::GetEntryNames(const String& path,
                                       wxArrayString& entries) const
{
   wxCHECK_MSG( !path, 0, _T("path can be only empty in AdbVCardImporter") );

   // we suppose that vCard contains info for just one person

   // how to construct a nick name for a vCard? there doesn't seem to be any
   // natural choice for this, so just take the family name instead
   wxString familyname, givenname;
   size_t countImported = 0,
          countAll = m_cards.GetCount();
   for ( size_t n = 0; n < countAll; n++ )
   {
      if ( m_cards[n]->GetName(&familyname, &givenname) )
      {
         // it may happen that we do have the name property but without the
         // family name field
         if ( familyname.empty() )
         {
            if ( givenname.empty() )
            {
               // what else can we do?
               wxLogDebug(_T("Skipping vCard without name."));

               continue;
            }
            else
            {
               familyname = givenname;
            }
         }

         if ( entries.Index(familyname) != wxNOT_FOUND )
         {
            // TODO: ask to rename it?
            wxLogDebug(_T("Skipping vCard with duplicate name."));
         }
         else
         {
            countImported++;
            entries.Add(familyname);
         }
      }
      //else: what to do with unnamed properties? (TODO: try full name?)
   }

   return countImported;
}

size_t AdbVCardImporter::GetGroupNames(const String& /* path */,
                                       wxArrayString& /* groups */) const
{
   // there are no groups in a vCard

   return 0;
}

bool AdbVCardImporter::ImportEntry(const String& path,
                                   size_t index,
                                   AdbEntry *entry)
{
   wxCHECK_MSG( !path && (index < m_cards.GetCount()), false,
                _T("unexpected params in AdbVCardImporter") );

   // set all simple fields
   wxString val;
   wxVCard *vcard = m_cards[index];

   #define COPY_FIELD(field, prop)  \
      if ( vcard->Get##prop(&val) ) entry->SetField(AdbField_##field, val)

   COPY_FIELD(FullName, FullName);
   COPY_FIELD(Title, BusinessRole);
   COPY_FIELD(Birthday, BirthDayString);
   COPY_FIELD(Comments, Comment);
   COPY_FIELD(HomePage, URL);

   #undef COPY_FIELD

   // now transfer name properties
   wxString familyName, givenName, namePrefix;
   if ( vcard->GetName(&familyName, &givenName, NULL, &namePrefix) )
   {
      if ( !!familyName )
         entry->SetField(AdbField_FamilyName, familyName);
      if ( !!givenName )
         entry->SetField(AdbField_FirstName, givenName);
      if ( !!namePrefix )
         entry->SetField(AdbField_Prefix, namePrefix);
   }

   // and org ones
   wxString org, dept;
   if ( vcard->GetOrganization(&org, &dept) )
   {
      // merge them together
      wxString orgfull;
      orgfull << org << _T("; ") << dept;
      entry->SetField(AdbField_Organization, orgfull);
   }

   // deal with emails
   void *cookie;

   bool hasPrimaryEMail = false;
   wxVCardEMail *email = vcard->GetFirstEMail(&cookie);
   while ( email )
   {
      if ( hasPrimaryEMail )
      {
         entry->AddEMail(email->GetEMail());
      }
      else
      {
         entry->SetField(AdbField_EMail, email->GetEMail());

         hasPrimaryEMail = true;
      }

      delete email;

      email = vcard->GetNextEMail(&cookie);
   }

   // and with addresses: the problem here is that vCard has an arbitrary number
   // of addresses each of them being a home one, work one or may be both at
   // once, and we want exactly one of each
   wxVCardAddress *addrHome = NULL,
                  *addrWork = NULL;
   wxVCardAddress *addr = vcard->GetFirstAddress(&cookie);
   while ( addr && (!addrHome || !addrWork) )
   {
      int flags = addr->GetFlags();
      bool isHome = (flags & wxVCardAddress::Home) != 0;
      bool isWork = (flags & wxVCardAddress::Work) != 0;

      // try to detect if it is a home address
      if ( isHome )
      {
         if ( !isWork )
         {
            // this is definitely the home address!
            if ( addrHome != addrWork )
               delete addrHome;

            addrHome = addr;
         }
         else
         {
            // it may be home address or may be not, take it if we don't have
            // anything else
            if ( !addrHome )
               addrHome = addr;
         }
      }

      // and now apply the same logic to the work address as well
      if ( isWork )
      {
         if ( !isHome )
         {
            if ( addrWork != addrHome )
               delete addrHome;

            addrWork = addr;
         }
         else
         {
            if ( !addrWork )
               addrWork = addr;
         }
      }

      // don't delete it if we must use it later
      if ( addr != addrHome && addr != addrWork )
      {
         delete addr;
      }

      addr = vcard->GetNextAddress(&cookie);
   }

   // import and delete one or both addresses
   if ( addrHome )
   {
      CopyAddress(addrHome, AdbField_H_AddrPageFirst, entry);

      if ( addrHome != addrWork )
         delete addrHome;
   }

   if ( addrWork )
   {
      CopyAddress(addrWork, AdbField_O_AddrPageFirst, entry);

      delete addrWork;
   }

   // finally, deal with phones: this is even worse than addresses as we have
   // the same problem here and we also want to distinguish between fax and
   // normal phone numbers while they are just all the same in vCard...
   //
   // FIXME well, so I decided to do it simply instead of thinking about how to
   //       do it really well... if someone ever complains about it, this should
   //       be fixed
   wxVCardPhoneNumber *phoneWork = NULL,
                      *phoneHome = NULL,
                      *faxWork = NULL,
                      *faxHome = NULL;
   wxVCardPhoneNumber *phone = vcard->GetFirstPhoneNumber(&cookie);
   while ( phone )
   {
      int flags = phone->GetFlags();
      if ( flags & wxVCardPhoneNumber::Home )
      {
         if ( flags & wxVCardPhoneNumber::Voice )
         {
            if ( !phoneHome )
            {
               entry->SetField(AdbField_H_Phone, phone->GetNumber());

               phoneHome = phone;
            }
         }
         if ( flags & wxVCardPhoneNumber::Fax )
         {
            if ( !faxHome )
            {
               entry->SetField(AdbField_H_Fax, phone->GetNumber());

               faxHome = phone;
            }
         }
      }
      if ( flags & wxVCardPhoneNumber::Work )
      {
         if ( flags & wxVCardPhoneNumber::Voice )
         {
            if ( !phoneWork )
            {
               entry->SetField(AdbField_O_Phone, phone->GetNumber());

               phoneWork = phone;
            }
         }
         if ( flags & wxVCardPhoneNumber::Fax )
         {
            if ( !faxWork )
            {
               entry->SetField(AdbField_O_Fax, phone->GetNumber());

               faxWork = phone;
            }
         }
      }

      delete phone;

      phone = vcard->GetNextPhoneNumber(&cookie);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

void AdbVCardImporter::CopyAddress(wxVCardAddress *addr,
                                   int field,
                                   AdbEntry *entry)
{
   wxString val;

   // note that we offset the index of SetField()
   #define COPY_ADDR_FIELD(adb, vcf)                                          \
      val = addr->Get##vcf();                                                 \
      if ( !!val )                                                            \
         entry->SetField(field + AdbField_H_##adb - AdbField_H_AddrPageFirst, \
                         val)

   COPY_ADDR_FIELD(StreetNo, ExtAddress);
   COPY_ADDR_FIELD(Street, Street);
   COPY_ADDR_FIELD(Locality, Region);
   COPY_ADDR_FIELD(City, Locality);
   COPY_ADDR_FIELD(Postcode, PostalCode);
   COPY_ADDR_FIELD(POBox, PostOffice);
   COPY_ADDR_FIELD(Country, Country);

   #undef COPY_ADDR_FIELD
}
