/*-*- c++ -*-********************************************************
 * Message class: entries for message                               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                *
 *
 *******************************************************************/

#ifndef MESSAGE_H
#define MESSAGE_H

#include   "kbList.h"

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

/// a type used by Address():
enum MessageAddressType { MAT_FROM, MAT_SENDER, MAT_REPLYTO };

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
class Message 
{   
public:
   /** get any header line
       @line name of header line
       @value string where result will be stored, or empty string
   */
   virtual void GetHeaderLine(const String &line, String &value) = 0;

   /** Get a complete header text.
       @return pointer to an internal buffer
   */
   virtual const char * GetHeader(void) const = 0;

   /** get Subject line
       @return Subject entry
   */
   virtual const String & Subject(void) const = 0;

   /** get an address line
       @param name where to store personal name if available
       @param type which address
       @return address entry
   */
   virtual const String Address(String &name,
                                MessageAddressType type = MAT_FROM) const = 0;
               
   /** get From line
       @return From entry
   */
   virtual const String From() const = 0;

   /** get Date line
       @return Date when message was sent
   */
   virtual const String & Date(void) const = 0;

   /** get message text
       @return the uninterpreted message body
   */
   virtual char *FetchText(void) = 0;

   /** return the number of body parts in message
       @return the number of body parts
   */
   virtual int CountParts(void) = 0;
   
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
   virtual int GetPartType(int n = 0) = 0;

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   virtual int GetPartEncoding(int n = 0) = 0;

   /** Query the size of the content, either in lines (TYPETEXT/TYPEMESSAGE) or bytes.
       @param  n part number
       @param forceBytes if true, the size will be returned in bytes always
       @return size
   */
   virtual size_t GetPartSize(int n = 0, bool forceBytes = false) = 0;

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
   /** Get a parameter value from the list.
       @param list a MessageParameterList
       @param parameter parameter to look up
       @param value set to new value if found
       @return true if found
   */
   bool ExpandParameter(MessageParameterList const & list,
                        String const &parameter,
                        String *value);
   
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

   /** Append the message to a String.
       @param str the string to write message text to
   */
   virtual void WriteString(String &str) const = 0;

   /** Append the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
   */
   virtual void WriteToString(String &str, bool headerFlag = true) const = 0;

   //@}
   /** virtual destructor */
   virtual ~Message() {}

   /// check whether object is initialised
   virtual bool IsInitialised(void) const = 0;

   /// return class name
   const char *GetClassName(void) const
      { return "MailFolder"; }

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
};

#endif
