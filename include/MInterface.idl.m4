/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */
/*
 * $Id$
 *
 * M4 definitions for converting .mid to ,idl and .h
*/

















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

interface MInterface
{


MAppBase * GetMApplication (void);

Profile * CreateProfile (
in string classname,
in Profile * parent
);

Profile * GetGlobalProfile (void);

Profile * CreateModuleProfile (
in string classname,
in Profile * parent
);

wxPListCtrl * CreatePListCtrl (
in string name,
in wxWindow * parent,
in long id,
in long style
);


Message * CreateMessage (
in string text,
in unsigned long uid,
in Profile * profile
);

void  MessageDialog (
in string message, in const wxWindow * parent,
in string title,
in string configPath )
;

void  Log (
in int level,
in string message
)
;

bool  YesNoDialog (
in string message, in const wxWindow * parent,
in string title,
in bool yesdefault,
in string configPath )
;

void  StatusMessage ( in string message )
;

SendMessage * CreateSendMessage (
in Profile *profile,
in Protocol  protocol
);

MFolder * GetMFolder (
in string name
);

MailFolder * OpenMailFolder (
in string  path
);

ASMailFolder * OpenASMailFolder (
in string  path
);

bool CreateMailFolder (
in string name,
in long type,
in long flags,
in string path,
in string comment
);



void strutil_tolower ( in String & str);

class strutil_RegEx * strutil_compileRegEx (
in const String & pattern,
in int flags);
bool strutil_matchRegEx (
in const class strutil_RegEx *regex ,
in const String & str,
in int flags
);
void strutil_freeRegEx (
in strutil_RegEx * re);

void RemoveModule (in MModuleCommon * module);

/* The following line is the end of the MInterface interface definition. */

};


