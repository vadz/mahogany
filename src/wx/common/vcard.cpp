///////////////////////////////////////////////////////////////////////////////
// Name:        common/vcard.cpp
// Purpose:     wxVCard class implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.05.00
// RCS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
    #pragma implementation "vcard.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/app.h"
    #include "wx/dynarray.h"
    #include "wx/filefn.h"
#endif //WX_PRECOMP

#include "vcc.h"

#define VOBJECT_DEFINED

#include "vcard.h"

// ============================================================================
// implementation
// ============================================================================
