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

/*
   Look for FIXME-SORTING to see the things which must be changed to implement
   sorting/threading
 */

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

   #include "Message.h"
#endif // USE_PCH

#include "HeaderInfoImpl.h"

#include "Sequence.h"

// ----------------------------------------------------------------------------
// HeaderInfo
// ----------------------------------------------------------------------------

HeaderInfo::HeaderInfo()
{
   m_Status = 0;
   m_Size =
   m_Lines = 0;
   m_UId = UID_ILLEGAL;
   m_Date = (time_t)-1;
   m_Encoding = wxFONTENCODING_SYSTEM;
}

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

   // the last modification counter: we're going to increase it each time we
   // invalidate the pointers in m_headers
   m_lastMod = 0;

   m_count = (size_t)mf->GetMessageCount();
   m_headers.Alloc(m_count);
}

void HeaderInfoListImpl::CleanUp()
{
   // TODO: maintain first..last range of allocated headers to avoid iterating
   //       over the entire array just to call delete for many NULL pointers?

   WX_CLEAR_ARRAY(m_headers);

   m_lastMod++;
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
   CleanUp();
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
   return m_count;
}

HeaderInfo *HeaderInfoListImpl::GetItemByIndex(size_t n) const
{
   CHECK( n < m_count, NULL, "invalid index in HeaderInfoList::GetItemByIndex" );

   if ( !IsHeaderValid(n) )
   {
      HeaderInfoListImpl *self = (HeaderInfoListImpl *)this; // const_cast

      self->ExpandToMakeIndexValid(n);

      // alloc space for new header
      m_headers[n] = new HeaderInfo;

      // get header info for the new header
      Sequence seq;
      seq.Add(n + 1);
      m_mf->GetHeaderInfo(self->m_headers, seq);
   }

   ASSERT_MSG( m_headers[n]->IsValid(), "returning invalid HeaderInfo" );

   return m_headers[n];
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl UID to index
// ----------------------------------------------------------------------------

size_t HeaderInfoListImpl::GetIdxFromUId(UIdType uid) const
{
   MsgnoType msgno = m_mf->GetMsgnoFromUID(uid);

   // this will return INDEX_ILLEGAL if msgno == MSGNO_ILLEGAL
   return GetIdxFromMsgno(msgno);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl index to/from position mapping
// ----------------------------------------------------------------------------

// FIXME-SORTING: this is trivial without sorting/threading

size_t HeaderInfoListImpl::GetIdxFromPos(size_t pos) const
{
   return pos;
}

size_t HeaderInfoListImpl::GetPosFromIdx(size_t n) const
{
   return n;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl methods called by MailFolder
// ----------------------------------------------------------------------------

// TODO: OnRemove() is very inefficient for array!

void HeaderInfoListImpl::OnRemove(size_t n)
{
   CHECK_RET( n < m_count, "invalid index in HeaderInfoList::OnRemove" );

   if ( n < m_headers.GetCount() )
   {
      delete m_headers[n];
      m_headers.RemoveAt(n);

      // the indices are shifted (and one is even removed completely)
      m_lastMod++;
   }
   //else: we have never looked that far

   // update the count anyhow
   m_count--;
}

void HeaderInfoListImpl::OnAdd(size_t countNew)
{
   m_count = countNew;

   // we probably don't need to do m_headers.Alloc() as countNew shouldn't be
   // much bigger than old count
}

void HeaderInfoListImpl::OnClose()
{
   CleanUp();

   m_count = 0;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl apperance parameters
// ----------------------------------------------------------------------------

size_t HeaderInfoListImpl::GetIndentation(size_t n) const
{
   // TODO: where to store it? we might either have a hash containing the
   //       indent (assuming few messages are indented...) or store it in
   //       HeaderInfo itself but this doesn't seem right as this is a purely
   //       GUI thing and HeaderInfo lives at mail level of abstraction...

   return 0;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl searching
// ----------------------------------------------------------------------------

class FindHeaderHelper
{
public:
   FindHeaderHelper(MailFolder *mf, MailFolder::MessageStatus flag, bool set)
   {
      m_msgnosFound = mf->SearchByFlag(flag, set);
   }

   const MsgnoArray *GetResults() const { return m_msgnosFound; }

   ~FindHeaderHelper()
   {
      delete m_msgnosFound;
   }

private:
   MsgnoArray *m_msgnosFound;
};

// return the first array element in range [from, to[ or UID_ILLEGAL
static MsgnoType
FindElementInRange(const MsgnoArray& array, MsgnoType from, MsgnoType to)
{
   size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      MsgnoType msgno = array[n];
      if ( msgno >= from && msgno < to )
      {
         return msgno;
      }
   }

   return MSGNO_ILLEGAL;
}

size_t
HeaderInfoListImpl::FindHeaderByFlag(MailFolder::MessageStatus flag,
                                     bool set,
                                     long indexFrom)
{
   FindHeaderHelper helper(m_mf, flag, set);

   const MsgnoArray *results = helper.GetResults();
   if ( !results )
      return UID_ILLEGAL;

   return GetIdxFromMsgno(FindElementInRange(*results, indexFrom + 1, m_count));
}

size_t
HeaderInfoListImpl::FindHeaderByFlagWrap(MailFolder::MessageStatus flag,
                                         bool set,
                                         long indexFrom)
{
   FindHeaderHelper helper(m_mf, flag, set);

   const MsgnoArray *results = helper.GetResults();
   if ( !results )
      return INDEX_ILLEGAL;

   MsgnoType msgno = FindElementInRange(*results, indexFrom + 1, m_count);
   if ( msgno == MSGNO_ILLEGAL )
      msgno = FindElementInRange(*results, 0, indexFrom);

   return GetIdxFromMsgno(msgno);
}

MsgnoArray *
HeaderInfoListImpl::GetAllHeadersByFlag(MailFolder::MessageStatus flag,
                                        bool set)
{
   return m_mf->SearchByFlag(flag, set);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl last modification "date" handling
// ----------------------------------------------------------------------------

HeaderInfoList::LastMod HeaderInfoListImpl::GetLastMod() const
{
   return m_lastMod;
}

bool HeaderInfoListImpl::HasChanged(const HeaderInfoList::LastMod since) const
{
   return m_lastMod > since;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl cache control
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::ExpandToMakeIndexValid(size_t n)
{
   // we need to make n a valid index into array and we do this by filling
   // it up to n with NULLs
   //
   // TODO: add SetCount() to wxArray instead
   size_t count = m_headers.GetCount();
   if ( count <= n )
   {
      // adding elements to array may need realloc()ing and invalidate the
      // pointers
      m_lastMod++;

      while ( count++ <= n )
      {
         m_headers.Add(NULL);
      }
   }
}

void HeaderInfoListImpl::Cache(const Sequence& seq)
{
   UIdType idxMax;
   seq.GetBounds(NULL, &idxMax);

   // make it an index from msgno
   idxMax--;

   CHECK_RET( idxMax < m_count, "HeaderInfoListImpl::Cache(): invalid range" );

   // create all headers we're going to cache first
   ExpandToMakeIndexValid(idxMax);

   size_t countNotInCache = 0;
   size_t n;
   for ( UIdType i = seq.GetFirst(n); i != UID_ILLEGAL; i = seq.GetNext(i, n) )
   {
      size_t idx = GetIdxFromMsgno(i);

      if ( !m_headers[idx] )
      {
         countNotInCache++;

         m_headers[idx] = new HeaderInfo;
      }
   }

   // cache the headers now if needed
   if ( countNotInCache )
   {
      m_mf->GetHeaderInfo(m_headers, seq);
   }
}

void HeaderInfoListImpl::CachePositions(const Sequence& seq)
{
   // FIXME-SORTING: transform sequence of positions into sequence of msgnos
   Cache(seq);
}

void HeaderInfoListImpl::CacheMsgnos(MsgnoType msgnoFrom, MsgnoType msgnoTo)
{
   Sequence seq;
   seq.AddRange(msgnoFrom, msgnoTo);

   Cache(seq);
}

bool HeaderInfoListImpl::IsInCache(size_t pos) const
{
   size_t idx = GetIdxFromPos(pos);

   CHECK( idx < m_count, false,
          "HeaderInfoListImpl::IsInCache(): invalid position" );

   return (idx < m_headers.GetCount()) && (m_headers[idx] != NULL);
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
