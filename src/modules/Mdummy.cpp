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
   MMODULE_DEFINE(DummyModule)
private:
   /** Dummy constructor.
       As the class has no usable interface, this doesn´t do much, but 
       it displays a small dialog to say hello.
       A real module would store the MInterface pointer for later
       reference and check if everything is set up properly.
   */
   DummyModule(MInterface *interface);
   DEFAULT_ENTRY_FUNC
};


MMODULE_IMPLEMENT(DummyModule,
                  "Mdummy",
                  "none",
                  "This module demonstrates the MModule plugin interface.",
                  "0.00")


///------------------------------
/// Own functionality:
///------------------------------

/* static */
MModule *
DummyModule::Init(int version_major, int version_minor, 
                  int version_release, MInterface *interface,
                  int *errorCode)
{
   if(! MMODULE_SAME_VERSION(version_major, version_minor,
                             version_release))
   {
      if(errorCode) *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS;
      return NULL;
   }
   return new DummyModule(interface);
}


DummyModule::DummyModule(MInterface *minterface)
{
   minterface->Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
}
