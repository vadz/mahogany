///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageCC.h - declares MessageCC class
// Purpose:     implementation of Message using c-client API
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 M Team
// License:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESSAGECC_H
#define _MESSAGECC_H

#ifdef __GNUG__
   #pragma interface "MessageCC.h"
#endif

#include "Message.h"

// fwd decl
class MailFolderCC;

/** Message class, containing the most commonly used message headers.
   */
class MessageCC : public Message
{
public:
   /// Structure holding information about the individual message parts.
   struct PartInfo
   {
      /// MIME/IMAP4 part id
      String   mimeId;
      /// string describing the MIME type
      String   type;
      /// numerical type id as used by c-client lib
      int   numericalType;
      /// numerical encoding id as used by c-client lib
      int   numericalEncoding;
      /// string containing the parameter settings
      String   params;
      /// list of parameters
      MessageParameterList parameterList;
      /// disposition type
      String dispositionType;
      /// list of disposition parameters
      MessageParameterList dispositionParameterList;
      /// description
      String   description;
      /// id
      String   id;
      /// size, either in lines or bytes, depending on type
      long  size_lines;
      /// size, either in lines or bytes, depending on type
      long  size_bytes;
   };

   // get specfied header lines
   virtual wxArrayString GetHeaderLines(const char **headers,
                                        wxArrayInt *encodings = NULL) const;

   /** Get a complete header text.
       @return string with multiline text containing the message headers
   */
   virtual String GetHeader(void) const;

   /** @name Envelop headers */
   //@{
   /** get Subject line
       @return Subject entry
   */
   virtual String Subject(void) const;

   /** return the date of the message */
   virtual time_t GetDate() const;

   /** Return message id. */
   virtual String GetId(void) const ;

   /** Return message references. */
   virtual String GetReferences(void) const;

   virtual String GetInReplyTo(void) const;

   virtual String GetNewsgroups() const;
   //@}

   virtual size_t GetAddresses(MessageAddressType type,
                               wxArrayString& addresses) const;

   /** Get an address line.
       Using MAT_REPLY should always return a valid return address.
       @param name where to store personal name if available
       @param type which address
       @return address entry
   */
   virtual String Address(String &name,
                          MessageAddressType type = MAT_REPLYTO) const;

   /** get From line
       @return From entry
   */
   virtual String From(void) const;

   /** get Date line
       @return Date when message was sent
   */
   virtual String Date(void) const;

   /** get message text
       @return the uninterpreted message body
   */
   virtual String FetchText(void);

   /** decode the MIME structure
     */
   void DecodeMIME(void);

   /** return the number of body parts in message
       @return the number of body parts
   */
   int CountParts(void);

   /** Returns a pointer to the folder. If the caller needs that
       folder to stay around, it should IncRef() it. It's existence is
       guaranteed for as long as the message exists.
       @return folder pointer (not incref'ed)
   */
   virtual MailFolder * GetFolder(void) const { return m_folder; }

   /**@name Methods accessing individual parts of a message. */
   //@{
   /** Return the content of the part.
       @param  n part number
       @param  len a pointer to a variable where to store length of data returned
       @return pointer to the content
   */
   const char *GetPartContent(int n = 0, unsigned long *len = NULL);

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   ContentType GetPartType(int n = 0);

   /** Query the encoding of the content.
       @param  n part number
       @return encoding type ID
   */
   int GetPartTransferEncoding(int n = 0);

   virtual wxFontEncoding GetTextPartEncoding(int n = 0);

   /** Query the size of the content.
       @param  n part number
       @param useNaturalUnits must be set to true to get size in lines for text
       @return size
   */
   virtual size_t GetPartSize(int n = 0, bool useNaturalUnits = false);

   /** Get the list of parameters for a given part.
       @param n part number, if -1, for the top level.
       @return list of parameters, must be freed by caller.
   */
   MessageParameterList const & GetParameters(int n = -1);

