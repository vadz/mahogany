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
   #include "MailFolder.h"
#endif // USE_PCH

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
// HeaderInfoListImpl creation and destruction
// ----------------------------------------------------------------------------

/* static */
HeaderInfoList *HeaderInfoList::Create(MailFolder *mf)
{
   CHECK( mf, NULL, "NULL mailfolder in HeaderInfoList::Create" );

   return new HeaderInfoListImpl(mf);
}

HeaderInfoListImpl::HeaderInfoListImpl(MailFolder *mf)
{
   // we do *not* IncRef() the mail folder as this would create a circular
   // reference (it holds on us too), instead it is supposed to delete us (and
   // so prevent us from using invalid mf pointer) when it is being deleted
   m_mf = mf;

   m_count = (size_t)mf->GetMessageCount();
   m_headers.Alloc(m_count);
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl item access
// ----------------------------------------------------------------------------

inline bool HeaderInfoListImpl::IsHeaderValid(size_t n) const
{
   return (n < m_headers.GetCount()) && (m_headers[n] != NULL);
}

size_t HeaderInfoListImpl::Count(void) const
{
   return m_headers.GetCount();
}

HeaderInfo *HeaderInfoListImpl::GetItemByIndex(size_t n) const
{
   CHECK( n < m_count, NULL, "invalid index in HeaderInfoList::GetItemByIndex" );

   if ( !IsHeaderValid(n) )
   {
      // we need to make n a valid index into array and we do this by filling
      // it up to n with NULLs
      //
      // TODO: add SetCount() to wxArray instead
      for ( size_t count = m_headers.GetCount(); count <= n; count++ )
      {
         wxConstCast(this, HeaderInfoListImpl)->m_headers.Add(NULL);
      }

      // alloc space for new header
      m_headers[n] = new HeaderInfo;

      // turn index into msgno
      n++;

      // get header info for the new header
      m_mf->GetHeaderInfo(m_headers[n], n, n);
   }

   return m_headers[n];
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl UID to index
// ----------------------------------------------------------------------------

UIdType HeaderInfoListImpl::GetIdxFromUId(UIdType uid) const
{
   // TODO: we should use SEARCH here
   FAIL_MSG( "TODO" );

   return UID_ILLEGAL;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl index to/from position mapping
// ----------------------------------------------------------------------------

// this is trivial without sorting/threading

size_t HeaderInfoListImpl::GetIdxFromPos(size_t pos) const
{
   return pos;
}

size_t HeaderInfoListImpl::GetPosFromIdx(size_t n) const
{
   return n;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl message adding/removing
// ----------------------------------------------------------------------------

// TODO: OnRemove() is very inefficient for array!

void HeaderInfoListImpl::OnRemove(size_t n)
{
   CHECK_RET( n < m_count, "invalid index in HeaderInfoList::OnRemove" );

   if ( n < m_headers.GetCount() )
   {
      delete m_headers[n];
      m_headers.RemoveAt(n);
   }
   //else: we have never looked that far
}

void HeaderInfoListImpl::OnAdd(size_t countNew)
{
   m_count = countNew;

   // we probably don't need to do m_headers.Alloc() as countNew shouldn't be
   // much bigger than old count
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
