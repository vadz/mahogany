/*-*- c++ -*-*******************************************************
* HeaderInfoImpl : header info listing for mailfolders
*                                                                  *
* (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
*                                                                  *
* $Id$
*******************************************************************/

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
   hi->m_Date = GetDate();
   hi->m_UId = GetUId();
   hi->m_References = GetReferences();
   hi->m_Status = GetStatus();
   hi->m_Size = GetSize();
   hi->SetIndentation(GetIndentation());
   hi->SetEncoding(GetEncoding());
   hi->m_Colour = GetColour();
   hi->m_Score = GetScore();
   hi->SetFolderData(GetFolderData());

   return hi;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl
// ----------------------------------------------------------------------------

HeaderInfoListImpl::HeaderInfoListImpl(size_t n)
{
   m_Listing = n == 0 ? NULL : new HeaderInfoImpl[n];
   m_NumEntries = n;
   m_TranslationTable = NULL;
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
   MOcheck();

   delete [] m_Listing;
   delete [] m_TranslationTable;
}

HeaderInfo *HeaderInfoListImpl::operator[](size_t n)
{
   MOcheck();
   ASSERT(n < m_NumEntries);
   if(n >= m_NumEntries)
      return NULL;
   if(m_TranslationTable)
      n = m_TranslationTable[n];
   return &m_Listing[n];
}

HeaderInfo *HeaderInfoListImpl::GetEntryUId(UIdType uid)
{
   MOcheck();
   for(size_t i = 0; i < m_NumEntries; i++)
   {
      if( m_Listing[i].GetUId() == uid )
         return & m_Listing[i];
   }
   return NULL;
}

UIdType HeaderInfoListImpl::GetIdxFromUId(UIdType uid) const
{
   MOcheck();
   for(size_t i = 0; i < m_NumEntries; i++)
   {
      if( m_Listing[i].GetUId() == uid )
         return i;
   }
   return UID_ILLEGAL;
}

void HeaderInfoListImpl::Swap(size_t index1, size_t index2)
{
   MOcheck();
   CHECK_RET( index1 < m_NumEntries && index2 < m_NumEntries,
              "invalid index in HeaderInfoList::Swap" );

   HeaderInfoImpl hicc;
   hicc = m_Listing[index1];
   m_Listing[index1] = m_Listing[index2];
   m_Listing[index2] = hicc;
}

void HeaderInfoListImpl::Remove(size_t n)
{
   CHECK_RET( n < m_NumEntries, "invalid index in HeaderInfoList::Remove" );

   // we'd like to use memmove() as efficiently as for m_TranslationTable for
   // m_Listing too here but it doesn't work because m_Listing is array of
   // objects, not pointers, and it can't be an array of pointers because there
   // is HeaderInfoList::GetArray() which is used by MailFolderCmn for sorting

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

   // first deal with the header objects
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

   // then with the translation table: notice that we need not only to remove
   // the entry but also to shift all entries greater than n by -1
   if ( n < m_NumEntries )
   {
      // deletign is easy in this case
      memmove(&m_TranslationTable[n], &m_TranslationTable[n + 1],
              (m_NumEntries - n)*sizeof(size_t));
   }

   for ( entry = 0; entry < m_NumEntries; entry++ )
   {
      if ( m_TranslationTable[entry] >= n )
         m_TranslationTable[entry]--;
   }
#endif // 0/1
}

#ifdef DEBUG

String HeaderInfoListImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
