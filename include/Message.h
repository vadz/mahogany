/*-*- c++ -*-********************************************************
 * Message class: entries for message                               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$                *
 *
 * $Log$
 * Revision 1.6  1998/05/30 17:55:32  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.5  1998/05/18 17:48:18  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 *
 *******************************************************************/

#ifndef MESSAGE_H
#define MESSAGE_H

#ifndef  USE_PCH
#   include  <Mcommon.h>
#endif

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
                                MessageAddressType type = MAT_FROM)
      const = 0;
               
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
};

#endif
