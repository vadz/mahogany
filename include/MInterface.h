/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */














#include "MApplication.h"
#include "strutil.h"
#include "MDialogs.h"
#include "MailFolderCC.h"
#include "MessageCC.h"
#include "SendMessageCC.h"


/* Interface MInterface*/

class MInterface
{
public:



virtual MAppBase * GetMApplication (void) = 0;



virtual ProfileBase * CreateProfile (
const char * classname ,
ProfileBase * parent =NULL
) = 0;



virtual ProfileBase * GetGlobalProfile (void) = 0;



virtual ProfileBase * CreateModuleProfile (
const char * classname ,
ProfileBase * parent =NULL
) = 0;



virtual void  Message (
const char * message , const MWindow * parent =NULL,
const char * title =MDIALOG_MSGTITLE,
const char * configPath =NULL )
 = 0;



virtual bool  YesNoDialog (
const char * message , const MWindow * parent =NULL,
const char * title =MDIALOG_YESNOTITLE,
bool yesdefault =true,
const char * configPath =NULL )
 = 0;



virtual void  StatusMessage ( const char * message  )
 = 0;



virtual SendMessageCC * CreateSendMessageCC (
ProfileBase *profile ,
Protocol  protocol =Prot_SMTP
) = 0;



virtual MailFolder * OpenFolder (
int typeAndFlags ,
const char *  path 
) = 0;


};



