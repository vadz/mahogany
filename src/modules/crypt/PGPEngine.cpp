///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/PGPEngine.cpp: implementation of MCryptoEngine using PGP
// Purpose:     this modules works by invoking a command line PGP or GPG tool
// Author:      Vadim Zeitlin (based on the original code by
//              Carlos H. Bauer <chbauer@acm.org>)
// Modified by:
// Created:     02.12.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
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
   #include "sysutil.h"
#endif //USE_PCH

#include "Mpers.h"

#include "modules/MCrypt.h"

#include <wx/file.h>
#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/textdlg.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_PGP_COMMAND;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_REMEMBER_PGP_PASSPHRASE;

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
   virtual Status Decrypt(const String& messageIn,
                          String& messageOut,
                          MCryptoEngineOutputLog *log);

   virtual Status Encrypt(const String& recipient,
                          const String& messageIn,
                          String &messageOut,
                          const String& user,
                          MCryptoEngineOutputLog *log);

   virtual Status Sign(const String& user,
                       const String& messageIn,
                       String& messageOut,
                       MCryptoEngineOutputLog *log);

   virtual Status VerifySignature(const String& messageIn,
                                  String& messageOut,
                                  MCryptoEngineOutputLog *log);

   virtual Status VerifyDetachedSignature(const String& message,
                                          const String& signature,
                                          MCryptoEngineOutputLog *log);

protected:
   /**
      Executes the tool with messageIn on stdin and puts stdout into messageOut
    */
   Status ExecCommand(const String& options,
                      const String& messageIn,
                      String& messageOut,
                      MCryptoEngineOutputLog *log);

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
   The format of "gnupg --status-fd" output as taken from the DETAILS file of
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

class PGPProcess : public wxProcess
{
public:
   PGPProcess() { m_done = false; Redirect(); }

   virtual void OnTerminate(int /* pid */, int /* status */)
   {
      m_done = true;
   }

   bool IsDone() const { return m_done; }

private:
   bool m_done;
};

PGPEngine::Status
PGPEngine::ExecCommand(const String& options,
                       const String& messageIn,
                       String& messageOut,
                       MCryptoEngineOutputLog *log)
{
   PGPProcess process;
   long pid = wxExecute
              (
               wxString::Format
               (
                "%s --status-fd=2 --command-fd 0 --output - -a %s",
                READ_APPCONFIG_TEXT(MP_PGP_COMMAND).c_str(),
                options.c_str()
               ),
               wxEXEC_ASYNC,
               &process
              );

   if ( !pid )
   {
      return CANNOT_EXEC_PROGRAM;
   }

   // if we have data to write to PGP stdin, do it
   wxOutputStream *in = process.GetOutputStream();
   CHECK( in, CANNOT_EXEC_PROGRAM, _T("where is PGP subprocess stdin?") );

   if ( !messageIn.empty() )
   {
      in->Write(messageIn.c_str(), messageIn.length());
      process.CloseOutput();
   }

   wxInputStream *out = process.GetInputStream(),
                 *err = process.GetErrorStream();

   wxTextInputStream errText(*err);

   Status status = MAX_ERROR;

   // the user hint and the passphrase
   String user,
          pass;

   messageOut.clear();
   char buf[4096];

   bool outEof = false,
        errEof = false;
   while ( !process.IsDone() || !outEof || !errEof )
   {
      wxYield();

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
            const char *pc;
            for ( pc = line.c_str(); *pc && !isspace(*pc); pc++ )
            {
               code += *pc;
            }

            if ( *pc )
               pc++; // skip the space/TAB

            if ( code == _T("GOODSIG") ||
                 code == _T("VALIDSIG") ||
                 code == _T("SIG_ID") ||
                 code == _T("DECRYPTION_OKAY") )
            {
               status = OK;
               wxLogStatus(_("Valid signature for public key \"%s\""), pc);

            }
            else if ( code == _T("EXPSIG") || code == _T("EXPKEYSIG") )
            {
               status = SIGNATURE_EXPIRED_ERROR;
               wxLogWarning(_("Expired signature for public key \"%s\""), pc);
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
            else if ( code == _T("USERID_HINT") )
            {
               // skip the key id
               while ( *pc && !isspace(*pc) )
                  pc++;

               if ( *pc )
                  pc++;

               // remember the user
               user = pc;
            }
            else if ( code == _T("NEED_PASSPHRASE") )
            {
               if ( user.empty() )
               {
                  FAIL_MSG( _T("got NEED_PASSPHRASE without USERID_HINT?") );

                  user = _("default user");
               }

               if ( !PassphraseManager::Get(user, pass) )
               {
                  status = OPERATION_CANCELED_BY_USER;

                  process.CloseOutput();

                  break;
               }
            }
            else if ( code == _T("GOOD_PASSPHRASE") )
            {
               PassphraseManager::Unget(user, pass);
            }
            else if ( code == _T("BAD_PASSPHRASE") )
            {
               wxLogWarning(_("The passphrase you entered was invalid."));
            }
            else if ( code == _T("MISSING_PASSPHRASE") )
            {
               wxLogError(_("Passphrase for the user \"%s\" unavailable."),
                          user.c_str());
            }
            else if ( code == _T("DECRYPTION_FAILED") )
            {
               status = DECRYPTION_ERROR;
            }
            else if ( code == _T("GET_BOOL") ||
                      code == _T("GET_LINE") ||
                      code == _T("GET_HIDDEN") )
            {
               // give gpg whatever it's asking for, otherwise we'd deadlock!
               if ( code == _T("GET_HIDDEN") )
               {
                  if ( strcmp(pc, _T("passphrase.enter")) == 0 )
                  {
                     // we're being asked for a passphrase
                     String pass2 = pass;
                     pass2 += wxTextFile::GetEOL();

                     in->Write(pass2.c_str(), pass2.length());
                  }
                  else
                  {
                     // TODO
                     FAIL_MSG( _T("unexpected GET_HIDDEN") );
                  }
               }
               else
               {
                  // TODO
                  FAIL_MSG( _T("unexpected GET_XXX") );
               }
            }
            else if ( code == _T("NO_PUBKEY") )
            {
               wxLogWarning(_("Failed to check signature: public key \"%s\" "
                              "not available."), pc);
            }
            else if ( code == _T("ENC_TO") ||
                      code == _T("NO_SECKEY") ||
                      code == _T("BEGIN_DECRYPTION") ||
                      code == _T("END_DECRYPTION") ||
                      code == _T("GOT_IT") )
            {
               // ignore these
            }
            else
            {
               wxLogWarning(_("Ignoring unexpected GnuPG status line: \"%s\""),
                            line.c_str());
            }
         }
         else // normal (free form) gpg output
         {
            // remember in the output log object if we have one
            if ( log )
            {
               log->AddMessage(line);
            }
         }
      }
   }

   ASSERT_MSG( status != MAX_ERROR, _T("GNUPG didn't return the status?") );

   // we must wait until the process terminates because its termination handler
   // access process object which is going to be destroyed when we exit this
   // scope
   while ( !process.IsDone() )
      wxYield();

   return status;
}

