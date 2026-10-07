///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   UIdArray.h
// Purpose:     declares UIdArray type
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _UIDARRAY_H_
#define _UIDARRAY_H_

#include <vector>

/// Array of message UIDs.
using UIdArray = std::vector<UIdType>;

/// Array of message numbers, currently the same type as UIdArray.
using MsgnoArray = UIdArray;

#endif // _UIDARRAY_H_

