///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/PGPEngine.cpp: implementation of MCryptoEngine using PGP
// Purpose:     this modules works by invoking a command line PGP or GPG tool
// Author:      Vadim Zeitlin (based on the original code by
//              Carlos H. Bauer <chbauer@acm.org>)
// Modified by:
// Created:     02.12.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>Á
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
#endif //USE_PCH

#include "Mpers.h"

#include "modules/MCrypt.h"

#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/textdlg.h>

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_REMEMBER_PGP_PASSPHRASE;

// ----------------------------------------------------------------------------
// MPassphrase: not used currently
// ----------------------------------------------------------------------------

#if 0

/**
  MPassphrase represents a passphrase needed to unlock a private key.

  MPassphrase is basicly just a string which is either stored in memory
  (insecure) or which the user is asked about every time it is needed. Each
  pass phrase is associated with a given user id.
 */
class MPassphrase
{
public:
   MPassphrase() { m_store = false; }

   /**
     Returns the passphrase -- called by MCryptoEngine when needed.

     This can either return the stored string or ask the user. In either case,
     Unget() must be called later if the passphrase was correct or Get() should
     be called again if it was not.

     @param user the user id of the user whose passphrase is requested
     @param passphrase is filled with the pass phrase if true is returned
     @return true if ok, false if user cancelled entering the passphrase
    */
   bool Get(const String& user, String& passphrase);

   /**
     Forget or store the passphrase previously returned by Get().

     @param passphrase the pass phrase previously returned by Get()
    */
   void Unget(const String& passphrase);

private:
   /// the user the passphrase was last requested for
   String m_user;

   /// the stored passphrase if m_store is true
   String m_passphrase;

   /// if true, we have a valid stored pass phrase
   bool m_store;
};

#endif // 0

// ----------------------------------------------------------------------------
// PassphraseManager: this class can be used to remember the passphrases
//                    instead of asking the user to reenter them again and
//                    again (and again...)
// ----------------------------------------------------------------------------

WX_DECLARE_STRING_HASH_MAP(String, UserPassMap);

class PassphraseManager
{
public:
   /**
     Get the passphrase for the given user id.

     If we don't have this passphrase stored, ask the user for it.

     @param user the user id to ask for the passphrase for
     @param passphrase is filled with the passphrase if true is returned
     @return true if the user entered the passphrase or we already had it
             or false if the user cancelled the dialog
    */
   static bool Get(const String& user, String& passphrase);

   /**
     Remember the passphrase.

     Ask the user if he wants to store this passphrase and do it if he agrees.
     If the user doesn't want to store the passphrase, its contents is
     overwritten in memory.

     @param user the user id to ask for the passphrase for
     @param passphrase the passphrase to store (presumably correct...)
    */
   static void Unget(const String& user, String& passphrase);

private:
   // this map stores user->passphrase pairs
   static UserPassMap m_map;
};

// ----------------------------------------------------------------------------
// PGPEngine class
// ----------------------------------------------------------------------------

class PGPEngine : public MCryptoEngine
{
public:
   // implement the base class pure virtuals
   virtual int Decrypt(const String& user,
                       const String& messageIn,
                       String& messageOut);

   virtual int Encrypt(const String& recipient,
                       const String& messageIn,
                       String &messageOut,
                       const String& user);

   virtual int Sign(const String& user,
                    const String& messageIn,
                    String& messageOut);

