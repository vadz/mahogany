/*-*- c++ -*-********************************************************
 * Mdummy - a dummy module for Mahogany                             *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mconfig.h"
#   include "Mcommon.h"
#   include "MDialogs.h"
#endif

#include "MModule.h"

#include "Mversion.h"
#include "MInterface.h"

///------------------------------
/// MModule interface:
///------------------------------
static int UnloadDummy(void);
static int DummyFunc(MInterface *minterface);

/** This line does everything.
    If you want to do something more sophisticated, like accepting
    non-matching Mahogany version numbers, etc, you need to copy the
    code from MModule.h, or better, provide another macro and use that 
    here. But this should do the job in 90% of the cases and
    transparently handles static and dynamic linkage. (KB) */

MMODULE_DEFINE_MODULE("Mdummy", "none", "This module demonstrates the MModule plugin interface.",
                      "0.00", DummyFunc, UnloadDummy)

///------------------------------
/// Own functionality:
///------------------------------

static int
DummyFunc(MInterface *minterface)
{
   minterface->Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
   return MMODULE_ERR_NONE;
}

static int UnloadDummy(void)
{
   return 1; // success
}
