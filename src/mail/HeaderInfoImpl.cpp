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

HeaderInfoImpl::HeaderInfoImpl()
{
   m_Indentation = 0;
   m_Score = 0;
   m_Encoding = wxFONTENCODING_SYSTEM;
   m_UId = UID_ILLEGAL;
   // all other fields are filled in by the MailFolderCC when creating
   // it
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
   else
   {
      if(m_TranslationTable)
         return & m_Listing[ m_TranslationTable[n] ];
      else
         return & m_Listing[n];
   }
   return & m_Listing[n];
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
}

#ifdef DEBUG

String HeaderInfoListImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
