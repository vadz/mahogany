/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
%nodefault
%module   MAppBase
%{
#include "Mswig.h"
#include "gui/wxMApp.h"
#include "MDialogs.h"
#include "MFrame.h"

// SWIG bug: it generates code which assignes the default argument to "char *"
// variable. To make it compile we need this hack
#define MDIALOG_ERRTITLE_C (char *)MDIALOG_ERRTITLE
#define MDIALOG_SYSERRTITLE_C (char *)MDIALOG_SYSERRTITLE
#define MDIALOG_FATALERRTITLE_C (char *)MDIALOG_FATALERRTITLE
#define MDIALOG_MSGTITLE_C (char *)MDIALOG_MSGTITLE
#define MDIALOG_YESNOTITLE_C (char *)MDIALOG_YESNOTITLE
%}

%import MString.i
%import MProfile.i

class MAppBase 
{
public:
   void Exit();
   MFrame *TopLevelFrame(void) const;
   const char *GetText(const char *in) const;
   String  GetGlobalDir(void) const;
   String  GetLocalDir(void) const;
   wxMimeTypesManager& GetMimeManager(void) const;
   Profile *GetProfile(void) const;
};

MAppBase &wxGetApp();


void  MDialog_ErrorMessage(const char  *message,
                           MFrame *parent = NULL,
                           const char *title = MDIALOG_ERRTITLE_C,
                           bool   modal = false);

void  MDialog_SystemErrorMessage(const char  *message,
                                 MFrame *parent = NULL,
                                 const char  *title = MDIALOG_SYSERRTITLE_C,
                                 bool modal = false);

void  MDialog_FatalErrorMessage(const char *message,
                                MFrame *parent = NULL,
                                const char  *title = MDIALOG_FATALERRTITLE_C);

void  MDialog_Message(const char *message,
                      MFrame *parent = NULL,
                      const char *title = MDIALOG_MSGTITLE_C);

bool MDialog_YesNoDialog(const char *message,
                           MWindow *parent = NULL,
                           const char *title = MDIALOG_YESNOTITLE_C,
                           bool YesDefault = true,
                           const char *configPath = NULL);

char *MDialog_FileRequester(String  &message,
                            MWindow *parent = NULL,
                            String &path = NULLstring,
                            String &filename = NULLstring,
                            String &extension = NULLstring,
                            String &wildcard = NULLstring,
                            bool save = false,
                            Profile *profile = NULL);

bool MInputBox(String *pstr,
               const char * caption,
               const char * prompt,
               MWindow *parent = NULL,
               const char *key = NULL,
               const char *def = NULL);
