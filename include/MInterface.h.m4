/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */
















#ifndef _MApplication_MID_H_
#define _MApplication_MID_H_

#include "MApplication.h"
#include "Profile.h"
#include "strutil.h"
#include "MDialogs.h"
#include "MFolder.h"
#include "MailFolder.h"
#include "Message.h"
#include "SendMessage.h"
#include "wx/persctrl.h"

/* Define the MInterface ABC: */

/* Interface MInterface*/

class MInterface
{
public:



virtual MAppBase * GetMApplication (void) = 0;



virtual Profile * CreateProfile (
const char * classname ,
Profile * parent =NULL
) = 0;



virtual Profile * GetGlobalProfile (void) = 0;



virtual Profile * CreateModuleProfile (
const char * classname ,
Profile * parent =NULL
) = 0;



virtual wxPListCtrl * CreatePListCtrl (
const char * name ,
wxWindow * parent =NULL,
long int id =-1,
long int style =0
) = 0;




virtual Message * CreateMessage (
const char * text ,
UIdType uid = UID_ILLEGAL,
Profile * profile =NULL
) = 0;



virtual void  MessageDialog (
const char * message , const wxWindow * parent =NULL,
const char * title =MDIALOG_MSGTITLE,
const char * configPath =NULL )
 = 0;



virtual void  Log (
int level ,
const char * message 
)
 = 0;



virtual bool  YesNoDialog (
const char * message , const wxWindow * parent =NULL,
const char * title =MDIALOG_YESNOTITLE,
bool yesdefault =true,
const char * configPath =NULL )
 = 0;



virtual void  StatusMessage ( const char * message  )
 = 0;



virtual SendMessage * CreateSendMessage (
Profile *profile ,
Protocol  protocol =Prot_SMTP
) = 0;



virtual MFolder * GetMFolder (
const char * name 
) = 0;



virtual MailFolder * OpenMailFolder (
const char *  path 
) = 0;



virtual ASMailFolder * OpenASMailFolder (
const char *  path 
) = 0;



virtual bool CreateMailFolder (
const char * name ,
long int type =MF_FILE,
long int flags = MF_FLAGS_DEFAULT,
const char * path = "",
const char * comment = ""
) = 0;





virtual void strutil_tolower ( String & str ) = 0;



virtual class strutil_RegEx * strutil_compileRegEx (
const String & pattern ,
int flags ) = 0;


virtual bool strutil_matchRegEx (
const class strutil_RegEx *regex  ,
const String & str ,
int flags 
) = 0;


virtual void strutil_freeRegEx (
strutil_RegEx * re ) = 0;



virtual void RemoveModule (MModuleCommon * module ) = 0;


/* The following line is the end of the MInterface interface definition. */

};


#endif

