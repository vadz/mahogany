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
   #include "Mdefaults.h"
   #include "MApplication.h"

   #include <wx/textdlg.h>
   #include <wx/hashmap.h>
#endif //USE_PCH

#include "modules/MCrypt.h"
#include "gui/wxMDialogs.h"

#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/txtstrm.h>

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_PGP_COMMAND;
extern const MOption MP_PGP_KEYSERVER;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_REMEMBER_PGP_PASSPHRASE;
extern const MPersMsgBox *M_MSGBOX_CONFIGURE_PGP_PATH;

// ----------------------------------------------------------------------------
// miscellaneous helper functions
// ----------------------------------------------------------------------------

namespace
{

void SkipSpaces(const wxChar *& pc)
{
   while ( *pc && wxIsspace(*pc) )
      pc++;
}

String ReadNumber(const wxChar *& pc)
{
   const wxChar * const start = pc;
   while ( *pc && ('0' <= *pc && *pc <= '9') )
      pc++;

   return String(start, pc - start);
}

} // anonymous namespace

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

   virtual Status GetPublicKey(const String& pk,
                               const String& server,
                               MCryptoEngineOutputLog *log) const;

protected:
   /**
      Executes PGP/GPG with messageIn on stdin and puts stdout into messageOut.

      @param options are the PGP/GPG program options
      @param messageIn will be written to child stdin
      @param messageOut will contain child stdout
      @param log may contain miscellaneous additional info
      @return the status code
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
                        "(c) 2002-2008 Vadim Zeitlin <vadim@wxwindows.org>");

// ----------------------------------------------------------------------------
// PGPEngine: running the external tool
// ----------------------------------------------------------------------------

/*
   The format of "gnupg --status-fd" output is described in the DETAILS file of
   gnupg distribution, see

   http://cvs.gnupg.org/cgi-bin/viewcvs.cgi/trunk/doc/DETAILS?rev=4820&root=GnuPG
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

   DECLARE_NO_COPY_CLASS(PGPProcess)
};

PGPEngine::Status
PGPEngine::ExecCommand(const String& options,
                       const String& messageIn,
                       String& messageOut,
                       MCryptoEngineOutputLog *log)
{
   PGPProcess process;
   long pid = 0;

   // check if we have a PGP command: it can be set to nothing to disable pgp
   // support
   String pgp = READ_APPCONFIG_TEXT(MP_PGP_COMMAND);
   bool pgpChanged = false;

   while ( !pid )
   {
      if ( !pgp.empty() )
      {
         // As we ask for the passphrase ourselves, disable the use of the
         // agent to avoid duplicate prompts. Alternative solution would be to
         // check if GPG_AGENT_INFO environment variable is set and not prompt
         // for the passphrase from Mahogany in this case, but it wouldn't work
         // if there were "no-use-agent" in gpg.conf while this solution always
         // works as the command line option overrides the config file one.
         const String
            command = wxString::Format
                      (
                       "%s --status-fd=2 --command-fd 0 "
                       "--output - -a --no-use-agent %s",
                       pgp,
                       options
                      );

         if ( log )
            log->AddMessage(String::Format(_("Executing \"%s\""), command));

         pid = wxExecute(command, wxEXEC_ASYNC, &process);
      }

      if ( pid )
         break;

      wxString msg;
      if ( pgp.empty() )
      {
         msg = _("PGP/GPG program location is not configured.");
      }
      else // have command but executing it failed
      {
         msg = String::Format(_("Failed to execute \"%s\"."), pgp);
      }

      msg += "\n";
      msg += _("If you have PGP/GPG installed on this system, would you "
               "like to specify its location now?"
               "\n"
               "If you don't, the current operation will be cancelled.");

      wxWindow * const parent = log ? log->GetParent() : NULL;
      if ( !MDialog_YesNoDialog
            (
               msg,
               parent,
               MDIALOG_YESNOTITLE,
               M_DLG_YES_DEFAULT,
               M_MSGBOX_CONFIGURE_PGP_PATH
            ) )
      {
         return CANNOT_EXEC_PROGRAM;
      }

      pgp = MDialog_FileRequester(_("Please choose PGP/GPG program"), parent);
      if ( pgp.empty() )
      {
         // cancelled by user
         return CANNOT_EXEC_PROGRAM;
      }

      pgpChanged = true;
   }

   // if we get here, we managed to launch the PGP subprocess successfully

   // if we had to change the PGP path in order to do it, remember the value
   // that worked for us
   if ( pgpChanged )
      mApplication->GetProfile()->writeEntry(MP_PGP_COMMAND, pgp);


   wxOutputStream *in = process.GetOutputStream();
   wxInputStream *out = process.GetInputStream(),
                 *err = process.GetErrorStream();

   CHECK( in && out && err, CANNOT_EXEC_PROGRAM,
            _T("where is PGP subprocess stdin/out/err?") );

   // Lines received from GPG are normally encoded in UTF-8, but they may
   // contain bytes sequences invalid in UTF-8 in practice, e.g. this happens
   // under Windows when the key description itself is not in UTF-8 and gpg
   // just seems to dump it on output directly. Because of this, we can't just
   // use wxConvUTF8 here but need to treat the input as raw bytes and then
   // carefully convert it wxString ourselves.
   wxTextInputStream errText(*err, " ", wxConvISO8859_1);
   wxMBConvUTF8 nonStrictUTF8(wxMBConvUTF8::MAP_INVALID_UTF8_TO_OCTAL);

   Status status = MAX_ERROR;

   // the user hint and the passphrase
   String user,
          pass;

   // the arrays of keys for which this message is encrypted and the number of
   // them for which we do _not_ have the private key -- if this number is
   // equal to the number of array elements it means that we have none of them
   wxArrayString keysEnc;
   size_t keysNoPrivate = 0;

   messageOut.clear();
   char bufOut[4096];

   wxCharBuffer bufIn(messageIn.ToUTF8());
   size_t lenIn = strlen(bufIn);
   const char *ptrIn = bufIn;

   // the value of the last NOTATION_NAME line, if any, and whether it's
   // critical
   wxString lastNotationName;
   bool lastNotationCritical = false;

   bool outEof = false,
        errEof = false;
   while ( !process.IsDone() || !outEof || !errEof )
   {
      wxYieldIfNeeded();

      // the order is important here, lest we deadlock: first get everything
      // GPG has for us and only then try to feed it more data
      if ( out->GetLastError() == wxSTREAM_EOF )
      {
         outEof = true;
      }
      else
      {
         while ( out->CanRead() )
         {
            // leave space for terminating NUL
            bufOut[out->Read(bufOut, WXSIZEOF(bufOut) - 1).LastRead()] = '\0';

            messageOut += wxString::FromUTF8(bufOut);
         }
      }

      if ( lenIn )
      {
         const size_t CHUNK_SIZE = 4096;
         in->Write(ptrIn, lenIn > CHUNK_SIZE ? CHUNK_SIZE : lenIn);

         const size_t lenChunk = in->LastWrite();

         lenIn -= lenChunk;
         if ( !lenIn )
         {
            // we don't have anything more to write, so close the stream
            process.CloseOutput();
            in = NULL;
         }
         else
         {
            ptrIn += lenChunk;
         }
      }

      if ( err->GetLastError() == wxSTREAM_EOF )
      {
         errEof = true;
      }
      else if ( err->CanRead() )
      {
         String line(errText.ReadLine().To8BitData(), nonStrictUTF8);

         // Log all GPG messages, this is useful for diagnosing problems
         if ( log )
            log->AddMessage(line);

         /*
            Some typical message sequences:

            + For signed messages:
               - SIG_ID
               - GOODSIG
               - VALIDSIG
               - TRUST_UNDEFINED

            + For encrypted messages:
               - ENC_TO
               - USERID_HINT
               - NEED_PASSPHRASE (only if the key needs it, of course, this
                 will be the last message if the user doesn't enter a valid
                 pass phrase)
               - GET_HIDDEN
               - GOT_IT
               - GOOD_PASSPHRASE
               - BEGIN_DECRYPTION
               - PLAINTEXT
               - DECRYPTION_OKAY
               - GOODMDC
               - END_DECRYPTION

          */
         if ( line.StartsWith(_T("[GNUPG:] "), &line) )
         {
            String code;
            const wxChar *pc;
            for ( pc = line.c_str(); *pc && !wxIsspace(*pc); pc++ )
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
               if ( status != SIGNATURE_EXPIRED_ERROR )
               {
                  wxLogStatus(_("Valid signature from \"%s\""),
                              log->GetUserID());
                  status = OK;
               }
            }
            else if ( code == _T("BADARMOR") )
            {
               status = BAD_ARGUMENT_ERROR;
               wxLogWarning(_("The PGP message is malformed, "
                              "processing aborted."));
               // If the message is not correctly formatted, GPG does not
               // output the resulting message, so we must copy the input
               // into the output
               messageOut = messageIn;
            }
            else if ( code == _T("EXPSIG") || code == _T("EXPKEYSIG") )
            {
               wxLogWarning(_("Expired signature from \"%s\""), pc);

               status = SIGNATURE_EXPIRED_ERROR;
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
               status = NO_DATA_ERROR;
            }
            else if ( code.StartsWith(_T("TRUST_")) )
            {
               if ( code == _T("TRUST_UNDEFINED") ||
                    code == _T("TRUST_NEVER") )
               {
                  status = SIGNATURE_UNTRUSTED_WARNING;
                  wxLogStatus(_("Valid signature from (invalid) \"%s\""),
                              log->GetUserID());
               }
               // else: "_MARGINAL, _FULLY and _ULTIMATE" do not trigger a warning
            }
            else if ( code == _T("USERID_HINT") )
            {
               // skip the key id
               while ( *pc && !wxIsspace(*pc) )
                  pc++;

               SkipSpaces(pc);

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
                          user);
            }
            else if ( code == _T("DECRYPTION_FAILED") )
            {
               status = DECRYPTION_ERROR;
            }
            else if ( code == _T("GET_BOOL") ||
                      code == _T("GET_LINE") ||
                      code == _T("GET_HIDDEN") )
            {
               if ( !in )
               {
                  wxLogError(_("Unexpected PGP request \"%s\"."), code);
                  break;
               }

               // give gpg whatever it's asking for, otherwise we'd deadlock!
               if ( code == _T("GET_HIDDEN") )
               {
                  if ( wxStrcmp(pc, _T("passphrase.enter")) == 0 )
                  {
                     // we're being asked for a passphrase
                     const String passNL = pass + wxTextFile::GetEOL();

                     const wxWX2MBbuf buf(passNL.ToUTF8());
                     const size_t len = strlen(buf);
                     in->Write(buf, len);
                     if ( in->LastWrite() != len )
                     {
                        wxLogSysError(_("Failed to pass pass phrase to PGP: "));
                     }
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
               log->SetPublicKey(pc);              // till the end of line

               status = NONEXISTING_KEY_ERROR;
            }
            else if ( code == _T("IMPORTED") )
            {
               const wxChar * const pSpace = wxStrchr(pc, _T(' '));
               if ( pSpace )
               {
                  log->SetPublicKey(String(pc, pSpace));
                  log->SetUserID(pSpace + 1);      // till the end of line

                  status = OK;
               }
               else // no space in "IMPORTED" line?
               {
                  wxLogDebug(_T("Weird IMPORTED reply: %s"), pc);
               }
            }
            else if ( code == _T("NO_SECKEY") )
            {
               // it's ok if we have no private key for some of the keys used
               // to encrypt this message, we need to have only one private key
               // to be able to decrypt it, so just remember the keys which we
               // may use to decrypt this message for now and only log an error
               // if we don't have any of them when decryption starts
               keysNoPrivate++;
            }
            else if ( code == _T("SIG_CREATED") )
            {
               status = OK;

               // extract hash algorithm, the caller needs it to create OpenPGP
               // message

               // the format of this line is:
               //
               // SIG_CREATED <type> <pkalg> <micalg> <cls> <timestamp> <fp>
               //
               // where <type> is one of 'D' (detached), 'C' (clear text) and
               // 'S' (standard); pkalg is a number (typical value is 17 for
               // DSA) and the known hash algorithm values are below, see
               // http://cvs.gnupg.org/cgi-bin/viewcvs.cgi/trunk/include/cipher.h
               static const struct
               {
                  unsigned micalg;
                  const char *name;
               } micalgNames[] =
               {
                  {  1, "md5" },
                  {  2, "sha1" },
                  {  3, "ripemd160" },
                  {  8, "sha256" },
                  {  9, "sha384" },
                  { 10, "sha512" },
               };

               String errmsg;
               if ( *pc++ != 'D' )
               {
                  errmsg.Printf(_("unexpected signature type '%c'"), pc[-1]);
               }
               else
               {
                  SkipSpaces(pc);

                  const String pkalg(ReadNumber(pc));
                  unsigned long n;
                  if ( !pkalg.ToULong(&n) )
                  {
                     errmsg.Printf(_("unexpected public key algorithm \"%s\""),
                                   pkalg);
                  }
                  else
                  {
                     SkipSpaces(pc);

                     const String micalg(ReadNumber(pc));
                     if ( !micalg.ToULong(&n) )
                     {
                        errmsg.Printf(_("unexpected hash algorithm \"%s\""),
                                      micalg);
                     }
                     else
                     {
                        bool found = false;
                        for ( size_t i = 0; i < WXSIZEOF(micalgNames); i++ )
                        {
                           if ( micalgNames[i].micalg == n )
                           {
                              if ( log )
                                 log->SetMicAlg(micalgNames[i].name);

                              found = true;
                              status = OK;
                              break;
                           }
                        }

                        if ( !found )
                        {
                           errmsg.Printf(_("unsupported hash algorithm \"%s\", "
                                           "please configure GPG to use a hash "
                                           "algorithm compatible with RFC 3156"),
                                         micalg);

                           status = SIGN_UNKNOWN_MICALG;
                        }
                     }
                  }
               }

               if ( !errmsg.empty() )
               {
                  // don't overwrite a more specific error code if set above
                  if ( status != SIGN_UNKNOWN_MICALG )
                     status = SIGN_ERROR;

                  wxLogError(_("Failed to sign message: %s"), errmsg);
               }
            }
            else if ( code == _T("BEGIN_SIGNING") )
            {
               // this indicates that we don't need to send anything more to
               // GPG, check that we did send everything
               ASSERT_MSG( !lenIn, "should have sent everything by now" );
            }
            else if ( code == _T("ENC_TO") )
            {
               keysEnc.push_back(pc);
            }
            else if ( code == _T("BEGIN_DECRYPTION") )
            {
               if ( !keysEnc.empty() &&
                        keysNoPrivate == keysEnc.size() )
               {
                  wxString keys = keysEnc.front();
                  for ( size_t n = 1; n < keysNoPrivate - 1; n++ )
                     keys << _(" or ") << keysEnc[n];
                  if ( keysNoPrivate > 1 )
                     keys += keysEnc[keysNoPrivate - 1];

                  wxLogWarning(_("No secret key which can decrypt this message "
                                 "(%s) is available."), keys);
               }
            }
            else if ( code == "NOTATION_NAME" )
            {
               lastNotationName = pc;
               lastNotationCritical = false;
            }
            else if ( code == "NOTATION_FLAGS" )
            {
               if ( *pc == '1' )
                  lastNotationCritical = true;

               // we ignore the "human readable" flag because it's not clear
               // how exactly should it be handled
            }
            else if ( code == "NOTATION_DATA" )
            {
               const wxString data(pc);
               if ( lastNotationCritical )
               {
                  wxLogWarning(_("Critical notation in the signature: %s=%s"),
                               lastNotationName, data);
               }
            }
            else if ( code == _T("END_DECRYPTION") ||
                      code == _T("GOODMDC") ||     // what does it mean?
                      code == _T("GOT_IT") ||
                      code == _T("SIGEXPIRED") || // we will give a warning
                      code == _T("KEYEXPIRED") || // when we get EXPKEYSIG
                      code == _T("KEY_CONSIDERED") ||
                      code == _T("PINENTRY_LAUNCHED") ||
                      code == _T("PLAINTEXT") ||
                      code == _T("PLAINTEXT_LENGTH") ||
                      code == _T("IMPORT_OK") ||
                      code == _T("IMPORT_RES") )
            {
               // ignore these
            }
            else
            {
               // Don't use wxLogWarning() here, new versions of gpg may (and
               // did, in the past) add new messages that we don't handle, but
               // it's not necessarily a problem that the user must be notified
               // about.
               log->AddMessage
                    (
                        wxString::Format
                        (
                         _("Ignoring unexpected GnuPG status line: \"%s\""), line
                        )
                    );
            }
            // Extract user id used to sign
            if ( log && (code == _T("GOODSIG") || code == _T("BADSIG")) )
            {
               String userId = String(pc).AfterFirst(' ');
               log->SetUserID(userId);
            }
         }
#ifndef DEBUG // In non-debug mode, log only free-form output
         else // normal (free form) GPG output
         {
            // remember in the output log object if we have one
            if ( log )
            {
               log->AddMessage(line);
            }
         }
#endif // release
      }
   }

   if ( in )
      process.CloseOutput();

   // Removing this assert:
   // There is at least one case (signature not detached) where GPG does not output
   // any line beginning with "[GNUPG:] ". So we can't rely on getting one.
   //ASSERT_MSG( status != MAX_ERROR, _T("GNUPG didn't return the status?") );

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

   // First copy from in to out, in case there is a problem and we can't
   // execute the command. At least, the original message will be visible.
   messageOut = messageIn;

   return ExecCommand(tmpfname.GetName(), wxEmptyString, messageOut, log);
}


