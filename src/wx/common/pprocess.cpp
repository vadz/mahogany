///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   pprocess.cpp
// Purpose:     An class for reading and writing to a background process
// Author:      Carlos Henrique Bauer
// Modified by:
// Created:     1999/06/01
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Carlos Henrique Bauer <chbauer@acm.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef EXPERIMENTAL

#include "Mpch.h"


#ifdef __GNUG__
#pragma implementation "pprocess.h"
#endif
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <wx/pprocess.h>

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(wxEnvVarList);


#ifndef WXEXECUTE_NARGS
#define WXEXECUTE_NARGS   127
#endif

IMPLEMENT_DYNAMIC_CLASS(wxPipedProcess, wxProcess);


typedef void (*wxSIGHANDLERTYPE)(int);


inline int min(int x1, int x2)
{
    return (x1 > x2) ? x1 : x2;
}

wxPipedProcess::wxPipedProcess() :
    m_childPid(invalid_pid),
    m_exitValue(0),
    m_childStatus(no_child),
    m_signal(0),
    m_setEnvVarList(wxKEY_STRING)
{
}
						  

wxPipedProcess::~wxPipedProcess()
{
    Kill();
}


long wxPipedProcess::Execute(wxChar * const * argv)
{
    wxCHECK_MSG( *argv, 0, _T("can't exec empty command") );

#if wxUSE_UNICODE
    int mb_argc = 0;
    char *mb_argv[WXEXECUTE_NARGS];

    while (argv[mb_argc]) {
      wxWX2MBbuf mb_arg = wxConv_libc.cWX2MB(argv[mb_argc]);
      mb_argv[mb_argc] = strdup(mb_arg);
      mb_argc++;
    }
    mb_argv[mb_argc] = (char *) NULL;
#else
    wxChar * const *mb_argv = argv;
#endif

    ResetStatus();

    pid_t childPid = 0;

    int input_fd[2], output_fd[2], error_fd[2];

    // setup the communication
    if (pipe(input_fd) < 0) {
	wxLogSysError(_T("pipe(input_fd) failed"));
    } else {
	if (pipe(output_fd) < 0) {

	    close(input_fd[0]);
	    close(input_fd[1]);

	    wxLogSysError(_T("pipe(output_fd) failed"));

	} else {
	    if (pipe(error_fd) < 0) {

		close(input_fd[0]);
		close(input_fd[1]);

		close(output_fd[0]);
		close(output_fd[1]);

		wxLogSysError(_T("pipe(error_fd) failed"));
	    } else {
		
#ifdef HAVE_VFORK
		childPid = vfork();
#else
		childPid = fork();
#endif

		switch(childPid) {
		
		case -1:
		    wxLogSysError(_T("Fork failed"));
		    m_childStatus = execution_failed;
		    break;
	 
		case 0:
	    
		    // this the child

		    // setup the child side of the communication
		    
		    
		    if(dup2(output_fd[READ], fileno(stdin)) < -1) {
			wxLogSysError(_T("dup2(output_fd[READ]) failed"));
		    }
		    
		    if(dup2(input_fd[WRITE], fileno(stdout)) < -1) {
			wxLogSysError(_T("dup2(input_fd[WRITE]) failed"));
		    }

		    if(dup2(error_fd[WRITE], fileno(stderr)) < -1) {
			wxLogSysError(_T("dup2(output_fd[READ]) failed"));
		    }

		    close(output_fd[WRITE]);
		    close(input_fd[READ]);
		    close(error_fd[WRITE]);

		    for ( int fd = 0; fd < FD_SETSIZE; fd++ ) {
			if ( fd != STDIN_FILENO && fd != STDOUT_FILENO &&
			     fd != STDERR_FILENO ) { 
			    close(fd);
			}
		    }

		    // set the environment variables

		    wxEnvVarList::Node *node;

                    {
                       wxString tmpstr;
                       for (node = m_setEnvVarList.GetFirst();
                            node; node = node->GetNext() ) {
                          tmpstr = node->GetKeyString();
                          tmpstr << '=' << *node->GetData();
                          putenv(tmpstr.c_str());
                       }

                       for (node = m_setEnvVarList.GetFirst();
                            node; node = node->GetNext() ) {
                          tmpstr = *node->GetData() + '=';
                          putenv(tmpstr.c_str());
                       }
                    }
		    execvp (*mb_argv, mb_argv);
	
		    // there is no return after successful exec()
		    wxFprintf(stderr, _T("Can't execute '%s'"), *mb_argv);
		    
		    _exit(-1);
							    
		default:
		    // this the parent
	 
		    // Add this process to the list of objects
		    //   to be processed in a SIGCHLD signal
		    wxLogDebug(_T("wxPipedProcess: child %d launched"), childPid);  
		    m_childPid = childPid;
		    
		    m_childStatus = running;

		    // setup the parend side of the comunications
		    close(input_fd[WRITE]);
		    close(output_fd[READ]);
		    close(error_fd[WRITE]);

		    // attach the file descriptors to the streams
		    m_input.attach(input_fd[READ]);
		    m_output.attach(output_fd[WRITE]);
		    m_error.attach(error_fd[READ]);
		    
		    
		    break;
		}
	    }
	}
    }
        
#if wxUSE_UNICODE
    mb_argc = 0;
    while (mb_argv[mb_argc])
	free(mb_argv[mb_argc++]);
#endif
  
    return childPid;
}


