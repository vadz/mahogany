///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/HeaderInfoImpl.cpp - implements HeaderInfo(List) classes
// Purpose:     implements HI in the simplest way: just store all data in
//              memory
// Author:      Karsten Ballüder, Vadim Zeitlin
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

   #include "Message.h"

   #include "Sorting.h"
#endif // USE_PCH

#include "HeaderInfoImpl.h"

#include "Sequence.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// FindHeaderHelper just calls SearchByFlag() and deletes the results returned
// by it automatically
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

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static UIdType MapIndexToMsgno(UIdType uid)
{
   return uid + 1;
}

// ============================================================================
// implementation
// ============================================================================

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
// HeaderInfoListImpl inline methods
// ----------------------------------------------------------------------------

inline bool HeaderInfoListImpl::IsSorting() const
{
   return m_tableMsgno != NULL || m_reverseOrder;
}

inline bool HeaderInfoListImpl::IsHeaderValid(size_t n) const
{
   return (n < m_headers.GetCount()) && (m_headers[n] != NULL);
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

   // preallocate the memory for headers
   m_count = (size_t)mf->GetMessageCount();
   m_headers.Alloc(m_count);

   // no sorting/threading yet
   m_tableMsgno =
   m_tablePos = NULL;

   m_reverseOrder = false;
   m_sortOrder = 0;
   m_detectOwnAddresses = false;
}

void HeaderInfoListImpl::CleanUp()
{
   // TODO: maintain first..last range of allocated headers to avoid iterating
   //       over the entire array just to call delete for many NULL pointers?

   WX_CLEAR_ARRAY(m_headers);

   m_lastMod++;

   FreeSortTables();
}

