///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/HeaderInfo.h: HeaderInfo and HeaderInfoList classes
// Purpose:     HeaderInfo contains all envelope information for one message,
//              HeaderInfoList knows how to retrieve HeaderInfos for the given
//              folder
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef HEADERINFO_H
#define HEADERINFO_H

#include <wx/fontenc.h>         // for wxFontEncoding

#include "MailFolder.h"    // for MailFolder::MessageStatus

class WXDLLEXPORT wxArrayString;
class Sequence;

struct SortParams;
struct ThreadParams;

/// illegal value of the message index (it nicely corresponds to 0 msgno)
#define INDEX_ILLEGAL UID_ILLEGAL

/**
   This class contains all the information from message header

   It doesn't contain any GUI-specific info (like the message colour, score or
   indentation level), this is now handled internally in FolderView.

   Also, now, this is a POD structure unlike an ABC it used to be.
*/
class HeaderInfo
{
public:
   HeaderInfo();

   /** @name Access header information

       Note that all header values are returned decoded, i.e. with all
       encoded words removed and without encoding info.
    */
   //@{
   /// return the message subject
   const String& GetSubject(void) const { return m_Subject; }

   /// return the message From header
   const String& GetFrom(void) const { return m_From; }

   /// return the message To header
   const String& GetTo(void) const { return m_To; }

   /// return the message Newsgroups header
   const String& GetNewsgroups(void) const { return m_NewsGroups; }

   /// return the time from the message header
   time_t GetDate(void) const { return m_Date; }

   /// return Message-Id
   const String& GetId(void) const { return m_Id; }

   /// return the unique identifier
   UIdType GetUId(void) const { return m_UId; }

   /// return References header (if any)
   const String& GetReferences(void) const { return m_References; }

   /// return In-Reply-To header (if any)
   const String& GetInReplyTo(void) const { return m_InReplyTo; }

   /// return msg status (combination of MailFolder::MSG_STAT_XXX flags)
   int GetStatus(void) const { return m_Status; }

   /// return message size in bytes
   unsigned long GetSize(void) const { return m_Size; }

   /// return (text) message size in lines or 0 if not text or unknown
   unsigned long GetLines(void) const { return m_Lines; }
   //@}

   /** @name Font encoding

       FIXME we suppose that all headers have the same encoding but it is
             perfectly valid to have "To" in one encoding and "From" in another
             and even have multiple encodings inside one header.

             probably the only way to handle it in a sane way is to use Unicode
             though and then we won't need m_Encoding at all...
    */
   //@{
   /// set encoding to use for display
   void SetEncoding(wxFontEncoding enc) { m_Encoding = enc; }
   /// get encoding to use for display
   wxFontEncoding GetEncoding() const { return m_Encoding; }
   /// Return true if we have non default encoding
   bool HasEncoding() const { return GetEncoding() != wxFONTENCODING_SYSTEM; }
   //@}

   /// is this header valid (may be false while we're retrieving it)
   bool IsValid() const { return m_UId != UID_ILLEGAL; }

   /// the kind of header returned by GetFromOrTo()
   enum HeaderKind
   {
      Invalid,
      From,
      To,
      Newsgroup
   };

   /**
     Return either the sender of the message or recipient of it if
     replaceFromWithTo is true and the sender address occurs in the array
     ownAddresses

     @param hi the header info to retrieve the from/to headers from
     @param replaceFromWithTo if false, always return from
     @param ownAddresses the addresses for which replacement is done
     @param value the string to return result in
     @return the kind of header returned
   */
   static HeaderKind GetFromOrTo(const HeaderInfo *hi,
                                 bool replaceFromWithTo,
                                 const wxArrayString& ownAddresses,
                                 String *value);

private:
   /// header values
   String m_Subject,
          m_From,
          m_To,
          m_NewsGroups,
          m_References,
          m_InReplyTo,
          m_Id;

   /// MailFolder::Flags combination
   int m_Status;

   /// size in bytes
   unsigned long m_Size;

   /// size in lines for text messages, 0 otherwise
   unsigned long m_Lines;

   /// as returned by cclient
   unsigned long m_UId;

   /// from headers
   time_t m_Date;