long wxPipedProcess::Execute( const wxString& command)
{
    if(m_childPid != invalid_pid) {
	wxLogDebug(_T("wxPipedProcess: can't handle more than one "
		      "child process"));
	return invalid_pid;
    }
    
    
    wxCHECK_MSG( !command.IsEmpty(), 0, _T("can't exec empty command") );

    wxChar * argv[4];

    const int COMMAND_POS = 2;
    const wxChar * realCommandFormat = _T("%s ; exit $?");
    int realCommandLength = command.length() +
       strlen(realCommandFormat);
   
   argv[0] = (wxChar *) _T("/bin/sh");
   argv[1] = (wxChar *) _T("-c");
   argv[COMMAND_POS] = new wxChar[realCommandLength];
   argv[3] = NULL;

   wxString tmpstr;
   tmpstr.Printf(realCommandFormat, command.c_str());
   strncpy(argv[COMMAND_POS], tmpstr.c_str(), realCommandLength);
   // do execute the command
   long lRc = Execute(argv);
   
   // clean up
   delete [] argv[COMMAND_POS];
   
   return lRc;
}


int wxPipedProcess::ReadInput(wxString & string, int len)
{
    if(!IsInputOpen()) {
	return 0;
    }
    

    wxChar buf[512];

    int ncount = 0;
    int ntemp;
    
    while((len == -1 || ncount < len) && m_input.good()) {
	ntemp = ReadInput(buf,
			   (len == -1) ? sizeof(buf) - 1 :
			   min(len - ncount, sizeof(buf) - 1));
	if(ntemp > 0) {
	    ncount += ntemp;
	    buf[ncount] = '\0';
	    string += buf;
	}
    }

    return ncount;
}


int wxPipedProcess::ReadInput(wxChar * buf, int len)
{
    int ncount = 0;
    
    if(IsInputOpen() && m_input.good()) {
	m_input.read(buf, len);
	ncount = m_input.gcount();
    }

    return ncount;
}


int wxPipedProcess::ReadInput(void * buf, int len)
{
    int ncount = 0;
    
    if(IsInputOpen() && m_input.good()) {
	m_input.read(buf, len);
	ncount = m_input.gcount();
    }

    return ncount;
}


int wxPipedProcess::ReadInputUntilEof(wxString & string)
{
    string.Empty();
    int ncount = 0;
    
    while(IsInputOk()) {
	wxString tmp;
	ncount += ReadInput(tmp);
	string += tmp;
    }
    return ncount;
}


int wxPipedProcess::ReadError(wxString & string, int len)
{
    if(!IsErrorOpen()) {
	return 0;
    }

    wxChar buf[512];

    int ncount = 0;
    int ntemp;
    
    while((len == -1 || ncount < len) && m_error.good()) {
	ntemp = ReadError(buf,
			  (len == -1) ? sizeof(buf) - 1 :
			  min(len - ncount, sizeof(buf) - 1));
	if(ntemp > 0) {
	    ncount += ntemp;
	    buf[ncount] = '\0';
	    string += buf;
	}
    }

    return ncount;
}


int wxPipedProcess::ReadError(wxChar * buf, int len)
{
    int ncount = 0;
    
    if(IsErrorOpen() && m_error.good()) {
	m_error.read(buf, len);
	ncount = m_error.gcount();
    }

    return ncount;
}


int wxPipedProcess::ReadError(void * buf, int len)
{
    int ncount = 0;
    
    if(IsErrorOpen() && m_error.good()) {
	m_error.read(buf, len);
	ncount = m_error.gcount();
    }

    return ncount;
}


