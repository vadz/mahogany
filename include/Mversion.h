///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   version.h - contains the M version and patchlevel
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _M_VERSION_H
#define   _M_VERSION_H

// ----------------------------------------------------------------------------
// version info constants
// ----------------------------------------------------------------------------

/// version info
#define M_VERSION_MAJOR   0
#define M_VERSION_MINOR   66
#define M_VERSION_RELEASE 0
#define M_VERSION_STATUS  "" // "a"=alpha

/// the macros to build the version string from the components
#define M_VER_STR(x) #x
#define M_MAKE_VERSION(a, b, c) M_VER_STR(a) "." M_VER_STR(b) "." M_VER_STR(c)

/// short version string (it should always have this format!)
#define M_VERSION         \
   M_MAKE_VERSION(M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE)

/// full version string
#define M_VERSION_STRING  M_VERSION " 'Clio'"

#endif  //_M_VERSION_H