   /// the "main" encoding of the headers
   wxFontEncoding m_Encoding;

   // these classes can create/manipulate the HeaderInfo objects directly
   friend class MailFolderCC;
   friend class MailFolderVirt;
};

/**
   This class provides access to HeaderInfo objects for the given folder

   The messages are stored internally in the same order as they appear in the
   folder (i.e. by msgno) folder but their displayed order may be different
   because of sorting and/or threading.

   So GetItem(n) (and operators[]) return the header info corresponding to the
   message which should appear in the n-th position when displayed, while all
   other functions taking index value (Remove()) interpret the index as
   internal index which is == msgno - 1 (msgnos start with 1, indices - with 0).

   Finally, a message also has an UID i.e. a unique identifier which doesn't
   change even as messages are added/deleted to/from the folder. We provide a
   function to find a message by UID (the reverse is simple as HeaderInfo has
   GetUId() method) but it for now it is implemented as linear search in all
   messages, i.e. it is very slow and should never be called.
*/
class HeaderInfoList : public MObjectRC
{
public:
   /// creator function
   static HeaderInfoList *Create(MailFolder *mf);

   /** @name Elements access */
   //@{
   /// Count the number of messages in listing.
   virtual MsgnoType Count(void) const = 0;

   /// Returns the entry for the given internal index
   virtual HeaderInfo *GetItemByIndex(MsgnoType n) const = 0;

   /// Returns the entry for the n-th msgno (msgnos are counted from 1)
   HeaderInfo *GetItemByMsgno(MsgnoType msgno) const
      { return GetItemByIndex(GetIdxFromMsgno(msgno)); }

   /// Returns the entry which should be shown in the given position.
   HeaderInfo *GetItem(MsgnoType pos) const
      { return GetItemByIndex(GetIdxFromPos(pos)); }

   /// Operator version of GetItem()
   HeaderInfo *operator[](MsgnoType pos) const { return GetItem(pos); }
   //@}

   /** @name UID/msgno to index mapping */
   //@{

   /** Returns the (internal) index for this UID or INDEX_ILLEGAL */
   virtual MsgnoType GetIdxFromUId(UIdType uid) const = 0;

   /// Returns pointer to entry with this UId
   virtual HeaderInfo *GetEntryUId(UIdType uid) const
   {
      return GetItemByIndex(GetIdxFromUId(uid));
   }

   /** Returns the (internal) index for the given msgno. */
   static MsgnoType GetIdxFromMsgno(MsgnoType msgno) { return msgno - 1; }

   /** Returns the msgno for the given index */
   static MsgnoType GetMsgnoFromIdx(MsgnoType idx) { return idx + 1; }

   //@}

   /** @name Position to/from index mapping */
   //@{

   /** Returns the (internal) index for the given display position. */
   virtual MsgnoType GetIdxFromPos(MsgnoType pos) const = 0;

   /** Returns the position at which this item should be displayed. */
   virtual MsgnoType GetPosFromIdx(MsgnoType n) const = 0;

   /**
       Returns the old position at which the item should be displayed.

       The existence of this method alongside GetPosFromIdx() merits some
       explanations. The problem is that we may need to call GetPosFromIdx()
       from inside cclient callback. The trouble is that at that moment we can
       not call any other cclient functions again because this would result in
       fatal reentrancy and so we can't resort/rethread the folder right then
       and this could be needed if we had got a new mail notification before
       which resulted in invalidating the old sort/thread information. We can't
       wait until later neither because the item we're interested in won't be
       there any more.

       So our solution is to keep the old sort/thread info around when we
       invalidate it and this function provides access to it. Unlike
       GetPosFromIdx() it is always safe to call.
    */
   virtual MsgnoType GetOldPosFromIdx(MsgnoType n) const = 0;

   //@}

   /** @name Appearance parameters */
   //@{

   /// get the indentation level of this message in thread (0 for root)
   virtual size_t GetIndentation(MsgnoType pos) const = 0;

   // TODO: possible score and colour settings for individual messages should
   //       be kept here as well

   //@}

   /** @name Functions called by MailFolder */
   //@{
   /// Called when the given (by index) message is expunged
   virtual void OnRemove(MsgnoType n) = 0;

