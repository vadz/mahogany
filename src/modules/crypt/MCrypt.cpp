/*-*- c++ -*-********************************************************
 * Project:   M                                                     *
 * File name: MCrypt.h: crypto classes implementation               *
 * Author:    Carlos Henrique Bauer                                 *
 * Modified by:                                                     *
 * CVS-ID:    $Id$    *
 * Copyright: (C) 2000 by Carlos H. Bauer <chbauer@acm.org>         *
 *                                        <bauer@atlas.unisinos.br> *
 * License: M license                                               *
 *******************************************************************/

#include <wx/intl.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

#ifdef __GNUG__
#pragma implementation "MCrypt.h"
#endif

#include <MCrypt.h>


wxString MCrypt::m_comment(_("Mahogany\\ crypto\\ library"));


BEGIN_EVENT_TABLE(MExtProcCrypt::CryptProcess, wxProcess)
    EVT_IDLE(MExtProcCrypt::CryptProcess::OnIdle)
END_EVENT_TABLE();


void MCrypt::SetComment(const wxString & comment)
{
    m_comment = comment;
    
    // PGP doesn't like not escaped spaces
    m_comment.Replace(" ", "\\ ", TRUE);
}


static int GetInput(wxInputStream * inStream,
		    wxString * output,
		    wxInputStream * errorStream,
		    wxString * errorString)
{
    char buffer[512];
    bool has_input = FALSE;

    if(inStream) {

	has_input = !inStream->Eof();

	while(!inStream->Eof()) {
	    inStream->Read(buffer, sizeof(buffer) - 1);

	    if(output) {
		/*
		  some programs like PGP doesn't like
		  interrupted output 
		  and report it as an error
		*/

		buffer[inStream->LastRead()] = '\0';
		*output += buffer;
	    }
	}
    }

    if(errorString) {
	
	has_input = !errorStream->Eof();

	while(!errorStream->Eof()) {
	    errorStream->Read(buffer, sizeof(buffer) - 1);

	    if(errorStream) {
		/*
		  some programs like PGP doesn't like
		  interrupted output 
		  and report it as an error
		*/
		buffer[errorStream->LastRead()] = '\0';
		*errorString += buffer;
	    }
	}
    }
    
    
    return has_input;
}


int MCryptIsEncrypted(const wxString & messageIn)
{
// PGP encrypted messsage
//-----BEGIN PGP MESSAGE-----  
}


int MCrypt::IsSigned(const wxString & messageIn)
{
// PGP signed message
//-----BEGIN PGP SIGNED MESSAGE-----
}


void MExtProcCrypt::CryptProcess::OnTerminate(int pid, int status)
{
    while(
	GetInput(GetInputStream(),
		 m_output,
		 GetErrorStream(),
		 m_error));

    wxLogDebug(*m_error);
 
    m_status = status;
    m_terminated = TRUE;
}


void MExtProcCrypt::CryptProcess::OnIdle(wxIdleEvent & event)
{
    if(GetInput(GetInputStream(),m_output,
		GetErrorStream(),m_error)) {
	event.RequestMore();
    }
}
