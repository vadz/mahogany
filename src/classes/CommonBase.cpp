/*-*- c++ -*-********************************************************
 * CommonBase: common base class                                    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/30 17:56:04  KB
 * Python integration mostly complete, added hooks and sample callbacks.
 * Wrote documentation on how to use it.
 *
 * Revision 1.3  1998/05/11 20:57:25  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#include  "Mpch.h"
#include  "Mcommon.h"

#ifdef USE_COMMONBASE

// define ClassInfo structure
CB_IMPLEMENT_CLASS(CommonBase, CommonBase);
DEFINE_MAGIC(CommonBase, MAGIC_COMMONBASE);

void
CommonBase::Debug(void) const
{
   cerr << "DebugInfo for class \""
	<< "this = " << this << endl
	<< "size = " << sizeof (*this) << endl;
}

#if USE_MEMDEBUG
void
CommonBase::Validate(void)
{
   if(this == NULL)
      abort();
   if(cb_class_magic != cb_magic)
      abort();
}
#endif


#endif // USE_COMMONBASE

