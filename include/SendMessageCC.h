//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   SendMessageCC.h: declaration of SendMessageCC
// Purpose:     sending/posting of mail messages with c-client lib
// Author:      Karsten Ballüder
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1999-2001 by M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _SENDMESSAGECC_H
#define _SENDMESSAGECC_H

#ifndef USE_PCH
#  include "Mcclient.h"
#endif  //USE_PCH

#include "lists.h"
#include "SendMessage.h"

class Profile;

// ----------------------------------------------------------------------------
// MessageHeadersList: the list of custom (or extra) headers
// ----------------------------------------------------------------------------

struct MessageHeader
{
   MessageHeader(const String& name, const String& value)
      : m_name(name), m_value(value)
   {
   }

   String m_name,
          m_value;
};

M_LIST_OWN(MessageHeadersList, MessageHeader);

// ----------------------------------------------------------------------------
// SendMessageCC: allows to send/post messages using c-client
// ----------------------------------------------------------------------------

class SendMessageCC : public SendMessage
{
public:
   // implement the base class pure virtuals

   // standard headers
   // ----------------

   virtual void SetSubject(const String &subject);

   virtual void SetAddresses(const String &To,
                             const String &CC = wxEmptyString,
                             const String &BCC = wxEmptyString);

   virtual void SetFrom(const String &from,
                        const String &replyaddress = wxEmptyString,
                        const String &sender = wxEmptyString);

   virtual void SetNewsgroups(const String &groups);

   virtual bool SetFcc(const String& fcc);

   virtual void SetHeaderEncoding(wxFontEncoding enc);

   // custom headers
   // --------------

   virtual void AddHeaderEntry(const String &entry, const String &value);

   virtual void RemoveHeaderEntry(const String& name);

   virtual bool HasHeaderEntry(const String& name) const;

   virtual String GetHeaderEntry(const String &key) const;

   // message body
   // ------------

   virtual void AddPart(MimeType::Primary type,
                        const void *buf, size_t len,
                        const String &subtype = M_EMPTYSTRING,
                        const String &disposition = "INLINE",
                        MessageParameterList const *dlist = NULL,
                        MessageParameterList const *plist = NULL,
                        wxFontEncoding enc = wxFONTENCODING_SYSTEM);

   virtual void EnableSigning(const String& user = "");

   virtual bool WriteToString(String  &output);

   virtual bool WriteToFile(const String &filename, bool append = true);

   virtual bool WriteToFolder(const String &foldername);

   virtual Result PrepareForSending(int flags = 0, String *outbox = NULL);
   virtual bool SendNow(String *errGeneral, String *errDetailed);
   virtual void AfterSending();

   virtual void Preview(String *text = NULL);

   /// destructor
   virtual ~SendMessageCC();

   enum Mode { Mode_SMTP, Mode_NNTP };

protected:
   /** @name Construction */
   //@{

   /**
     Ctor for base class static Create()
   */
   SendMessageCC(const Profile *profile,
                 Protocol protocol,
                 wxFrame *frame,
                 const Message *message = NULL);

   /// init the fields for a new message
   void InitNew();

   /// init the fields for a resent message
   void InitResent(const Message *message);

   /// init the header and contents from an existing message
   void InitFromMsg(const Message *message, const wxArrayInt *partsToOmit);

   //@}

   /// set sender address fields
   void SetupFromAddresses();

   /**
      Sign the message cryptographically.

      This is called by Build() if m_sign == true.

      @return false if signing failed, this is a fatal error for this message
    */
   bool Sign();

   /**
      Builds the message prior to sending or saving it.

      @param forStorage if this is true, store some extra information that is
                        not supposed to be sent, like BCC header.
      @return true if ok or false if we failed to build the message
    */
   bool Build(bool forStorage = false);

   /// translate the (wxWin) encoding to (MIME) charset
   String EncodingToCharset(wxFontEncoding enc);

   /// write the message using the specified writer function
   bool WriteMessage(soutr_t writer, void *where);

   /// sets one address field in the envelope
   void SetAddressField(ADDRESS **pAdr, const String& address);

   /// filters out erroneous addresses
   void CheckAddressFieldForErrors(ADDRESS *adr);

   /// get the iterator pointing to the given header or m_extraHeaders.end()
   MessageHeadersList::iterator FindHeaderEntry(const String& name) const;

