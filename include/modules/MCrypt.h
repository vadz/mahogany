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

class WXDLLEXPORT wxWindow;

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
      NO_PASSPHRASE,
      OUT_OF_ENV_SPACE_ERROR,
      BATCH_MODE_ERROR,
      BAD_ARGUMENT_ERROR,
      PROCESS_INTERRUPTED_ERROR,
      OUT_OF_MEM_ERROR,
      NONEXISTING_KEY_ERROR,
      KEYRING_ADD_ERROR,
      KEYRING_EXTRACT_ERROR,
      KEYRING_EDIT_ERROR,
      KEYRING_VIEW_ERROR,
      KEYRING_CHECK_ERROR,
      KEYRING_SIGNATURE_ERROR,
      SIGNATURE_EXPIRED_ERROR,
      SIGNATURE_UNTRUSTED_WARNING,
      SIGNATURE_ERROR,
      PUBLIC_KEY_ENCRIPTION_ERROR,
      ENCRYPTION_ERROR,
      COMPRESSION_ERROR,
      SIGNATURE_CHECK_ERROR,
      PUBLIC_KEY_DECRIPTION_ERROR,
      DECRYPTION_ERROR,
      DECOMPRESSION_ERROR,
      NO_DATA_ERROR,                // or no signature in VerifySignature()
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
                          const String& user = _T(""),
                          MCryptoEngineOutputLog *log = NULL) = 0;

   /**
      Just signs the message.

      Takes the user id to use and creates a signed message on output.
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

#if 0
   virtual Status CheckRecipient(const String & recipient) const = 0;

   virtual Status GetFingerprint(String & fp) const = 0;

   virtual Status GetPublicKey(String & pk) const = 0;
#endif // 0
   //@}

   /** @name Other */
   //@{

   /**
      Shows the engine configuration dialog.

      @param window the parent window for the dialog we show
    */
   //virtual void Configure(wxWindow *parent) = 0;

   //@}

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
#define CRYPTO_ENGINE_INTERFACE _T("CryptoEngine")

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
   MMODULE_BEGIN_IMPLEMENT(cname ## Factory, _T(#cname),                      \
                           CRYPTO_ENGINE_INTERFACE, desc, _T("1.00"))         \
      MMODULE_PROP(_T("author"), cpyright)                                    \
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

