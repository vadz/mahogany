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
   1. sorting/threading support, obviously: this involves using
      MailFolder::CanSort/Thread() and server-side threading if it can and
      falling back to the existing sorting/threading code otherwise. We also
      should handle the "smart sorting" option, i.e. sort the messages only if
      there are reasonably few of them (as we have to retrieve all of them to
      sort!) and leave them unsorted otherwise.

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
   virtual size_t Count(void) const;

   virtual HeaderInfo *GetItemByIndex(size_t n) const;
   virtual size_t GetIdxFromUId(UIdType uid) const;

   virtual size_t GetIdxFromPos(size_t pos) const;
   virtual size_t GetPosFromIdx(size_t n) const;

   virtual void OnRemove(size_t n);
   virtual void OnAdd(size_t countNew);
   virtual void OnClose();

   virtual size_t GetIndentation(size_t n) const;

   virtual size_t FindHeaderByFlag(MailFolder::MessageStatus flag,
                                   bool set, long posFrom);
   virtual size_t FindHeaderByFlagWrap(MailFolder::MessageStatus  flag,
                                       bool set, long posFrom);
   virtual MsgnoArray *GetAllHeadersByFlag(MailFolder::MessageStatus flag,
                                           bool set);

   virtual LastMod GetLastMod() const;
   virtual bool HasChanged(const LastMod since) const;

   virtual void CachePositions(const Sequence& seq);
   virtual void CacheMsgnos(MsgnoType msgnoFrom, MsgnoType msgnoTo);
   virtual bool IsInCache(size_t pos) const;

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

   /// is the given entry valid (i.e. already cached)?
   inline bool IsHeaderValid(size_t n) const;

   /// do we sort messages at all?
   bool IsSorting() const { return m_tableMsgno != NULL; }

   /// expand m_headers array so that the given index is valid
   void ExpandToMakeIndexValid(size_t n);

   /// cache the sequence of msgnos
   void Cache(const Sequence& seqmMsgnos);

   /**
      Find first position in the given range containing a msgno from array
      @return position or UID_ILLEGAL
   */
   size_t FindFirstInRange(const MsgnoArray& array,
                           size_t posFrom, size_t posTo) const;

   /// the folder we contain the listinf for
   MailFolder *m_mf;

   /// the array of headers
   ArrayHeaderInfo m_headers;

   /// the number of messages in the folder
   size_t m_count;

   /**
     @name Sorting/threading data

     We maintain 2 tables which define the correspondence between the indices
     (which are just msgnos, i.e. the internal message indices in the folder)
     and the position on the screen where they should appear.

     The tables below are synhronized if not NULL (they are both NULL if no
     sorting/threading is done)
    */
   //@{

   /// the translation table allowing to get msgno (index) from position
   size_t *m_tableMsgno;

   /// the translation table allowing to get position from msgno (index)
   size_t *m_tablePos;

   //@}

   /// last modification "date": incremented each time the listing changes
   LastMod m_lastMod;

   // let it create us
   friend HeaderInfoList *HeaderInfoList::Create(MailFolder *mf);

   MOBJECT_DEBUG(HeaderInfoListImpl)
};

#endif // HEADERINFOIMPL_H
