//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Message.h: declaration of the Message class interface
// Purpose:     Message is an ABC for the classes representing a mail message
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGE_H
#define MESSAGE_H

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "MObject.h"

#include "MimePart.h"

#ifndef USE_PCH
#  include "FolderType.h"    // for Protocol enum
#endif // USE_PCH

class WXDLLEXPORT wxArrayString;

class AddressList;
class MailFolder;
class Profile;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// all (standard) address headers
enum MessageAddressType
{
   MAT_FROM,
   MAT_SENDER,
   MAT_RETURNPATH,
   MAT_REPLYTO,
   MAT_TO,
   MAT_CC,
   MAT_BCC
};

// ----------------------------------------------------------------------------
// HeaderIterator
// ----------------------------------------------------------------------------

/**
  HeaderIterator extracts the individual headers from the full message header
  and returns them to the caller one by one
 */
class HeaderIterator
{
public:
   /**
     Ctor takes the full message header. Normally it is only used by the
     Message::GetHeaderIterator() function.
    */
   HeaderIterator(const String& header);

   /**
     Fills the provided name and value pointers with the next header from the
     message

     @param name is the pointer to the header name, can't be NULL
     @param value is the pointer which receives the header value and may be NULL
     @return true if ok, false if no more headers
    */
   bool GetNext(String *name, String *value);

   /**
     Get all headers at once. If a header occurs more than once, its values are
     concatenated together with "\r\n" separating them.

     @param names the array to return the header names in
     @param values the array to return the header values in
     @return the number of headers in the arrays
    */
   size_t GetAll(wxArrayString *names, wxArrayString *values);

   /**
     Resets the iterator so that the next call to GetNext() will return the
     first header of the message (again).
    */
   void Reset();

private:
   /// the full header
   String m_header;

   /// temp string which we don't realloc all the time for efficiency
   String m_str;

   /// the pointer to the current position inside m_header
   const char *m_pcCurrent;
};

// ----------------------------------------------------------------------------
// Message
// ----------------------------------------------------------------------------

/**
   Message class represents a message in a mail folder. It provides access to
   the message headers as well as to the message contents (in conjunction with
   MimePart class)
 */
class Message : public MObjectRC
{
public:
   /** This constructor creates a Message from a string.
    */
   static Message *Create(const char * itext,
                          UIdType uid = UID_ILLEGAL,
                          Profile *profile = NULL);

   /** @name Headers access
    */
   //@{

   /**
     @name Accessing generic headers
    */
   //@{

   /** get any header line

       USE GetHeaderLines() INSTEAD OF MULTIPLE CALLS To GetHeaderLine(),
       IT IS MUCH MORE EFFICIENT AS IT INVOLVES ONLY ONE TRIP TO SERVER!

       @param line name of header line
       @param value string where result will be stored, or empty string
       @return true if header was found in the headers
   */
   bool GetHeaderLine(const String &line,
                      String &value,
                      wxFontEncoding *encoding = NULL) const;

   /** Get the values of the specified headers.

       @param headers the NULL-terminated array of the headers to retrieve
       @param encodings if non NULL, filled with encodings of the headers
       @return the array containing the header values
   */
   virtual wxArrayString GetHeaderLines(const wxChar **headers,
                                        wxArrayInt *encodings = NULL) const = 0;

   /**
     Return the object which may be used for iterating over the headers.

     @return iterator object
    */
   HeaderIterator GetHeaderIterator() const
      { return HeaderIterator(GetHeader()); }

   /**
     NB: this method is deprecated because its API is bad and doesn't deal
         satisfactory with the repeating headers (if some header occurs twice
         or more the corresponding value will be the concatenation of all of
         its values which is not always what the caller wants), use
         HeaderIterator instead of it!

     Get the names and values of all headers of the message.

     @param names the array to return the header names in
     @param values the array to return the header values in
     @return the number of headers in the arrays
   */
   size_t GetAllHeaders(wxArrayString *names, wxArrayString *values) const;

   /** Get the complete header text.
       @return string with multiline text containing the message headers
   */
   virtual String GetHeader(void) const = 0;

   //@}

