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

static void DummyFunc(MInterface *minterface);

// this is compiler dependent
#ifdef OS_WIN
#   ifdef _MSC_VER
#         define MDLLEXPORT __declspec( dllexport )
#else
#      error "don't know how export functions from DLL with this compiler"
#   endif
#else
#   define MDLLEXPORT
#endif

///------------------------------
/// MModule interface:
///------------------------------

extern "C"
{
   /// Gets called to initialise module:
   MDLLEXPORT int InitMModule(int version_major,
                              int version_minor,
                              int version_release,
                              MInterface *minterface)
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
      DummyFunc(minterface);
      
      return MMODULE_ERR_NONE;;
   }
   /// Gets called before module gets unloaded, return 0 to veto dll unloading:
   MDLLEXPORT int
   UnLoadMModule(void)
   {
      return 1;
   }

   /// Return module name, must be identical to filename without extension:
   MDLLEXPORT const char *
   GetName(void)
   {
      return "Mdummy";
   }

   /// Return name of the interface implemented.
   MDLLEXPORT const char *
   GetInterface(void)
   {
      return "none";
   }

   /// Return a short description (one line):
   MDLLEXPORT const char *
   GetDescription(void)
   {
      return "A simple demonstration of Mahogany's plugin modules.";
   }

   /// Return module version as a string:
   MDLLEXPORT const char *
   GetModuleVersion(void)
   {
      return "0.00";
   }

   /// Return the Mahogany version this module was compiled for:
   MDLLEXPORT void
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
DummyFunc(MInterface *minterface)
{
#if 0
   typedef void (*FptrT)(const char *msg, void *dummy, const char *title);

   FptrT fptr = (FptrT) MModule_GetSymbol("MDialog_Message");
   ASSERT(fptr);
#endif
   
   minterface->Message(
      "This message is created by the DummyModule plugin\n"
      "for Mahogany. This module has been loaded at runtime\n"
      "and is not part of the normal Mahogany executable.",
      NULL,
      "Welcome from DummyModule!");
   
}
