/*-*- c++ -*-********************************************************
 * Script.cc -  scripting code                                      *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                 *
 *******************************************************************/


#include	"Mpch.h"

#ifndef	USE_PCH
#	include	"Mcommon.h"
#	include	"strutil.h"
#	include "guidef.h"	// wxGetTempFile()
#endif

#ifdef CC_GCC
#	pragma implementation "Script.h"
#endif
#include	"Script.h"

#ifdef	USE_PYTHON
#	include	"Python.h"
#endif

#ifdef	OS_UNIX
#	include	<unistd.h>
#	include	<sys/types.h>
#	include	<sys/stat.h>
#	include	<sys/wait.h>
#	include	<fcntl.h>
#endif


int
Script::Run(String const &parameters)
{
   int	retval;
   
   String
      cmd = command + String(" ") + parameters,
      stdinFile = wxGetTempFileName("stdin"),
      stdoutFile = wxGetTempFileName("stdout"),
      tmp;
   
   ofstream str;
   ifstream ostr;

   str.open(stdinFile.c_str());
   str << input;
   str.close();
   
#ifdef	OS_UNIX
   pid_t
      pid;
   int
      status, fd;

   pid = fork();
   
   if(pid == 0)	// child
   {
      close(STDIN_FILENO);
      fd = open(stdinFile.c_str(),O_RDONLY,S_IRUSR|S_IWUSR);
      close(STDOUT_FILENO);
      fd = open(stdoutFile.c_str(),O_CREAT|O_TRUNC|O_WRONLY,S_IWUSR);
      exit(wxExecute(WXSTR(cmd), FALSE));
   }
   else
      waitpid(pid,&status, 0);

   retval = WEXITSTATUS(status);
#else
   retval = wxExecute(WXSTR(cmd), FALSE);	//FIXME: no redirection!!
#endif

   output = "";
   ostr.open(stdoutFile.c_str());
   while(! ostr.eof() && ! ostr.fail())
   {
      ostr >> tmp;
      output += tmp;
   }
   ostr.close();
   wxRemoveFile(WXSTR(stdinFile));
   wxRemoveFile(WXSTR(stdoutFile));
   
   return retval;
}


ExternalScript::ExternalScript(String const & iCmdLine,
			       String const & iName,
			       String const & iDescription)
{
   command = iCmdLine;
   name = iName;
   description = iDescription;
}

int
ExternalScript::run(String const &cmd)
{
   return wxExecute(WXSTR(cmd));
}

#ifdef	USE_PYTHON

InternalScript::InternalScript(String const & CmdLine,
			       String const & iName,
			       String const & iDescription)
{
   command = CmdLine;
   name = iName;
   description = iDescription;
}


int
InternalScript::run(String const &parameters)
{
   PyRun_SimpleString((char *) command.c_str());
   return 0;
}

#endif // USE_PYTHON
