/*-*- c++ -*-*********************************************************
 * Project    : M                                                    *
 * File name  : MCrypt.h: crypto classes implementation              *
 * Author     : Carlos Henrique Bauer                                *
 * Modified by:                                                      *
 * CVS-ID     :                                                      *
 *         $Id$ *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>        *
 *                                          <bauer@atlas.unisinos.br>*
 * License  : M license                                              *
 ********************************************************************/

#ifdef EXPERIMENTAL_crypto

#include <wx/intl.h>
#include <wx/log.h>

#ifdef __GNUG__
#pragma implementation "MExtProcCrypt.h"
#endif

#include "modules/MExtProcCrypt.h"


BEGIN_EVENT_TABLE(MExtProcCrypt::CryptProcess, wxProcess)
   EVT_IDLE(MExtProcCrypt::CryptProcess::OnIdle)
END_EVENT_TABLE();


int MExtProcCrypt::CryptProcess::GetInput(wxInputStream * inStream,
					  wxString * output,
					  wxInputStream * errorStream,
					  wxString * errorString)
{
   char buffer[512];
   bool has_input = FALSE;

   if(inStream) {

      has_input = !inStream->Eof();

      while(!inStream->Eof()) 
      {
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

   if(errorString) 
   {
	
      has_input = !errorStream->Eof();

      while(!errorStream->Eof())
      {
	 errorStream->Read(buffer, sizeof(buffer) - 1);

	 if(errorStream) 
	 {
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


void MExtProcCrypt::CryptProcess::OnTerminate(int pid, int status)
{
   while(GetInput(GetInputStream(), m_output,
		  GetErrorStream(), m_error));

   wxLogTrace(*m_error);
 
   m_status = status;
   m_terminated = TRUE;
}


void MExtProcCrypt::CryptProcess::OnIdle(wxIdleEvent & event)
{
   if(GetInput(GetInputStream(),m_output,
	       GetErrorStream(),m_error)) 
   {
      event.RequestMore();
   }
}

#endif // EXPERIMENTAL_crypto
