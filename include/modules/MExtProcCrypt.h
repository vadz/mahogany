/*-*- c++ -*-*********************************************************
 * Project    : M                                                    *
 * File name  : MExtProcCrypt.h: Crypto class for invoking external  *
 *                               processes                           *
 * Author     : Carlos Henrique Bauer                                *
 * Created on : 2000/07/08                                           *
 * Modified by:                                                      *
 * CVS-ID     : $Id$     *
 * Copyright  : (C) 2000 by Carlos H. Bauer <chbauer@acm.org>        *
 *                                          <bauer@atlas.unisinos.br>*
 * License    : M license                                            *
 *********************************************************************/

#ifdef EXPERIMENTAL_crypto

#include <stdlib.h>

#ifndef MEXTPROCCRYPT_H
#define MEXTPROCCRYPT_H

#ifndef MCRYPT_H
#include "modules/MCrypt.h"
#endif // MCRYPT_H

#ifdef __GNUG__
#pragma interface "MExtProcCrypt.h"
#endif // __GNUG__



/**********************************************************************
 * a crypto class for invoking external programs
 *********************************************************************/

class MExtProcCrypt : public MCrypt
{
public:

   MExtProcCrypt(const wxString & commandPath);


   void SetPath(const wxString & path);
   const wxString & GetPath() const;

   class CryptProcess : public wxProcess
   {
   public:
      CryptProcess( wxString * output =  NULL,
		    wxString * error = NULL);

      virtual void OnTerminate(int pid, int status);

      virtual int GetInput(wxInputStream * inStream,
			   wxString * output,
			   wxInputStream * errorStream,
			   wxString * errorString);

      int GetStatus() const;
      int HasTerminated() const;

      static bool GetEnv(const wxString & varName,
			 wxString & value);
      static bool GetEnv(const char * varName,
			 wxString & value);
      
      static bool SetEnv(const wxString & varName,
			 const wxString & value);
      static bool SetEnv(const char * varName,
			 const char * value);

      static void UnsetEnv(const wxString & variable);
      static void UnsetEnv(const char * variable);

   protected:
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


inline MExtProcCrypt::MExtProcCrypt(const wxString & commandPath) :
   m_path(commandPath)    
{
}


inline void MExtProcCrypt::SetPath(const wxString & path)
{
   m_path = path;
}


inline const wxString & MExtProcCrypt::GetPath() const
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


inline int MExtProcCrypt::CryptProcess::GetStatus() const
{
   return m_status;
}


inline int MExtProcCrypt::CryptProcess::HasTerminated() const
{
   return m_terminated;
}


inline bool MExtProcCrypt::CryptProcess::GetEnv(
   const wxString & varName, wxString & value)
{
   return GetEnv(varName.c_str(), value);
}


inline bool MExtProcCrypt::CryptProcess::GetEnv(
   const char * varName, wxString & value)
{
   char * res = getenv(varName);

   if(res) 
   {
      value = res;
      return TRUE;
   }

   return FALSE;
}


inline bool MExtProcCrypt::CryptProcess::SetEnv(
   const wxString & varName,
   const wxString & value)
{
   return SetEnv(varName.c_str(), value.c_str());
}


inline bool MExtProcCrypt::CryptProcess::SetEnv(
   const char * varName,
   const char * value)
{
   return !::putenv((char*) wxString::Format("%s=%s",
					    varName,
					    value).c_str());
}
   
   
inline void MExtProcCrypt::CryptProcess::UnsetEnv(
   const wxString & varName)
{
   UnsetEnv(varName.c_str());
}
   
	 
inline void MExtProcCrypt::CryptProcess::UnsetEnv(
   const char * varName)
{
   if(getenv(varName)) 
   {
      ::putenv((char*) wxString::Format("%s=", (char *)
					varName).c_str());
   }
}


#endif // EXTPROCCRYPT
#endif // EXPERIMENTAL_crypto