   /**
      Return the password needed to login to the SMTP server.

      This function should be only called if m_UserName is non empty and must
      be used instead of accessing m_Password directly because we might not
      have it. In this case this function shows an interactive dialog asking
      the user for the password which also means that it can block.

      @param password the string filled with the password on successful return
      @return true if ok, false if the user cancelled the password dialog
    */
   bool GetPassword(String& password) const;

   /**
       Preview message before sending it if necessary.

       If the appropriate option is set, this function will show show the
       message preview and also ask the user to confirm sending it if
       necessary.

       Normally this doesn't do anything at all as default value for both
       options is false.

       @return true if it's ok to continue with sending the message or false if
         it was cancelled by user.
    */
   bool PreviewBeforeSending();

private:
   /** @name Description of the message being sent */
   //@{

   /// the envelope
   ENVELOPE *m_Envelope;

   /**
      The top level part.

      We only used the BODY member of this part but using a PART (which
      contains a BODY and a pointer to the next PART) allows us to efficiently
      change the structure of the message by just switching pointers and
      without copying any data. E.g. the first part added to this message
      becomes m_partTop but if another part is added later we change m_partTop
      to be a MULTIPART/MIXED part and set its nested part pointer to the
      previous value. And if we need to sign the message at the end it's again
      enough to simply replace m_partTop with a MULTIPART/SIGNED part and move
      the previous m_partTop value to be its nested part.
    */
   PART *m_partTop;

   /// Return the message BODY structure.
   BODY *GetBody() const { return &m_partTop->body; }

   //@}

   /// the profile containing our settings
   const Profile * const m_profile;

   /// server name to use
   String m_ServerHost;

   /// for servers requiring authentication
   String m_UserName,
          m_Password;

#ifdef USE_SSL
   /// use SSL ?
   SSLSupport m_UseSSLforSMTP,
              m_UseSSLforNNTP;

   /// check validity of ssl-cert? <-> self-signed certs
   SSLCert m_UseSSLUnsignedforSMTP,
           m_UseSSLUnsignedforNNTP;
#endif // USE_SSL

   /// has the message been properly created (set by Build())?
   bool m_wasBuilt;

   /** @name Address fields
    */
   //@{

   /// the full From: address
   String m_From;

   /// the full value of Reply-To: header (may be empty)
   String m_ReplyTo;

   /// the full value of Sender: header (may be empty)
   String m_Sender;

   /// the saved value of Bcc: set by call to SetAddresses()
   String m_Bcc;
   //@}

   /// if not empty, name of xface file
   String m_XFaceFile;

   /// Outgoing folder name or empty
   String m_OutboxName;

   /// Default charset
   String m_CharSet;

   /// default hostname
   String m_DefaultHost;

#ifdef OS_UNIX
   /// command for Sendmail if needed
   String m_SendmailCmd;
#endif // OS_UNIX

   /// the header encoding (wxFONTENCODING_SYSTEM if none)
   wxFontEncoding m_encHeaders;

   /// Protocol used for sending
   Protocol m_Protocol;

   /** @name variables managed by Build() */
   //@{

   /**
     m_headerNames and m_headerValues are the "final" arrays of headers, i.e.
     they are passed to c-client when we send the message while the extra
     headers are used to construct them.
    */

   /// names of header lines
   const char **m_headerNames;
   /// values of header lines
   const char **m_headerValues;

   /// extra headers to be added before sending
   MessageHeadersList m_extraHeaders;

   /**
      True if we're a copy of an existing message, false if this is a new
      message.
    */
   bool m_cloneOfExisting;

   /// a list of folders to save copies of the message in after sending
   M_LIST_OWN(StringList, String) m_FccList;

   //@}

   /// the parent frame (only used for the dialogs)
   wxFrame *m_frame;


   /// @name Cryptographic stuff.
   //@{

   /// If true, we must sign the message. False by default.
   bool m_sign;

   /// The user name to sign the message as, only used if m_sign and can be
   /// empty even if m_sign is true.
   String m_signAsUser;

   //@}

   // it uses our ctor
   friend class SendMessage;

   // give it access to m_headerNames nad m_headerValues
   friend class Rfc822OutputRedirector;
};

#endif // _SENDMESSAGECC_H

