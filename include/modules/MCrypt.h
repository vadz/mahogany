/*-*- c++ -*-********************************************************
 * Project:   M                                                     *
 * File name: MCrypt.h: Crypto base classes                         *
 * Author:    Carlos Henrique Bauer                                 *
 * Modified by:                                                     *
 * CVS-ID:    $Id$    *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                        <bauer@atlas.unisinos.br> *
 * License: M license                                               *
 *******************************************************************/

#ifdef EXPERIMENTAL

#ifndef MCRYPT_H
#define MCRYPT_H

#ifndef PSSPHRS_H
#include <pssphrs.h>
#endif // PSSPHRS_H

#ifdef __GNUG__
#pragma interface "MCrypt.h"
#endif // __GNUG__

#ifndef _WX_PROCESSH__
#include <wx/process.h>
#endif // _WX_PROCESSH__


/**********************************************************************
 * the common interface of all crypto classes
 *********************************************************************/

class MCrypt
{
public:

    // MCrypt functions return status values

    // temporary values

    enum {
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
	SIGNATURE_ERROR,
	PUBLIC_KEY_ENCRIPTION_ERROR,
	ENCRYPTION_ERROR,
	COMPRESSION_ERROR,
	SIGNATURE_CHECK_ERROR,
	PUBLIC_KEY_DECRIPTION_ERROR,
	DECRIPTION_ERROR,
	DECOMPRESSION_ERROR
    };


    // Encryption methods IDs
    enum {
	M_PGP = 1
    };

public:

    MCrypt(const wxString & user, MPassPhrase & passPhrase);
    virtual ~MCrypt();

    virtual int Encrypt(const wxString & recipient,
			 const wxString & messageIn,
			 wxString & messageOut,
			 bool sign = TRUE) = 0;

    virtual int VerifySignature(const wxString & messageIn, wxString &
				messageOut) const = 0;

    virtual int Sign(const wxString & messageIn,
		      wxString & messageOut) = 0;

    virtual int CheckRecipient(const wxString & recipient)
	const = 0;
    

    virtual int Decrypt(const wxString & messageIn,
			 wxString & messageOut) = 0;

    virtual int GetFingerprint(wxString & fp) const = 0;

    virtual int GetPublicKey(wxString & pk) const = 0;

    static void SetComment(const wxString & comment);

    static const wxString & GetComment();

    static int IsEncrypted(const wxString & messageIn);
    static int IsSigned(const wxString & messageIn);


protected:
    static wxString m_comment;

    const wxString & m_user;
    MPassPhrase &  m_passPhrase;

private:
    // declared but not defined
    MCrypt();
};


/**********************************************************************
 * a crypto class for invoking external programs
 *********************************************************************/

class MExtProcCrypt : public MCrypt
{
public:

    MExtProcCrypt(const wxString & commandPath,
		  const wxString & user,
		  MPassPhrase & passPhrase);


    void SetPath(const wxString & path);
    const wxString & GetPath();

    class CryptProcess : public wxProcess
	{
	public:
	    CryptProcess( wxString * output =  NULL,
			 wxString * error = NULL);

	    virtual void OnTerminate(int pid, int status);

	    int GetStatus();
	    int HasTerminated();
	    
	    void OnIdle(wxIdleEvent & process);

	protected:
	    wxString * m_output;
	    wxString * m_error;

	    int m_status;
	    int m_terminated;
	    
	private:
	    DECLARE_EVENT_TABLE()
	};

protected:

    // path to the command to executed
    wxString m_path;
};


inline MCrypt::MCrypt(const wxString & user, MPassPhrase & passPhrase) :
    m_user(user), m_passPhrase(passPhrase)
{
}


inline MCrypt::~MCrypt()
{
}


inline const wxString & MCrypt::GetComment()
{
    return m_comment;
}


inline MExtProcCrypt::MExtProcCrypt(const wxString & commandPath,
				    const wxString & user,
				    MPassPhrase & passPhrase) :
    MCrypt(user, passPhrase),
    m_path(commandPath)    
{
}


inline void MExtProcCrypt::SetPath(const wxString & path)
{
    m_path = path;
}


inline const wxString & MExtProcCrypt::GetPath()
{
    return m_path;
}


inline MExtProcCrypt::CryptProcess::CryptProcess(wxString * output,
						 wxString * error) :
    wxProcess(),
    m_output(output),
    m_error(error),
    m_status(0),
    m_terminated(FALSE)
{
    Redirect();
}


inline int MExtProcCrypt::CryptProcess::GetStatus()
{
    return m_status;
}


inline int MExtProcCrypt::CryptProcess::HasTerminated()
{
    return m_terminated;
}


#endif // MCRYPT_H
#endif // EXPERIMENTAL
