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


void DummyFunc(void);

///------------------------------
/// MModule interface:
///------------------------------

extern "C"
{
   int InitMModule(int version_major,
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
         return MMODULE_ERR_INCOMPATIBLE_VERSIONS;


      // Call own initialisation functions here.
      DummyFunc();
      
      return MMODULE_ERR_NONE;;
   }

   const char *
   GetName(void)
   {
      return "DummyModule";
   }

   const char *
   GetDescription(void)
   {
      return "A simple demonstration of Mahogany's plugin modules.";
   }

   const char *
   GetVersion(void)
   {
      return "0.00";
   }

   void
   GetMVersion(int *version_major, int *version_minor,
               int *version_release)
   {
      ASSERT(version_major);
      ASSERT(version_minor);
      ASSERT(version_release);
      *version_major = M_VERSION_MAJOR;
      *version_minor = M_VERSION_MINOR;
      *version_release = M_VERSION_RELEASE;
   }
} // extern "C"



///------------------------------
/// Own functionality:
///------------------------------

void
DummyFunc(void)
{
   typedef void (*FptrT)(const char *msg, void *dummy, const char *title);

   FptrT fptr = (FptrT) MModule_GetSymbol("MDialog_Message");
   ASSERT(fptr);
   
   (*fptr)(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");

}
