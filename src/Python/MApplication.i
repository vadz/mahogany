/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/05/30 17:55:52  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.4  1998/05/24 07:54:13  KB
 * works?
 *
 * Revision 1.3  1998/05/24 08:28:56  KB
 * eventually fixed the type problem, now python works as expected
 *
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

%import String.i

class MApplication 
{
public:
   MApplication(void);
   ~MApplication();
   void	Exit(int force);
   MFrame *TopLevelFrame(void) ;
    char *GetText( char *in);
   void	ErrorMessage(String &message, MFrame *parent, bool modal);
   void	SystemErrorMessage(String  &message);
   void	FatalErrorMessage(String  &message);
   void	Message(String  &message);
   int	YesNoDialog(String  &message);
   String  & GetGlobalDir(void) ;
   String  & GetLocalDir(void) ;
   MimeList * GetMimeList(void) ;
   MimeTypes * GetMimeTypes(void) ;
   ProfileBase *GetProfile(void) ;
   Adb *GetAdb(void) ;
   void	ShowConsole(int display = true);
   void Log(int level, String  &message);
};

MApplication mApplication;
