/*-*- c++ -*-********************************************************
 * Message class: handling of single messages                       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/

// %nodefault

%module Message

%{
#include   "Mswig.h"
#include   "Mcommon.h"
#include   "Message.h"
%}

%import MString.i
%import MProfile.i

/**
   Message class, containing the most commonly used message headers.
   */
// ----------------------------------------------------------------------------
// Message class
// ----------------------------------------------------------------------------

/// a type to store parameters and their values
class MessageParameter
{
public:
   /// the parameter name
   String name;
   /// the parameter value
   String value;
};


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
   virtual void GetHeaderLine(const String &line, String &value);

   /** Get a complete header text (const char *)!!.
       @return pointer to an internal buffer
   */
   virtual String GetHeader(void);

   /** get Subject line
       @return Subject entry
   */
   virtual String Subject(void);

   /** get From line
       @return From entry
   */
   virtual String From();

   /** get Date line
       @return Date when message was sent
   */
   virtual const String & Date(void);

   /** get message text
       @return the uninterpreted message body
   */
   virtual const String & FetchText(void);

   /** return the number of body parts in message
       @return the number of body parts
   */
   virtual int CountParts(void);

   /**@name Methods accessing individual parts of a message. */
   //@{
   /** Return the content of the part.
       @param  n part number
       @param  len a pointer to a variable where to store length of data returned
       @return pointer to the content
   */
   virtual const char *GetPartContent(int n = 0, unsigned long *len = NULL);

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   virtual int GetPartType(int n = 0);

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
   virtual int GetTextPartEncoding(int n = 0);

   /** Query the size of the content, either in lines (TYPETEXT/TYPEMESSAGE) or bytes.
       @param  n part number
       @param forceBytes if true, the size will be returned in bytes always
       @return size
   */
   virtual size_t GetPartSize(int n = 0, bool forceBytes = false);

   /** Get the list of parameters for a given part.
       @param n part number, if -1, for the top level.
       @return list of parameters, must be freed by caller.
   */
   virtual const MessageParameterList & GetParameters(int n = -1);

   /** Get the list of disposition parameters for a given part.
       @param n part number, if -1, for the top level.
       @param disptype string where to store disposition type
       @return list of parameters, must be freed by caller.
   */
   virtual const MessageParameterList & GetDisposition(int n = -1, String *disptype = NULL);

   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
   virtual const String & GetPartMimeType(int n = 0);

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
   virtual const String & GetPartDesc(int n = 0);

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   virtual const String & GetPartSpec(int n = 0);

   /** Return the numeric status of message.
       @return flags of message
   */
   virtual int GetStatus();

   /** Return the size of message in bytes
   */
   virtual unsigned long GetSize();

   /** Return the date of message
   */
   virtual time_t GetDate();

   /** Write the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
   */
   virtual void WriteToString(String &str, bool headerFlag = true);

   //@}
};

