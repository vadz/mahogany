/*
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */














#include "MApplication.h"
#include "gui/wxMDialogs.h"


/* Interface MInterface*/

class MInterface
{
public:



wxMApp * GetMApplication (void);



ProfileBase * CreateProfile (
const char * classname ,
ProfileBase * parent =NULL
);



ProfileBase * GetGlobalProfile (void);



void  Message (
const char * message , const MWindow * parent =NULL,
const char * title =MDIALOG_MSGTITLE,
const char * configPath =NULL )
;





};



