/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */


















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
const char * message, const wxWindow * parent,
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
const char * message, const wxWindow * parent,
const char * title
)

{

   return MDialog_YesNoDialog(message,parent,title);

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

 return MailFolder::OpenFolder(MFolder_obj(path));

}



virtual ASMailFolder * OpenASMailFolder (
const char *  path
)
{

 return ASMailFolder::OpenFolder(MFolder_obj(path));

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



virtual bool contains_own_address (
      const String & str,
      Profile * profile
   )
{
return ::ContainsOwnAddress(str, profile);

}



virtual void RemoveModule (MModuleCommon * module)
{
mApplication->RemoveModule(module);
}


/* The following line is the end of the MInterface interface definition. */

};

static MInterfaceImpl gs_MInterface;


