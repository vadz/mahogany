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

#ifdef __GNUG__
#   pragma interface "HeaderInfo.h"
#endif

#include <wx/fontenc.h>

#include "FolderType.h"    // for MsgnoType

class wxArrayString;

/**
   This class contains all the information from message header

   It doesn't contain any GUI-specific info (like the message colour, score or
   indentation level), this is now handled internally in FolderView.

   Also, now, this is a POD structure unlike an ABC it used to be.
*/
class HeaderInfo
{
public:
   /** @name Access header information */
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

   /// the encoding of the headers
   wxFontEncoding m_Encoding;

   // it is the only one which can create these objects for now, later we
   // should find some better way to allow other classes do it as well
   friend class MailFolderCC;
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
   virtual size_t Count(void) const = 0;

   /// Returns the entry for the given internal index
   virtual HeaderInfo *GetItemByIndex(size_t n) const = 0;

   /// Returns the entry for the n-th msgno (msgnos are counted from 1)
   HeaderInfo *GetItemByMsgno(MsgnoType msgno) const
      { return GetItemByIndex(GetIdxFromMsgno(msgno)); }

   /// Returns the entry which should be shown in the given position.
   HeaderInfo *GetItem(size_t pos) const
      { return GetItemByIndex(GetIdxFromPos(pos)); }

   /// Operator version of GetItem()
   HeaderInfo *operator[](size_t pos) const { return GetItem(pos); }
   //@}

   /** @name UID/msgno to index mapping */
   //@{
   /** Returns the (internal) index for this UId or UID_ILLEGAL */
   virtual UIdType GetIdxFromUId(UIdType uid) const = 0;

   /// Returns pointer to entry with this UId
   virtual HeaderInfo *GetEntryUId(UIdType uid) const
   {
      return GetItemByIndex(GetIdxFromUId(uid));
   }

   /** Returns the (internal) index for the given msgno. */
   size_t GetIdxFromMsgno(MsgnoType msgno) const
   {
      ASSERT_MSG( msgno != MSGNO_ILLEGAL, "invalid msgno" );

      return msgno - 1;
   }
   //@}

   /** @name Position to/from index mapping */
   //@{
   /** Returns the (internal) index for the given display position. */
   virtual size_t GetIdxFromPos(size_t pos) const = 0;

   /** Returns the position at which this item should be displayed. */
   virtual size_t GetPosFromIdx(size_t n) const = 0;
   //@}

   /** @name Functions called by MailFolder */
   //@{
   /// Called when the given (by index) message is expunged
   virtual void OnRemove(size_t n) = 0;

   /// Called when the number of messages in the folder increases
   virtual void OnAdd(size_t countNew) = 0;
   //@}

   MOBJECT_NAME(HeaderInfoList)
};

// declare an auto ptr class for HeaderInfoList which adds an operator[]
BEGIN_DECLARE_AUTOPTR(HeaderInfoList)
public:
   const HeaderInfo *operator[](size_t n) const { return (*m_ptr)[n]; }
   HeaderInfo *operator[](size_t n) { return (*m_ptr)[n]; }
END_DECLARE_AUTOPTR();

#endif // HEADERINFO_H

