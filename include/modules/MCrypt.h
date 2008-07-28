///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/MCrypt.h: declaration of MCryptoEngine ABC
// Purpose:     MCryptoEngine defines the interface of a crypto engine
//              which can be used for en/decrypting messages and so on
// Author:      Carlos H. Bauer <chbauer@acm.org>
// Modified by: Vadim Zeitlin at 02.12.02 to integrate it with the rest of M
// Created:     2000
// CVS-ID:      $Id$
// Copyright:   (c) 2000-2002 M-Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MODULES_MCRYPT_H_
#define _MODULES_MCRYPT_H_

#include "MModule.h"

class MCryptoEngineOutputLog;

class WXDLLIMPEXP_FWD_CORE wxWindow;

/**
   MCryptoEngine defines an abstract interface for all supported cryptographic
   engines.

   Note that this class doesn't need a virtual dtor because the objects are
   never deleted anyhow -- they are only created and destroyed by
   MCryptoEngineFactory
 */
class MCryptoEngine
{
public:
   /// return codes for all the crypto operations
   enum Status
   {
      OK = 0,
      CANNOT_EXEC_PROGRAM,
      OPERATION_CANCELED_BY_USER,
      BAD_ARGUMENT_ERROR,           // bad input data format
      NONEXISTING_KEY_ERROR,        // no public key found

      SIGNATURE_EXPIRED_ERROR,      // expired signature in VerifySignature()
      SIGNATURE_UNTRUSTED_WARNING,  // valid signature from untrusted source
      SIGNATURE_ERROR,              // incorrect signature
      SIGNATURE_CHECK_ERROR,        // generic failure to verify signature

      DECRYPTION_ERROR,             // generic failure of decryption
      NO_DATA_ERROR,                // or no signature in VerifySignature()

      SIGN_ERROR,                   // generic signing error
      SIGN_UNKNOWN_MICALG,          // unknown hash algorithm used in Sign()

      NOT_IMPLEMENTED_ERROR,        // operation not implemented by this engine
      MAX_ERROR
   };

   /**
      @name Crypto operations

      All methods in this section return a Status enum code, in particular they
      all return OK (0) on success.

      They also all take an optional last parameter to collect the detailed
      output of the crypto engine into.
    */
   //@{

   /**
      Decrypts an encrypted message and returns the result.

      @param messageIn the encrypted text
      @param messageOut the decrypted text (only valid if OK is returned)
      @param log the object to place output log into, if not NULL
      @return Status code, OK if decryption succeeded
    */
   virtual Status Decrypt(const String& messageIn,
                          String& messageOut,
                          MCryptoEngineOutputLog *log = NULL) = 0;

   /**
      This method encrypts and signs a message.

      To sign the message the function must receives valid
      {\bf user} and {\bf passphrase} arguments. If either of them is
      NULL or empty message signing will be disabled.

      @param recipient The ID of the public key used to encrypt the
      message. Usually is the email address of the recipient.

      @param messageIn The clear text message to be signed.

      @param messageOut A buffer where the encrypted message will be stored.

      @param user A pointer to the Id of the private key of the
      sender, usually is the email address used to create that key.

      @param log the object to place output log into, if not NULL
   */
   virtual Status Encrypt(const String& recipient,
                          const String& messageIn,
                          String &messageOut,
                          const String& user = wxEmptyString,
                          MCryptoEngineOutputLog *log = NULL) = 0;

   /**
      Just signs the message.

      Takes the user id to use and creates a signed message on output.

      Notice that to create an RFC 3156 compliant message the caller must
      retrieve the name of the hashing ("Message Integrity Check") algorithm
      used during signing using MCryptoEngineOutputLog::GetMicAlg().

      @param user
         The user name to sign the message as, may be empty to use the default
         user name.
      @param messageIn
         The contents of the message to sign, including any MIME headers (see
         section 6.1 of the RFC 3156 for an example).
      @param messageOut
         Contains the input signature on successful return.
      @param log
         Optional sink for the messages generated during the operation.
    */
   virtual Status Sign(const String& user,
                       const String& messageIn,
                       String& messageOut,
                       MCryptoEngineOutputLog *log = NULL) = 0;

   /**
      Verifies the message signature.

      On success, returns OK and puts the message text without the signature in
      messageOut. If no signature was found, returns NO_SIG_ERROR.
      If the signature is good, but from an untrusted key, returns 
      SIGNATURE_UNTRUSTED_WARNING
    */
   virtual Status VerifySignature(const String& messageIn,
                                  String& messageOut,
                                  MCryptoEngineOutputLog *log = NULL) = 0;

   /**
      Verifies the detached signature.

      On success returns OK but does nothing else.
    */
   virtual Status
      VerifyDetachedSignature(const String& message,
                              const String& signature,
                              MCryptoEngineOutputLog *log = NULL) = 0;

