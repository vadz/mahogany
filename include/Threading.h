///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   Threading.h - sorting related constants and functions
// Purpose:     defines ThreadParams
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.09.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _THREADING_H_
#define _THREADING_H_

#ifdef __GNUG__
   #pragma interface "Threading.h"
#endif

class Profile;
class HeaderInfoList;

/// struct holding all threading parameters
struct ThreadParams
{
   /// do we thread at all?
   bool useThreading;

   /// the strings to use to bring subject to canonical form
   String simplifyingRegex;
   String replacementString;

   /// Should we gather in same thread messages with same subject
   bool gatherSubjects;

   /// Should we break thread when subject changes
   bool breakThread;

   /// Should we indent messages with missing ancestor
   bool indentIfDummyNode;

   /// def ctor
   ThreadParams();

   /// read the sort params from profile
   void Read(Profile *profile);

   /// compare SortParams
   bool operator==(const ThreadParams& other) const;
   bool operator!=(const ThreadParams& other) const { return !(*this == other); }
};

/// the function which threads messages according to the JWZ algorithm
extern void JWZThreadMessages(const ThreadParams& thrParams,
                              const HeaderInfoList *hilp,
                              MsgnoType *indices,
                              size_t *indents);

#endif // _THREADING_H_

