/* -*- text -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */



















#include "MApplication.h"
#include "Profile.h"
#include "strutil.h"
#include "gui/wxMDialogs.h"
#include "MFolder.h"
#include "MailFolder.h"
#include "ASMailFolder.h"
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
const wxChar * classname,
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
const wxChar * classname,
Profile * parent
)
{

 return Profile::CreateModuleProfile(classname,parent);

}



virtual wxPListCtrl * CreatePListCtrl (
const wxChar * name,
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
const String & message, const wxWindow * parent,
const String & title,
const char * configPath )

{

 MDialog_Message(message,parent,title,configPath);

}



virtual void  Log (
int level,
const wxChar * message
)

{

   wxLogGeneric(level, message);

}



virtual bool  YesNoDialog (
const wxChar * message, const wxWindow * parent,
const wxChar * title
)

{

   return MDialog_YesNoDialog(message,parent,title);

}



virtual void  StatusMessage ( const wxChar * message )

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
const wxChar * name
)
{
 return MFolder::Get(name); 
}



virtual MailFolder * OpenMailFolder (
const wxChar *  path
)
{

 return MailFolder::OpenFolder(MFolder_obj(path));

}



virtual ASMailFolder * OpenASMailFolder (
const wxChar *  path
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


