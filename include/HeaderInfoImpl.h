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
   /// Get Score setting (name or empty string)
   virtual int GetScore(void) const { return m_Score; }
   /// Change Score setting (default = 0)
   virtual void AddScore(int delta) { m_Score += delta; }
   /// Set Colour setting (name or empty string)
   virtual void SetColour(const String &col) { m_Colour = col; }
   virtual void SetEncoding(wxFontEncoding enc) { m_Encoding = enc; }
   virtual wxFontEncoding GetEncoding() const { return m_Encoding; }

   virtual HeaderInfo *Clone() const;

   /// folder internal use:
   virtual FolderDataType GetFolderData(void) const
      { return m_FolderData; }
   virtual void SetFolderData(const FolderDataType & fd)
      { m_FolderData = fd; }

protected:
   String m_Subject, m_From, m_To, m_References;
   String m_Id;
   int m_Status;
   unsigned long m_Size;
   unsigned long m_UId;
   time_t m_Date;
   unsigned m_Indentation;
   String m_Colour;
   int m_Score;
   wxFontEncoding m_Encoding;

   FolderDataType m_FolderData;
   friend class MailFolderCC;
};

/** This class holds a complete list of all messages in the folder. */
class HeaderInfoListImpl : public HeaderInfoList
{
public:
   ///@name Interface
   //@{
   /// Count the number of messages in listing.
   virtual size_t Count(void) const
      { return m_NumEntries; }
   /// Returns the n-th entry.
   virtual const HeaderInfo * operator[](size_t n) const
      {
         // simply call non-const operator:
         return (*(HeaderInfoListImpl *)this)[n];
      }
   //@}
   ///@name Implementation
   //@{
   /// Returns the n-th entry.
   virtual HeaderInfo * operator[](size_t n);

   /// Returns pointer to entry with this UId
   virtual HeaderInfo * GetEntryUId(UIdType uid);

   /** Returns the index in the list for a UId or UID_ILLEGAL */
   virtual UIdType GetIdxFromUId(UIdType uid) const;

   /// Returns pointer to array of data:
   virtual HeaderInfo *GetArray(void) { MOcheck(); return m_Listing; }

   /// For use by folder only: corrects size downwards:
   void SetCount(size_t newcount)
      {
         MOcheck();
         CHECK_RET( newcount <= m_NumEntries, "invalid headers count" );
         m_NumEntries = newcount;
      }
   //@}

   /// Returns an empty list of same size.
   virtual HeaderInfoListImpl *DuplicateEmpty(void) const
      {
         return Create(m_NumEntries);
      }

   static HeaderInfoListImpl * Create(size_t n)
      { return new HeaderInfoListImpl(n); }
   /// Swaps two elements:
   virtual void Swap(size_t index1, size_t index2);

   /** Sets a translation table re-mapping index values.
       Will be freed in destructor.
       @param array an array of indices or NULL to remove it.
   */
   virtual void SetTranslationTable(size_t array[] = NULL)
      {
         delete [] m_TranslationTable;
         m_TranslationTable = array;
      }

   virtual void Remove(size_t n);

protected:
   HeaderInfoListImpl(size_t n);
   ~HeaderInfoListImpl();

   /// The current listing of the folder
   HeaderInfoImpl *m_Listing;

   /// number of entries
   size_t m_NumEntries;

   /// translation of indices
   size_t *m_TranslationTable;

   MOBJECT_DEBUG(HeaderInfoListImpl)
};

#endif // HEADERINFOIMPL_H
