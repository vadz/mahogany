/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */


















#include "MApplication.h"
#include "strutil.h"
#include "miscutil.h"
#include "MDialogs.h"
#include "MFolder.h"
#include "MailFolder.h"
#include "Message.h"
#include "SendMessage.h"
#include "wx/persctrl.h"

/* Define the MInterface ABC: */

/* Interface MInterface*/

class MInterfaceImpl : public MInterface
{
public:



virtual MAppBase * GetMApplication (void)
{

 return mApplication;

}



virtual Profile * CreateProfile (
const char * classname,
Profile * parent
)
{

 return Profile::CreateProfile(classname,parent);

}



virtual Profile * GetGlobalProfile (void)
{

 return mApplication->GetProfile();

}



virtual Profile * CreateModuleProfile (
const char * classname,
Profile * parent
)
{

 return Profile::CreateModuleProfile(classname,parent);

}



virtual wxPListCtrl * CreatePListCtrl (
const char * name,
wxWindow * parent,
long int id,
long int style
)
{

return new wxPListCtrl(name, parent, id, wxDefaultPosition,
wxDefaultSize, style);

}




virtual Message * CreateMessage (
const char * text,
UIdType uid,
Profile * profile
)
{

 return Message::Create(text, uid, profile);

}



virtual void  MessageDialog (
const char * message, const MWindow * parent,
const char * title,
const char * configPath )

{

 MDialog_Message(message,parent,title,configPath);

}



virtual void  Log (
int level,
const char * message
)

{

   wxLogGeneric(level, message);

}



virtual bool  YesNoDialog (
const char * message, const MWindow * parent,
const char * title,
bool yesdefault,
const char * configPath )

{

   return MDialog_YesNoDialog(message,parent,title,yesdefault, configPath);

}



virtual void  StatusMessage ( const char * message )

{

  STATUSMESSAGE((message));

}



virtual SendMessage * CreateSendMessage (
Profile *profile,
Protocol  protocol
)
{

 return SendMessage::Create(profile,protocol);

}



virtual MFolder * GetMFolder (
const char * name
)
{
 return MFolder::Get(name); 
}



virtual MailFolder * OpenMailFolder (
const char *  path
)
{

 return MailFolder::OpenFolder(path);

}



virtual ASMailFolder * OpenASMailFolder (
const char *  path
)
{

 return ASMailFolder::OpenFolder(path);

}



virtual bool CreateMailFolder (
const char * name,
long int type,
long int flags,
const char * path,
const char * comment
)
{
 return MailFolder::CreateFolder(name,(FolderType)type,flags,path,comment); 
}





virtual void strutil_tolower ( String & str)
{
::strutil_tolower(str);
}



virtual class strutil_RegEx * strutil_compileRegEx (
const String & pattern,
int flags)
{
return ::strutil_compileRegEx(pattern, flags);
}


virtual bool strutil_matchRegEx (
const class strutil_RegEx *regex ,
const String & str,
int flags
)
{
return ::strutil_matchRegEx(regex, str, flags);
}


virtual void strutil_freeRegEx (
strutil_RegEx * re)
{
::strutil_freeRegEx(re);
}



virtual void RemoveModule (MModuleCommon * module)
{
mApplication->RemoveModule(module);
}



virtual String miscutil_GetReplyAddress (
Profile * p)
{
return ::miscutil_GetReplyAddress(p);
}



virtual String miscutil_GetFromAddress (
Profile * p)
{
return ::miscutil_GetFromAddress(p);
}


/* The following line is the end of the MInterface interface definition. */

};

static MInterfaceImpl gs_MInterface;


