/*-*- c++ -*-********************************************************
 * Message class: entries for message header                        *
 *                      implementation for MailFolderCC             *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MESSAGECC_H
#define MESSAGECC_H

#ifdef __GNUG__
#pragma interface "MessageCC.h"
#endif

#include "Message.h"

// fwd decl
class MailFolderCC;

/** Message class, containing the most commonly used message headers.
   */
class MessageCC : public Message, public CommonBase
{
private:
   /// reference to the folder this mail is stored in
   const MailFolderCC   *folder;
   /// text of the mail if not linked to a folder
   char *text; 
   /// sequence number in folder
   unsigned long  seq_no;
   /// unique message id
   unsigned long  uid;
   /// holds the pointer to a text buffer allocated by cclient lib
   char *mailText;
   /// refresh information in this structure
   void  Refresh(void);
   /// Subject line
   String   hdr_subject;
   /// date line
   String   hdr_date;
   /// number of parts
   int   numOfParts;
   /// has it been initialised?
   bool     initialisedFlag;
   /// body of message
   BODY  *body;
   /// envelope for messages to be sent
   ENVELOPE *envelope;
   /** Get the body information and update body variable.
       @return the body value, NULL on failure.
   */
   BODY * GetBody(void);

   /// a profile:
   Profile  *profile;
   
   /** A temporarily allocated buffer for GetPartContent().
       It holds the information returned by that function and is only
       valid until its next call.
   */
   char *partContentPtr;
   
   /** Structure holding information about the individual message parts.
     */
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
      /// description
      String   description;
      /// id
      String   id;
      /// size, either in lines or bytes, depending on type
      long  size_lines;
      /// size, either in lines or bytes, depending on type
      long  size_bytes;
   };
 
   /// a vector of all the body part information
   PartInfo *partInfos;
   
   /** A function to recursively collect information about all the
       body parts. It is taken from the IMAP/mtest example.
       @param  body the body part to look at
       @param   pfx  the prefix part of the spec
       @param  i    the running index ??
       @param   count an integer variable to be used for indexing the partInfos array
       @param  write whether to write data to the partInfos structure or just count the parts
       @param  firsttime an internal flag used to decide if to use prefix
       
   */
   void decode_body(BODY *body, String &pfx,long i, int *count,
          bool write, bool firsttime = true);
public:
   /**@name Constructors and Destructors */
   //@{
   /** constructor, required associated folder reference
       @param  folder where this mail is stored
       @param  num   sequence number of the message
       @param  uid   unique message id
   */
   MessageCC(MailFolderCC *folder, unsigned long num, unsigned
        long uid);

   /** Constructor, creating an unitialised object.
     */
   MessageCC(Profile *profile = NULL);

   /** Constructor, creating an object from a text buffer.
       Incomplete!! There are still references to the folder pointer,
       so this won't work yet.
       */
   MessageCC(const char *itext,  Profile *iprofile);

   /** 2nd stage constructor, used to initialise object.
     */
   void Create(Profile *profile = NULL);

   /** destructor */
   ~MessageCC();
   //@}

   /** get any header line
       @line name of header line
       @value string where result will be stored, or empty string
   */
   void GetHeaderLine(const String &line, String &value);

   /** get Subject line
       @return Subject entry
   */
   const String   & Subject(void) const;

   /** get an address line
       @param name where to store personal name if available
       @param type which address
       @return address entry
   */
   virtual const String Address(String &name,
                                MessageAddressType type = MAT_FROM) const;

   /** get From line
       @return From entry
   */
   virtual const String From(void) const;

   /** get Date line
       @return Date when message was sent
   */
   const String & Date(void) const;

   /** get message text
       @return the uninterpreted message body
   */
   char *FetchText(void);

   /** decode the MIME structure
     */
   void DecodeMIME(void);

   /** return the number of body parts in message
       @return the number of body parts
   */
   int CountParts(void);
#if 0
   /** return the n-th body part
       @return the body part
   */
   MessageBodyPart *GetPart(int n);
#endif
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
   int GetPartType(int n = 0);

   /** Query the encoding of the content.
       @param  n part number
       @return encoding type ID
   */
   int GetPartEncoding(int n = 0);

   /** Query the size of the content.
       @param  n part number
       @param forceBytes force size to be reported in bytes, even if TYPETEXT/TYPEMESSAGE
       @return size
   */
   size_t   GetPartSize(int n = 0, bool forceBytes = false);

   /** Get the list of parameters for a given part.
       @param n part number, if -1, for the top level.
       @return list of parameters, must be freed by caller.
   */
   MessageParameterList *GetParameters(int n = -1);
   
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

   /** Query the section specification string of body part.
       @param  n part number
       @return MIME/IMAP4 section specifier #.#.#.#
   */
   String const & GetPartSpec(int n = 0);

   /** Append the message to a String.
       @param str the string to write message text to
       @param headerFlag if true, include header
   */
   void WriteToString(String &str, bool headerFlag = true) const;
   
   //@}
   /** check if it is ok to use
       @param  true if it is ok
   */
   bool     IsInitialised(void) const { return initialisedFlag; }

   CB_DECLARE_CLASS(MessageCC, Message);
};

#ifndef  MESSAGECC_FROMLEN
///   the width of the  From: field 
#  define   MESSAGECC_FROMLEN 40
#endif


#endif