   /// Called when the number of messages in the folder increases
   virtual void OnAdd(MsgnoType countNew) = 0;

   /// Called when the underlying folder is closed
   virtual void OnClose() = 0;
   //@}

   /** @name Header searching

       Instead of iterating over all headers (thus forcing us to retrieve all
       of them from server) the functions in this section should be used to
       find the messages satisfying the given criteria.

       Note that indexFrom parameter below is exclusiv, i.e. the search is
       started from the next item.
    */
   //@{
   /**
       Get the index of the first header with[out] the given flag after the
       given position.

       @return the position or INDEX_ILLEGAL if none found
    */
   virtual MsgnoType FindHeaderByFlag(MailFolder::MessageStatus flag,
                                      bool set = true,
                                      long posFrom = -1) = 0;

   /**
       Same as GetHeaderByFlag() but wraps to the start if the header is not
       found between indexFrom and the end of the listing
    */
   virtual MsgnoType FindHeaderByFlagWrap(MailFolder::MessageStatus flag,
                                          bool set = true,
                                          long posFrom = -1) = 0;

   /**
       Return the array containing all headers with[out] the given flag.
       The array must be deleted by the caller (but may be NULL)
    */
   virtual MsgnoArray *GetAllHeadersByFlag(MailFolder::MessageStatus flag,
                                           bool set = true) = 0;

   //@}

   /** @name Sorting/threading
    */
   //@{

   /**
      Set the sort order to use for the messages.

      @param sortParams specifies the sorting to do
      @return true if the order of messages really changed
    */
   virtual bool SetSortOrder(const SortParams& sortParams) = 0;

   /**
      Set the threading parameters (thread or not and, if yes, various options
      governing the threading)

      @param thrParams specifies if we thread or not and, if yes, then how
      @return true if the order of messages really changed
    */
   virtual bool SetThreadParameters(const ThreadParams& thrParams) = 0;

   //@}

   /** @name Cache control
    */
   //@{

   /// opaque typedef used by GetLastMod() and HasChanged()
   typedef unsigned long LastMod;

   /**
      The outside code may want to store pointers returned by GetItemByIndex()
      and other methods. But these pointers are invalidated each time the
      messages are added/removed to/from the listing so they may need to be
      updated. Using GetLastMod() and HasChanged() allows to check for this.

      The value returned by this function is opaque and shouldn't be used for
      anything but passing it to HasChanged() later. The only other operation
      with it is comparing it with 0 which will always be false.

      @return the opaque value which can be later passed to HasChanged()
    */
   virtual LastMod GetLastMod() const = 0;

   /**
      Return true if the pointers were invalidated since the moment recorded
      in since.

      @param since an opaque value previously returned by GetLastMod()
      @return true if the listing changed
    */
   virtual bool HasChanged(const LastMod since) const = 0;

   /**
      Cache all items at positions in this sequence. IsInCache() must return
      true for all of these items when Cache() returns.

      This is called from GUI code before retrieving all/many of items of this
      sequence.
    */
   virtual void CachePositions(const Sequence& seq) = 0;

   /**
      Cache all items between these msgnos.
    */
   virtual void CacheMsgnos(MsgnoType msgnoFrom, MsgnoType msgnoTo) = 0;

   /**
      Is the item which should be shown at this position already cached?

      If it is, calling GetItem() for it is fast, otherwise it may involve a
      trip to server and so should be done asynchronously.
    */
   virtual bool IsInCache(MsgnoType pos) const = 0;

   /**
      Do retrieve this header from server, we need it right now.
    */
   virtual bool ReallyGet(MsgnoType pos) = 0;

   //@}

   MOBJECT_NAME(HeaderInfoList)
};

// declare an auto ptr class for HeaderInfoList which adds an operator[]
#ifndef SWIG
BEGIN_DECLARE_AUTOPTR(HeaderInfoList)
public:
   const HeaderInfo *operator[](MsgnoType n) const { return (*m_ptr)[n]; }
   HeaderInfo *operator[](MsgnoType n) { return (*m_ptr)[n]; }
END_DECLARE_AUTOPTR();
#endif // SWIG

#endif // HEADERINFO_H

