///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/HeaderInfoImpl.cpp - implements HeaderInfo(List) classes
// Purpose:     implements HI in the simplest way: just store all data in
//              memory
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "HeaderInfoImpl.h"
#   pragma implementation "HeaderInfo.h"
#endif

#include  "Mpch.h"

#ifndef  USE_PCH
#   include "strutil.h"
#   include "MApplication.h"
#   include "Profile.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "HeaderInfoImpl.h"

// ----------------------------------------------------------------------------
// HeaderInfo
// ----------------------------------------------------------------------------

/* static */
HeaderInfo::HeaderKind
HeaderInfo::GetFromOrTo(const HeaderInfo *hi,
                        bool replaceFromWithTo,
                        const wxArrayString& ownAddresses,
                        String *value)
{
   CHECK( hi && value, Invalid, "invalid parameter in HeaderInfo::GetFromOrTo" );

   *value = hi->GetFrom();

   // optionally replace the "From" with "To: someone" for messages sent by
   // the user himself
   if ( replaceFromWithTo )
   {
      size_t nAdrCount = ownAddresses.GetCount();
      for ( size_t nAdr = 0; nAdr < nAdrCount; nAdr++ )
      {
         if ( Message::CompareAddresses(*value, ownAddresses[nAdr]) )
         {
            // sender is the user himself, do the replacement
            *value = hi->GetTo();

            if ( value->empty() )
            {
               // hmm, must be a newsgroup message
               String ng = hi->GetNewsgroups();
               if ( !ng.empty() )
               {
                  *value = ng;
                  return Newsgroup;
               }
               //else: weird, both to and newsgroup are empty??
            }

            return To;
         }
      }
   }

   return From;
}

// ----------------------------------------------------------------------------
// HeaderInfoImpl
// ----------------------------------------------------------------------------

HeaderInfoImpl::HeaderInfoImpl()
{
   m_Indentation = 0;
   m_Score = 0;
   m_Encoding = wxFONTENCODING_SYSTEM;
   m_UId = UID_ILLEGAL;

   // all other fields are filled in by the MailFolderCC when creating it
}

