/* -*- c++ -*-
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



virtual MAppBase * GetMApplication (void) = 0;



virtual ProfileBase * CreateProfile (
const char * classname ,
ProfileBase * parent =NULL
) = 0;



virtual ProfileBase * GetGlobalProfile (void) = 0;



virtual void  Message (
const char * message , const MWindow * parent =NULL,
const char * title =MDIALOG_MSGTITLE,
const char * configPath =NULL )
 = 0;





};



