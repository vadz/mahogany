/*-*- c++ -*-********************************************************
 * Mdummy - a dummy module for Mahogany                             *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
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

class DummyModule : public MModule
{
   MMODULE_DEFINE();

private:
   /** Dummy constructor.
       As the class has no usable interface, this doesn´t do much, but
       it displays a small dialog to say hello.
       A real module would store the MInterface pointer for later
       reference and check if everything is set up properly.
   */
   DummyModule(MInterface *minterface);

   MInterface *m_Minterface;

   DEFAULT_ENTRY_FUNC
};


MMODULE_BEGIN_IMPLEMENT(DummyModule,
                        "Mdummy",
                        "none",
                        "This module demonstrates the MModule plugin interface.",
                        "0.00")
MMODULE_END_IMPLEMENT(DummyModule)


///------------------------------
/// Own functionality:
///------------------------------

/* static */
MModule *
DummyModule::Init(int version_major, int version_minor, int version_release,
                  MInterface *minterface, int *errorCode)
{
   return new DummyModule();
}


DummyModule::DummyModule(MInterface *minterface)
           : MModule(minterface)
{
   minterface->Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
}
