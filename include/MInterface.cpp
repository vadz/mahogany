/*
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
    MAppBase * GetMApplication (void);

    ProfileBase * CreateProfile (
    const char * classname,
    ProfileBase * parent
    );


    ProfileBase *GetGlobalProfile (void);

    void  Message (
    const char * message, const MWindow * parent,
    const char * title,
    const char * configPath );
};


MAppBase * MInterfaceImpl::GetMApplication (void)
{

 return mApplication;

}



ProfileBase * MInterfaceImpl::CreateProfile (
const char * classname,
ProfileBase * parent
)
{

 return ProfileBase::CreateProfile(classname,parent);

}



ProfileBase * MInterfaceImpl::GetGlobalProfile (void)
{

 return mApplication->GetProfile();

}



void  MInterfaceImpl::Message (
const char * message, const MWindow * parent,
const char * title,
const char * configPath )

{

 MDialog_Message(message,parent,title,configPath);

}