   //@}

   /// @name Key management.
   //@{

   /**
      Attempt to retrieve the specified public key from a public key server.

      @param pk the public key to retrieve
      @param server the server to get it from
    */
   virtual Status GetPublicKey(const String& pk,
                               const String& server,
                               MCryptoEngineOutputLog *log) const = 0;

   //@}

   // TODO
#if 0
   /** @name Other */
   //@{

   virtual Status CheckRecipient(const String & recipient) const = 0;

   virtual Status GetFingerprint(String & fp) const = 0;

   /**
      Shows the engine configuration dialog.

      @param window the parent window for the dialog we show
    */
   //virtual void Configure(wxWindow *parent) = 0;

   //@}
#endif // 0

protected:
   /// virtual dtor
   virtual ~MCryptoEngine() { }
};

// ----------------------------------------------------------------------------
// MCryptoEngineOutputLog: collects the crypto engine execution log
// ----------------------------------------------------------------------------

class MCryptoEngineOutputLog
{
public:
   MCryptoEngineOutputLog(wxWindow *parent) { m_parent = parent; }

   void AddMessage(const String& line) { m_messages.Add(line); }

   size_t GetMessageCount() const { return m_messages.GetCount(); }
   const String& GetMessage(size_t n) const { return m_messages[n]; }

   const String& GetUserID() const { return m_userID; }
   void  SetUserID(const String& userID) { m_userID = userID; }

   const String& GetPublicKey() const { return m_pubkey; }
   void  SetPublicKey(const String& pubkey) { m_pubkey = pubkey; }

   /**
      Return the name of the hashing algorithm used for generating the
      signature.

      This only makes sense to use after calling MCryptoEngine::Sign().

      The name of the method comes from "micalg" parameter of multipart/signed
      in OpenPGP message format, which in turn is an abbreviation of "Message
      Integrity Check Algorithm".
    */
   const String& GetMicAlg() const { return m_micalg; }
   void SetMicAlg(const String& micalg) { m_micalg = micalg; }

   wxWindow *GetParent() const { return m_parent; }

private:
   // the parent window for the dialogs which we may shown
   wxWindow     *m_parent;

   // uninterpreted output of PGP program
   wxArrayString m_messages;

   // the user id mentioned in the PGP output
   String        m_userID;

   // the public key mentioned in the PGP output
   String        m_pubkey;

   // the name of the hash algorithm used for signing
   String        m_micalg;
};

// ----------------------------------------------------------------------------
// Loading the crypto engines from modules is done using MCryptoEngineFactory
// ----------------------------------------------------------------------------

class MCryptoEngineFactory : public MModule
{
public:
   /// returns a pointer to the crypton engine, should @b not be deleted by the
   /// caller
   virtual MCryptoEngine *Get() = 0;
};

/// the interface implemented by crypto modules
#define CRYPTO_ENGINE_INTERFACE "CryptoEngine"

/**
   This macro must be used inside the declaration of MCryptoEngine-derived
   classes.

   @param cname the name of the class in which declaration this macro is used
 */
#define DECLARE_CRYPTO_ENGINE(cname)                                          \
   public:                                                                    \
      static MCryptoEngine *Get()                                             \
      {                                                                       \
         static cname s_The ## cname ## Engine;                               \
         return &s_The ## cname ## Engine;                                    \
      }

/**
   This macro must be used anywehere in the implementation part.

   @param cname the name of the class in which declaration this macro is used
   @param desc     the short description shown in the filters dialog
   @param cpyright the module author/copyright string
 */
#define IMPLEMENT_CRYPTO_ENGINE(cname, desc, cpyright)                        \
   class cname ## Factory : public MCryptoEngineFactory                       \
   {                                                                          \
   public:                                                                    \
      virtual MCryptoEngine *Get() { return cname::Get(); }                   \
                                                                              \
      MMODULE_DEFINE();                                                       \
      DEFAULT_ENTRY_FUNC;                                                     \
   };                                                                         \
   MMODULE_BEGIN_IMPLEMENT(cname ## Factory, #cname,                          \
                           CRYPTO_ENGINE_INTERFACE, desc, "1.00")             \
      MMODULE_PROP("author", cpyright)                                        \
   MMODULE_END_IMPLEMENT(cname ## Factory)                                    \
   MModule *cname ## Factory::Init(int /* version_major */,                   \
                                   int /* version_minor */,                   \
                                   int /* version_release */,                 \
                                   MInterface * /* minterface */,             \
                                   int * /* errorCode */)                     \
   {                                                                          \
      return new cname ## Factory();                                          \
   }

#endif // _MODULES_MCRYPT_H_

