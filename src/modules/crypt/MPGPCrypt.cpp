/*-*- c++ -*-********************************************************
 * Project:   M                                                     *
 * File name: MPGPcrpt.cpp: PGP crypto class implementation         *
 * Author:    Carlos Henrique Bauer                                 *
 * Modified by:                                                     *
 * CVS-ID:    $Id$    *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                        <bauer@atlas.unisinos.br> *
 * License: M license                                               *
 *******************************************************************/

/********************************************************************
 * TODO: Make it work with PGP 2.6.1 and PGP 5.x.
 * Test it in Windows. It will not work with PGP 5.x in windows.
 ******************************************************************/

#ifdef EXPERIMENTAL

#include <stdlib.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/process.h>
#include <wx/string.h>
#include <wx/txtstrm.h>

#ifdef __GNUG__
#pragma implementation "MPGPCrypt.h"
#endif // __GNU__

#include "modules/MPGPCrypt.h"

static const int ConversionTable[][2] =
{
   {  0, MCrypt::OK },
   {  1, MCrypt::OK },
   {  4, MCrypt::BATCH_MODE_ERROR },
   {  5, MCrypt::BAD_ARGUMENT_ERROR },
   {  6, MCrypt::PROCESS_INTERRUPTED_ERROR },
   {  7, MCrypt::OUT_OF_MEM_ERROR },
   { 11, MCrypt::NONEXISTING_KEY_ERROR },
   { 12, MCrypt::KEYRING_ADD_ERROR },
   { 13, MCrypt::KEYRING_EXTRACT_ERROR },
   { 14, MCrypt::KEYRING_EDIT_ERROR },
   { 15, MCrypt::KEYRING_VIEW_ERROR },
   { 17, MCrypt::KEYRING_CHECK_ERROR },
   { 18, MCrypt::KEYRING_SIGNATURE_ERROR },
   { 20, MCrypt::SIGNATURE_ERROR },
   { 21, MCrypt::PUBLIC_KEY_ENCRIPTION_ERROR },
   { 22, MCrypt::ENCRYPTION_ERROR },
   { 23, MCrypt::COMPRESSION_ERROR },
   { 30, MCrypt::SIGNATURE_CHECK_ERROR },
   { 31, MCrypt::PUBLIC_KEY_DECRIPTION_ERROR },
   { 32, MCrypt::DECRIPTION_ERROR },
   { 33, MCrypt::DECOMPRESSION_ERROR },
};

static const char * pgpPassPhraseInputDialogPrompt =
gettext_noop("Please enter the PGP passphrase for the key id:\n%s");

static const char * PGPPASSFD = "PGPPASSFD";

int convertExitStatus(int status)
{
   unsigned int i;
    
   for(i = 0;
       i < sizeof(ConversionTable)/sizeof(ConversionTable[0]);
       i++)
   {
      if(ConversionTable[i][0] == status)
      {
	 return ConversionTable[i][1];
      }
   }

   // shouldn't arrive here
   wxFAIL_MSG("PGP: unhandled exit status");
   return 0;
}


static int ExecPGP(const wxString & command,
		   const wxString & args,
		   const wxString * input = NULL,
		   wxString * output = NULL,
		   wxString * error = NULL,
		   MDecryptedPassphrase * passphrase = NULL)
{
   MExtProcCrypt::CryptProcess process(output, error);

   // execute PGP

   // tells pgp to use the first line in stdin as the passphrase
   if(passphrase && !process.SetEnv(PGPPASSFD, "0"))
   {
      return MCrypt::OUT_OF_ENV_SPACE_ERROR;
   }

   wxLogTrace(MCRYPT_TRM, "Executing: %s %s", command.c_str(),
	      args.c_str());

   if(wxExecute(command + " " + args, FALSE, &process))
   {
      wxOutputStream & outStream = *process.GetOutputStream();
	
      // send the passphrase
      if(passphrase)
      {
	 outStream.Write(passphrase->c_str(), passphrase->Len());
	 outStream.Write("\n", 1);
      }

      // send the message to PGP
      if(input) {
	 wxLogTrace(MCRYPT_TRM, "Sending the following text to PGP\n%s",
		    input->c_str());
	 outStream.Write(input->c_str(), input->Len());
	    
	 // PGP seems to like messages that end in \n
	 if(input->Last() != '\n')
	 {
	    outStream.Write("\n", 1);
	 }
      }

      // close the output and let PGP do the work
      process.CloseOutput();

      wxInputStream inStream = *process.GetInputStream();
    
      while(!process.HasTerminated())
      {
	 wxYield();
      }

      // We MUST unset PGPPASSFD otherwise pgp will fail to verify
      // signatures
      if(passphrase)
      {
	 process.UnsetEnv(PGPPASSFD);
      }

      wxLogTrace(MCRYPT_TRM, "PGP exited with status: %i",
		 process.GetStatus());

      return convertExitStatus(process.GetStatus());

   }
    
   return MCrypt::CANNOT_EXEC_PROGRAM;
}


