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

#include <wx/scopedptr.h>

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
       Possible return values of PrepareForSending().
    */
   enum Result
   {
      /// Message sending was cancelled by user.
      Result_Cancelled = -1,

      /// Message was successfully prepared for sending.
      Result_Prepared,

      /// Message was queued in the outbox and doesn't need to be sent.
      Result_Queued,

      /// An error occurred while preparing the message for sending.
      Result_Error
   };

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
   static SendMessage *Create(const Profile *profile,
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
   static SendMessage *CreateResent(const Profile *profile,
                                    const Message *message,
                                    wxFrame *frame = NULL);

   /**
      Creates a new SendMessage object and initializes it with the existing
      message contents.

      @param profile pointer to the profile to use (must be non NULL)
      @param message the original message (can't be NULL)
      @param protocol which protocol to use for sending
      @param frame the parent window for dialogs, may be NULL
      @param partsToOmit if non NULL, contains the indices of the MIME parts of
                         the message (consistent with Message::GetMimePart()
                         interpretation) to not copy to the new one
    */
   static SendMessage *CreateFromMsg(const Profile *profile,
                                     const Message *message,
                                     Protocol protocol = Prot_Default,
                                     wxFrame *frame = NULL,
                                     const wxArrayInt *partsToOmit = NULL);

   //@}


   /**
      @name Various static helper functions.
    */
   //@{

   /**
      Bounce (i.e. resend) a message to the given address.
    */
   static bool Bounce(const String& address,
                      const Profile *profile,
                      const Message& message,
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
                        const String &disposition = "INLINE",
                        MessageParameterList const *dlist = NULL,
                        MessageParameterList const *plist = NULL,
                        wxFontEncoding enc = wxFONTENCODING_SYSTEM) = 0;

   /**
      Indicate whether the message should be cryptographically signed.

      By default the message is not signed, this method must be called to try
      signing it.

      @param user The user name to use for signing, can be empty to use the
                  default one.
    */
   virtual void EnableSigning(const String& user = "") = 0;

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

   /**
      Prepares the message for sending.

      This method must be called in the main thread context as it may perform
      GUI operations, e.g. ask the user to confirm message sending.

      After calling this method successfully, SendNow() may be called from
      another thread to avoid blocking the main thread while the sending (or
      saving of the message in the outbox) takes place.

      @param flags Combination of NeverQueue and Silent or 0 (default).
      @param outbox If non-NULL, filled with the name of the folder where the
         message was queued for later sending if the function returns
         Result_Queued.
      @return One of Result enum elements. Notice that SendNow() should only be
         called if this method returns @c Result_Prepared.
    */
   virtual Result PrepareForSending(int flags = 0, String *outbox = NULL) = 0;

   /**
       Do send the message.

       This method sends a message previously prepared by PrepareForSending()
       and may be called from a background thread.

       @param errGeneral Filled with the general error message which should be
         presented to the user if an error occurs.
       @param errDetailed Filled with details of the error message on error.
       @return True if the message was successfully sent, false if it wasn't.
    */
   virtual bool SendNow(String *errGeneral, String *errDetailed) = 0;

   /**
       Post-processing after sending the message to be done in the main thread.

       Currently this function copies the message to the folders in its FCC
       list because opening the folders from another thread doesn't work. In
       the future this should become possible and this method might disappear
       entirely then.
    */
   virtual void AfterSending() = 0;

   /** Sends the message or stores it in the outbox queue, depending
       on profile settings.

       This method simply calls both PrepareForSending() and SendNow(). As
       it calls PrepareForSending(), it may be used from the main thread
       only. And as it calls SendNow() from the main thread, it may block the
       UI for a relatively long time while the message is being sent so its use
       is not recommended.

       @param flags is the combination of NeverQueue and Silent or 0
       @return true on success
   */
   bool SendOrQueue(int flags = 0);

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

/// SendMessage_obj is a smart pointer to SendMessage.
typedef wxScopedPtr<SendMessage> SendMessage_obj;

#endif // SENDMESSAGE_H

