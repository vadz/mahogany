/*-*- c++ -*-********************************************************
 * HeaderInfo : header info listing for mailfolders
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

/**
   @package Mailfolder access
   @version $Revision$
   @author  Karsten Ballüder
*/

#ifndef HEADERINFO_H
#define HEADERINFO_H

#ifdef __GNUG__
#   pragma interface "HeaderInfo.h"
#endif

/// type for user data
typedef unsigned long FolderDataType;

/**
   This class contains all the information about the message such as its
   (important) headers and its appearance parameters (colour, indent, font
   encoding to use)
*/
class HeaderInfo
{
public:
   // we have to have the default ctor even though it doesn't do anything
   // because of copy ctor declaration below
   HeaderInfo() { }

   // accessors
   virtual const String &GetSubject(void) const = 0;
   virtual const String &GetFrom(void) const = 0;
   virtual const String &GetTo(void) const = 0;
   virtual const String &GetNewsgroups(void) const = 0;
   virtual time_t GetDate(void) const = 0;
   virtual const String &GetId(void) const = 0;
   virtual const String &GetReferences(void) const = 0;
   virtual UIdType GetUId(void) const = 0;
   virtual int GetStatus(void) const = 0;
   virtual unsigned long GetSize(void) const = 0;
   virtual unsigned long GetLines(void) const = 0;

   /// Return the indentation level for message threading.
   virtual unsigned GetIndentation() const = 0;
   /// Set the indentation level for message threading.
   virtual void SetIndentation(unsigned level) = 0;
   /// Get Colour setting (name or empty string)
   virtual String GetColour(void) const = 0;
   /// Get Score setting (default = 0)
   virtual int GetScore(void) const = 0;
   /// Change Score setting (default = 0)
   virtual void AddScore(int delta) = 0;
   /// Set Colour setting (name or empty string)
   virtual void SetColour(const String &col) = 0;

   /// Set the encoding used (wxFONTENCODING_SYSTEM for default)
   virtual void SetEncoding(wxFontEncoding encoding) = 0;
   /// Get the encoding used (wxFONTENCODING_SYSTEM for default)
   virtual wxFontEncoding GetEncoding() const = 0;
   /// Return true if we have non default encoding
   bool HasEncoding() const { return GetEncoding() != wxFONTENCODING_SYSTEM; }
   /// Return some extra data which is folder driver specific
   virtual FolderDataType GetFolderData(void) const = 0;

   /// Create and return the copy of this object (allocated with new)
   virtual HeaderInfo *Clone() const = 0;

   virtual ~HeaderInfo() {}

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
   /// Disallow copy construction, use Clone() instead
   HeaderInfo(const HeaderInfo &);

   GCC_DTOR_WARN_OFF
};

/**
   This class holds a complete list of all messages in the folder.

   The messages are stored internally in the order they were retrieved by the
   folder but their displayed order may be different because of sorting and/or
   threading. So GetItem(n) (and operators[]) return the header info
   corresponding to the message which should appear in the n-th position when
   displayed, while all other functions taking index value (Remove())
   interpret the index as internal index. There is also GetItemByMsgno() which
   returns the item from its msgno in the cclient sense. They correspond to the
   internal index in the following sense:

   1. msgnos are consecutive

   2. but they decrease while index increases (this is because we build the
      listing in reversed order to allow the user to retrieve only some headers
      from server and abort without having to retrieve the older ones)

   3. the msgno of the first message would usually be just Count(), but it may
      be greater if the listing building was aborted

   To allow sorting of messages, we provide direct access to the translation
   table used to map internal indices to the displayed ones:
*/
class HeaderInfoList : public MObjectRC
{
public:
   /** @name Elements access */
   //@{
   /// Count the number of messages in listing.
   virtual size_t Count(void) const = 0;

   /// Returns the entry for the given internal index
   virtual HeaderInfo *GetItemByIndex(size_t n) const = 0;

   /// Returns the entry for the n-th msgno (msgnos are counted from 1)
   HeaderInfo *GetItemByMsgno(size_t msgno) const
      { return GetItemByIndex(GetIdxFromMsgno(msgno)); }

   /// Returns the entry which should be shown in the given position.
   HeaderInfo *GetItem(size_t pos) const
      { return GetItemByIndex(GetIdxFromPos(pos)); }
   /// Returns the n-th entry.
   HeaderInfo *operator[](size_t pos) const { return GetItem(pos); }
   //@}

   /// Returns pointer to entry with this UId
   virtual HeaderInfo *GetEntryUId(UIdType uid) = 0;

   /** Returns the (internal) index for this UId or UID_ILLEGAL */
   virtual UIdType GetIdxFromUId(UIdType uid) const= 0;

   /** Returns the (internal) index for the given msgno. */
   virtual size_t GetIdxFromMsgno(size_t msgno) const = 0;

   /** Returns the (internal) index for the given display position. */
   virtual size_t GetIdxFromPos(size_t pos) const = 0;

   /** Returns the position at which this item should be displayed. */
   virtual size_t GetPosFromIdx(size_t n) const = 0;

   /** @name Translation table

       As explained above, we use a translation table to get the real message
       index of the message which is displayed in the given position. You can
       set a translation table, replacing any existing one, or add a table
       combining it with the existing one.
   */
   //@{
   /**
       Sets a translation table re-mapping index values or removes the table if
       the pointer is NULL (otherwise it will be delete[]d in destructor).

       @param array an array of indices allocated with new[] or NULL
   */
   virtual void SetTranslationTable(size_t *array) = 0;

   /**
       Adds a translation table. The pointer will not be deallocated by this
       class and can be deleted by caller after this function returns.

       @param array an array of indices
   */
   virtual void AddTranslationTable(const size_t *array) = 0;
   //@}

   /// For use by folder only: corrects size downwards:
   virtual void SetCount(size_t newcount) = 0;

   /// Removes the given element from the listing
   virtual void Remove(size_t n) = 0;

   MOBJECT_NAME(HeaderInfoList)
};

// declare an auto ptr class for HeaderInfoList which adds an operator[]
BEGIN_DECLARE_AUTOPTR(HeaderInfoList)
public:
   const HeaderInfo *operator[](size_t n) const { return (*m_ptr)[n]; }
   HeaderInfo *operator[](size_t n) { return (*m_ptr)[n]; }
END_DECLARE_AUTOPTR();

#endif
