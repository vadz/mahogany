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
   /** Creates an empty object, setting some initial values.
       @param iprof optional pointer for a parent profile
       @param protocol which protocol to use for sending
   */
   static SendMessage *Create(Profile *iprof,
                              Protocol protocol = Prot_Default);

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
       @param personal personal name
       @param returnaddress address for Reply-To
       @param sender setting if needed
   */
   virtual void SetFrom(const String &from,
                        const String &personal = "",
                        const String &replyaddress = "",
                        const String &sender = "") = 0;

   virtual void SetNewsgroups(const String &groups) = 0;

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
   virtual void AddPart(MessageContentType type,
                        const char *buf, size_t len,
                        const String &subtype = M_EMPTYSTRING,
                        const String &disposition = "INLINE",
                        MessageParameterList const *dlist = NULL,
                        MessageParameterList const *plist = NULL,
                        wxFontEncoding enc = wxFONTENCODING_SYSTEM) = 0;

   /// set the encoding to use for 8bit characters in the headers
   virtual void SetHeaderEncoding(wxFontEncoding enc) = 0;

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

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   virtual void AddHeaderEntry(const String &entry, const String &value) = 0;

   /** Sends the message or stores it in the outbox queue, depending
       on profile settings.

       @param sendAlways overrides the config settings if true
       @return true on success
   */
   virtual bool SendOrQueue(bool sendAlways = false) = 0;

   /// virtual destructor
   virtual ~SendMessage();
};

#endif // SENDMESSAGE_H