   virtual int VerifySignature(const String& messageIn,
                               String& messageOut);

protected:
   /**
      Executes the tool with messageIn on stdin and puts stdout into messageOut
    */
   int ExecCommand(const String& messageIn, String& messageOut);

private:
   DECLARE_CRYPTO_ENGINE(PGPEngine)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// PGPEngine
// ----------------------------------------------------------------------------

IMPLEMENT_CRYPTO_ENGINE(PGPEngine,
                        _("PGP/GPG crypto engine"),
                        _T("(c) 2002 Vadim Zeitlin <vadim@wxwindows.org>"));

// ----------------------------------------------------------------------------
// PGPEngine: running the external tool
// ----------------------------------------------------------------------------

/*
   The format og "gnupg --status-fd" output as taken from the DETAILS file of
   gnupg distribution:

Format of the "--status-fd" output
==================================
Every line is prefixed with "[GNUPG:] ", followed by a keyword with
the type of the status line and a some arguments depending on the
type (maybe none); an application should always be prepared to see
more arguments in future versions.


    GOODSIG        <long keyid>  <username>
        The signature with the keyid is good.  For each signature only
        one of the three codes GOODSIG, BADSIG or ERRSIG will be
        emitted and they may be used as a marker for a new signature.
        The username is the primary one encoded in UTF-8 and %XX
        escaped.

    EXPSIG        <long keyid>  <username>
        The signature with the keyid is good, but the signature is
        expired. The username is the primary one encoded in UTF-8 and
        %XX escaped.

    EXPKEYSIG        <long keyid>  <username>
        The signature with the keyid is good, but the signature was
        made by an expired key. The username is the primary one
        encoded in UTF-8 and %XX escaped.

    BADSIG        <long keyid>  <username>
        The signature with the keyid has not been verified okay.
        The username is the primary one encoded in UTF-8 and %XX
        escaped.

    ERRSIG  <long keyid>  <pubkey_algo> <hash_algo> \
            <sig_class> <timestamp> <rc>
        It was not possible to check the signature.  This may be
        caused by a missing public key or an unsupported algorithm.
        A RC of 4 indicates unknown algorithm, a 9 indicates a missing
        public key. The other fields give more information about
        this signature.  sig_class is a 2 byte hex-value.

    VALIDSIG        <fingerprint in hex> <sig_creation_date> <sig-timestamp>
                <expire-timestamp>

        The signature with the keyid is good. This is the same
        as GOODSIG but has the fingerprint as the argument. Both
        status lines are emitted for a good signature.
        sig-timestamp is the signature creation time in seconds after
        the epoch. expire-timestamp is the signature expiration time
        in seconds after the epoch (zero means "does not expire").

    SIG_ID  <radix64_string>  <sig_creation_date>  <sig-timestamp>
        This is emitted only for signatures of class 0 or 1 which
        have been verified okay.  The string is a signature id
        and may be used in applications to detect replay attacks
        of signed messages.  Note that only DLP algorithms give
        unique ids - others may yield duplicated ones when they
        have been created in the same second.

    ENC_TO  <long keyid>  <keytype>  <keylength>
        The message is encrypted to this keyid.
        keytype is the numerical value of the public key algorithm,
        keylength is the length of the key or 0 if it is not known
        (which is currently always the case).

    NODATA  <what>
        No data has been found. Codes for what are:
            1 - No armored data.
            2 - Expected a packet but did not found one.
            3 - Invalid packet found, this may indicate a non OpenPGP message.
        You may see more than one of these status lines.

    UNEXPECTED <what>
        Unexpected data has been encountered
            0 - not further specified               1


    TRUST_UNDEFINED <error token>
    TRUST_NEVER  <error token>
    TRUST_MARGINAL
    TRUST_FULLY
    TRUST_ULTIMATE
        For good signatures one of these status lines are emitted
        to indicate how trustworthy the signature is.  The error token
        values are currently only emiited by gpgsm.

    SIGEXPIRED
        This is deprecated in favor of KEYEXPIRED.

    KEYEXPIRED <expire-timestamp>
        The key has expired.  expire-timestamp is the expiration time
        in seconds after the epoch.

    KEYREVOKED
        The used key has been revoked by its owner.  No arguments yet.

    BADARMOR
        The ASCII armor is corrupted.  No arguments yet.

    RSA_OR_IDEA
        The IDEA algorithms has been used in the data.  A
        program might want to fallback to another program to handle
        the data if GnuPG failed.  This status message used to be emitted
        also for RSA but this has been dropped after the RSA patent expired.
        However we can't change the name of the message.

    SHM_INFO
    SHM_GET
    SHM_GET_BOOL
    SHM_GET_HIDDEN

    GET_BOOL
    GET_LINE
    GET_HIDDEN
    GOT_IT

    NEED_PASSPHRASE <long main keyid> <long keyid> <keytype> <keylength>
        Issued whenever a passphrase is needed.
        keytype is the numerical value of the public key algorithm
        or 0 if this is not applicable, keylength is the length
        of the key or 0 if it is not known (this is currently always the case).

    NEED_PASSPHRASE_SYM <cipher_algo> <s2k_mode> <s2k_hash>
        Issued whenever a passphrase for symmetric encryption is needed.

    MISSING_PASSPHRASE
        No passphrase was supplied.  An application which encounters this
        message may want to stop parsing immediately because the next message
        will probably be a BAD_PASSPHRASE.  However, if the application
        is a wrapper around the key edit menu functionality it might not
        make sense to stop parsing but simply ignoring the following
        BAD_PASSPHRASE.

    BAD_PASSPHRASE <long keyid>
        The supplied passphrase was wrong or not given.  In the latter case
        you may have seen a MISSING_PASSPHRASE.

    GOOD_PASSPHRASE
        The supplied passphrase was good and the secret key material
        is therefore usable.

    DECRYPTION_FAILED
        The symmetric decryption failed - one reason could be a wrong
        passphrase for a symmetrical encrypted message.

    DECRYPTION_OKAY
        The decryption process succeeded.  This means, that either the
        correct secret key has been used or the correct passphrase
        for a conventional encrypted message was given.  The program
        itself may return an errorcode because it may not be possible to
        verify a signature for some reasons.

    NO_PUBKEY  <long keyid>
    NO_SECKEY  <long keyid>
        The key is not available

    IMPORTED   <long keyid>  <username>
        The keyid and name of the signature just imported

    IMPORT_RES <count> <no_user_id> <imported> <imported_rsa> <unchanged>
        <n_uids> <n_subk> <n_sigs> <n_revoc> <sec_read> <sec_imported> <sec_dups>
        Final statistics on import process (this is one long line)

    FILE_START <what> <filename>
        Start processing a file <filename>.  <what> indicates the performed
        operation:
            1 - verify
            2 - encrypt
            3 - decrypt

    FILE_DONE
        Marks the end of a file processing which has been started
        by FILE_START.

    BEGIN_DECRYPTION
    END_DECRYPTION
        Mark the start and end of the actual decryption process.  These
        are also emitted when in --list-only mode.

    BEGIN_ENCRYPTION  <mdc_method> <sym_algo>
    END_ENCRYPTION
        Mark the start and end of the actual encryption process.

    DELETE_PROBLEM reason_code
        Deleting a key failed.        Reason codes are:
            1 - No such key
            2 - Must delete secret key first

    PROGRESS what char cur total
        Used by the primegen and Public key functions to indicate progress.
        "char" is the character displayed with no --status-fd enabled, with
        the linefeed replaced by an 'X'.  "cur" is the current amount
        done and "total" is amount to be done; a "total" of 0 indicates that
        the total amount is not known.        100/100 may be used to detect the
        end of operation.

    SIG_CREATED <type> <pubkey algo> <hash algo> <class> <timestamp> <key fpr>
        A signature has been created using these parameters.
            type:  'D' = detached
                   'C' = cleartext
                   'S' = standard
                   (only the first character should be checked)
            class: 2 hex digits with the signature class

    KEY_CREATED <type>
        A key has been created
            type: 'B' = primary and subkey
                  'P' = primary
                  'S' = subkey

    SESSION_KEY  <algo>:<hexdigits>
        The session key used to decrypt the message.  This message will
        only be emmited when the special option --show-session-key
        is used.  The format is suitable to be passed to the option
        --override-session-key

    NOTATION_NAME <name>
    NOTATION_DATA <string>
        name and string are %XX escaped; the data may be splitted
        among several notation_data lines.

    USERID_HINT <long main keyid> <string>
        Give a hint about the user ID for a certain keyID.

    POLICY_URL <string>
        string is %XX escaped

    BEGIN_STREAM
    END_STREAM
        Issued by pipemode.

    INV_RECP <reason> <requested_recipient>
        Issued for each unusable recipient. The reasons codes
        currently in use are:
          0 := "No specific reason given".
          1 := "Not Found"
          2 := "Ambigious specification"

    NO_RECP <reserved>
        Issued when no recipients are usable.

    ALREADY_SIGNED <long-keyid>
        Warning: This is experimental and might be removed at any time.

    TRUNCATED <maxno>
        The output was truncated to MAXNO items.  This status code is issued
        for certain external requests

    ERROR <error location> <error code>
        This is a generic error status message, it might be followed
        by error location specific data. <error token> and
        <error_location> should not contain a space.

    ATTRIBUTE <fpr> <octets> <type> <index> <count>
              <timestamp> <expiredate> <flags>
        This is one long line issued for each attribute subpacket when
        an attribute packet is seen during key listing.  <fpr> is the
        fingerprint of the key. <octets> is the length of the
        attribute subpacket. <type> is the attribute type
        (1==image). <index>/<count> indicates that this is the Nth
        indexed subpacket of count total subpackets in this attribute
        packet.  <timestamp> and <expiredate> are from the
        self-signature on the attribute packet.  If the attribute
        packet does not have a valid self-signature, then the
        timestamp is 0.  <flags> are a bitwise OR of:
                0x01 = this attribute packet is a primary uid
                0x02 = this attribute packet is revoked
                0x04 = this attribute packet is expired
 */
int
PGPEngine::ExecCommand(const String& messageIn, String& messageOut)
{
   wxProcess *process = wxProcess::Open
                        (
                           wxString::Format
                           (
                            "%s --status-fd=2",
                            "G:\\Internet\\PGP\\GPG-1.2.1\\gpg.exe" // TODO
                           )
                        );
   if ( !process )
   {
      return CANNOT_EXEC_PROGRAM;
   }

   process->GetOutputStream()->Write(messageIn.c_str(), messageIn.length());
   process->CloseOutput();

   wxInputStream *out = process->GetInputStream(),
                 *err = process->GetErrorStream();

   wxTextInputStream errText(*err);

   Status status = MAX_ERROR;

   messageOut.clear();
   char buf[4096];

   bool outEof = false,
        errEof = false;
   while ( !outEof && !errEof )
   {
      if ( out->GetLastError() == wxSTREAM_EOF )
      {
         outEof = true;
      }
      else if ( out->CanRead() )
      {
         buf[out->Read(buf, WXSIZEOF(buf)).LastRead()] = '\0';

         messageOut += buf;
      }

      if ( err->GetLastError() == wxSTREAM_EOF )
      {
         errEof = true;
      }
      else if ( err->CanRead() )
      {
         String line = errText.ReadLine();
         if ( line.StartsWith(_T("[GNUPG:] "), &line) )
         {
            String code;
            for ( const char *pc = line.c_str(); !isspace(*pc); pc++ )
            {
               code += *pc;
            }

            if ( code == _T("GOODSIG") ||
                 code == _T("VALIDSIG") ||
                 code == _T("SIG_ID") )
            {
               // signature checked ok
               status = OK;
            }
            else if ( code == _T("EXPSIG") || code == _T("EXPKEYSIG") )
            {
               // FIXME: at least give a warning
               status = OK;
            }
            else if ( code == _T("BADSIG") )
            {
               status = SIGNATURE_ERROR;
            }
            else if ( code == _T("ERRSIG") )
            {
               // TODO: analyze the last field
               status = SIGNATURE_CHECK_ERROR;
            }
            else if ( code == _T("NODATA") )
            {
               status = NO_SIG_ERROR;
            }
            else if ( code.StartsWith(_T("TRUST_")) )
            {
               // TODO: something
            }
            else
            {
               wxLogWarning(_("Ignoring unexpected GnuPG status line: \"%s\""),
                            line.c_str());
            }
         }
         //else: some babble, ignore
      }
   }

   ASSERT_MSG( status != MAX_ERROR, _T("GNUPG didn't return the status?") );

   return status;
}

// ----------------------------------------------------------------------------
// PGPEngine: encryption
// ----------------------------------------------------------------------------

int
PGPEngine::Decrypt(const String& user,
                   const String& messageIn,
                   String& messageOut)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}


