/*-*- c++ -*-********************************************************
 * Message class: entries for message                               *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/



#ifndef MESSAGE_H
#define MESSAGE_H

#include "FolderType.h"
#include "MObject.h"
#include "kbList.h"

#include <wx/fontenc.h>

class WXDLLEXPORT wxArrayString;
class MailFolder;

// ----------------------------------------------------------------------------
// C-client compatibility defines
// ----------------------------------------------------------------------------

#ifdef   OS_WIN
#  define   TEXT_DATA_CAST(x)    ((unsigned char *)x)
#else
#  define   TEXT_DATA_CAST(x)    ((char *)x)
#endif

// ----------------------------------------------------------------------------
// Message class
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

/// a define to make scandoc work
#define MessageContentType Message::ContentType

/// a type to store parameters and their values
class MessageParameter
{
public:
   /// the parameter name
   String name;
   /// the parameter value
   String value;
};

/// define a parameter list
KBLIST_DEFINE(MessageParameterList, MessageParameter);

/**
   Message class, containing the most commonly used message headers.
   */
class Message : public MObjectRC
{
public:
   /**@name Constants (correspoding to c-client's mail.h */
   //@{
   /** Primary body types
       If you change any of these you must also change body_types in
       rfc822.c */
   enum ContentType
   {
      /// unformatted text
      MSG_TYPETEXT = 0,
      /// multipart content
      MSG_TYPEMULTIPART = 1,
      /// encapsulated message
      MSG_TYPEMESSAGE = 2,
      /// application data
      MSG_TYPEAPPLICATION = 3,
      /// audio
      MSG_TYPEAUDIO = 4,
      /// static image
      MSG_TYPEIMAGE = 5,
      /// video
      MSG_TYPEVIDEO = 6,
      /// model
      MSG_TYPEMODEL = 7,
      /// unknown
      MSG_TYPEOTHER = 8,
      /// maximum type code
      MSG_TYPEMAX = 15
   };
   //@}

   /** This constructor creates a Message from a string.
    */
   static class Message * Create(
      const char * itext,
      UIdType uid = UID_ILLEGAL,
      class Profile *iprofile = NULL);

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
   virtual wxArrayString GetHeaderLines(const char **headers,
                                        wxArrayInt *encodings = NULL) const = 0;

   /** Get the complete header text.
       @return string with multiline text containing the message headers
   */
   virtual String GetHeader(void) const = 0;

   /** get Subject line
       @return Subject entry
   */
   virtual String Subject(void) const = 0;

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

   /** Get an address line.
       Using MAT_REPLY should always return a valid return address.
       @param name where to store personal name if available
       @param type which address
       @return address entry
   */
   virtual String Address(String &name,
                          MessageAddressType type = MAT_REPLYTO) const = 0;

   /** Extract the first name from the result returned by calling Address()
   */
   virtual String GetAddressFirstName(MessageAddressType type = MAT_REPLYTO) const;

   /** Extract the last name from the result returned by calling Address()
   */
   virtual String GetAddressLastName(MessageAddressType type = MAT_REPLYTO) const;

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

   /** Get message text.
       @return the uninterpreted message body
   */
   virtual String FetchText(void) = 0;

   /** return the number of body parts in message
       @return the number of body parts
   */
   virtual int CountParts(void) = 0;

   /** Returns a pointer to the folder. If the caller needs that
       folder to stay around, it should IncRef() it. It's existence is
       guaranteed for as long as the message exists.
       @return folder pointer (not incref'ed)
   */
   virtual MailFolder * GetFolder(void) const = 0;

   /**@name Methods accessing individual parts of a message. */
   //@{
   /** Return the content of the part.
       @param  n part number
       @param  len a pointer to a variable where to store length of data returned
       @return pointer to the content
   */
   virtual const char *GetPartContent(int n = 0, unsigned long *len = NULL) = 0;

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   virtual ContentType GetPartType(int n = 0) = 0;

   /** Query the type of the content.
       @param  n part number
       @return font encoding (ASCII-7, ISO8859-1, KOI8-R, ...)
   */
   virtual wxFontEncoding GetTextPartEncoding(int n = 0) = 0;

   /** Query the type of the content.
       @param  n part number
       @return content type ID (ENCBASE64, ENCQUOTEDPRINTABLE, ...)
   */
   virtual int GetPartTransferEncoding(int n = 0) = 0;

   /** Returns the size of the part in bytes or lines (only for the text
       messages and only if useNaturalUnits is true)
       @param n part number
       @param useNaturalUnits must be set to true to get size in lines for text
       @return size
   */
   virtual size_t GetPartSize(int n = 0, bool useNaturalUnits = false) = 0;

   /** Get the list of parameters for a given part.
       @param n part number, if -1, for the top level.
       @return list of parameters, must be freed by caller.
   */
   virtual MessageParameterList const & GetParameters(int n = -1) = 0;

   /** Get the list of disposition parameters for a given part.
       @param n part number, if -1, for the top level.
       @param disptype string where to store disposition type
       @return list of parameters, must be freed by caller.
   */
   virtual MessageParameterList const & GetDisposition(int n = -1, String *disptype = NULL) = 0;

   /**
       Get the list of all unique addresses appearing in this message headers
       (including from, to, reply-to, cc, bcc, ...)

       @param [out] array filled with unique addresses
       @return the number of addresses retrieved
   */
   virtual size_t ExtractAddressesFromHeader(wxArrayString& addresses);

   /** Get a parameter value from the list.
       @param list a MessageParameterList
       @param parameter parameter to look up
       @param value set to new value if found
       @return true if found
   */
   bool ExpandParameter(MessageParameterList const & list,
                        String const &parameter,
                        String *value) const;

   /** Get parameter by name.
       @param n part number, if -1, for the top level.
       @return true if parameter was found
   */
   bool GetParameter(int n, const String& param, String *value)
   {
      return ExpandParameter(GetParameters(n), param, value);
   }

   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
   virtual String const & GetPartMimeType(int n = 0) = 0;

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
   virtual String const & GetPartDesc(int n = 0) = 0;

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   virtual String const & GetPartSpec(int n = 0) = 0;

   /** Return the numeric status of message.
       @return flags of message (combination of MailFolder::MSG_STAT_XXX flags)
   */
   virtual int GetStatus() const = 0;

   /** return the size of the message in bytes */
   virtual unsigned long GetSize() const = 0;

   /** return the date of the message */
   virtual time_t GetDate() const = 0;

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

   /// Return the numeric uid
   virtual UIdType GetUId(void) const = 0;
   //@}
protected:
   /** virtual destructor */
   virtual ~Message() {}
   GCC_DTOR_WARN_OFF
   MOBJECT_NAME(Message)
};

#endif