inline bool wxPipedProcess::WriteOutput(const wxChar * buf, int len)
{
    if(IsOutputOpen()) {
	m_output.write(buf, len);
    }
    
    return m_output.good();
}


void wxPipedProcess::SetEnv(const wxString & varName,
			    const wxString & value)
{
    {
	wxEnvVarList::Node * var;
	var = m_setEnvVarList.Find(varName);

	if(var != NULL) {
	    *(var->GetData()) = value;
	} else {
	    m_setEnvVarList.Append(varName.c_str(),
				   (void*) new wxString(value));
	}
    }

    wxStringList::Node * node;

    for (node = m_unsetEnvVarList.GetFirst();
	 node; node = node->GetNext() ) {
	if(*(node->GetData()) == varName) {
	    m_unsetEnvVarList.DeleteNode(node);
	}
    }
}


bool wxPipedProcess::GetEnv(const wxString & varName, wxString value)
{
    wxEnvVarList::Node * var = m_setEnvVarList.Find(varName);

    if(var != NULL) {
	value = *(var->GetData());
	return true;
    } else {

	// the child process inherits the parent's
	// environment variables
	char * envValue = getenv(varName);

	if(envValue != NULL) {
	    value = envValue;
	    return true;
	}
    }

    return false;
}


void wxPipedProcess::UnsetEnv(const wxString & varName)
{
    {
	wxEnvVarList::Node * var = m_setEnvVarList.Find(varName);

	if(var != NULL) {
	    m_setEnvVarList.DeleteNode(var);
	}
    }

    wxStringList::Node * node;

    for (node = m_unsetEnvVarList.GetFirst();
	 node; node = node->GetNext() ) {
	if(*(node->GetData()) == varName) {
	    break;
	}
    }

    if(!node) {
	m_unsetEnvVarList.Add(varName);
    }

}


void wxPipedProcess::CloseInput()
{
   if(IsInputOpen()) {
       m_input.close();
   }
}


void wxPipedProcess::CloseOutput()
{
   if(IsOutputOpen()) {
       m_output.close();
   }
}


void wxPipedProcess::CloseError()
{
   if(IsErrorOpen()) {
       m_error.close();
   }
}


void wxPipedProcess::Kill()
{
   if(IsChildAlive()) {
      wxKill(m_childPid, wxSIGKILL);
      WaitChild();
   }
}


void wxPipedProcess::ResetStatus()
{
    m_exitValue = 0;
    m_childStatus = no_child;
    m_signal = 0;
}


void wxPipedProcess::SetStatus(int exitValue, int childStatus, int signal)
{
    m_exitValue = exitValue;
    m_childStatus = childStatus;
    m_signal = signal;
}


bool wxPipedProcess::WaitChild()
{
    if(m_childPid == invalid_pid) {
	return false;
    }

    wxSIGHANDLERTYPE istat = signal(SIGINT, SIG_IGN);
    wxSIGHANDLERTYPE qstat = signal(SIGQUIT, SIG_IGN);
    wxSIGHANDLERTYPE hstat = signal(SIGHUP, SIG_IGN);

    int status;
    pid_t pid = waitpid(m_childPid, &status , WNOHANG);

    int exitValue = 0;
    int childStatus = no_child;
    int sig = 0;

    if(pid > 0) {

	if(WIFEXITED(status)) {
	    exitValue = WEXITSTATUS(status);
	    childStatus = exited;
	    if(WIFSIGNALED(status)) {
		childStatus = exited;
		sig = WTERMSIG(status);
	    }
	} else {
	    if(WIFSTOPPED(status)) {
		childStatus = stoped;
		sig =  WSTOPSIG(status);
	    }
	}
    } else {
	if(errno == ECHILD) {
	    childStatus = exited;
	} else {
	    wxLogDebug(_T("waitpid  failed: %1"), errno);
	}
    }

    signal(SIGINT, istat);
    signal(SIGQUIT, qstat);
    signal(SIGHUP, hstat);

    
    if(childStatus == exited) {
	wxLogDebug("Child %i exited, exit value: %i, sinal %i", pid,
		   exitValue, sig);
	SetStatus(exitValue, childStatus, sig);
	AfterChildDied();
	return true;
    }
    return false;
}


void wxPipedProcess::AfterChildDied()
{
    m_childPid = invalid_pid;
    m_childStatus = exited;

    CloseInput();
    CloseOutput();
    CloseError();
}

#endif
