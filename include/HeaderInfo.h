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
/** This class essentially maps to the c-client Overview structure,
    which holds information for showing lists of messages.

    IMPORTANT: When sorting messages, the instances of this class will
    be copied around in memory bytewise, but not duplicates of the
    object will be created. So the reference counting in wxString
    objects should be compatible with this, as at any time only one
    object exists.
*/
class HeaderInfo
{
public:
   virtual const String &GetSubject(void) const = 0;
   virtual const String &GetFrom(void) const = 0;
   virtual const String &GetTo(void) const = 0;
   virtual time_t GetDate(void) const = 0;
   virtual const String &GetId(void) const = 0;
   virtual const String &GetReferences(void) const = 0;
   virtual UIdType GetUId(void) const = 0;
   virtual int GetStatus(void) const = 0;
   virtual unsigned long const &GetSize(void) const = 0;
   virtual size_t SizeOf(void) const = 0;
   HeaderInfo() {}
   virtual ~HeaderInfo() {}
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

private:
   /// Disallow copy construction
   HeaderInfo(const HeaderInfo &);
   GCC_DTOR_WARN_OFF
};

/** This class holds a complete list of all messages in the folder.
    It must be an array!*/
class HeaderInfoList : public MObjectRC
{
public:
   /// Count the number of messages in listing.
   virtual size_t Count(void) const = 0;
   /// Returns the n-th entry.
   virtual const HeaderInfo * operator[](size_t n) const = 0;
   /// Returns the n-th entry.
   virtual HeaderInfo * operator[](size_t n) = 0;
   /// Returns pointer to array of data:
   virtual HeaderInfo * GetArray(void) = 0;
   /// Returns pointer to entry with this UId
   virtual HeaderInfo * GetEntryUId(UIdType uid) = 0;
   /** Returns the index in the info list sequence number for a UId or UID_ILLEGAL */
   virtual UIdType GetIdxFromUId(UIdType uid) const= 0;
   /// Swaps two elements:
   virtual void Swap(size_t index1, size_t index2) = 0;
   /** Sets a translation table re-mapping index values.
       Will be freed in destructor.
       @param array an array of indices or NULL to remove it.
   */
   virtual void SetTranslationTable(size_t array[] = NULL) = 0;
   /// For use by folder only: corrects size downwards:
   virtual void SetCount(size_t newcount) = 0;
   MOBJECT_NAME(HeaderInfoList)
};

#endif
