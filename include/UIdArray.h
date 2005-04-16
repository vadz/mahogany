///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   UIdArray.h
// Purpose:     declares UIdArray class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _UIDARRAY_H_
#define _UIDARRAY_H_

/** The UIdArray define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.

    We use wxArrayInt to implement it.
*/

#ifndef USE_PCH
#  include <wx/dynarray.h>        // for WX_DEFINE_ARRAY_INT
#endif // USE_PCH

WX_DEFINE_ARRAY_LONG(UIdType, UIdArray);

#ifndef MsgnoArray
   #define MsgnoArray UIdArray
#endif

// this is unused so far
#if 0

/// UIdArray which maintains its items always sorted
WX_DEFINE_SORTED_ARRAY(UIdType, UIdArraySortedBase);

/// the function used for comparing UIDs
inline int CMPFUNC_CONV UIdCompareFunction(UIdType uid1, UIdType uid2)
{
   // an invalid UID is less than all the others
   return uid1 == UID_ILLEGAL ? -1
                              : uid2 == UID_ILLEGAL ? 1
                                                    : (long)(uid1 - uid2);
}

class UIdArraySorted : public UIdArraySortedBase
{
public:
   UIdArraySorted() : UIdArraySortedBase(UIdCompareFunction) { }
};

#endif // 0

#endif // _UIDARRAY_H_

