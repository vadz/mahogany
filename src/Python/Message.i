/*-*- c++ -*-********************************************************
 * Message class: handling of single messages                       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/

%module Message

%{
#include	"Mpch.h"
#ifndef	USE_PCH
#   include   "Mcommon.h"
#   include   "Message.h"
#endif
%}

%import String.i

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
    void GetHeaderLine( String &line, String &value);

   /** get Subject line
       @return Subject entry
   */
     String & Subject(void) ;

   /** get From line
       @return From entry
   */
     String From() ;

   /** get Date line
       @return Date when message was sent
   */
     String & Date(void) ;

   /** get message text
       @return the uninterpreted message body
   */
    char *FetchText(void);

   /** return the number of body parts in message
       @return the number of body parts
   */
    int CountParts(void);
   
   /**@name Methods accessing individual parts of a message. */
   //@{
   /** Return the content of the part.
       @param  n part number
       @param  len a pointer to a variable where to store length of data returned
       @return pointer to the content
   */
     char *GetPartContent(int n = 0, unsigned long *len = NULL);

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
    int GetPartType(int n = 0);

   /** Query the type of the content.
       @param  n part number
       @return content type ID
   */
    int GetPartEncoding(int n = 0);

   /** Query the size of the content, either in lines (TYPETEXT/TYPEMESSAGE) or bytes.
       @param  n part number
       @param forceBytes if true, the size will be returned in bytes always
       @return size
   */
    size_t GetPartSize(int n = 0, bool forceBytes = false);

   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
    String  & GetPartMimeType(int n = 0);

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
    String  & GetPartDesc(int n = 0);

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
    String  & GetPartSpec(int n = 0);

   /** Append the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
   */
    void WriteToString(String &str, bool headerFlag = true) ;

   //@}
   /**  destructor */
    ~Message() {}

   /// check whether object is initialised
    bool IsInitialised(void) ;
};

