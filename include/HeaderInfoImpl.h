/*-*- c++ -*-********************************************************
 * HeaderInfoImpl : header info listing for mailfolders
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef HEADERINFOIMPL_H
#define HEADERINFOIMPL_H

#ifdef __GNUG__
#   pragma interface "HeaderInfoImpl.h"
#endif

#include "HeaderInfo.h"

/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages.
    The m_uid member is also used to map any access to the n-th message in
    the folder to the correct message. I.e. when requesting
    GetMessage(5), it will use the message with the m_uid from the 6th
    entry in the list.
*/
class HeaderInfoImpl : public HeaderInfo
{
public:
   HeaderInfoImpl();

   // implement base class pure virtuals
   virtual String const &GetSubject(void) const { return m_Subject; }
   virtual String const &GetFrom(void) const { return m_From; }
   virtual String const &GetTo(void) const { return m_To; }
   virtual time_t GetDate(void) const { return m_Date; }
   virtual String const & GetId(void) const { return m_Id; }
   virtual unsigned long GetUId(void) const { return m_UId; }
   virtual String const &GetReferences(void) const { return m_References; }
   virtual int GetStatus(void) const { return m_Status; }
   virtual unsigned long const &GetSize(void) const { return m_Size; }
   virtual size_t SizeOf(void) const { return sizeof(HeaderInfoImpl); }

   /// Return the indentation level for message threading.
   virtual unsigned GetIndentation() const { return m_Indentation; }
   /// Set the indentation level for message threading.
   virtual void SetIndentation(unsigned level) { m_Indentation = level; }

   /// Get Colour setting (name or empty string)
   virtual String GetColour(void) const { return m_Colour; }
   /// Set Colour setting (name or empty string)
   virtual void SetColour(const String &col) { m_Colour = col; }

   /// Get Score setting (name or empty string)
   virtual int GetScore(void) const { return m_Score; }
   /// Change Score setting (default = 0)
   virtual void AddScore(int delta) { m_Score += delta; }

   /// set encoding to use for display
   virtual void SetEncoding(wxFontEncoding enc) { m_Encoding = enc; }
   /// get encoding to use for display
   virtual wxFontEncoding GetEncoding() const { return m_Encoding; }

   /// create a copy of this object
   virtual HeaderInfo *Clone() const;

   /// folder internal use:
   virtual FolderDataType GetFolderData(void) const { return m_FolderData; }
   virtual void SetFolderData(const FolderDataType & fd) { m_FolderData = fd; }

protected:
   /// header values
   String m_Subject,
          m_From,
          m_To,
          m_References;

   String m_Id;

   /// MailFolder::Flags combination
   int m_Status;

   /// size in bytes
   unsigned long m_Size;

   /// as returned by cclient
   unsigned long m_UId;

   /// from headers
   time_t m_Date;

   /// @name appearance paramaters
   //@{
   unsigned int m_Indentation;
   String m_Colour;
   int m_Score;
   wxFontEncoding m_Encoding;
   //@}

   FolderDataType m_FolderData;

   friend class MailFolderCC;
};

/** This class holds a complete list of all messages in the folder. */
class HeaderInfoListImpl : public HeaderInfoList
{
protected:
   // helper function
   HeaderInfo *GetItemAt(size_t n) const
   {
      CHECK( n < m_NumEntries, NULL, "invalid index in HeaderInfoList" );

      return &m_Listing[n];
   }

public:
   virtual size_t Count(void) const { return m_NumEntries; }

   virtual HeaderInfo *GetItemByIndex(size_t n) const
   {
      return GetItemAt(n);
   }

   virtual size_t GetIdxFromMsgno(size_t msgno) const
   {
      return m_msgnoMax - msgno;
   }

   virtual size_t GetIdxFromPos(size_t pos) const
   {
      return GetTranslatedIndex(pos);
   }

   virtual size_t GetPosFromIdx(size_t n) const
   {
      return GetUntranslatedIndex(n);
   }

   virtual HeaderInfo *GetEntryUId(UIdType uid);

   virtual UIdType GetIdxFromUId(UIdType uid) const;

   virtual void SetTranslationTable(size_t *array);
   virtual void AddTranslationTable(const size_t *array);

   virtual void Remove(size_t n);

   virtual void SetCount(size_t newcount);

   /// Returns an empty list of same size.
   virtual HeaderInfoListImpl *DuplicateEmpty(void) const
      { return Create(m_NumEntries); }

   /// Creates a list of given size
   static HeaderInfoListImpl * Create(size_t n)
      { return new HeaderInfoListImpl(n); }

protected:
   HeaderInfoListImpl(size_t n);
   ~HeaderInfoListImpl();

   /// get the real index after translation
   size_t GetTranslatedIndex(size_t n) const
   {
      if ( m_TranslationTable )
      {
         ASSERT_MSG( n < m_NumEntries, "invalid index" );

         n = m_TranslationTable[n];
      }

      ASSERT_MSG( n < m_NumEntries, "invalid index" );

      return n;
   }

   /// get the index which will translate into the given one
   size_t GetUntranslatedIndex(size_t n) const;

   /// The current listing of the folder
   HeaderInfoImpl *m_Listing;

   /// number of entries
   size_t m_NumEntries;

   /// translation of indices
   size_t *m_TranslationTable;

   /// the msgno of the first entry (the last msgno)
   size_t m_msgnoMax;

   MOBJECT_DEBUG(HeaderInfoListImpl)
};

#endif // HEADERINFOIMPL_H
