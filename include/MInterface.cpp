/* -*- c++ -*-
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */















#include "MApplication.h"
#include "gui/wxMDialogs.h"


/* Interface MInterface*/

class MInterfaceImpl : public MInterface
{
public:



virtual MAppBase * GetMApplication (void)
{

 return mApplication;

}



virtual ProfileBase * CreateProfile (
const char * classname,
ProfileBase * parent
)
{

 return ProfileBase::CreateProfile(classname,parent);

}



virtual ProfileBase * GetGlobalProfile (void)
{

 return mApplication->GetProfile();

}



virtual void  Message (
const char * message, const MWindow * parent,
const char * title,
const char * configPath )

{

 MDialog_Message(message,parent,title,configPath);

}





};

static MInterfaceImpl gs_MInterface;



