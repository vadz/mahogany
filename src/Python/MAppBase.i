/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
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
//   Adb *GetAdb(void) ;
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

char *MDialog_FileRequester(String  &message,
                            MWindow *parent = NULL,
                            String &path = NULLstring,
                            String &filename = NULLstring,
                            String &extension = NULLstring,
                            String &wildcard = NULLstring,
                            bool save = false,
                            ProfileBase *profile = NULL
   );

/*
  AdbEntry *
MDialog_AdbLookupList(AdbExpandListType *adblist,
		      MFrame *parent = NULL);
*/
