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
#include "Mconfig.h"
#include "Mcommon.h"
#include "MModule.h"

#include "MDialogs.h"

class DummyModule : public MModule
{
public:
   static MModule *Create(void);

   /// Returns the Module's name as used in LoadModule().
   virtual const char * GetName(void) const;
   /// Returns a brief description of the module.
   virtual const char * GetDescription(void) const;
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void) const;
private:
   DummyModule();
};

///------------------------------
/// MModule interface:
///------------------------------

const char *
DummyModule::GetName(void) const
{
   return "DummyModule";
}

const char *
DummyModule::GetDescription(void) const
{
   return "A simple demonstration of Mahogany's plugin modules.";
}

const char *
DummyModule::GetVersion(void) const
{
   return "0.00";
}

///------------------------------
/// Own functionality:
///------------------------------

DummyModule::DummyModule(void)
{
   MDialog_Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
}





///------------------------------
/// Loader functions:
///------------------------------

/* static */
MModule *DummyModule::Create(void)
{
   return new DummyModule();
}


extern "C"
{
   MModule * CreateModule(void)
   {
      return DummyModule::Create();
   }
};