MPGPCrypt::MPGPCrypt() :
   MExtProcCrypt("pgp")
{
   // we must be sure this environment is set just under our
   // control
   CryptProcess::UnsetEnv(PGPPASSFD);
}


int MPGPCrypt::Decrypt(MPassphrase & passphrase,
		       const wxString & messageIn,
		       wxString & messageOut)
{
   messageOut.Clear();

   // get a temporary decrypted passphrase
   MDecryptedPassphrase decPassphrase;

   {
      int res = passphrase.Get(decPassphrase);

      if(res != OK)
      {
	 return res;
      }
   }

   wxString args("-f +batchmode +verbose=1");

   wxString errorString;

   return ExecPGP(m_path, args, &messageIn, &messageOut,
		  &errorString, &decPassphrase);
}


int MPGPCrypt::Encrypt(const wxString & recipient,
		       const wxString & messageIn,
		       wxString & messageOut,
		       const wxString * user,
		       MPassphrase * passphrase)
{
   bool sign = user && passphrase;
   
   messageOut.Clear();

   // get a temporary decrypted passphrase
   MDecryptedPassphrase decPassphrase;

   if(sign) {
      int res = passphrase->Get(decPassphrase);

      if(res != OK) {
	 return res;
      }
   }

   char * args_fmt = "-fe%sta +batchmode +verbose=1 -u "
      "\"%s\" +comment=\"%s\" \"%s\"";

   wxString args;

   args.sprintf(args_fmt, (sign) ? "s" : "",
		(sign) ? user->c_str() : "",
		GetComment().c_str(), recipient.c_str());

   wxString errorString;

   return ExecPGP(m_path, args, &messageIn, &messageOut,
		  &errorString, (sign) ? &decPassphrase : NULL);
}


int MPGPCrypt::Sign(const wxString & user,
		    MPassphrase & passphrase,
		    const wxString & messageIn,
		    wxString & messageOut)
{
   messageOut.Clear();

   // get a temporary decrypted passphrase
   MDecryptedPassphrase decPassphrase;

   {
      int res = passphrase.Get(decPassphrase);

      if(res != OK)
      {
	 return res;
      }
   }

   wxString args("-fsta +batchmode +verbose=1 -u \"" +
		 user + "\" +clearsig=on +comment=\"" +
		 GetComment() + "\"");

   wxString errorString;

   // TODO:  Discover why PGP is returning a status 1, which means
   // invalid file, although it reports in stderr the signature is OK
    
   return ExecPGP(m_path, args, &messageIn, &messageOut,
		  &errorString, &decPassphrase);
}


int MPGPCrypt::VerifySignature(const wxString & messageIn,
			       wxString & messageOut) const
{
   messageOut.Clear();

   wxString args("-f +batchmode +verbose=1");

   wxString errorString;

   return ExecPGP(m_path, args, &messageIn, &messageOut,
		  &errorString);
}


int MPGPCrypt::CheckRecipient(const wxString & recipient) const
{
   return 0;
}


int MPGPCrypt::GetFingerprint(wxString & fp) const
{
   return 0;
}
     

int MPGPCrypt::GetPublicKey(wxString & pk) const
{
   return 0;
}
     

#endif // EXPERIMENTAL
