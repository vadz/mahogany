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
   #define DUMP_TABLE(table, msg) \
      printf(#table " (count = %ld) ", m_count); \
      printf msg ; \
      puts("") ; \
      DumpTable(m_count, table); \
      puts("")
   #define DUMP_TRANS_TABLES(msg) \
      printf("Translation tables state (count = %ld)", m_count); \
      printf msg ; \
      DumpTransTables(m_count, m_tableMsgno, m_tablePos); \
      puts("")
#else
   #define CHECK_TABLES()
   #define DUMP_TABLE(table, msg)
   #define DUMP_TRANS_TABLES(msg)
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
static void DumpTransTables(MsgnoType count,
                            const MsgnoType *msgnos,
                            const MsgnoType *pos)
{
   printf("\n\tmsgnos:");
   DumpTable(count, msgnos);

   printf("\n\tpos   :");
   DumpTable(count, pos);
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
   return m_sortParams.sortOrder != MSO_NONE || m_reverseOrder;
}

inline bool HeaderInfoListImpl::IsThreading() const
{
   return m_thrParams.useThreading;
}

inline bool HeaderInfoListImpl::IsHeaderValid(MsgnoType n) const
{
   return (n < m_headers.GetCount()) && (m_headers[n] != NULL);
}

inline bool HeaderInfoListImpl::HasTransTable() const
{
   return m_tableMsgno != NULL;
}

inline bool HeaderInfoListImpl::IsTranslatingIndices() const
{
   return HasTransTable() || m_reverseOrder;
}

inline bool HeaderInfoListImpl::MustRebuildTables() const
{
   // if we already have the tables, we don't have to rebuild them
   //
   // we also don't need them if we are not really sorting but just reveresing
   // the indices
   return !HasTransTable() &&
          (IsThreading() ||
           GetSortCritDirect(m_sortParams.sortOrder) != MSO_NONE);
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
   m_tableSort =
   m_tableMsgno =
   m_tablePos = NULL;

   m_thrData = NULL;

   m_dontFreeMsgnos = false;

   m_reverseOrder = false;
}

void HeaderInfoListImpl::CleanUp()
{
   // TODO: maintain first..last range of allocated headers to avoid iterating
   //       over the entire array just to call delete for many NULL pointers?

   WX_CLEAR_ARRAY(m_headers);

   m_lastMod++;

   FreeSortData();
   FreeThreadData();
   FreeTables();
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
   if ( MustRebuildTables() )
   {
      ((HeaderInfoListImpl *)this)->BuildTables(); // const_cast
   }

   if ( m_reverseOrder )
   {
      ASSERT_MSG( !IsThreading(), "can't reverse threaded listing!" );

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
      // it is wasteful to call us if we don't perform any index translation at
      // all and this is not supposed to happen
      CHECK( m_tableMsgno, 0, "shouldn't be called at all in this case" );

      return m_tableMsgno[pos];
   }
}

MsgnoType HeaderInfoListImpl::GetIdxFromPos(MsgnoType pos) const
{
   CHECK( pos < m_count, INDEX_ILLEGAL, "invalid position in GetIdxFromPos" );

   return IsTranslatingIndices() ? GetMsgnoFromPos(pos) - 1 : pos;
}

MsgnoType HeaderInfoListImpl::GetPosFromIdx(MsgnoType n) const
{
   CHECK( n < m_count, INDEX_ILLEGAL, "invalid index in GetPosFromIdx" );

   // calculate the table on the fly if needed
   if ( MustRebuildTables() )
   {
      ((HeaderInfoListImpl *)this)->BuildTables(); // const_cast
   }

   if ( m_reverseOrder )
   {
      ASSERT_MSG( !IsThreading(), "can't reverse threaded listing!" );

      if ( m_tablePos )
      {
         // reverse the order specified by the table
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
//    being invalid (somehow...) and ignore it later (?)

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
      DUMP_TRANS_TABLES(("before removing element %ld:", n));

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

      DUMP_TRANS_TABLES(("after removing"));
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
   if ( IsSorting() || IsThreading() )
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
      FreeSortData();
      FreeThreadData();
      FreeTables();
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
   return m_thrData ? m_thrData->m_indents[GetIdxFromPos(pos)] : 0;
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

MsgnoType HeaderInfoListImpl::GetNthSortedMsgno(MsgnoType n) const
{
   if ( m_reverseOrder )
   {
      // reverse either the table order or the natural order
      return m_tableSort ? m_tableSort[m_count - 1 - n]
                         : m_count - n;
   }
   else // !reversed order
   {
      // must have a table, otherwise how do we sort messages?
      return m_tableSort[n];
   }
}

void HeaderInfoListImpl::CombineSortAndThread()
{
   // don't crash below if we don't have it somehow
   CHECK_RET( m_thrData, "must be already threaded" );

   // check it now or we'd crash in the loop below (and I don't want to
   // put the check inside the loop obviously)
   CHECK_RET( m_tableSort || m_reverseOrder, "how do we sort them?" );

   // normally we're called from BuildTables() so we shouldn't have it yet
   ASSERT_MSG( !m_tableMsgno, "unexpected call to CombineSortAndThread" );

   m_tableMsgno = AllocTable();
   memset(m_tableMsgno, 0, m_count * sizeof(MsgnoType));

   /*
      Here is the idea: we scan the sorting table from top to bottom and
      extract from it first the root messages (i.e. with 0 indent in the
      threading table) which we can immediately put in order into
      m_tableMsgno because we know that the first root message will be at
      position 0, the next one - at position 1 + total number of children
      of the previous one &c (this index is idxCur below).

      Next we rescan the table for messages of indent 1 and put them in
      the free slots of m_tableMsgno (this is why we initialize it with
      0s above), i.e. do the same thing as above but skipping over the
      slots already taken by roots &c.

      The code below optimizes the algorithm outlined above: while the
      latter would require "max thread depth" passes over the sorting
      table (and the last passes are very inefficient as we need to scan
      the whole huge table only to find a few messages), the former does
      only 2 passes:

         1. position all roots as described above and count the number
            of messages at level N
         2. store the indices in the sort table of all messages at level
            N in the preallocated arrays
         3. traverse each of these arrays putting messages in place
    */

   // old multi-pass code, keep it here for now for reference (it should
   // produce the same results as the new one!)
#if 0
   size_t level = 0;
   MsgnoType countDone = 0;
   while ( countDone < m_count )
   {
      MsgnoType idxCur = 0;

      for ( MsgnoType n = 0; n < m_count; n++ )
      {
         MsgnoType msgno;
         if ( m_reverseOrder )
         {
            // reverse either the table order or the natural order
            msgno = m_tableSort ? m_tableSort[m_count - 1 - n]
                                : m_count - n;
         }
         else // !reversed order
         {
            // must have a table, otherwise how do we sort messages?
            msgno = m_tableSort[n];
         }

         if ( m_thrData->m_indents[msgno - 1] != level )
         {
            // ignore it now, we either have already seen it or will see
            // it later
            continue;
         }

         // skip the slots already taken (by messages of the lesser
         // level)
         while ( idxCur < m_count && m_tableMsgno[idxCur] )
         {
            idxCur++;
         }

         // there must be a free slot somewhere left!
         ASSERT_MSG( idxCur < m_count, "logic error" );

         // this is the next message we have here, so put it in the first
         // available slot in the msgno table
         m_tableMsgno[idxCur++] = msgno;

         // one more message put in place
         countDone++;

         // reserve some places for the children of this item
         idxCur += m_thrData->m_children[msgno - 1];
      }

      // now process the messages at the next level (if there are any, of
      // course)
      level++;
   }
#endif // 0

   // first pass: position the root messages and remember the number of other
   // ones in the array
   wxArrayInt numAtLevel;

   // the next available slot in m_tableMsgno
   MsgnoType idxCur = 0;

   MsgnoType n;
   for ( n = 0; n < m_count; n++ )
   {
      MsgnoType msgno = GetNthSortedMsgno(n);

      size_t level = m_thrData->m_indents[msgno - 1];
      if ( !level ) // root message?
      {
         // this is the next message we have here, so put it in the first
         // available slot in the msgno table
         m_tableMsgno[idxCur++] = msgno;

         // reserve some places for the children of this item
         idxCur += m_thrData->m_children[msgno - 1];
      }
      else // child
      {
         // array indices start from 0, not 1
         level--;

         // make sure the array has enough elements expanding it if necessary
         size_t count = numAtLevel.GetCount();
         for ( size_t n = count; n <= level; n++ )
         {
            numAtLevel.Add(0);
         }

         // one more
         numAtLevel[level]++;
      }
   }

   // alloc memory for the indices
   size_t level,
          depthMax = numAtLevel.GetCount();
   MsgnoType **indicesAtLevel = new MsgnoType *[depthMax];
   for ( level = 0; level < depthMax; level++ )
   {
      indicesAtLevel[level] = new MsgnoType[numAtLevel[level]];
      numAtLevel[level] = 0;
   }

   // second pass: fill the indicesAtLevel arrays
   for ( n = 0; n < m_count; n++ )
   {
      MsgnoType msgno = GetNthSortedMsgno(n);

      size_t level = m_thrData->m_indents[msgno - 1];
      if ( level > 0 )
      {
         level--;
         indicesAtLevel[level][numAtLevel[level]++] = msgno;
      }
   }

   // and finally, traverse the arrays for all depths putting the messages in
   // place
   for ( level = 0; level < depthMax; level++ )
   {
      idxCur = 0;

      MsgnoType countLevel = numAtLevel[level];
      for ( n = 0; n < countLevel; n++ )
      {
         MsgnoType msgno = indicesAtLevel[level][n];

         // skip the slots already taken (by messages of the lesser
         // level)
         while ( idxCur < m_count && m_tableMsgno[idxCur] )
         {
            idxCur++;
         }

         // there must be a free slot somewhere left!
         ASSERT_MSG( idxCur < m_count, "logic error" );

         // this is the next message we have here, so put it in the first
         // available slot in the msgno table
         m_tableMsgno[idxCur++] = msgno;

         // reserve some places for the children of this item
         idxCur += m_thrData->m_children[msgno - 1];
      }
   }

   // free memory
   for ( level = 0; level < depthMax; level++ )
   {
      delete [] indicesAtLevel[level];
   }

   delete [] indicesAtLevel;
}

void HeaderInfoListImpl::BuildTables()
{
   // maybe we have them already?
   if ( m_tableMsgno )
   {
      ASSERT_MSG( m_tablePos, "should have inverse table as well!" );

      return;
   }

   // no, check if we need them

   // sort the messages if needed and not done yet: we'll need the sort table
   // anyhow, whether we thread messages or not
   if ( IsSorting() && !m_tableSort )
   {
      Sort();
   }

   if ( IsThreading() )
   {
      // first thread the messages if not done yet
      if ( !m_thrData )
      {
         Thread();
      }

      // how to sort threaded messages here, i.e. construct m_tableMsgno and
      // m_tablePos from m_thrData and m_tableSort
      if ( IsSorting() )
      {
         // create m_tableMsgno from m_tableSort and m_tableThread
         CombineSortAndThread();
      }
      else // no sorting
      {
         // just reuse the same table
         m_tableMsgno = m_thrData->m_tableThread;
         m_dontFreeMsgnos = true;
      }

      // reset it as it doesn't make sense to use it with threading (thread
      // would be inverted as well and grow upwards - hardly what we want)
      m_reverseOrder = false;
   }
   else if ( IsSorting() )
   {
      // don't create another table, just reuse the same one
      m_tableMsgno = m_tableSort;
      m_dontFreeMsgnos = true;
   }
   //else: no sorting/no threading at all

   // if we have the direct table compute the inverse one as well
   //
   // NB: we might not have m_tableMsgno if we don't sort/thread at all _or_ if
   //     we do sorting according to MSO_NONE_REV, i.e. don't use the table but
   //     just m_reverseOrder
   if ( m_tableMsgno && !m_tablePos )
   {
      m_tablePos = AllocTable();

      // build the inverse table from the direct one
      for ( MsgnoType n = 0; n < m_count; n++ )
      {
         m_tablePos[m_tableMsgno[n] - 1] = n;
      }
   }

   CHECK_TABLES();
}

void HeaderInfoListImpl::FreeTables()
{
   if ( m_tableMsgno )
   {
      // don't free it if it is the same as either m_tableThread or m_tableSort
      if ( m_dontFreeMsgnos )
         m_dontFreeMsgnos = false;
      else
         free(m_tableMsgno);
      m_tableMsgno = NULL;
   }

   if ( m_tablePos )
   {
      free(m_tablePos);
      m_tablePos = NULL;
   }
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl sorting
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::FreeSortData()
{
   if ( m_tableSort )
   {
      free(m_tableSort);
      m_tableSort = NULL;
   }
}

// sort the messages according to the current sort parameters
void HeaderInfoListImpl::Sort()
{
   // caller must check if we need to be sorted
   ASSERT_MSG( !m_tableMsgno && !m_tableSort,
               "shouldn't be called again" );

   switch ( GetSortCrit(m_sortParams.sortOrder) )
   {
      case MSO_NONE:
         // the easiest case: don't sort at all
         break;

      case MSO_NONE_REV:
         // we just need to reverse the messages order
         m_reverseOrder = true;
         break;

      default:
         // we do need to sort messages
         m_tableSort = AllocTable();

         if ( m_mf->SortMessages(m_tableSort, m_sortParams) )
         {
            DUMP_TABLE(m_tableSort,
                       ("after sorting with sort order %08lx",
                        m_sortParams.sortOrder));
         }
         else // sort failed
         {
            FreeSortData();
         }
   }
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
   if ( sortParams == m_sortParams )
   {
      // nothing changed at all
      return false;
   }

   // ok, so some of the sorting parameters changed, but make sure this change
   // will really affect us before resorting

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

   if ( canReverseOnly )
   {
      m_reverseOrder = !m_reverseOrder;
   }
   else
   {
      m_reverseOrder = false;
   }

   // sorting order can hardly change the listing if we have less than 2
   // elements
   if ( m_count < 2 )
   {
      return false;
   }

   if ( canReverseOnly )
   {
      // if we are threading messages, we must rebuild the translation table as
      // there is no easy way to invert the threading table, but otherwise we
      // may reuse the current sorting table as inverting it is as easy as
      // shifting the indices
      if ( IsThreading() )
      {
         FreeTables();
      }

      // in any case, don't throw away the existing sort data, we can perfectly
      // well continue using them
   }
   else // sort order really changed
   {
      // we will resort messages when needed
      FreeSortData();
      FreeTables();
   }

   // assume it changed
   m_lastMod++;

   return true;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl threading
// ----------------------------------------------------------------------------

void HeaderInfoListImpl::FreeThreadData()
{
   if ( m_thrData )
   {
      delete m_thrData;
      m_thrData = NULL;
   }
}

void HeaderInfoListImpl::Thread()
{
   // the caller must check that we need to be threaded
   ASSERT_MSG( !m_thrData && IsThreading(), "shouldn't be called" );

   m_thrData = new ThreadData(m_count);

   if ( m_mf->ThreadMessages(m_thrData, m_thrParams) )
   {
      DUMP_TABLE(m_thrData->m_tableThread, ("after threading"));
   }
   else // threading failed
   {
      FreeThreadData();
   }
}

bool HeaderInfoListImpl::SetThreadParameters(const ThreadParams& thrParams)
{
   if ( thrParams == m_thrParams )
   {
      // nothing changed at all
      return false;
   }

   m_thrParams = thrParams;

   if ( m_count < 2 )
   {
      // nothing can change for us anyhow
      return false;
   }

   // we will rethread messages and rebuild the translation tables the next time
   // they are needed
   FreeThreadData();
   FreeTables();

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
   // update the translation tables if necessary
   if ( MustRebuildTables() )
   {
      BuildTables();
   }

   // transform sequence of positions into sequence of msgnos
   Sequence seqMsgnos;
   if ( IsTranslatingIndices() )
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
   if ( MustRebuildTables() )
      return false;

   MsgnoType idx = GetIdxFromPos(pos);

   CHECK( idx < m_count, false,
          "HeaderInfoListImpl::IsInCache(): invalid position" );

   return (idx < m_headers.GetCount()) && (m_headers[idx] != NULL);
}

bool HeaderInfoListImpl::ReallyGet(MsgnoType pos)
{
   // we must be already sorted/threaded by now
   CHECK( !IsTranslatingIndices() || HasTransTable(), false,
          "can't be called now" );

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