   /**
     @name Headers containing the addresses
    */
   //@{

   /**
       Get all addresses of the given type (more efficient than GetHeader as it
       only uses the envelope and doesn't need to fetch all headers)

       May return empty string if the corresponding header was not found.

       @param type which address
       @param addresses the array to append addresses to
       @return number of addresses added to the array
   */
   virtual size_t GetAddresses(MessageAddressType type,
                               wxArrayString& addresses) const = 0;

   /**
       Get the address list of all address of the given type. The caller must
       DecRef() the returned pointer.

       @return address list or NULL if no such addresses
    */
   virtual AddressList *GetAddressList(MessageAddressType type) const = 0;

   /**
       Get the string containing all addresses

       @return string containing all addresses of given type
    */
   String GetAddressesString(MessageAddressType type) const;

   /**
       Get the list of all unique addresses appearing in this message headers
       (including from, to, reply-to, cc, bcc, ...)

       @param [out] array filled with unique addresses
       @return the number of addresses retrieved
   */
   virtual size_t ExtractAddressesFromHeader(wxArrayString& addresses);

   //@}

   /**
     @name Special accessors for the common headers
    */
   //@{

   /**
     Get the subject (this is more efficient than GetHeaderLine() as it takes
     the subject from the envelope)

     @return the subject header value
   */
   virtual String Subject(void) const = 0;

   /** get From line
       @return From entry
   */
   virtual String From() const = 0;

   /** get Date line
       @return Date when message was sent
   */
   virtual String Date(void) const = 0;

   /** Return message id. */
   virtual String GetId(void) const = 0;

   /** Return message references. */
   virtual String GetReferences(void) const = 0;

   /** Return In-Reply-To header value or empty string */
   virtual String GetInReplyTo(void) const = 0;

   /** Return the list of newsgroups the messages was posted to or empty
       string
    */
   virtual String GetNewsgroups() const = 0;

   /** Return the numeric status of message.
       @return flags of message (combination of MailFolder::MSG_STAT_XXX flags)
   */
   virtual int GetStatus() const = 0;

   /** return the size of the message in bytes */
   virtual unsigned long GetSize() const = 0;

   /** return the date of the message */
   virtual time_t GetDate() const = 0;

   //@}

   //@}

   /** @name Other accessors
    */

   //@{

   /** Returns a pointer to the folder. If the caller needs that
       folder to stay around, it should IncRef() it. It's existence is
       guaranteed for as long as the message exists.
       @return folder pointer (not incref'ed)
   */
   virtual MailFolder *GetFolder(void) const = 0;

   /// Return the numeric uid
   virtual UIdType GetUId(void) const = 0;

   //@}

   /** @name Methods accessing individual parts of a message.

       All of them but GetTopMimePart() are deprecated now, use MimePart class
       directly instead
    */
   //@{

   /// get the top level MIME part of the message
   virtual const MimePart *GetTopMimePart() const = 0;

   /** return the number of body parts in message
       @return the number of body parts
   */
   virtual int CountParts(void) const = 0;

   /// return the MimePart object for the given part number
   virtual const MimePart *GetMimePart(int n) const = 0;

   /** Return the content of the part.
       @param  n part number
       @param  len a pointer to a variable where to store length of data returned
       @return pointer to the content
   */
   const void *GetPartContent(int n, unsigned long *len = NULL) const
      { return GetMimePart(n)->GetContent(len); }

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   MessageContentType GetPartType(int n) const
      { return GetMimePart(n)->GetType().GetPrimary(); }

   /** Query the type of the content.
       @param  n part number
       @return font encoding (ASCII-7, ISO8859-1, KOI8-R, ...)
   */
   wxFontEncoding GetTextPartEncoding(int n) const
      { return GetMimePart(n)->GetTextEncoding(); }

   /** Query the type of the content.
       @param  n part number
       @return content type ID (ENCBASE64, ENCQUOTEDPRINTABLE, ...)
   */
   MimeXferEncoding GetPartTransferEncoding(int n) const
      { return GetMimePart(n)->GetTransferEncoding(); }

