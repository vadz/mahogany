// ////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   pprocess.h
// Purpose:     A class for reading and writing to a background process
// Author:      Carlos Henrique Bauer
// Modified by:
// Created:     01.06.1999
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Carlos Henrique Bauer <chbauer@acm.org>
// Licence:     M license
// ////////////////////////////////////////////////////////////////////////////


#ifndef _WX_PPROCESSH__
#define _WX_PPROCESSH__

#ifdef __GNUG_
   #pragma interface "pprocess.h"
#endif

#ifdef EXPERIMENTAL

#ifndef  USE_PCH
#  include "Mdefaults.h"
#endif // USE_PCH

#include <wx/process.h>
#include <wx/textfile.h>

#if wxUSE_IOSTREAMH
   #include <fstream.h>
#else
   #include <fstream>
#endif

WX_DECLARE_LIST(wxString, wxEnvVarList);



class WXDLLEXPORT wxPipedProcess : public wxProcess
{
    DECLARE_DYNAMIC_CLASS(wxPipedProcess);

public:

    enum {
    invalid_fd = -1
    };


    enum {
    invalid_pid = -1
    };


    enum
    {
    no_child,
    exited,
    stoped,
    running,
    execution_failed
    };


protected:
    enum {
    READ = 0,
    WRITE = 1
    };


public:

    // ctors
    // -----
    // def ctor
    wxPipedProcess();
    ~wxPipedProcess();

    long Execute(const wxString & command);
    long Execute(wxChar * const *argv);

    int ReadInput(void * buf, int len);
    int ReadInput(wxChar * buf, int len);
    int ReadInput(wxString & string, int len = -1);
    int ReadInputUntilEof(wxString & string);

    bool WriteOutput(const void * buf, int len);
    bool WriteOutput(const wxChar * buf, int len);
    bool WriteOutput(const wxString & string);

    int ReadError(void * buf, int len);
    int ReadError(wxChar * buf, int len);
    int ReadError(wxString & string, int len = -1);

    void CloseInput();
    void CloseOutput();
    void CloseError();

    void Kill();

    void SetEnv(const wxString & varName, const wxString & value);
    void UnsetEnv(const wxString & varName);
    bool GetEnv(const wxString & varName, wxString value);


    bool WaitChild();

    bool IsChildAlive();
    bool IsInputOk();
    bool IsInputOpen();
    bool IsOutputOk();
    bool IsOutputOpen();
    bool IsErrorOk();
    bool IsErrorOpen();

    wxPipedProcess & operator << (const wxString & string);
    wxPipedProcess & operator >> (wxString & string);

protected:
    void ResetStatus();
    void SetStatus(int exitValue, int childStatus, int signal);
    void AfterChildDied();
    void Signal_handler(int signal);
    void InstallSignalHandler();

protected:
    pid_t m_childPid;
    int m_exitValue;
    int m_childStatus;
    int m_signal;

    wxEnvVarList m_setEnvVarList;
    wxStringList m_unsetEnvVarList;

    ifstream m_input;
    ofstream m_output;
    ifstream m_error;

private:
    // copy ctor and assignment operator are private because
    // it doesn't make sense to copy files this way:
    // attempt to do it will provoke a compile-time error.
    wxPipedProcess(const wxPipedProcess&);
    wxPipedProcess& operator=(const wxPipedProcess&);
};


inline bool wxPipedProcess::IsInputOk()
{
    return m_input.good();
}


inline bool wxPipedProcess::IsInputOpen()
{
    return m_input.is_open();
}


inline bool wxPipedProcess::IsOutputOk()
{
    return m_output.good();
}


inline bool wxPipedProcess::IsOutputOpen()
{
    return m_output.is_open();
}


inline bool wxPipedProcess::IsErrorOk()
{
    return m_error.good();
}


inline bool wxPipedProcess::IsErrorOpen()
{
    return m_error.is_open();
}


inline bool wxPipedProcess::WriteOutput(const wxString & string)
{
   return WriteOutput((const wxChar *)string, string.Length());
}


inline bool wxPipedProcess::WriteOutput(const void * buf, int len)
{
    return m_output.write(buf, len);
}


inline bool wxPipedProcess::IsChildAlive()
{
    return (m_childPid != invalid_pid) ?
    wxKill(m_childPid, wxSIGNONE) != -1 : false;
}


inline wxPipedProcess & wxPipedProcess::operator <<(const wxString & string)
{
    WriteOutput(string);
    return *this;
}


inline wxPipedProcess & wxPipedProcess::operator >>(wxString & string)
{
    ReadInput(string);
    return *this;
}


inline long wxExecute(const wxString& command,
                      wxPipedProcess *handler)
{
    return handler->Execute(command);
}

#endif // EXPERIMENTAL

#endif // _WX_PROCESSH__



