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

#ifndef   USE_PCH
#  include "FolderType.h"
#endif // USE_PCH

#include "MimePart.h"
#include "MObject.h"

class Message;
class Profile;

// ----------------------------------------------------------------------------
// SendMessage: ABC for a class allowing to send a message
// ----------------------------------------------------------------------------

/**
  SendMessage represents an outgoing message: usually you first create an
  object of this class, then fill the headers and finally call SendOrQueue()
  which either sends the message or stores it in the Outbox to be sent later.

  Alternatively, you may use CreateResent() to redirect an existing message.
 */
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

   /** @name Creating outgoing messages

       A new message is created with Create(), to redirect/bounce an existing
       one, use CreateResent() and to send an existing message (typically from
       some kind of Outbox folder) for the first time use CreateFromMsg().
   */
   //@{

   /**
      Creates an empty object, setting some initial values.

      @param profile pointer to the profile to use (must be non NULL)
      @param protocol which protocol to use for sending
      @param frame the parent window for dialogs, may be NULL
   */
   static SendMessage *Create(Profile *profile,
                              Protocol protocol = Prot_Default,
                              wxFrame *frame = NULL);

   /**
      Creates a duplicate of an existing message.

      This must be used only to resend a message which had been previously sent
      (and received), as its name indicates. Use CreateFromMsg() to create a
      SendMessage from a message which has never been sent.

      @param profile pointer to the profile to use (must be non NULL)
      @param message the original message (can't be NULL)
      @param frame the parent window for dialogs, may be NULL
   */
   static SendMessage *CreateResent(Profile *profile,
                                    const Message *message,
                                    wxFrame *frame = NULL);

   /**
      Creates a new SendMessage object and initializes it with the existing
      message contents.

      @param profile pointer to the profile to use (must be non NULL)
      @param message the original message (can't be NULL)
      @param frame the parent window for dialogs, may be NULL
    */
   static SendMessage *CreateFromMsg(Profile *profile,
                                     const Message *message,
                                     Protocol protocol = Prot_Default,
                                     wxFrame *frame = NULL);

   //@}

   /** @name Standard headers */
   //@{

   /** Sets the message subject.
       @param subject the subject
   */
   virtual void SetSubject(const String &subject) = 0;

   /** Sets the address fields, To:, CC: and BCC:.
       @param to primary address to send mail to
       @param cc carbon copy addresses
       @param bcc blind carbon copy addresses
   */
   virtual void SetAddresses(const String &to,
                             const String &cc = wxEmptyString,
                             const String &bcc = wxEmptyString) = 0;

   /** Sets the value for the from field.
       @param from sender address
       @param returnaddress address for Reply-To
       @param sender setting if needed
   */
   virtual void SetFrom(const String &from,
                        const String &replyaddress = wxEmptyString,
                        const String &sender = wxEmptyString) = 0;

   /**
     Sets the Newsgroup header value

     @param groups comma-separated list of newsgroups
    */
   virtual void SetNewsgroups(const String &groups) = 0;

   /**
     Sets the folders to copy the message to after sending it. By default, the
     FCC value is the value of MP_OUTGOINGFOLDER entry in the profile
     associated with this message if MP_USEOUTGOINGFOLDER is true and empty
     otherwise, but if this method is used it overrides this value and so
     specifying an empty string here disables copying the message to
     SentMail.

     @param fcc comma-separated list of folders to copy the message to
     @return true if all folders were ok, false if there was an error
    */
   virtual bool SetFcc(const String& fcc) = 0;

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
                        const String &disposition = _T("INLINE"),
                        MessageParameterList const *dlist = NULL,
                        MessageParameterList const *plist = NULL,
                        wxFontEncoding enc = wxFONTENCODING_SYSTEM) = 0;

   /** Writes the message to a String
       @param output string to write to
       @return true if ok, false if an error occured
   */
   virtual bool WriteToString(String& output) = 0;

   /** Writes the message to a file
       @param filename file where to write to
       @param append if false, overwrite existing contents
       @return true if ok, false if an error occured
   */
   virtual bool WriteToFile(const String &filename, bool append = true) = 0;

   /** Writes the message to a folder.
       @param foldername file where to write to
       @return true if ok, false if an error occured
   */
   virtual bool WriteToFolder(const String &foldername) = 0;

   /** Sends the message or stores it in the outbox queue, depending
       on profile settings.

       @param flags is the combination of NeverQueue and Silent or 0
       @return true on success
   */
   virtual bool SendOrQueue(int flags = 0) = 0;

   /**
     A simple wrapper for WriteToString(): this writes the message contents
     into the provided string (if not NULL) and shows it in a standard text
     view dialog to the user.

     @param text the pointer to the string receiving the text, may be NULL
   */
   virtual void Preview(String *text = NULL) = 0;

   /// virtual destructor
   virtual ~SendMessage();
};

// SendMessage_obj is a smart reference to SendMessage
DECLARE_AUTOPTR_NO_REF(SendMessage);

#endif // SENDMESSAGE_H

