///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/SendMessage.h: SendMessage ABC declaration
// Purpose:     handling of mail folders with c-client lib
// Author:      Vadim Zeitlin (based on SendMessageCC.h)
// Modified by:
// Created:     26.01.01
// CVS-ID:      $Id$
// Copyright:   (C) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef SENDMESSAGE_H
#define SENDMESSAGE_H

class Profile;

// ----------------------------------------------------------------------------
// SendMessage: ABC for a class allowing to send a message
// ----------------------------------------------------------------------------

class SendMessage
{
public:
   /**
     Flags for SendOrQueue()
    */
   enum
   {
      /// always send the message directly immediately, never queue it
      NeverQueue = 1,

      /// don't give any messages
      Silent = 2
   };

   /** Creates an empty object, setting some initial values.
       @param iprof optional pointer for a parent profile
       @param protocol which protocol to use for sending
   */
   static SendMessage *Create(Profile *iprof,
                              Protocol protocol = Prot_Default);

   /** @name Standard headers */
   //@{

   /** Sets the message subject.
       @param subject the subject
   */
   virtual void SetSubject(const String &subject) = 0;

   /** Sets the address fields, To:, CC: and BCC:.
       @param To primary address to send mail to
       @param CC carbon copy addresses
       @param BCC blind carbon copy addresses
   */
   virtual void SetAddresses(const String &To,
                     const String &CC = "",
                     const String &BCC = "") = 0;

   /** Sets the value for the from field.
       @param from sender address
       @param returnaddress address for Reply-To
       @param sender setting if needed
   */
   virtual void SetFrom(const String &from,
                        const String &replyaddress = "",
                        const String &sender = "") = 0;

   virtual void SetNewsgroups(const String &groups) = 0;

   //@}

   /** @name Custom (or extra) headers stuff */
   //@{

   /// set the encoding to use for 8bit characters in the headers
   virtual void SetHeaderEncoding(wxFontEncoding enc) = 0;

   /** Adds an extra header line.

       @param name name of header entry to add
       @param value value of header entry
   */
   virtual void AddHeaderEntry(const String& name, const String& value) = 0;

   /** Remove the header with the specified name

       @param name name of header entry to remove
   */
   virtual void RemoveHeaderEntry(const String& name) = 0;

   /**
     Return true if we (already) have the header with the given name.

     @param name name of header entry to query
     @return true if we have it, false otherwise
   */
   virtual bool HasHeaderEntry(const String& name) const = 0;

   /**
     Get the value for the header entry.

     @param name name of header entry to query
     @return header value if we have it, empty string otherwise
   */
   virtual String GetHeaderEntry(const String& name) const = 0;

   //@}

   /** Adds a part to the message.
       @param type numeric mime type
       @param buf  pointer to data
       @param len  length of data
       @param subtype if not empty, mime subtype to use
       @param disposition either INLINE or ATTACHMENT
       @param dlist list of disposition parameters
       @param plist list of parameters
       @param enc the text encoding (only for TEXT parts)
   */
   virtual void AddPart(MimeType::Primary type,
                        const void *buf, size_t len,
                        const String &subtype = M_EMPTYSTRING,
                        const String &disposition = "INLINE",
                        MessageParameterList const *dlist = NULL,
                        MessageParameterList const *plist = NULL,
                        wxFontEncoding enc = wxFONTENCODING_SYSTEM) = 0;

   /** Writes the message to a String
       @param output string to write to
   */
   virtual void WriteToString(String& output) = 0;

   /** Writes the message to a file
       @param filename file where to write to
       @param append if false, overwrite existing contents
   */
   virtual void WriteToFile(const String &filename, bool append = true) = 0;

   /** Writes the message to a folder.
       @param foldername file where to write to
   */
   virtual void WriteToFolder(const String &foldername) = 0;

   /** Sends the message or stores it in the outbox queue, depending
       on profile settings.

       @param flags is the combination of NeverQueue and Silent or 0
       @return true on success
   */
   virtual bool SendOrQueue(int flags = 0) = 0;

   /// virtual destructor
   virtual ~SendMessage();
};

#endif // SENDMESSAGE_H