   /** Get the list of disposition parameters for a given part.
       @param n part number, if -1, for the top level.
       @param disptype string where to store disposition type
       @return list of parameters, must be freed by caller.
   */
   MessageParameterList const & GetDisposition(int n = -1,
                                               String *disptype = NULL);

   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
   String const & GetPartMimeType(int n = 0);

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
   String const & GetPartDesc(int n = 0);

   /** Return the numeric status of message.
       @return flags of message
   */
   virtual int GetStatus() const;

   // get the size in bytes
   virtual unsigned long GetSize() const;

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   String const & GetPartSpec(int n = 0);

   /** Write the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
       @return FALSE on error
   */
   virtual bool WriteToString(String &str, bool headerFlag = true) const;


   /** Takes this message and tries to send it. Only useful for
       messages in some kind of Outbox folder.
       @param protocol how to send the message or Prot_Illegal to auto-detect
       @param send if true, send, otherwise send or queue depending on setting
       @return true on success
   */
   virtual bool SendOrQueue(Protocol protocol = Prot_Illegal,
                            bool send = FALSE);

   /// Return the numeric uid
   virtual UIdType GetUId(void) const { return m_uid; }
   //@}

   static MessageCC *Create(const char *text,
                            UIdType uid = UID_ILLEGAL,
                            Profile *profile = NULL)
   {
      return new MessageCC(text, uid, profile);
   }

protected:
   /**@name Constructors and Destructors */
   //@{
   /** constructor, required associated folder reference
       @param folder where this mail is stored
       @param hi header info for this message
   */
   static MessageCC *Create(MailFolderCC *folder, const HeaderInfo& hi);

   /// The MailFolderCC class creates MessageCC objects.
   friend class MailFolderCC;
   //@}
protected:
   /// constructors called by Create()
   MessageCC(MailFolderCC *folder, const HeaderInfo& hi);
   MessageCC(const char *text,
             UIdType uid = UID_ILLEGAL,
             Profile *profile = NULL);

   /** destructor */
   ~MessageCC();

   /// get the ADDRESS struct for the given address header
   ADDRESS *GetAddressStruct(MessageAddressType type) const;

   /// get name/email from ADDRESS
   static
   void AddressToNameAndEmail(ADDRESS *addr, wxString& name, wxString& email);

private:
   /// common part of all ctors
   void Init();

   /// Get the body and envelope information into member variables
   void GetBody(void);

   /// call GetBody() if necessary
   void CheckBody() const { if ( !m_Body ) ((MessageCC *)this)->GetBody(); }

   /// get the envelope information only (faster than GetBody!)
   void GetEnvelope();

   /// get the envelope information only if necessary
   void CheckEnvelope() const
      { if ( !m_Envelope ) ((MessageCC *)this)->GetEnvelope(); }

   /// get the cache element for this message
   MESSAGECACHE *GetCacheElement() const;

   /// get the PartInfo struct for the given part number
   const PartInfo *GetPartInfo(int n) const;

   /// parse the MIME structure of the message and fill m_partInfos array
   bool ParseMIMEStructure();

   /// reference to the folder this mail is stored in
   MailFolderCC *m_folder;

   /// text of the mail if not linked to a folder
   char *m_msgText;

   /// unique message id
   UIdType m_uid;

   /// the parsed date value which we cache
   time_t m_date;

   /// holds the pointer to a text buffer allocated by cclient lib
   char *m_mailFullText;

   /// length of m_mailFullText
   unsigned long m_MailTextLen;

   /// body of message
   BODY *m_Body;

   /// m_Envelope for messages to be sent
   ENVELOPE *m_Envelope;

   /// Profile pointer, may be NULL
   Profile *m_Profile;

   /** A temporarily allocated buffer for GetPartContent().
       It holds the information returned by that function and is only
       valid until its next call.
   */
   char *m_partContentPtr;

   /// a vector of all the body part information
   class PartInfoArray *m_partInfos;
};

#endif // _MESSAGECC_H

