#line 1 "MInterface.mid"
/*
 * $Id$
 *
 *.mid file for generating IDL and .h files for Mahogany C interface
 */
#line 3 "mid2h.m4"

#line 7





#line 15





#line 29



#line 6 "MInterface.mid"


#include "MApplication.h"

#ifdef __cplusplus
#line 10
extern "C" {
#line 10
#endif

#line 19
/* Interface MApi*/
#line 19

#line 19

#line 19

#line 19

#line 19
#ifdef MINTERFACE_IMPLEMENTATION
#line 19
extern ProfileBase * MApi_GetGlobalProfile (void)
#line 19
{
#line 19

#line 19
 return mApplication->GetProfile();
#line 19

#line 19
}
#line 19
#else
#line 19
typedef ProfileBase * (* MApi_GetGlobalProfile_Type) (void);
#line 19
MApi_GetGlobalProfile_Type MApi_GetGlobalProfile = (MApi_GetGlobalProfile_Type)MModule_GetSymbol("PREFIX_GetGlobalProfile");
#line 19
#endif
#line 19

#line 19

#line 19



#ifdef __cplusplus
#line 22
}
#line 22
#endif
#line 22

