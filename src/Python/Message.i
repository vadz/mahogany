/*-*- c++ -*-********************************************************
 * Message class: handling of single messages                       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/

%module Message

%{
#include   "Mpch.h"
#include   "Mcommon.h"   
#include   "Message.h"
#include   "kbList.h"
   
// we don't want to export our functions as we don't build a shared library
#undef SWIGEXPORT
#define SWIGEXPORT(a,b) a b

%}

%import String.i
%import Profile.i

/**
   Message class, containing the most commonly used message headers.
   */
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

   /** Get a complete header text.
       @return pointer to an internal buffer
   */
   virtual const char * GetHeader(void) ;

   /** get Subject line
       @return Subject entry
   */
   virtual const String & Subject(void) ;

   /** get an address line
       @param name where to store personal name if available
       @param type which address
       @return address entry
   */
   virtual const String Address(String &name,
                                MessageAddressType type = MAT_FROM) ;
               
   /** get From line
       @return From entry
   */
   virtual const String From() ;

   /** get Date line
       @return Date when message was sent
   */
   virtual const String & Date(void) ;

   /** get message text
       @return the uninterpreted message body
   */
   virtual char *FetchText(void);

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
   virtual int GetPartEncoding(int n = 0);

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
   virtual MessageParameterList & GetParameters(int n = -1);
   
   /** Get the list of disposition parameters for a given part.
       @param n part number, if -1, for the top level.
       @param disptype string where to store disposition type
       @return list of parameters, must be freed by caller.
   */
   virtual MessageParameterList & GetDisposition(int n = -1, String *disptype = NULL);
   /** Get a parameter value from the list.
       @param list a MessageParameterList
       @param parameter parameter to look up
       @param value set to new value if found
       @return true if found
   */
   bool ExpandParameter(MessageParameterList & list,
                        String &parameter,
                        String *value);
   
   /** Query the MimeType of the content.
       @param  n part number
       @return string describing the Mime type
   */
   virtual String & GetPartMimeType(int n = 0);

   /** Query the description of the part.
       @param  n part number
       @return string describing the part.
   */
   virtual String & GetPartDesc(int n = 0);

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   virtual String & GetPartSpec(int n = 0);

   /** Append the message to a String.
       @param str the string to write message text to
   */
   virtual void WriteString(String &str) ;

   /** Append the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
   */
   virtual void WriteToString(String &str, bool headerFlag = true) ;

   //@}
   /** virtual destructor */
   virtual ~Message() {}

   /// check whether object is initialised
   virtual bool IsInitialised(void) ;

   /// return class name
   const char *GetClassName(void) 
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

