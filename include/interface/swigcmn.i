///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/swigcmn.i: common stuff for several SWIG files
// Purpose:     hacks to make SWIG grok our headers
// Author:      Vadim Zeitlin
// Modified by:
// Created:     28.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// we don't want to export MObjectRC (it doesn't provide anything except
// reference counting which any self respecting script language already has
// anyhow), so just pretend it is empty for SWIG
class MObjectRC { };

// SWIG can't deal with the macros below -- no big deal, we don't need to
// export them anyhow, so just hide them from SWIG
#define WXDLLEXPORT
#define GCC_DTOR_WARN_OFF
#define MOBJECT_NAME(x)

#define DECLARE_AUTOPTR(x)
#define DECLARE_AUTOPTR_WITH_CONVERSION(x)
#define DECLARE_AUTOPTR_NO_REF(x)

