/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/04/29 19:55:42  KB
 * some more scripting and configure changes for python
 *
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

%module 	MApplication
%{
#include	"Mpch.h"
#ifndef	USE_PCH
#	include	"Mcommon.h"
#	include	"strutil.h"
#	include	"Profile.h"
#	include	"MFrame.h"
#	include	"MLogFrame.h"
#	include	"MimeList.h"
#	include	"MimeTypes.h"
#	include	"Mdefaults.h"
#	include	"MApplication.h"
#endif
%}

class MApplication 
{
public:
   MApplication(void);
   ~MApplication();
   void	Exit(int force);
   MFrame *TopLevelFrame(void) ;
    char *GetText( char *in);
   void	ErrorMessage(string &message, MFrame *parent, bool modal);
   void	SystemErrorMessage(string  &message,
		     MFrame *parent,
			int modal);
   void	FatalErrorMessage(string  &message,
		   MFrame *parent);
   void	Message(string  &message,
		MFrame *parent,
		int modal);
   int	YesNoDialog(string  &message,
		    MFrame *parent,
		    int modal,
		    int YesDefault = true);
   string  & GetGlobalDir(void) ;
   string  & GetLocalDir(void) ;
   MimeList * GetMimeList(void) ;
   MimeTypes * GetMimeTypes(void) ;
   ProfileBase *GetProfile(void) ;
   Adb *GetAdb(void) ;
   void	ShowConsole(int display = true);
   void Log(int level, string  &message);
};

