///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   strlist.h
// Purpose:     Declares StringList typedef
// Author:      Vadim Zeitlin
// Created:     2010-06-29
// Copyright:   (c) 2010 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_STRLIST_H_
#define M_STRLIST_H_

#include <list>
#include <string>

/// Just a convenient typedef for a list of (narrow char) strings.
typedef std::list<std::string> StringList;

#endif // M_STRLIST_H_
