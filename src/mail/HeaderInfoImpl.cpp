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
   #include "Mcclient.h"      // need THREADNODE
#endif // USE_PCH

#include "HeaderInfoImpl.h"

#include "Sequence.h"
#include "UIdArray.h"

#include "MDialogs.h"         // for MProgressInfo

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_SHOWBUSY_DURING_SORT;

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// define this to do some (expensive!) run-time checks for translation tables
// consistency
//#define DEBUG_SORTING

#ifdef DEBUG_SORTING
   #define CHECK_TABLES() VerifyTables(m_sizeTables, m_tableMsgno, m_tablePos)
   #define CHECK_THREAD_DATA() VerifyThreadData(m_thrData)
   #define DUMP_TABLE(table, msg) \
      printf(#table " (count = %ld) ", m_sizeTables); \
      printf msg ; \
      puts("") ; \
      DumpTable(m_sizeTables, table); \
      puts("")
   #define DUMP_TRANS_TABLES(msg) \
      printf("Translation tables state (count = %ld) ", m_sizeTables); \
      printf msg ; \
      DumpTransTables(m_sizeTables, m_tableMsgno, m_tablePos); \
      puts("")
#else
   #define CHECK_TABLES()
   #define CHECK_THREAD_DATA()
   #define DUMP_TABLE(table, msg)
   #define DUMP_TRANS_TABLES(msg)
#endif

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// wrapper for compatibility with the old SearchByFlag()
static inline MsgnoArray *SearchFolderByFlag(MailFolder *mf,
                                             MailFolder::MessageStatus flag,
                                             bool set)
{
   return mf->SearchByFlag(flag,
                           MailFolder::SEARCH_UNDELETED |
                           (set ? MailFolder::SEARCH_SET :
                                  MailFolder::SEARCH_UNSET));
}

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
      m_msgnosFound = SearchFolderByFlag(mf, flag, set);
   }

   const MsgnoArray *GetResults() const { return m_msgnosFound; }

   ~FindHeaderHelper()
   {
      delete m_msgnosFound;
   }

private:
   MsgnoArray *m_msgnosFound;
};

// small class which puts the given message into the frame status bar (if we
// have any interactive frame associated with us) and then either appends
// "done" to it if Fail() is not called or replaces it with the Fail() message
// otherwise
//
// the objects of this class are meant to be created on stack and not kept
// between function invocations
class StatusIndicator
{
public:
   // show the status message specified by the remaining arguments if frame is
   // not NULL
   StatusIndicator(wxFrame *frame, const char *fmt, ...);

   // append "done" to the message given by the ctor if Fail() hadn't been
   // called
   ~StatusIndicator();

   // put an error message into the status bar
   void Fail(const String& msgError) { m_msgFinal = msgError; }

protected:
   // just for derived classes, they must call Init() themselves
   StatusIndicator() { m_frame = NULL; }

   // ctor version which can be also called by derived classes
   void Init(wxFrame *frame, const char *fmt, va_list argptr);

   // our frame - may be NULL!
   wxFrame *m_frame;

   // the message given in the ctor
   String m_msgInitial;

   // the message given to Fail() or empty
   String m_msgFinal;
};

// same as StatusIndicator but also shows a "busy info" screen while we're busy
class BusyIndicator : public StatusIndicator
{
public:
   BusyIndicator(bool nonInteractive, MailFolder *mf, const char *fmt, ...);
   ~BusyIndicator();

   void SetLabel(const char *fmt, ...);

private:
   // the busy info screen we show
   MProgressInfo *m_progInfo;
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
                  _T("invalid msgno in the translation table") );

      if ( pos[msgno - 1] != n )
      {
         FAIL_MSG( _T("translation tables are in inconsistent state!") );
      }

      if ( msgnosFound.Index(msgno) != wxNOT_FOUND )
      {
         FAIL_MSG( _T("duplicate msgno in translation table!") );
      }
      else
      {
         msgnosFound.Add(msgno);
      }
   }

   ASSERT_MSG( msgnosFound.GetCount() == count,
               _T("missing msgnos in translation table!?") );
}