HeaderInfo * HeaderInfoImpl::Clone() const
{
   HeaderInfoImpl *hi = new HeaderInfoImpl();

   hi->m_Subject = GetSubject();
   hi->m_From = GetFrom();
   hi->m_To = GetTo();
   hi->m_NewsGroups = GetNewsgroups();
   hi->m_Date = GetDate();
   hi->m_UId = GetUId();
   hi->m_References = GetReferences();
   hi->m_InReplyTo = GetInReplyTo();
   hi->m_Status = GetStatus();
   hi->m_Size = GetSize();
   hi->m_Lines = GetLines();
   hi->SetIndentation(GetIndentation());
   hi->SetEncoding(GetEncoding());
   hi->m_Colour = GetColour();
   hi->m_Score = GetScore();
   hi->SetFolderData(GetFolderData());

   return hi;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl creation
// ----------------------------------------------------------------------------

HeaderInfoListImpl::HeaderInfoListImpl(size_t n, size_t nTotal)
{
   m_Listing = n == 0 ? NULL : new HeaderInfoImpl[n];

   // this is the number of entries we have
   m_NumEntries = n;

   // this is the total number of messages in the folder (>= n)
   m_msgnoMax = nTotal;

   m_TranslationTable = NULL;
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
   MOcheck();

   delete [] m_Listing;
   delete [] m_TranslationTable;
}

// ----------------------------------------------------------------------------
// HeaderInfoImpl translation table stuff
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::SetTranslationTable(size_t *array)
{
   delete [] m_TranslationTable;
   m_TranslationTable = array;
}

void HeaderInfoListImpl::AddTranslationTable(const size_t *array)
{
   // it doesn't make sense to call us with NULL table
   CHECK_RET( array, "NULL translation table" );

   size_t *transTable = new size_t[m_NumEntries];
   for ( size_t n = 0; n < m_NumEntries; n++ )
   {
      size_t i = array[n];
      if ( m_TranslationTable )
      {
         // combine with existing table
         i = m_TranslationTable[i];
      }

      transTable[n] = i;
   }

   SetTranslationTable(transTable);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl lookup by UID
// ----------------------------------------------------------------------------

HeaderInfo *HeaderInfoListImpl::GetEntryUId(UIdType uid)
{
   UIdType idx = GetIdxFromUId(uid);
   if ( idx == UID_ILLEGAL )
      return NULL;

   return &m_Listing[idx];
}

UIdType HeaderInfoListImpl::GetIdxFromUId(UIdType uid) const
{
   MOcheck();

   for ( size_t i = 0; i < m_NumEntries; i++ )
   {
      if ( m_Listing[i].GetUId() == uid )
         return i;
   }

   return UID_ILLEGAL;
}

size_t HeaderInfoListImpl::GetUntranslatedIndex(size_t n) const
{
   // FIXME: very inefficient - would it be better to keep the inverse
   //        translation table to speed it up?
   for ( size_t i = 0; i < m_NumEntries; i++ )
   {
      if ( GetTranslatedIndex(i) == n )
         return i;
   }

   FAIL_MSG("invalid index in GetUntranslatedIndex");

   return UID_ILLEGAL;
}

// ----------------------------------------------------------------------------
// modifying the listing
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::SetCount(size_t newcount)
{
   MOcheck();
   CHECK_RET( newcount <= m_NumEntries, "invalid headers count" );

   m_NumEntries = newcount;

   // m_msgnoMax stays unchanged
}

void HeaderInfoListImpl::Remove(size_t n)
{
   MOcheck();
   CHECK_RET( n < m_NumEntries, "invalid index in HeaderInfoList::Remove" );

   // the total number of messages in the folder decrements too
   m_msgnoMax--;

   // calc the position of the element being deleted before changing
   // m_NumEntries below
   size_t pos = GetUntranslatedIndex(n);

   // we'd like to use memmove() as efficiently as for m_TranslationTable for
   // m_Listing too here but it doesn't work because m_Listing is array of
   // objects, not pointers -- we really should change this though (TODO)
#if 0
   // first delete the entry
   m_Listing[n].~HeaderInfoImpl();

   if ( n < m_NumEntries - 1 )
   {
      // remove the entry from the array
      memmove(&m_Listing[n], &m_Listing[n + 1],
              (m_NumEntries - n - 1)*sizeof(HeaderInfoImpl));

      // then delete the corresponding entry in the translation table
      memmove(&m_TranslationTable[n], &m_TranslationTable[n + 1],
              (m_NumEntries - n - 1)*sizeof(size_t));
   }

   // finally adjust the number of items
   m_NumEntries--;
#else // 1
   m_NumEntries--;

   // first deal with the header objects: here we simply have to remove the
   // object from the list - unfortunately, because we use an array of objects
   // and not pointers we have to copy it around
   HeaderInfoImpl *listingNew =
      m_NumEntries == 0 ? NULL : new HeaderInfoImpl[m_NumEntries];
   HeaderInfoImpl *p1 = listingNew,
                  *p2 = m_Listing;
   size_t entry;
   for ( entry = 0; entry <= m_NumEntries; entry++, p2++ )
   {
      if ( entry != n )
      {
         *p1++ = *p2; // p2++ is done anyhow in the loop line
      }
   }

   delete [] m_Listing;
   m_Listing = listingNew;

   if ( m_TranslationTable )
   {
      // then with the translation table: here the situation is more
      // complicated as we need to not only remove the entry, but to adjust
      // indices as well: all indices greater than the one being removed must
      // be decremented to account for the index shift
      if ( pos < m_NumEntries )
      {
         // deleting is easy in this case
         memmove(&m_TranslationTable[pos], &m_TranslationTable[pos + 1],
                 (m_NumEntries - pos)*sizeof(size_t));
      }
      //else: no need to memmove(), last element can be just discarded

      for ( entry = 0; entry < m_NumEntries; entry++ )
      {
         if ( m_TranslationTable[entry] >= n )
            m_TranslationTable[entry]--;
      }
   }
#endif // 0/1
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl debugging
// ----------------------------------------------------------------------------

#ifdef DEBUG

String HeaderInfoListImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