int
PGPEngine::Encrypt(const String& recipient,
                   const String& messageIn,
                   String &messageOut,
                   const String& user)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}

// ----------------------------------------------------------------------------
// PGPEngine: signing
// ----------------------------------------------------------------------------

int
PGPEngine::Sign(const String& user,
                const String& messageIn,
                String& messageOut)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}


int
PGPEngine::VerifySignature(const String& messageIn,
                           String& messageOut)
{
   return ExecCommand(messageIn, messageOut);
}

// ----------------------------------------------------------------------------
// PassphraseManager
// ----------------------------------------------------------------------------

UserPassMap PassphraseManager::m_map;

/* static */
bool
PassphraseManager::Get(const String& user, String& passphrase)
{
   // do we already have this passphrase?
   UserPassMap::iterator i = m_map.find(user);
   if ( i != m_map.end() )
   {
      passphrase = i->second;

      return true;
   }

   // no, ask the user
   //
   // note that we can't directly use wxGetPasswordFromUser() because it
   // doesn't distinguish between cancelling and enteting an empty string
   wxTextEntryDialog dialog(NULL,
                            wxString::Format
                            (
                              _("Passphrase is required to unlock the "
                                "secret key for \n"
                                "user \"%s\":"), user.c_str()
                            ),
                            _("Mahogany: Please enter the passphrase"),
                            _T(""),
                            wxOK | wxCANCEL | wxTE_PASSWORD);

    if ( dialog.ShowModal() != wxID_OK )
    {
       // cancelled by user
       return false;
    }

    passphrase = dialog.GetValue();

    return true;
}

/* static */
void
PassphraseManager::Unget(const String& user, String& passphrase)
{
   // first check if we don't already have this passphrase stored
   UserPassMap::iterator i = m_map.find(user);
   if ( i == m_map.end() )
   {
      // ask the user if it's ok to store it

      // TODO: allow remembering different values for different passphrases?
      if ( MDialog_YesNoDialog
           (
            wxString::Format
            (
               _("Would you like to keep the passphrase for the "
                 "user \"%s\" in memory?"), user.c_str()
            ),
            NULL,
            _("Mahogany: Remember the passphrase?"),
            M_DLG_NO_DEFAULT,
            M_MSGBOX_REMEMBER_PGP_PASSPHRASE
           ) )
      {
         m_map[user] = passphrase;
      }
      else // user doesn't want to remember it
      {
         // make sure we overwrite it in memory
         //
         // TODO: in fact, because of COW used by wxString we can't be 100%
         //       sure about this...
         const size_t len = passphrase.length();
         for ( size_t n = 0; n < len; n++ )
         {
            passphrase[n] = _T('\0');
         }
      }
   }
}