// do some basic checks on thread data for consitency
static void VerifyThreadData(const ThreadData *thrData)
{
   size_t n,
          count = thrData->m_count;

   // first check the thread table: it should be a permutation of all messages
   wxArrayInt msgnosFound;
   for ( n = 0; n < count; n++ )
   {
      MsgnoType msgno = thrData->m_tableThread[n];

      ASSERT_MSG( msgno > 0 && msgno <= count,
                  _T("invalid msgno in the thread table") );

      if ( msgnosFound.Index(msgno) != wxNOT_FOUND )
      {
         FAIL_MSG( _T("duplicate msgno in thread table!") );
      }
      else
      {
         msgnosFound.Add(msgno);
      }
   }

   ASSERT_MSG( msgnosFound.GetCount() == count,
               _T("missing msgnos in thread table!?") );

   // second check: indents must be positive
   for ( n = 0; n < count; n++ )
   {
      if ( thrData->m_indents[n] > (size_t)1000 )
      {
         FAIL_MSG( _T("suspiciously big indent - overflow?") );
      }
   }

   // third check: children must be consistent with the other 2 tables
   for ( n = 0; n < count; n++ )
   {
      MsgnoType idx = thrData->m_tableThread[n] - 1;
      size_t indent = thrData->m_indents[idx];

      size_t m;
      for ( m = n + 1; m < count; m++ )
      {
         MsgnoType idx2 = thrData->m_tableThread[m] - 1;
         if ( thrData->m_indents[idx2] <= indent )
            break;
      }

      ASSERT_MSG( thrData->m_children[idx] == m - n - 1,
                  _T("inconsistent thread data") );
   }
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
// StatusIndicator
// ----------------------------------------------------------------------------

StatusIndicator::StatusIndicator(wxFrame *frame, const char *fmt, ...)
{
   va_list argptr;
   va_start(argptr, fmt);
   Init(frame, fmt, argptr);
   va_end(argptr);
}

void StatusIndicator::Init(wxFrame *frame, const char *fmt, va_list argptr)
{
   m_msgInitial.PrintfV(fmt, argptr);

   m_frame = frame;
   if ( m_frame )
   {
      wxLogStatus(m_frame, m_msgInitial);
   }
}

StatusIndicator::~StatusIndicator()
{
   if ( m_frame )
   {
      if ( m_msgFinal.empty() )
      {
         // give success message by default
         m_msgFinal << m_msgInitial << _(" done.");
      }
      //else: we have the failure message in m_msgFinal

      wxLogStatus(m_frame, m_msgFinal);
   }
}

// ----------------------------------------------------------------------------
// BusyIndicator
// ----------------------------------------------------------------------------

BusyIndicator::BusyIndicator(bool nonInteractive,
                             MailFolder *mf,
                             const char *fmt, ...)
{
   va_list argptr;
   va_start(argptr, fmt);
   Init(nonInteractive ? NULL : mf->GetInteractiveFrame(), fmt, argptr);
   va_end(argptr);

   // only show the busy dialog when we're opening the folder manually,
   // otherwise it would be very annoying as it would popup without any user
   // intervention
   if ( m_frame && READ_CONFIG(mf->GetProfile(), MP_SHOWBUSY_DURING_SORT) )
   {
      m_progInfo = new MProgressInfo(m_frame, m_msgInitial);
      wxYield();
   }
   else // busy indicator disabled
   {
      m_progInfo = NULL;
   }
}

void BusyIndicator::SetLabel(const char *fmt, ...)
{
   if ( m_progInfo )
   {
      va_list argptr;
      va_start(argptr, fmt);
      m_progInfo->SetLabel(wxString::FormatV(fmt, argptr));
      va_end(argptr);
   }
}

BusyIndicator::~BusyIndicator()
{
   delete m_progInfo;
}

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
   CHECK( hi && value, Invalid, _T("invalid parameter in HeaderInfo::GetFromOrTo") );

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

inline bool HeaderInfoListImpl::ShouldHaveTables() const
{
   // note that we can't use IsSorting() here because we don't need need the
   // table is we just reverse the order (in which case IsSorting() would have
   // returned true)
   return IsThreading() ||
           (GetSortCritDirect(m_sortParams.sortOrder) != MSO_NONE);
}

inline bool HeaderInfoListImpl::MustRebuildTables() const
{
   // if we don't have tables anyhow, we surely are not going to rebuild them
   if ( !ShouldHaveTables() )
      return false;

   // if the tables exist but are out of date, i.e. we got some new messages
   // since they were built we have to rebuild them -- as we do if they don't
   // exist at all yet
   if ( !HasTransTable() || m_sizeTables < m_count || m_mustRebuildTables )
   {
      ((HeaderInfoListImpl *)this)->FreeSortAndThreadData(); // const_cast

      // there is no need to rebuild tables for 1 msesage onl
      return m_count >= 2;
   }

   // nothing to do: tables are up to date
   return false;
}

inline bool HeaderInfoListImpl::IsTranslatingIndices() const
{
   // if we must rebuild the tables, it means that we are going to have them
   // and we will do the index translations
   return MustRebuildTables() || HasTransTable() || m_reverseOrder;
}

inline void HeaderInfoListImpl::ScheduleTableRebuild()
{
   // simply set the flag: we keep the table data for GetOldPosFromIdx() needs,
   // but we will rebuild them as soon as possible
   m_mustRebuildTables = true;
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl creation and destruction
// ----------------------------------------------------------------------------

/* static */
HeaderInfoList *HeaderInfoList::Create(MailFolder *mf)
{
   CHECK( mf, NULL, _T("NULL mailfolder in HeaderInfoList::Create") );

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
   m_sizeTables = 0;
   m_tableSort =
   m_tableMsgno =
   m_tablePos = NULL;

   m_thrData = NULL;

   m_dontFreeMsgnos = false;
   m_firstSort = true;

   m_reverseOrder = false;
   m_reversedTables = false;
   m_mustRebuildTables = false;
}

void HeaderInfoListImpl::CleanUp()
{
   // TODO: maintain first..last range of allocated headers to avoid iterating
   //       over the entire array just to call delete for many NULL pointers?

   WX_CLEAR_ARRAY(m_headers);

   m_lastMod++;

   FreeSortAndThreadData();
}

void HeaderInfoListImpl::FreeSortAndThreadData()
{
   FreeSortData();
   FreeThreadData();
   FreeTables();

   m_sizeTables = 0;
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
   CHECK( n < m_count, NULL, _T("invalid index in HeaderInfoList::GetItemByIndex") );

   if ( !IsHeaderValid(n) )
   {
      HeaderInfoListImpl *self = (HeaderInfoListImpl *)this; // const_cast

      self->ExpandToMakeIndexValid(n);

      // alloc space for new header
      m_headers[n] = new HeaderInfo;

      // get header info for the new header
      Sequence seq;
      seq.Add(n + 1);   // make msgno from index
      m_mf->GetHeaderInfo(self->m_headers, seq);
   }

   // the caller will crash...
   ASSERT_MSG( m_headers[n], _T("returning NULL HeaderInfo?") );

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
      ASSERT_MSG( !IsThreading(), _T("can't reverse threaded listing!") );

      if ( pos < m_sizeTables )
      {
         // flip the table (which always corresponds to the direct ordering)
         return m_tableMsgno[m_sizeTables - 1 - pos];
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
      CHECK( m_tableMsgno, 0, _T("shouldn't be called at all in this case") );

      return m_tableMsgno[pos];
   }
}

MsgnoType HeaderInfoListImpl::GetIdxFromPos(MsgnoType pos) const
{
   CHECK( pos < m_count, INDEX_ILLEGAL, _T("invalid position in GetIdxFromPos") );

   return IsTranslatingIndices() ? GetMsgnoFromPos(pos) - 1 : pos;
}

MsgnoType HeaderInfoListImpl::GetPosFromIdx(MsgnoType n) const
{
   CHECK( n < m_count, INDEX_ILLEGAL, _T("invalid index in GetPosFromIdx") );

   // calculate the table on the fly if needed
   if ( MustRebuildTables() )
   {
      ((HeaderInfoListImpl *)this)->BuildTables(); // const_cast
   }

   return GetOldPosFromIdx(n);
}

MsgnoType HeaderInfoListImpl::GetOldPosFromIdx(MsgnoType n) const
{
   // use the information which we have, do *not* rebuild the tables from here
   // as we are called from a cclient callback and so can't call cclient again

   if ( m_reverseOrder )
   {
      ASSERT_MSG( !IsThreading(), _T("can't reverse threaded listing!") );

      if ( n < m_sizeTables )
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
      // maybe we haven't yet updated the table for the new messages, in this
      // case assume that these new messages are after all the old ones
      return m_tablePos && n < m_sizeTables ? m_tablePos[n] : n;
   }
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl methods called by MailFolder
// ----------------------------------------------------------------------------

// TODO: OnRemove() is rather expensive operation, we shouldn't call it for
//       each message being removed but rather for the whole sequence of them
//       (this should be also done to minimize GUI updates...)

// small OnRemove() helper: removes an element from the array
static void
RemoveElementFromTable(MsgnoType *table,
                       MsgnoType idxRemoved,
                       MsgnoType count)
{
   if ( idxRemoved < count - 1 )
   {
      // shift everything
      MsgnoType *p = table + idxRemoved;
      memmove(p, p + 1, sizeof(MsgnoType)*(count - idxRemoved - 1));
   }
   //else: the expunged msgno was the last one, nothing to do
}

#ifndef wxSIZE_T_IS_ULONG

// same as above but for size_t (yes, one day it will be a template...)
static void
RemoveElementFromTable(size_t *table,
                       MsgnoType idxRemoved,
                       MsgnoType count)
{
   if ( idxRemoved < count - 1 )
   {
      // shift everything
      size_t *p = table + idxRemoved;
      memmove(p, p + 1, sizeof(size_t)*(count - idxRemoved - 1));
   }
   //else: the expunged msgno was the last one, nothing to do
}

#endif // !wxSIZE_T_IS_ULONG

void HeaderInfoListImpl::OnRemove(MsgnoType n)
{
   CHECK_RET( n < m_count, _T("invalid index in HeaderInfoList::OnRemove") );

   ASSERT_MSG( m_count, _T("removing a message from empty folder?") );

   if ( n < m_headers.GetCount() )
   {
      delete m_headers[n];
      m_headers.RemoveAt(n);

      // the indices are shifted (and one is even removed completely), so
      // invalidate the pointers into m_headers
      m_lastMod++;
   }
   //else: we have never looked that far

   /*
      In a normal situation (m_sizeTables == m_count) we update the existing
      sort/thread data, if any as it is less expensive to do it here than to
      resort/thread everything.

      However we may also be called when m_sizeTables < m_count. In this case
      there are two possibilities:

      1. a yet unsorted/threaded msgno is deleted (i.e. n > m_sizeTables) -
         then we don't have to do anything as it doesn't appear in the existing
         table anyhow

      2. an already sorted/threaded msgno (n <= m_sizeTables) is deleted in
         which case we have to invalidate everything we have so far and do full
         resort/thread the next time it is needed
    */

   if ( m_sizeTables != m_count )
   {
      // m_sizeTables must always be <= m_count
      ASSERT_MSG( m_sizeTables < m_count, _T("more sorted messages than total?") );

      // note that if m_sizeTables == 0, we don't have any tables at all so
      // don't try to free them
      if ( m_sizeTables && n <= m_sizeTables )
      {
         // we will resort/thread everything as soon as possible
         ScheduleTableRebuild();
      }
      //else: nothing to do
   }
   else // update the existing sort/thread data
   {
      // use this for convenience instead of writing "n + 1" everywhere
      MsgnoType msgnoRemoved = n + 1;

      // update the sorting table
      if ( m_tableSort )
      {
         DUMP_TABLE(m_tableSort, ("before removing msgno %ld", msgnoRemoved));

         MsgnoType idxRemovedInMsgnos = INDEX_ILLEGAL;
         for ( MsgnoType i = 0; i < m_sizeTables; i++ )
         {
            MsgnoType msgno = m_tableSort[i];
            if ( msgno == msgnoRemoved )
            {
               // found the position of msgno which was expunged, store it
               idxRemovedInMsgnos = i;
            }
            else if ( msgno > msgnoRemoved )
            {
               // subsequent indices get shifted
               m_tableSort[i]--;
            }
            //else: leave the msgnos preceding the one removed as is
         }

         // delete the removed element from the sort table
         ASSERT_MSG( idxRemovedInMsgnos != INDEX_ILLEGAL,
                     _T("expunged item not found in m_tableSort?") );

         RemoveElementFromTable(m_tableSort, idxRemovedInMsgnos, m_sizeTables);

         DUMP_TABLE(m_tableSort, ("after removing"));
      }

      // update the threading table
      if ( m_thrData )
      {
         // The tree becomes invalid, because we don't scan it
         // to find and remove the item corresponding to the
         // message deleted.
         m_thrData->killTree();
         ASSERT(m_thrData->m_root == 0);

         CHECK_THREAD_DATA();

         DUMP_TABLE(m_thrData->m_tableThread,
                    ("before removing msgno %ld", msgnoRemoved));
         DUMP_TABLE((MsgnoType *)m_thrData->m_indents, (" "));
         DUMP_TABLE(m_thrData->m_children, (" "));

         MsgnoType i,
                   idxRemovedInMsgnos = INDEX_ILLEGAL;
         for ( i = 0; i < m_sizeTables; i++ )
         {
            MsgnoType msgno = m_thrData->m_tableThread[i];
            if ( msgno == msgnoRemoved )
            {
               // found the position of msgno which was expunged, store it
               idxRemovedInMsgnos = i;

               // reduce indent of all its children unless it is a root item
               // and we are configured to show indent for the root items which
               // are not real thread roots
               if ( m_thrData->m_indents[n] != 0 ||
                     !m_thrParams.indentIfDummyNode )
               {
                  // all of this item children immediately follow it
                  MsgnoType firstChild = i + 1;
                  MsgnoType lastChild = firstChild + m_thrData->m_children[n];
                  for ( MsgnoType j = firstChild; j < lastChild; j++ )
                  {
                     MsgnoType idx = m_thrData->m_tableThread[j] - 1;

                     // our children must have non zero indents!
                     ASSERT_MSG( m_thrData->m_indents[idx] > 0,
                                 _T("error in OnRemove() logic") );

                     m_thrData->m_indents[idx]--;
                  }
               }

               // also update the children count of the parent items to account
               // for the fact that they have one child less
               for ( MsgnoType nCur = i; ; )
               {
                  // note the double indirection we have to use to access the
                  // indent from the index in the visible thread order!

                  MsgnoType idxCur = m_thrData->m_tableThread[nCur] - 1;
                  size_t indent = m_thrData->m_indents[idxCur];

                  if ( !indent )
                  {
                     // it's a root, don't have to update anything [more]
                     break;
                  }

                  // find the parent of this child: it is the first item
                  // preceding it with indent strictly less
                  //
                  // NB: actually we use parent + 1 as loop variable to avoid
                  //     problems with wrapping at 0
                  MsgnoType nParent;
                  for ( nParent = nCur; nParent > 0; nParent-- )
                  {
                     MsgnoType idx = m_thrData->m_tableThread[nParent - 1] - 1;

                     if ( m_thrData->m_indents[idx] < indent )
                     {
                        // found
                        break;
                     }
                  }

                  if ( !nParent )
                  {
                     // actually I'm not completely sure it really can't happen
                     FAIL_MSG( _T("no parent of the item with non null indent?") );

                     break;
                  }

                  nCur = nParent - 1;

                  idxCur = m_thrData->m_tableThread[nCur] - 1;

                  ASSERT_MSG( m_thrData->m_children[idxCur] > 0,
                              _T("our parent doesn't have any children?") );

                  m_thrData->m_children[idxCur]--;
               }
            }
         }

         // FIXME: we can't adjust indices while we're traversing the table
         //        above because it confuses the code dealing with unindenting
         //        the parent, but it would still be nice to avoid traversing
         //        the entire array twice, but I don't have any time to
         //        understand how to do it now (OTOH, who cares about array
         //        traversal...?)
         for ( i = 0; i < m_sizeTables; i++ )
         {
            MsgnoType msgno = m_thrData->m_tableThread[i];
            if ( msgno > msgnoRemoved )
            {
               // subsequent indices get shifted
               m_thrData->m_tableThread[i]--;
            }
            //else: leave the msgnos preceding the one removed as is
         }

         // delete the removed element from the thread table
         ASSERT_MSG( idxRemovedInMsgnos != INDEX_ILLEGAL,
                     _T("expunged item not found in m_thrData->m_tableThread?") );

         RemoveElementFromTable(m_thrData->m_tableThread,
                                idxRemovedInMsgnos, m_sizeTables);

         // delete the removed element from the other table too (this is easy
         // as they are indexed directly by msgnos)
         RemoveElementFromTable(m_thrData->m_indents, n, m_sizeTables);
         RemoveElementFromTable(m_thrData->m_children, n, m_sizeTables);

         // keep it consistent with us
         m_thrData->m_count--;

#ifdef DEBUG_SORTING
         // last index is invalid now, don't let CHECK_TABLES() check it
         m_sizeTables--;

         DUMP_TABLE(m_thrData->m_tableThread, ("after removing"));
         DUMP_TABLE((MsgnoType *)m_thrData->m_indents, (" "));
         DUMP_TABLE(m_thrData->m_children, (" "));

         CHECK_THREAD_DATA();

         m_sizeTables++;
#endif // DEBUG_SORTING
      }

      // update the actual mappings if we already have them - otherwise they
      // are going to be recalculated from the sort and thread data the next
      // time we need them anyhow
      if ( HasTransTable() )
      {
         if ( m_dontFreeMsgnos )
         {
            // the trans table is really the same as either thread table or
            // sort table which were already updated above, don't try to update
            // it once again - but do update the inverse table
            if ( m_tablePos )
            {
               delete [] m_tablePos;
               m_tablePos = NULL;
            }

            // we must have the correct (i.e. updated) value of m_sizeTables
            // for BuildPosTable() to work properly
            m_sizeTables--;

            BuildPosTable();

            // but now restore it back as it is decremented in the end of this
            // function
            m_sizeTables++;
         }
         else // the trans tables are independent of the other ones, do update
         {
            DUMP_TRANS_TABLES(("before removing msgno %ld", msgnoRemoved));

            // we will determine the index of the message being expunged (in
            // m_tableMsgno) below
            MsgnoType idxRemovedInMsgnos = INDEX_ILLEGAL;

            // init it just to avoid warnings from stupid optimizing compilers
            MsgnoType posRemoved = 0;
            if ( m_tablePos )
            {
               posRemoved = m_tablePos[n];
            }

            // update the translation tables
            for ( MsgnoType i = 0; i < m_sizeTables; i++ )
            {
               // first work with msgnos table
               MsgnoType msgno = m_tableMsgno[i];
               if ( msgno == msgnoRemoved )
               {
                  // found the position of msgno which was expunged, store it
                  idxRemovedInMsgnos = i;
               }
               else if ( msgno > msgnoRemoved )
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
                        _T("expunged item not found in m_tableMsgno?") );

            RemoveElementFromTable(m_tableMsgno, idxRemovedInMsgnos,
                                   m_sizeTables);

            if ( m_tablePos )
            {
               RemoveElementFromTable(m_tablePos, n, m_sizeTables);
            }
         }

#ifdef DEBUG_SORTING
         // last index is invalid now, don't let CHECK_TABLES() check it
         m_sizeTables--;

         DUMP_TRANS_TABLES(("after removing"));
         CHECK_TABLES();

         // restore for -- below
         m_sizeTables++;
#endif // DEBUG_SORTING
      }

      // always update the table size
      m_sizeTables--;
   }

   // always update the number of messages in the folder
   m_count--;
}

void HeaderInfoListImpl::OnAdd(MsgnoType countNew)
{
   /*
      The code here used to call FreeSortAndThreadData() if we were sorting
      and/or threading the messages however we don't do it any more because:

      1. it is inefficient as it means that the messages are going to be
         resorted/rethreaded even if some messages are added to the folder and
         removed from it without anybody caring for their position in the GUI
         view (this often happens for the folders with filters as we typically
         detect new mail and immediately move it to other folder(s) without
         ever showing it to the user in the incoming folder)

      2. even more importantly, this leads to problem with the server-side
         sorting/threading because we can receive a new mail notification from
         inside Sort() or Thread() called from BuildTables() and if we freed
         the tables - which we are in process of building - here, it would
         break BuildTables()

      So instead we just leave m_sizeTables unchanged and we will invalidate
      the sort/thread data only when/if needed, i.e. if we notice that it is
      different from m_count
    */

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
         if ( !IsSorting() && !IsThreading() )
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
   return SearchFolderByFlag(m_mf, flag, set);
}

// ----------------------------------------------------------------------------
// HeaderInfoListImpl common sorting/threading stuff
// ----------------------------------------------------------------------------

MsgnoType *HeaderInfoListImpl::AllocTable() const
{
   return new MsgnoType[m_count];
}

MsgnoType HeaderInfoListImpl::GetNthSortedMsgno(MsgnoType n) const
{
   if ( m_reverseOrder )
   {
      // reverse either the table order or the natural order
      return n < m_sizeTables ? m_tableSort[m_sizeTables - 1 - n]
                              : m_count - n;
   }
   else // !reversed order
   {
      // must have a table, otherwise how do we sort messages?
      return m_tableSort[n];
   }
}


size_t* globalInvSortTable = 0;

extern "C"
{

   static MsgnoType FindMsgno(THREADNODE* p) {
      // This function relies on the fact that the children of a node
      // are sorted before the node is sorted wrt its siblings. This
      // way, we simply get the first non-dummy item.
      MsgnoType msgno = p->num;
      size_t count = 0;
      while (msgno == 0) {
         p = p->next;
         if (p == 0) {
            break;
         }
         msgno = FindMsgno(p);
         count++;
         ASSERT(count < 1000);
      }
      return msgno;
   }

   static int CompareThreadNodes(const void *p1, const void *p2)
   {
      THREADNODE* th1 = *(THREADNODE**)p1;
      MsgnoType msgno1 = FindMsgno(th1);
      CHECK(msgno1 != 0, 0, _T("No message number found in CompareThreadNodes"));
      size_t pos1 = globalInvSortTable[msgno1-1];

      THREADNODE* th2 = *(THREADNODE**)p2;
      MsgnoType msgno2 = FindMsgno(th2);
      CHECK(msgno2 != 0, 0, _T("No message number found in CompareThreadNodes"));
      size_t pos2 = globalInvSortTable[msgno2-1];

      return pos1 - pos2;
   }
}


static void ApplyOrdering(THREADNODE* th) {
   if (th->next == 0) {
      // No children to sort
      return;
   }

   // Count the children of this node, and sort their own children
   size_t nbKids = 0;
   THREADNODE* kid = th->next;
   for (; kid; kid = kid->branch) {
      nbKids++;
      ApplyOrdering(kid);
   }

   if (nbKids < 2) {
      // no need to sort if only one child
      return;
   }

   // Build an array to store all the children of this node
   THREADNODE** kids = new THREADNODE*[nbKids];

   // Store them
   nbKids = 0;
   for (kid = th->next; kid; kid = kid->branch) {
      kids[nbKids++] = kid;
   }
   // And sort the resulting array.
   qsort(kids, nbKids, sizeof(THREADNODE*), CompareThreadNodes);

   // Rebuild the links
   th->next = kids[0];
   for (size_t i = 0; i < nbKids-1; i++) {
      kids[i]->branch = kids[i+1];
   }
   kids[nbKids-1]->branch = 0;

   // We're done. Clean up.
   delete [] kids;
}



THREADNODE* ReOrderTree(THREADNODE* thrNode, MsgnoType *sortTable,
                        bool reverseOrder, size_t count) {

   // FIXME: Have a mutex to protect the globalInvSortTable ?

   // First compute the inverse of the sort table.
   globalInvSortTable = new size_t[count];
   for (size_t i = 0; i < count; ++i) {
      if (sortTable) {
         globalInvSortTable[sortTable[i]-1] = (reverseOrder ? count - i - 1 : i);
      } else {
         globalInvSortTable[i] = i;
      }
   }
   // Now use this information to reorder the children
   // of each node. But this implies that we have a real
   // root node, not a list, at first level. So we build
   // a fake one here.
   THREADNODE fakeRoot;
   fakeRoot.branch = 0;
   fakeRoot.next = thrNode;
   ApplyOrdering(&fakeRoot);

   // Clean up
   delete [] globalInvSortTable;
   globalInvSortTable = 0;

   return fakeRoot.next;
}

#if 0

static size_t FillThreadTables(THREADNODE* node, ThreadData* thrData,
                               size_t &threadedIndex, size_t indent, bool indentIfDummyNode)
{
   if (node->num > 0) {
      // This is not a dummy node
      thrData->m_tableThread[threadedIndex++] = node->num;
      thrData->m_indents[node->num-1] = indent;
   }

   size_t nbChildren = 0;
   if (node->next != 0)
   {
      if (indentIfDummyNode)
         nbChildren = FillThreadTables(node->next, thrData, threadedIndex,
                                       indent+1, indentIfDummyNode);
      else
         nbChildren = FillThreadTables(node->next, thrData, threadedIndex,
                                       indent+((node->num == 0) ? 0 : 1),
                                       indentIfDummyNode);
   }
   if (node->num > 0) {
      thrData->m_children[node->num-1] = nbChildren;
   }
   if (node->branch != 0)
      nbChildren += FillThreadTables(node->branch, thrData, threadedIndex,
                                     indent, indentIfDummyNode);
   return nbChildren + (node->num == 0 ? 0 : 1);  // + 1 for oneself if not dummy
}

#else

static size_t FillThreadTables(THREADNODE* node, ThreadData* thrData,
                               size_t &threadedIndex, size_t indent, bool indentIfDummyNode)
{
   size_t nbChildrenAndBrothers = 0;
   while (node)
   {
      if (node->num > 0) {
         // This is not a dummy node
         thrData->m_tableThread[threadedIndex++] = node->num;
         thrData->m_indents[node->num-1] = indent;
      }
   
      size_t nbChildren = 0;
      if (node->next != 0)
      {
         // Node has at least one child
         size_t indentForKid = indent;
         if (indentIfDummyNode)
         {
            // No need to wonder: always indent
            indentForKid++;
         } else {
            // Indent if current node is not dummy
            indentForKid += (node->num == 0) ? 0 : 1;
         }         
         nbChildren += FillThreadTables(node->next, thrData, threadedIndex,
                                        indentForKid, indentIfDummyNode);
      }
      if (node->num > 0) {
         thrData->m_children[node->num-1] = nbChildren;
      } // else: dummy node, don't store anything

      nbChildrenAndBrothers += nbChildren + (node->num == 0 ? 0 : 1);
      node = node->branch;
   }

   return nbChildrenAndBrothers;
}

#endif


void HeaderInfoListImpl::CombineSortAndThread()
{
   // don't crash below if we don't have it somehow
   CHECK_RET( m_thrData, _T("must be already threaded") );

   // normally we're called from BuildTables() so we shouldn't have it yet
   ASSERT_MSG( !m_tableMsgno, _T("unexpected call to CombineSortAndThread") );

   /*
     We have, on one side, an array of msgno sorted correctly and,
     on the other side, a tree structure resulting from threading.

     Actually, maybe we do not have the sorting table (because no
     sorting order is defined). In this case, we build a fake one
     before reordering.

     First, we reorder the tree so that all the children of each
     node are sorted according to the sorted array, then we map
     the tree structure to the tables.
    */

   m_thrData->m_root =
      ReOrderTree(m_thrData->m_root, m_tableSort, m_reverseOrder, m_sizeTables);

   size_t threadedIndex = 0;
   (void)FillThreadTables(m_thrData->m_root, m_thrData, threadedIndex,
                          0, m_thrParams.indentIfDummyNode);

   DUMP_TABLE(m_thrData->m_tableThread, ("after threading"));

   m_tableMsgno = m_thrData->m_tableThread;
   m_dontFreeMsgnos = true;

   m_tablePos = AllocTable();
   for (size_t i = 0; i < m_sizeTables; ++i)
   {
      m_tablePos[m_tableMsgno[i]-1] = i;
   }

   CHECK_TABLES();
   CHECK_THREAD_DATA();
}

void HeaderInfoListImpl::BuildTables()
{
   // maybe we have them already?
   if ( m_tableMsgno )
   {
      ASSERT_MSG( m_tablePos, _T("should have inverse table as well!") );

      return;
   }

   // what is it inverse for?
   ASSERT_MSG( !m_tablePos, _T("shouldn't have inverse table neither!") );

   // we are rebuilding them, so reset the flag
   m_mustRebuildTables = false;

   // we are going to have the valid tables for that many messages only:
   // m_count may change under our feet from inside Sort() and/or Thread() if
   // we use server-side sorting/threading!
   m_sizeTables = m_count;

   // note that we only show any interactive dialogs during the first
   // sorting/threading because I didn't find any simple way to suppress them
   // when they are not done in response to a users action (this is always
   // called during the idle time, long time after execution of any function in
   // which I can put a MFSuspendInteractivity object...) and it is so annoying
   // to have the busy dialog popup while you're doing something else that I
   // decided to only show it the first time the folder is opened
   //
   // this also makes sense because the server side sorting/threading seems to
   // be much faster the subsequent times (the server probably keeps some data
   // alive) but the first time it's really slow
   BusyIndicator *busy = NULL;

   // no tables, check if we need them

   // sort the messages if needed and not done yet: we'll need the sort table
   // anyhow, whether we thread messages or not
   if ( IsSorting() && !m_tableSort )
   {
      wxString msg = _("Sorting %lu messages...");

      if ( busy )
         busy->SetLabel(msg, m_sizeTables);
      else
         busy = new BusyIndicator(!m_firstSort, m_mf, msg, m_sizeTables);

      if ( !Sort() )
         busy->Fail(_("Sorting failed!"));
   }

   if ( IsThreading() )
   {
      // first thread the messages if not done yet
      if ( !m_thrData || !m_thrData->m_root)
      {
         wxString msg = _("Threading %lu messages...");

         if ( busy )
            busy->SetLabel(msg, m_sizeTables);
         else
            busy = new BusyIndicator(!m_firstSort, m_mf, msg, m_sizeTables);

         if ( !Thread() )
            busy->Fail(_("Threading failed!"));
      }

      // create m_tableMsgno from m_tableSort and m_tableThread
      CombineSortAndThread();

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
      BuildPosTable();
   }

   delete busy;

   // first sorting/threading done
   m_firstSort = false;

   CHECK_TABLES();
}

MsgnoType *HeaderInfoListImpl::BuildInverseTable(MsgnoType *table) const
{
   MsgnoType *tableInverse = AllocTable();

   // build the inverse table from the direct one
   for ( MsgnoType n = 0; n < m_sizeTables; n++ )
   {
      tableInverse[table[n] - 1] = n;
   }

   return tableInverse;
}

void HeaderInfoListImpl::BuildPosTable()
{
   CHECK_RET( m_tableMsgno, _T("can't build inverse table without direct one") );

   ASSERT_MSG( !m_tablePos, _T("rebuilding inverse table (and leaking memory)?") );

   m_tablePos = BuildInverseTable(m_tableMsgno);
}

void HeaderInfoListImpl::FreeTables()
{
   if ( m_tableMsgno )
   {
      // don't free it if it is the same as either m_tableThread or m_tableSort
      if ( m_dontFreeMsgnos )
         m_dontFreeMsgnos = false;
      else
         delete [] m_tableMsgno;
      m_tableMsgno = NULL;
   }

   if ( m_tablePos )
   {
      delete [] m_tablePos;
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
      delete [] m_tableSort;
      m_tableSort = NULL;
   }
}

// sort the messages according to the current sort parameters
bool HeaderInfoListImpl::Sort()
{
   // caller must check if we need to be sorted
   ASSERT_MSG( !m_tableMsgno && !m_tableSort,
               _T("shouldn't be called again") );

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

         // init it just to avoid warnings from stupid optimizing compilers
         long sortOrderOld = 0;
         if ( m_reverseOrder )
         {
            // we do the reversal ourselves
            sortOrderOld = m_sortParams.sortOrder;
            m_sortParams.sortOrder &= ~1;
         }

         if ( m_mf->SortMessages(m_tableSort, m_sortParams) )
         {
            DUMP_TABLE(m_tableSort,
                       ("after sorting with sort order %08lx",
                        m_sortParams.sortOrder));
         }
         else // sort failed
         {
            FreeSortData();

            return false;
         }

         if ( m_reverseOrder )
         {
            // restore the real sort order so that comparing against it in
            // SetSortOrder() has expected result when it is called later
            m_sortParams.sortOrder = sortOrderOld;
         }

         m_reversedTables = IsSortCritReversed(m_sortParams.sortOrder);
   }

   return true;
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
      if ( IsThreading() )
      {
         // don't use !m_reverseOrder in the RHS because it is always set to false
         // if we're threading messages
         m_reverseOrder = (IsSortCritReversed(m_sortParams.sortOrder) != m_reversedTables);
      } else {
         m_reverseOrder = ! m_reverseOrder;
      }
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

      // show the busy info dialog the next time because it will be done in
      // response to the users change (only the user can change the sort order)
      m_firstSort = true;
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

bool HeaderInfoListImpl::Thread()
{
   // the caller must check that we need to be threaded
   ASSERT_MSG( (!m_thrData || !m_thrData->m_root) && IsThreading(), _T("shouldn't be called") );

   delete m_thrData;
   m_thrData = new ThreadData(m_sizeTables);

   if ( !m_mf->ThreadMessages(m_thrParams, m_thrData) )
   {
      FreeThreadData();

      return false;
   }

   return true;
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

   // show the busy info dialog the next time we thread messages because it
   // will be a result of a users action
   m_firstSort = true;

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

   CHECK_RET( idxMax < m_count, _T("HeaderInfoListImpl::Cache(): invalid range") );

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
      StatusIndicator status(m_mf->GetInteractiveFrame(),
                             _("Retrieving %u headers..."), countNotInCache);

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
         ASSERT_MSG( pos < m_count, _T("invalid position in the sequence") );

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
          _T("HeaderInfoListImpl::IsInCache(): invalid position") );

   return (idx < m_headers.GetCount()) && (m_headers[idx] != NULL);
}

bool HeaderInfoListImpl::ReallyGet(MsgnoType pos)
{
   // we must be already sorted/threaded by now
   CHECK( !IsTranslatingIndices() || HasTransTable(), false,
          _T("can't be called now") );

   MsgnoType idx = GetIdxFromPos(pos);

   CHECK( idx < m_count, false,
          _T("HeaderInfoListImpl::IsInCache(): invalid position") );

   ExpandToMakeIndexValid(idx);

   if ( !m_headers[idx] )
   {
      // the idea is that this method is only called if the header had been
      // "retrieved" before but the transfer was cancelled by the user, so we
      // don't really have any information for it - but in this case the element
      // corresponding to it must have been already created, so assert
      FAIL_MSG( _T("not supposed to be called") );

      // but still don't crash
      m_headers[idx] = new HeaderInfo;
   }

   if ( m_headers[idx]->IsValid() )
   {
      FAIL_MSG( _T("why call ReallyGet() then?") );

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
   s2.Printf("%u entries", (unsigned int)Count());

   return s1 + s2;
}

#endif // DEBUG