void HeaderInfoListImpl::FreeSortTables()
{
   if ( m_tableMsgno )
   {
      free(m_tableMsgno);
      free(m_tablePos);

      m_tableMsgno =
      m_tablePos = NULL;
   }
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
   CleanUp();
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl item access
// ----------------------------------------------------------------------------

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

MsgnoType HeaderInfoListImpl::GetMsgnoFromPos(size_t pos) const
{
   if ( m_reverseOrder )
   {
      if ( m_tableMsgno )
      {
         // flip the table (which always corresponds to the direct ordering)
         return m_tableMsgno[m_count - 1 - pos];
      }
      else // no table, reverse the natural message order
      {
         return m_count - pos;
      }
   }
   else // just use the table
   {
      CHECK( m_tableMsgno, 0, "should only be called if IsSorting()" );

      return m_tableMsgno[pos];
   }
}

size_t HeaderInfoListImpl::GetIdxFromPos(size_t pos) const
{
   CHECK( pos < m_count, (size_t)-1, "invalid position in GetIdxFromPos" );

   return IsSorting() ? GetMsgnoFromPos(pos) - 1 : pos;
}

size_t HeaderInfoListImpl::GetPosFromIdx(size_t n) const
{
   CHECK( n < m_count, (size_t)-1, "invalid index in GetPosFromIdx" );

   if ( m_reverseOrder )
   {
      if ( m_tablePos )
      {
         return m_count - 1 - m_tablePos[n];
      }
      else // no table, reverse the natural message order
      {
         return m_count - 1 - n;
      }
   }
   else // use the table if any
   {
      return m_tablePos ? m_tablePos[n] : n;
   }
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl methods called by MailFolder
// ----------------------------------------------------------------------------

// TODO: OnRemove() is very inefficient for array!

void HeaderInfoListImpl::OnRemove(size_t n)
{
   ASSERT_MSG( !IsSorting(), "FIXME-SORTING" );

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
   ASSERT_MSG( !IsSorting(), "FIXME-SORTING" );

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

// return the first position between from and to at which an element from the
// array appears
size_t
HeaderInfoListImpl::FindFirstInRange(const MsgnoArray& array,
                                     size_t posFrom, size_t posTo) const
{
   size_t posFirst = UID_ILLEGAL;

   size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      size_t pos = GetPosFromIdx(array[n] - 1);
      if ( pos >= posFrom && pos <= posTo && pos < posFirst )
      {
         posFirst = pos;

         // optimization: if we don't sort messages at all, the first msgno we
         // find will be the first one by position as well (as position is
         // equal to msgno and array is supposed to be sorted)
         if ( !IsSorting() )
            break;
      }

      if ( posFirst == posFrom )
      {
         // no need to search further, we can't find anything better
         break;
      }
   }

   return posFirst;
}

size_t
HeaderInfoListImpl::FindHeaderByFlag(MailFolder::MessageStatus flag,
                                     bool set,
                                     long posFrom)
{
   FindHeaderHelper helper(m_mf, flag, set);

   const MsgnoArray *results = helper.GetResults();
   if ( !results )
      return UID_ILLEGAL;

   return FindFirstInRange(*results, posFrom + 1, m_count);
}

size_t
HeaderInfoListImpl::FindHeaderByFlagWrap(MailFolder::MessageStatus flag,
                                         bool set,
                                         long posFrom)
{
   FindHeaderHelper helper(m_mf, flag, set);

   const MsgnoArray *results = helper.GetResults();
   if ( !results )
      return INDEX_ILLEGAL;

   size_t pos = FindFirstInRange(*results, posFrom + 1, m_count);
   if ( pos == UID_ILLEGAL )
      pos = FindFirstInRange(*results, 0, posFrom);

   return pos;
}

MsgnoArray *
HeaderInfoListImpl::GetAllHeadersByFlag(MailFolder::MessageStatus flag,
                                        bool set)
{
   return m_mf->SearchByFlag(flag, set);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl sorting
// ----------------------------------------------------------------------------

// sort the messages according to the current sort parameters
void HeaderInfoListImpl::Sort()
{
   // easy case: don't sort at all
   if ( GetSortCritDirect(m_sortOrder) == MSO_NONE )
   {
      // we may still need to reverse the messages order
      m_reverseOrder = GetSortCrit(m_sortOrder) == MSO_NONE_REV;

      FreeSortTables();
   }
   else // we do need to sort messages
   {
#if 0 // testing code
      m_tableMsgno = (size_t *)malloc(m_count * sizeof(size_t));
      m_tablePos = (size_t *)malloc(m_count * sizeof(size_t));

      if ( m_sortOrder & 1 )
      {
         // reverse
         for ( size_t n = 0; n < m_count; n++ )
         {
            m_tableMsgno[n] = m_count - n;
            m_tablePos[n] = m_count - n - 1;
         }
      }
      else // normal
      {
         for ( size_t n = 0; n < m_count; n++ )
         {
            m_tableMsgno[n] = n + 1;
            m_tablePos[n] = n;
         }
      }
#endif
   }
}

// change the sorting order
bool HeaderInfoListImpl::SetSortOrder(long sortOrder,
                                      bool detectOwnAddresses,
                                      const wxArrayString& ownAddresses)
{
   // we are only called if some of the sorting parameters changed, but make
   // sure this change will really affect us
   if ( sortOrder == m_sortOrder )
   {
      // something related to detectOwnAddresses changed, do we use this at
      // all?
      bool useSender = false;
      while ( sortOrder != MSO_NONE )
      {
         if ( GetSortCritDirect(sortOrder) == MSO_SENDER )
         {
            useSender = true;
            break;
         }

         sortOrder = GetSortNextCriterium(sortOrder);
      }

      if ( !useSender )
      {
         // nothing changes for us
         return false;
      }
   }

   // remember new sort order
   m_sortOrder = sortOrder;
   m_detectOwnAddresses = detectOwnAddresses;
   m_ownAddresses = ownAddresses;

   // sorting order can hardly change the listing if we have less than 2
   // elements
   if ( m_count < 2 )
   {
      return false;
   }

   // recalculate the tables
   Sort();

   // assume it changed
   m_lastMod++;

   return true;
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
   // transform sequence of positions into sequence of msgnos
   Sequence seqMsgnos;
   if ( IsSorting() )
   {
      // a contiguous range of positions doesn't correspond in general to a
      // contiguous set of msgnos so the best we can do (or at least the best
      // thing I see) is to retreieve all messages between the two extreme
      // positions
      MsgnoType msgnoMin = 0,
                msgnoMax = 0;

      size_t n;
      for ( UIdType pos = seq.GetFirst(n);
            pos != UID_ILLEGAL;
            pos = seq.GetNext(pos, n) )
      {
         ASSERT_MSG( pos < m_count, "invalid position in the sequence" );

         MsgnoType msgno = GetMsgnoFromPos(pos);

         if ( !msgnoMin || msgno < msgnoMin )
            msgnoMin = msgno;

         if ( !msgnoMax || msgno > msgnoMax )
            msgnoMax = msgno;
      }

      seqMsgnos.AddRange(msgnoMin, msgnoMax);
   }
   else // no sorting/threading at all
   {
      seqMsgnos = seq.Apply(MapIndexToMsgno);
   }

   Cache(seqMsgnos);
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
