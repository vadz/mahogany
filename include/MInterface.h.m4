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
#include "Address.h"
#include "wx/persctrl.h"

/* Define the MInterface ABC: */

/* Interface MInterface*/

class MInterface
{
public:



virtual MAppBase * GetMApplication (void) = 0;



virtual Profile * CreateProfile (
const wxChar * classname ,
Profile * parent =NULL
) = 0;



virtual Profile * GetGlobalProfile (void) = 0;



virtual Profile * CreateModuleProfile (
const wxChar * classname ,
Profile * parent =NULL
) = 0;



virtual wxPListCtrl * CreatePListCtrl (
const wxChar * name ,
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
const wxChar * message , const wxWindow * parent =NULL,
const wxChar * title =MDIALOG_MSGTITLE,
const wxChar * configPath =NULL )
 = 0;



virtual void  Log (
int level ,
const wxChar * message 
)
 = 0;



virtual bool  YesNoDialog (
const wxChar * message , const wxWindow * parent =NULL,
const wxChar * title =MDIALOG_YESNOTITLE
)
 = 0;



virtual void  StatusMessage ( const wxChar * message  )
 = 0;



virtual SendMessage * CreateSendMessage (
Profile *profile ,
Protocol  protocol =Prot_SMTP
) = 0;



virtual MFolder * GetMFolder (
const wxChar * name 
) = 0;



virtual MailFolder * OpenMailFolder (
const wxChar *  path 
) = 0;



virtual ASMailFolder * OpenASMailFolder (
const wxChar *  path 
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



virtual bool contains_own_address (
      const String & str ,
      Profile * profile 
   ) = 0;



virtual void RemoveModule (MModuleCommon * module ) = 0;


/* The following line is the end of the MInterface interface definition. */

};


#endif

