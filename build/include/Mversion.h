///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   Mversion.h - contains the M version and patchlevel
// Purpose:     Version information for Mahogany
// Author:      Vadim Zeitlin
// Modified by: CMake build system
// Created:     20.08.98
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef   _M_VERSION_H
#define   _M_VERSION_H

// ----------------------------------------------------------------------------
// version info constants (configured by CMake)
// ----------------------------------------------------------------------------

/// version info
#define M_VERSION_MAJOR   0
#define M_VERSION_MINOR   68
#define M_VERSION_RELEASE 0
#define M_VERSION_STATUS  wxEmptyString // "a"=alpha

/// the macros to build the version string from the components
#define M_VER_STR(x) _T(#x)
#define M_MAKE_VERSION(a, b, c) M_VER_STR(a) _T(".") M_VER_STR(b) _T(".") M_VER_STR(c)

/// short version string (it should always have this format!)
#define M_VERSION         \
   M_MAKE_VERSION(M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE)

/// full version string
#define M_VERSION_STRING  M_VERSION _T(" 'Cynthia'")

/// CMake build system version
#define M_CMAKE_VERSION "0.68.0"

#endif  //_M_VERSION_H
