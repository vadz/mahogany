///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   mail/HeaderInfoImpl.cpp - implements HeaderInfo(List) classes
// Purpose:     implements HI in the simplest way: just store all data in
//              memory
// Author:      Vadim Zeitlin, Karsten Ballüder
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
   #pragma implementation "HeaderInfoImpl.h"
   #pragma implementation "HeaderInfo.h"
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
// macros
// ----------------------------------------------------------------------------

// define this to do some (expensive!) run-time checks for translation tables
// consistency
#undef DEBUG_SORTING
#define DEBUG_SORTING

#ifdef DEBUG_SORTING
   #define CHECK_TABLES() VerifyTables(m_count, m_tableMsgno, m_tablePos)
   #define DUMP_TABLES(msg) \
      printf("Translation tables (count = %ld) state ", m_count); \
      printf msg ; \
      DumpTables(m_count, m_tableMsgno, m_tablePos)
#else
   #define CHECK_TABLES()
   #define DUMP_TABLES(msg)
#endif

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

int CMPFUNC_CONV MsgnoCmpFunc(UIdType *msgno1, UIdType *msgno2)
{
   return *msgno1 - *msgno2;
}

static UIdType MapIndexToMsgno(UIdType uid)
{
   return uid + 1;
}

#ifdef DEBUG_SORTING

// verify that msgno/pos tables are at least consistent
static void VerifyTables(MsgnoType count,
                         const MsgnoType *msgnos,
                         const MsgnoType *pos)
{
   wxArrayInt msgnosFound;

   for ( size_t n = 0; n < count; n++ )
   {
      MsgnoType msgno = msgnos[n];

      ASSERT_MSG( msgno > 0 && msgno <= count,
                  "invalid msgno in the translation table" );

      if ( pos[msgno - 1] != n )
      {
         FAIL_MSG( "translation tables are in inconsistent state!" );
      }

      if ( msgnosFound.Index(msgno) != wxNOT_FOUND )
      {
         FAIL_MSG( "duplicate msgno in translation table!" );
      }
      else
      {
         msgnosFound.Add(msgno);
      }
   }

   ASSERT_MSG( msgnosFound.GetCount() == count,
               "missing msgnos in translation table!?" );
}

// dump a table
static void DumpTable(MsgnoType count, const MsgnoType *table)
{
   for ( size_t n = 0; n < count; n++ )
      printf(" %2ld", table[n]);
}

// dump the translation tables
static void DumpTables(MsgnoType count,
                       const MsgnoType *msgnos,
                       const MsgnoType *pos)
{
   printf("\n\tmsgnos:");
   DumpTable(count, msgnos);
   printf("\n\tpos   :");
   DumpTable(count, pos);
   puts("");
}

#endif // DEBUG_SORTING

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

inline bool HeaderInfoListImpl::IsHeaderValid(MsgnoType n) const
{
   return (n < m_headers.GetCount()) && (m_headers[n] != NULL);
}

inline bool HeaderInfoListImpl::HasTransTable() const
{
   return m_tableMsgno != NULL;
}

inline bool HeaderInfoListImpl::NeedsSort() const
{
   return m_needsResort;
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
   m_count = mf->GetMessageCount();
   m_headers.Alloc(m_count);

   // no sorting/threading yet
   m_tableMsgno =
   m_tablePos = NULL;

   m_indents = NULL;

   m_reverseOrder = false;
   m_needsResort = false;
}

void HeaderInfoListImpl::CleanUp()
{
   // TODO: maintain first..last range of allocated headers to avoid iterating
   //       over the entire array just to call delete for many NULL pointers?

   WX_CLEAR_ARRAY(m_headers);

   m_lastMod++;

   FreeSortTables();
   FreeIndentTable();
}