PGPEngine::Status
PGPEngine::Encrypt(const String& /* recipient */,
                   const String& /* messageIn */,
                   String & /* messageOut */,
                   const String& /* user */,
                   MCryptoEngineOutputLog * /* log */)
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
   // as in Decrypt(), using stdin to pass both the input file contents and the
   // passphrase doesn't work well, so use a temporary file
   wxFile f;
   MTempFileName tmpfname(&f);
   if ( !tmpfname.IsOk() || !f.Write(messageIn) )
   {
      wxLogError(_("Can't pass the encrypted data to PGP."));

      return CANNOT_EXEC_PROGRAM;
   }

   f.Close(); // close it before calling gpg, otherwise it fails to open it

   String options("--detach-sign");
   if ( !user.empty() )
      options += " --local-user " + user;

   options += ' ' + tmpfname.GetName();
   return ExecCommand(options, wxEmptyString, messageOut, log);
}


PGPEngine::Status
PGPEngine::VerifySignature(const String& messageIn,
                           String& messageOut,
                           MCryptoEngineOutputLog *log)
{
   return ExecCommand(wxEmptyString, messageIn, messageOut, log);
}

PGPEngine::Status
PGPEngine::VerifyDetachedSignature(const String& message,
                                   const String& signature,
                                   MCryptoEngineOutputLog *log)
{
   // create temporary files to store the signature and the message text:
   // Using a temp file for both is necessary because GPG does not allow anything
   // else for this (detached signature) case.
   MTempFileName tmpfileSig, tmpfileText;
   bool ok = tmpfileSig.IsOk() && tmpfileText.IsOk();

   if ( ok )
   {
      wxFile fileSig(tmpfileSig.GetName(), wxFile::write);
      ok = fileSig.IsOpened() && fileSig.Write(signature);
   }

   if ( ok )
   {
      wxFile fileText(tmpfileText.GetName(), wxFile::write);
      ok = fileText.IsOpened() && fileText.Write(message);
   }

   if ( !ok )
   {
      wxLogError(_("Failed to verify the message signature."));

      return SIGNATURE_CHECK_ERROR;
   }

   wxString messageOut;

   return ExecCommand("--verify " + tmpfileSig.GetName() +
                      " " + tmpfileText.GetName(),
                      wxEmptyString, messageOut, log);
}

// ----------------------------------------------------------------------------
// PGPEngine key management
// ----------------------------------------------------------------------------

PGPEngine::Status
PGPEngine::GetPublicKey(const String& pk,
                        const String& keyserver,
                        MCryptoEngineOutputLog *log) const
{
   String dummyOut;
   Status status = const_cast<PGPEngine *>(this)->ExecCommand
                   (
                     wxString::Format
                     (
                        "--keyserver %s --recv-keys %s",
                        keyserver,
                        pk
                     ),
                     wxEmptyString,
                     dummyOut,
                     log
                   );
   switch ( status )
   {
      default:
         wxLogWarning(_("Importing public key failed for unknown "
                        "reason."));
         break;

      case NO_DATA_ERROR:
         wxLogWarning(_("Public key not found on the key server \"%s\"."),
                      keyserver);
         break;

      case OK:
         wxLogMessage(_("Successfully imported public key \"%s\"."),
                      pk);
         break;
   }

   return status;
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
                                "user \"%s\":"), user
                            ),
                            _("Mahogany: Please enter the passphrase"),
                            wxEmptyString,
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
                 "user \"%s\" in memory?"), user
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