// ----------------------------------------------------------------------------
// PGPEngine: encryption
// ----------------------------------------------------------------------------

PGPEngine::Status
PGPEngine::Decrypt(const String& messageIn,
                   String& messageOut,
                   MCryptoEngineOutputLog *log)
{
   // as wxExecute() doesn't allow redirecting anything but stdin/out/err we
   // have no way to pass both the encrypted data and the passphrase to PGP on
   // the same stream -- and so we must use a temp file :-(
   MTempFileName tmpfname;
   bool ok = tmpfname.IsOk();
   if ( ok )
   {
      wxFile file(tmpfname.GetName(), wxFile::write);
      ok = file.IsOpened() && file.Write(messageIn);
   }

   if ( !ok )
   {
      wxLogError(_("Can't pass the encrypted data to PGP."));

      return CANNOT_EXEC_PROGRAM;
   }

   return ExecCommand(tmpfname.GetName(), _T(""), messageOut, log);
}


PGPEngine::Status
PGPEngine::Encrypt(const String& recipient,
                   const String& messageIn,
                   String &messageOut,
                   const String& user,
                   MCryptoEngineOutputLog *log)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}

// ----------------------------------------------------------------------------
// PGPEngine: signing
// ----------------------------------------------------------------------------

PGPEngine::Status
PGPEngine::Sign(const String& user,
                const String& messageIn,
                String& messageOut,
                MCryptoEngineOutputLog *log)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}


PGPEngine::Status
PGPEngine::VerifySignature(const String& messageIn,
                           String& messageOut,
                           MCryptoEngineOutputLog *log)
{
   return ExecCommand(_T(""), messageIn, messageOut, log);
}

PGPEngine::Status
PGPEngine::VerifyDetachedSignature(const String& message,
                                   const String& signature,
                                   MCryptoEngineOutputLog *log)
{
   FAIL_MSG( _T("TODO") );

   return NOT_IMPLEMENTED_ERROR;
}

// ============================================================================
// PassphraseManager
// ============================================================================

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
