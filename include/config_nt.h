///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   config_nt.h - config.h replacement for Win32
// Purpose:     this file contains hand-generated options for Windows
// Author:      Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/** Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/** Define if you have the <compface.h> header file.  -- we always have it */
#define HAVE_COMPFACE_H 1

/** Do we have the pisock library and headers? */
#undef USE_PISOCK

/** Do we have a pisock library with setmaxspeed? */
#undef HAVE_PI_SETMAXSPEED

/** Do we have a pisock library with pi_accept_to? */
#undef HAVE_PI_ACCEPT_TO

/** Define if you use Python. */
#define USE_PYTHON 1

/** Define if you have libswigpy */
#undef HAVE_SWIGLIB

/** Define this if you want to use RBL to lookup SPAM domains. */
#undef USE_RBL

/** Define this if you want to compile in SSL support. */
#define USE_SSL

/** Define this to use built-in i18n support */
#define USE_I18N

/** Define this to build M with dialup support */
#define USE_DIALUP

/** Only static modules are supported under Windows so far */
#define USE_MODULES
#define USE_MODULES_STATIC
