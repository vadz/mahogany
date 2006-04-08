//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   tests/layout/Mpch.h: replacement of Mpch.h for the tests
// Purpose:     when we need to include the source files including Mpch.h
//              in the tests, we don't want to include the real header as it
//              pulls in too much stuff, so provide a minimal replacement
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-04-08
// CVS-ID:      $Id$
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#include "Mcommon.h"
#include "kbList.h"

// this is used in src/gui/wxl*.cpp to detect whether it's being built inside M
// or not, #undef it to show that we're building it separately
#undef M_BASEDIR