   /** Returns the size of the part in bytes or lines (only for the text
       messages and only if useNaturalUnits is true)
       @param n part number
       @param useNaturalUnits must be set to true to get size in lines for text
       @return size
   */
   size_t GetPartSize(int n, bool useNaturalUnits = false) const
   {
      const MimePart *part = GetMimePart(n);
      if ( useNaturalUnits && part->GetType().IsText() )
         return part->GetNumberOfLines();
      else
         return part->GetSize();
   }

   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
   String GetPartMimeType(int n) const
      { return GetMimePart(n)->GetType().GetFull(); }

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
   String GetPartDesc(int n) const
      { return GetMimePart(n)->GetDescription(); }

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   String GetPartSpec(int n) const
      { return GetMimePart(n)->GetPartSpec(); }

   /** Get the list of parameters for a given part.
       @param n part number
       @return list of parameters, must be freed by caller.
   */
   const MimeParameterList& GetParameters(int n) const
      { return GetMimePart(n)->GetParameters(); }

   /** Get the list of disposition parameters for a given part.
       @param n part number
       @param disptype string where to store disposition type
       @return list of parameters, must be freed by caller.
   */
   const MimeParameterList& GetDisposition(int n,
                                           String *disptype = NULL) const
   {
      const MimePart *part = GetMimePart(n);
      if ( disptype )
         *disptype = part->GetDisposition();

      return part->GetDispositionParameters();
   }

   //@}

   /** @name Functions working with the message contents
    */
   //@{

   /** Get message text.
       @return the uninterpreted message body
   */
   virtual String FetchText(void) const = 0;

   /** Write the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
       @return FALSE on error
   */
   virtual bool WriteToString(String &str, bool headerFlag = true) const = 0;

   /** Takes this message and tries to send it. Only useful for
       messages in some kind of Outbox folder.
       @param protocol how to send the message, or Prot_Illegal to autodetect
       @param send if true, send, otherwise send or queue depending on setting
       @return true on success
   */
   virtual bool SendOrQueue(Protocol protocol = Prot_Illegal,
                            bool send = FALSE) = 0;

   //@}

   // for backwards compatibility only, don't use
   enum ContentType
   {
      /// unformatted text
      MSG_TYPETEXT = MimeType::TEXT,
      /// multipart content
      MSG_TYPEMULTIPART = MimeType::MULTIPART,
      /// encapsulated message
      MSG_TYPEMESSAGE = MimeType::MESSAGE,
      /// application data
      MSG_TYPEAPPLICATION = MimeType::APPLICATION,
      /// audio
      MSG_TYPEAUDIO = MimeType::AUDIO,
      /// static image
      MSG_TYPEIMAGE = MimeType::IMAGE,
      /// video
      MSG_TYPEVIDEO = MimeType::VIDEO,
      /// model
      MSG_TYPEMODEL = MimeType::MODEL,
      /// unknown
      MSG_TYPEOTHER = MimeType::OTHER,
      /// invalid type code
      MSG_TYPEINVALID = MimeType::INVALID
   };

   /** @name Address stuff

       THESE FUNCTIONS ARE DEPRECATED, USE Address AND AddressList INSTEAD!
    */
   //@{

   /// return "Foo" from address of form "Foo Bar <baz>"
   static String GetFirstNameFromAddress(const String& address);

   /// return "Bar" from address of form "Foo Bar <baz>"
   static String GetLastNameFromAddress(const String& address);

   /// return "Foo Bar" from address of form "Foo Bar <baz>"
   static String GetNameFromAddress(const String& address);

   /// return "baz" from address of form "Foo Bar <baz>"
   static String GetEMailFromAddress(const String &address);

   /// compare 2 addresses, return TRUE if they're the same
   static bool CompareAddresses(const String& adr1, const String& adr2);

   /// return the address index in the array of addresses or wxNOT_FOUND
   static int FindAddress(const wxArrayString& addresses, const String& addr);

   //@}

protected:
   /** virtual destructor */
   virtual ~Message();

   GCC_DTOR_WARN_OFF

   MOBJECT_NAME(Message)
};

DECLARE_AUTOPTR(Message);

#endif // MESSAGE_H
