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
#include "Mversion.h"


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
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release) const;
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

void
DummyModule::GetMVersion(int *version_major, int *version_minor,
                         int *version_release) const
{
   ASSERT(version_major);
   ASSERT(version_minor);
   ASSERT(version_release);
   *version_major = M_VERSION_MAJOR;
   *version_minor = M_VERSION_MINOR;
   *version_release = M_VERSION_RELEASE;
}

///------------------------------
/// Own functionality:
///------------------------------

DummyModule::DummyModule(void)
{
/*
   FIXME:
   This currently does not work, as we cannot access symbols
   inside the main executable. So I will split the main program
   up into a tiny "dummy" executable and a large shared library,
   against which we will link this module, too. That should do the
   trick.
   Though, I'm still trying to find a more intelligent solution...
*/

/*   MDialog_Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
*/
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
   MModule * CreateMModule(int version_major,
                           int version_minor,
                           int version_release)
   {
      /* This test is done here rather than in the MModule.cpp loading 
         code, as a module can be more flexible in accepting certain
         versions as compatible than Mahogany.
         I.e. the module knows Mahogany, but Mahogany doesn't
         necesarily know the module.
      */
      if(! MMODULE_VERSION_COMPATIBLE() )
         return NULL;
      return DummyModule::Create();
   }
};
