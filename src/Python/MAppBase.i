/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/06/08 08:34:55  KB
 * Python adapted to use MAppBase/MDialog instead of MApplication.
 *
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
%module 	MAppBase
%{
#include	"Mpch.h"
#ifndef	USE_PCH
#   include   "Mcommon.h"
#   include   "strutil.h"
#   include   "Profile.h"
#   include   "MFrame.h"
#   include   "MLogFrame.h"
#   include   "MimeList.h"
#   include   "MimeTypes.h"
#   include   "Mdefaults.h"
#   include   "MApplication.h"
#   include   "wxMApp.h"
#   include   "gui/wxMDialogs.h"
#endif
%}

%import String.i

class MAppBase 
{
public:
   void	Exit(int force);
   MFrame *TopLevelFrame(void) ;
    char *GetText( char *in);
   String  & GetGlobalDir(void) ;
   String  & GetLocalDir(void) ;
   MimeList * GetMimeList(void) ;
   MimeTypes * GetMimeTypes(void) ;
   ProfileBase *GetProfile(void) ;
   Adb *GetAdb(void) ;
};

MAppBase &wxGetApp();


void	MDialog_ErrorMessage(char  *message,
		     MFrame *parent = NULL,
			     char  *title = MDIALOG_ERRTITLE,
                             bool modal = false);

void	MDialog_SystemErrorMessage(char  *message,
			   MFrame *parent = NULL,
			   char  *title = MDIALOG_SYSERRTITLE,
                                   bool modal = false);

void	MDialog_FatalErrorMessage(char  *message,
			  MFrame *parent = NULL,
			  char  *title = MDIALOG_FATALERRTITLE
   );

void	MDialog_Message(char *message);

/*
		MFrame *parent = NULL,
		char  *title = MDIALOG_MSGTITLE,
		bool modal = false);
                */
bool	MDialog_YesNoDialog(char  *message,
		    MFrame *parent = NULL,
		    bool modal = false,
		    char  *title = MDIALOG_YESNOTITLE,
		    bool YesDefault = true);

 char *MDialog_FileRequester(char *message,
				      MFrame *parent = NULL,
				      char  *path = NULL,
				      char  *filename = NULL,
				      char  *extension = NULL,
				      char  *wildcard = NULL,
				      bool save = false,
				      ProfileBase *profile = NULL
   );

AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist,
		      MFrame *parent = NULL);
