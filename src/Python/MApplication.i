/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/05/13 19:01:54  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
 *
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
   void	ErrorMessage(String &message, MFrame *parent, bool modal);
   void	SystemErrorMessage(String  &message,
		     MFrame *parent,
			int modal);
   void	FatalErrorMessage(String  &message,
		   MFrame *parent);
   void	Message(String  &message,
		MFrame *parent,
		int modal);
   int	YesNoDialog(String  &message,
		    MFrame *parent,
		    int modal,
		    int YesDefault = true);
   String  & GetGlobalDir(void) ;
   String  & GetLocalDir(void) ;
   MimeList * GetMimeList(void) ;
   MimeTypes * GetMimeTypes(void) ;
   ProfileBase *GetProfile(void) ;
   Adb *GetAdb(void) ;
   void	ShowConsole(int display = true);
   void Log(int level, String  &message);
};

