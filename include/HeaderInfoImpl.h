///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/HeaderInfoCC.h: HeaderInfoListImpl class
// Purpose:     provide implementation of the ABC HeaderInfoList
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////


#ifndef HEADERINFOIMPL_H
#define HEADERINFOIMPL_H

#ifdef __GNUG__
#   pragma interface "HeaderInfoImpl.h"
#endif

#ifndef  USE_PCH
   #include "Sorting.h"
   #include "Threading.h"
#endif // USE_PCH

#include "HeaderInfo.h"

#include <wx/dynarray.h>

WX_DEFINE_ARRAY(HeaderInfo *, ArrayHeaderInfo);

/**
  This is a very simple HeaderInfoList implementation. It preallocates an
  array big enough to store the info for all the messages and doesn't support
  sorting nor threading.

  It should be considered just as a prototype. The final version should
  support folders with up to 100000 messages efficiently (probably not
  sorting/threading them though?).

  TODO:
   1. combine sorting and threading: we need to thread messages first and then
      to sort the siblings _separately_ (i.e. first sort the roots, then the
      children of the first root, &c) - but for this we need to store the
      parent-children relationships and not only the order of the threaded
      messages...

   2. although there is nothing wrong with preallocating all the memory we need
      (even for 100000 message we take just 400Kb), it would still be nice to
      have some smart way of storing HeaderInfo objects as using array is less
      than ideal because adding/removing messages is a common operation.

   3. batch processing of OnRemove() calls: instead of removing the item
      immediately, remember the range of items to be removed. If the new
      item to remove is not contiguous to the existing range, DoRemove()
      immediately and start new range. Also call DoRemove() in the beginning
      of each call using the actual listing.

      this should speed up things dramatically as messages are often removed in
      bunches...
 */
class HeaderInfoListImpl : public HeaderInfoList
{
public:
   virtual MsgnoType Count(void) const;

   virtual HeaderInfo *GetItemByIndex(MsgnoType n) const;
   virtual MsgnoType GetIdxFromUId(UIdType uid) const;

   virtual MsgnoType GetIdxFromPos(MsgnoType pos) const;
   virtual MsgnoType GetPosFromIdx(MsgnoType n) const;

   virtual void OnRemove(MsgnoType n);
   virtual void OnAdd(MsgnoType countNew);
   virtual void OnClose();

   virtual size_t GetIndentation(MsgnoType n) const;

   virtual MsgnoType FindHeaderByFlag(MailFolder::MessageStatus flag,
                                      bool set, long posFrom);
   virtual MsgnoType FindHeaderByFlagWrap(MailFolder::MessageStatus  flag,
                                          bool set, long posFrom);
   virtual MsgnoArray *GetAllHeadersByFlag(MailFolder::MessageStatus flag,
                                           bool set);

   virtual bool SetSortOrder(const SortParams& sortParams);
   virtual bool SetThreadParameters(const ThreadParams& thrParams);

   virtual LastMod GetLastMod() const;
   virtual bool HasChanged(const LastMod since) const;

   virtual void CachePositions(const Sequence& seq);
   virtual void CacheMsgnos(MsgnoType msgnoFrom, MsgnoType msgnoTo);
   virtual bool IsInCache(MsgnoType pos) const;
   virtual bool ReallyGet(MsgnoType pos);

   virtual ~HeaderInfoListImpl();

private:
   /// private ctor for use of HeaderInfoList::Create()
   HeaderInfoListImpl(MailFolder *mf);

   /// no copy ctor
   HeaderInfoListImpl(const HeaderInfoListImpl&);

   /// no assignment operator
   HeaderInfoListImpl& operator=(const HeaderInfoListImpl&);

   /// perform the clean up (called from dtor)
   void CleanUp();

   /// allocate a table of m_count MsgnoTypes
   MsgnoType *AllocTable() const;

   /// build m_tableMsgno/m_tablePos from sort/thread data if not done yet
   void BuildTables();

   /// combine sorting and threading data into m_tableMsgno
   void CombineSortAndThread();

   /// CombineSortAndThread() helper: get N-th msgno from m_tableSort
   MsgnoType GetNthSortedMsgno(MsgnoType n) const;

   /// free m_tableMsgno/m_tablePos tables
   void FreeTables();

   /// free the sort table
   void FreeSortData();

   /// free the thread data (thread table and indent data)
   void FreeThreadData();

   /// is the given entry valid (i.e. already cached)?
   inline bool IsHeaderValid(MsgnoType n) const;

   /// do we sort messages at all?
   inline bool IsSorting() const;

   /// do we [need to] thread messages?
   inline bool IsThreading() const;

   /// do we have the translation table mapping indices to positions?
   inline bool HasTransTable() const;

   /// do we do any kind of index mapping (trans tables or reverse order)?
   inline bool IsTranslatingIndices() const;

   /// do we need to rebuild the translation table?
   inline bool MustRebuildTables() const;

   /// get the msgno which should appear at the given display position
   MsgnoType GetMsgnoFromPos(MsgnoType pos) const;

   /// expand m_headers array so that the given index is valid
   void ExpandToMakeIndexValid(MsgnoType n);

   /// cache the sequence of msgnos
   void Cache(const Sequence& seqmMsgnos);

   /// sort messages, i.e. set m_tableSort
   void Sort();

   /// thread the messages according to m_thrParams
   void Thread();

   /**
      Find first position in the given range containing a msgno from array
      @return position or UID_ILLEGAL
   */
   MsgnoType FindFirstInRange(const MsgnoArray& array,
                              MsgnoType posFrom, MsgnoType posTo) const;

   /// the folder we contain the listinf for
   MailFolder *m_mf;

   /// the array of headers
   ArrayHeaderInfo m_headers;

   /// the number of messages in the folder
   MsgnoType m_count;

   /**
     @name Sorting/threading data

     The translation tables allow to get the msgno shown at given position
     and the position at which given msgno (index) is shown. They are
     calculated when needed from m_tableSort and m_tableThread and may be NULL
     even if we do sort/thread messages if we hadn't needed them before, so they
     should never be accessed directly.

     The translation tables are synhronized if not NULL.

     Finally, we also keep m_reverseOrder flag which allows us to flip the
     order of messages in the folder quickly. Note that it may be set even if
     m_tableSort == m_tablePos == NULL!
    */
   //@{

   /// the translation table containing the msgnos in sorted order
   MsgnoType *m_tableSort;

   /// should we reverse the order of messages in the folder?
   bool m_reverseOrder;

   /// the threading data, NULL if not threading
   ThreadData *m_thrData;

   /// position -> msgno mapping
   MsgnoType *m_tableMsgno;

   /// index -> position mapping
   MsgnoType *m_tablePos;

   /// sorting parameters
   SortParams m_sortParams;

   /// threading parameters
   ThreadParams m_thrParams;

   /// if true, don't free the msgno table as it's the same as thread/sort
   bool m_dontFreeMsgnos;

   //@}

   /// last modification "date": incremented each time the listing changes
   LastMod m_lastMod;

   // let it create us
   friend HeaderInfoList *HeaderInfoList::Create(MailFolder *mf);

   MOBJECT_DEBUG(HeaderInfoListImpl)
};

#endif // HEADERINFOIMPL_H