HeaderInfoListImpl::~HeaderInfoListImpl()
{
   CleanUp();
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl item access
// ----------------------------------------------------------------------------

MsgnoType HeaderInfoListImpl::Count(void) const
{
   return m_count;
}

HeaderInfo *HeaderInfoListImpl::GetItemByIndex(MsgnoType n) const
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

   // the caller will crash...
   ASSERT_MSG( m_headers[n], "returning NULL HeaderInfo?" );

   return m_headers[n];
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl UID to index
// ----------------------------------------------------------------------------

MsgnoType HeaderInfoListImpl::GetIdxFromUId(UIdType uid) const
{
   MsgnoType msgno = m_mf->GetMsgnoFromUID(uid);

   // this will return INDEX_ILLEGAL if msgno == MSGNO_ILLEGAL
   return GetIdxFromMsgno(msgno);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl index to/from position mapping
// ----------------------------------------------------------------------------

MsgnoType HeaderInfoListImpl::GetMsgnoFromPos(MsgnoType pos) const
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

MsgnoType HeaderInfoListImpl::GetIdxFromPos(MsgnoType pos) const
{
   // we have to resort before using m_tableMsgno!
   ASSERT_MSG( !NeedsSort(), "can't be called now" );

   CHECK( pos < m_count, INDEX_ILLEGAL, "invalid position in GetIdxFromPos" );

   return IsSorting() ? GetMsgnoFromPos(pos) - 1 : pos;
}

MsgnoType HeaderInfoListImpl::GetPosFromIdx(MsgnoType n) const
{
   // we have to resort before using m_tablePos!
   ASSERT_MSG( !NeedsSort(), "can't be called now" );

   CHECK( n < m_count, INDEX_ILLEGAL, "invalid index in GetPosFromIdx" );

   // calculate it on the fly if needed
   if ( !m_tablePos )
   {
      ((HeaderInfoListImpl *)this)->UpdateInverseTable(); // const_cast
   }

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

// TODO: OnRemove() is very inefficient, there are a few possible
//       optimizations:
//
// 1. call it for a bunch of messages when there are several messages
//    being expunged
//
// 2. don't modify the array but just mark the index being removed as
//    being invalid (somehow...) and ignore it later

void HeaderInfoListImpl::OnRemove(MsgnoType n)
{
   CHECK_RET( n < m_count, "invalid index in HeaderInfoList::OnRemove" );

   if ( n < m_headers.GetCount() )
   {
      delete m_headers[n];
      m_headers.RemoveAt(n);

      // the indices are shifted (and one is even removed completely), so
      // invalidate the pointers into m_headers
      m_lastMod++;
   }
   //else: we have never looked that far

   if ( HasTransTable() )
   {
      DUMP_TABLES(("before removing element %ld", n));

      // we will determine the index of the message being expunged (in
      // m_tableMsgno) below
      MsgnoType idxRemovedInMsgnos = INDEX_ILLEGAL;

      MsgnoType posRemoved;
      if ( m_tablePos )
      {
         posRemoved = m_tablePos[n];
      }

      // update the translation tables
      for ( MsgnoType i = 0; i < m_count; i++ )
      {
         // first work with msgnos table
         MsgnoType msgno = m_tableMsgno[i];
         if ( msgno == n + 1 ) // +1 because n is an index, not msgno
         {
            // found the position of msgno which was expunged, store it
            idxRemovedInMsgnos = i;
         }
         else if ( msgno > n )
         {
            // indices are shifted
            m_tableMsgno[i]--;
         }
         //else: leave it as is

         // and also update the positions table if we have it
         if ( m_tablePos )
         {
            MsgnoType pos = m_tablePos[i];
            if ( pos > posRemoved )
            {
               // here the indices shift too
               m_tablePos[i]--;
            }
         }
      }

      // delete the removed element from the translation tables

      ASSERT_MSG( idxRemovedInMsgnos != INDEX_ILLEGAL,
                  "expunged item not found in m_tableMsgno?" );

      if ( idxRemovedInMsgnos < m_count - 1 )
      {
         // shift everything
         MsgnoType *p = m_tableMsgno + idxRemovedInMsgnos;
         memmove(p, p + 1,
                 sizeof(MsgnoType)*(m_count - idxRemovedInMsgnos - 1));
      }
      //else: the expunged msgno was the last one

      if ( m_tablePos )
      {
         if ( n < m_count - 1 )
         {
            MsgnoType *p = m_tablePos + n;
            memmove(p, p + 1, sizeof(MsgnoType)*(m_count - n - 1));
         }
         //else: the removed element was at the last position
      }

#ifdef DEBUG_SORTING
      // last index is invalid now, don't let CHECK_TABLES() check it
      m_count--;

      DUMP_TABLES(("after removing"));
      CHECK_TABLES();

      // restore for -- below
      m_count++;
#endif // DEBUG_SORTING
   }

   // always update the count
   m_count--;
}

void HeaderInfoListImpl::OnAdd(MsgnoType countNew)
{
   if ( HasTransTable() )
   {
      // FIXME: resort everything :-(
      //
      // what we should do instead is to remember the last sorted msgno and
      // check the header info being retrieved against it in GetHeaderInfo():
      // if it is greater, do a binary search in the sorted arrays and insert
      // the new headers into the correct places
      //
      // however there are many problems with it:
      //  1. we can't do it from here because c-client is not reentrant
      //  2. retrieving a lot of headers from GetHeaderInfo() may be too slow
      //  3. what to do if this hadn't been done yet when OnRemove() is called?
      FreeSortTables();

      m_needsResort = true;
   }

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

size_t HeaderInfoListImpl::GetIndentation(MsgnoType pos) const
{
   if ( !m_indents )
      return 0;

   return m_indents[GetIdxFromPos(pos)];
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl searching
// ----------------------------------------------------------------------------

// return the first position between from and to at which an element from the
// array appears
MsgnoType
HeaderInfoListImpl::FindFirstInRange(const MsgnoArray& array,
                                     MsgnoType posFrom, MsgnoType posTo) const
{
   MsgnoType posFirst = UID_ILLEGAL;

   size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      MsgnoType pos = GetPosFromIdx(array[n] - 1);
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

MsgnoType
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

MsgnoType
HeaderInfoListImpl::FindHeaderByFlagWrap(MailFolder::MessageStatus flag,
                                         bool set,
                                         long posFrom)
{
   FindHeaderHelper helper(m_mf, flag, set);

   const MsgnoArray *results = helper.GetResults();
   if ( !results )
      return INDEX_ILLEGAL;

   MsgnoType pos = FindFirstInRange(*results, posFrom + 1, m_count);
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
// HeaderInfoListImpl common sorting/threading stuff
// ----------------------------------------------------------------------------

MsgnoType *HeaderInfoListImpl::AllocTable() const
{
   return (MsgnoType *)malloc(m_count * sizeof(MsgnoType));
}

void HeaderInfoListImpl::FreeIndentTable()
{
   if ( m_indents )
   {
      free(m_indents);
      m_indents = NULL;
   }
}

void HeaderInfoListImpl::FreeSortTables()
{
   if ( m_tableMsgno )
   {
      free(m_tableMsgno);
      m_tableMsgno = NULL;
   }

   if ( m_tablePos )
   {
      free(m_tablePos);
      m_tablePos = NULL;
   }
}

void HeaderInfoListImpl::UpdateInverseTable()
{
   // if there is no direct table, then there is no inverse table neither
   if ( !m_tableMsgno )
   {
      ASSERT_MSG( !m_tablePos, "have inverse table but not direct one?" );

      return;
   }

   // it can be NULL as we delay calculating the inverse mapping until we
   // really need it: this way we may save extra calculation if the listing is
   // first sorted and then threaded as we'll only do it once, after both
   // operations even if m_tableMsgno changes twice
   if ( !m_tablePos )
   {
      m_tablePos = AllocTable();
   }

   // build the inverse table from the direct one
   for ( MsgnoType n = 0; n < m_count; n++ )
   {
      m_tablePos[m_tableMsgno[n] - 1] = n;
   }
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl sorting
// ----------------------------------------------------------------------------

// sort the messages according to the current sort parameters
void HeaderInfoListImpl::Sort()
{
   // easy case: don't sort at all
   if ( GetSortCritDirect(m_sortParams.sortOrder) == MSO_NONE )
   {
      // we may still need to reverse the messages order
      m_reverseOrder = GetSortCrit(m_sortParams.sortOrder) == MSO_NONE_REV;

      FreeSortTables();
   }
   else // we do need to sort messages
   {
      FreeSortTables();

      m_tableMsgno = AllocTable();

      if ( m_mf->SortMessages((MsgnoType *)m_tableMsgno, m_sortParams) )
      {
#ifdef DEBUG_SORTING
         UpdateInverseTable();

         DUMP_TABLES(("after sorting with sort order %08lx",
                     m_sortParams.sortOrder));

         CHECK_TABLES();

         // free it to make the program behave in the same way with
         // DEBUG_SORTING as without it
         free(m_tablePos);
         m_tablePos = NULL;
#endif // DEBUG_SORTING
      }
      else // sort failed
      {
         FreeSortTables();
      }
   }
}

void HeaderInfoListImpl::Resort()
{
   // don't call us again
   m_needsResort = false;

   // FIXME: this should just insert new elements into the already sorted
   //        array, see comments in OnAdd()
   Sort();
}

void HeaderInfoListImpl::Reverse()
{
   m_reverseOrder = !m_reverseOrder;
}

// check if a sort order includes MSO_SENDER
static bool UsesSenderForSorting(long sortOrder)
{
   while ( sortOrder != MSO_NONE )
   {
      if ( GetSortCritDirect(sortOrder) == MSO_SENDER )
      {
         return true;
      }

      sortOrder = GetSortNextCriterium(sortOrder);
   }

   return false;
}

// change the sorting order
bool HeaderInfoListImpl::SetSortOrder(const SortParams& sortParams)
{
   // we are only called if some of the sorting parameters changed, but make
   // sure this change will really affect us before resorting

   // if true, we can only reverse the existing sorting instead of doing full
   // fledged sort
   bool canReverseOnly = false;

   // is the sort order the same (up to reversal if there is only one sort
   // criterium)?
   long sortOrder = sortParams.sortOrder;
   if ( sortOrder == m_sortParams.sortOrder ||
         (!GetSortNextCriterium(sortOrder) &&
          (sortOrder & ~1) == (m_sortParams.sortOrder & ~1)) )
   {
      if ( !UsesSenderForSorting(sortOrder) )
      {
         if ( sortOrder == m_sortParams.sortOrder )
         {
            // nothing changes for us as we don't use the own addresses
            return false;
         }

         // the sort order does change, but we can just reverse the existing
         // tables instead of resorting everything
         canReverseOnly = true;
      }
   }

   // remember new sort order
   m_sortParams = sortParams;

   // sorting order can hardly change the listing if we have less than 2
   // elements
   if ( m_count < 2 )
   {
      return false;
   }

   if ( canReverseOnly )
   {
      Reverse();
   }
   else
   {
      // recalculate the tables
      Sort();
   }

   // assume it changed
   m_lastMod++;

   return true;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl threading
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::Thread()
{
   if ( !m_thrParams.useThreading )
   {
      // this one is surely not needed any longer
      FreeIndentTable();

      // but the trouble is that we also have to resort the messages as we
      // can't "unthread" them
      Sort();
   }
   else // do thread
   {
      MsgnoType *tableThr = AllocTable();
      m_indents = (size_t *)malloc(m_count * sizeof(size_t));

      if ( m_mf->ThreadMessages(tableThr, m_indents, m_thrParams) )
      {
         // unfortunately it simply doesn't work this way... you can't combine
         // them in such way!
#if 0
         if ( HasTransTable() )
         {
            // combine the threading index translation with the sorting one:
            // this is just the multiplication of permutations

            // unfortunately we can't do it in place
            MsgnoType *tableMsgnoNew = AllocTable();
            for ( size_t n = 0; n < m_count; n++ )
            {
               // don't forget to subtract 1 as tableThr contains msgnos
               tableMsgnoNew[n] = m_tableMsgno[tableThr[n] - 1];
            }

            FreeSortTables();
            m_tableMsgno = tableMsgnoNew;

            free(tableThr);
         }
         else
         {
            // it will be just the one we have built
            m_tableMsgno = tableThr;
         }
#else // 1
         FreeSortTables();
         m_tableMsgno = tableThr;
#endif // 0/1

#ifdef DEBUG_SORTING
         UpdateInverseTable();

         DUMP_TABLES(("after threading"));
         CHECK_TABLES();

         // free it to make the program behave in the same way with
         // DEBUG_SORTING as without it
         free(m_tablePos);
         m_tablePos = NULL;
#endif // DEBUG_SORTING
      }
      else // threading failed
      {
         // FIXME: what to do?
      }
   }
}

void HeaderInfoListImpl::Rethread()
{
   if ( m_thrParams.useThreading )
   {
      // TODO: there is surely a more efficient way of doing this
      Thread();
   }
}

bool HeaderInfoListImpl::SetThreadParameters(const ThreadParams& thrParams)
{
   m_thrParams = thrParams;

   if ( m_count < 2 )
   {
      // nothing can change for us anyhow
      return false;
   }

   Thread();

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

void HeaderInfoListImpl::ExpandToMakeIndexValid(MsgnoType n)
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
      MsgnoType idx = GetIdxFromMsgno(i);

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
   // first thing to do is to (re)establish the correct mapping between msgnos
   // and positions, if necessary
   if ( NeedsSort() )
   {
      Resort();
      Rethread();
   }

   // transform sequence of positions into sequence of msgnos
   Sequence seqMsgnos;
   if ( IsSorting() )
   {
      // a contiguous range of positions doesn't correspond in general to a
      // contiguous set of msgnos so constructing a new sequence is,
      // potentially, quite time consuming - any way to optimize this?

      UIdArray msgnos;
      size_t n;
      for ( UIdType pos = seq.GetFirst(n);
            pos != UID_ILLEGAL;
            pos = seq.GetNext(pos, n) )
      {
         ASSERT_MSG( pos < m_count, "invalid position in the sequence" );

         msgnos.Add(GetMsgnoFromPos(pos));
      }

      msgnos.Sort(MsgnoCmpFunc);

      seqMsgnos.AddArray(msgnos);
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

bool HeaderInfoListImpl::IsInCache(MsgnoType pos) const
{
   // we can't use GetIdxFromPos() if our sorting tables are out of date, tell
   // them to re-retrieve all positions they're interested in
   //
   // NB: don't call Resort() from here as this function is supposed to always
   //     return immediately and Resort() is a long function (it access the
   //     folder)
   if ( NeedsSort() )
      return false;

   MsgnoType idx = GetIdxFromPos(pos);

   CHECK( idx < m_count, false,
          "HeaderInfoListImpl::IsInCache(): invalid position" );

   return (idx < m_headers.GetCount()) && (m_headers[idx] != NULL);
}

bool HeaderInfoListImpl::ReallyGet(MsgnoType pos)
{
   CHECK( !NeedsSort(), false, "can't be called now" );

   MsgnoType idx = GetIdxFromPos(pos);

   ExpandToMakeIndexValid(idx);

   if ( !m_headers[idx] )
   {
      // the idea is that this method is only called if the header had been
      // "retrieved" before but the transfer was cancelled by the user, so we
      // don't really have any information for it - but in this case the element
      // corresponding to it must have been already created, so assert
      FAIL_MSG( "not supposed to be called" );

      // but still don't crash
      m_headers[idx] = new HeaderInfo;
   }

   if ( m_headers[idx]->IsValid() )
   {
      FAIL_MSG( "why call ReallyGet() then?" );

      return true;
   }

   Sequence seq;
   seq.Add(GetMsgnoFromIdx(idx));

   return m_mf->GetHeaderInfo(m_headers, seq) == 1;
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
