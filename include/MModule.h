/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/



#ifndef MMODULE_H
#define MMODULE_H

#ifdef __GNUG__
#   pragma interface "MModule.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "MObject.h"

#include <wx/dynlib.h>
// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/**@name Mahogany Module management classes. */
//@{

/**
   This is the interface that every MModule must implement.
*/
class MModule : public MObjectRC
{
public:
   /** This will load the module of a given name if not already
       loaded.
       @param name name of the module.
       @return pointer to the freshly loaded MModule or NULL.
   */
   static MModule * LoadModule(const String & name);

   /// Returns the Module's name as used in LoadModule().
   virtual const char * GetName(void) const = 0;
   /// Returns a brief description of the module.
   virtual const char * GetDescription(void) const = 0;
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void) const = 0;
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release) const = 0;
};

/** Function type for CreateModule() function.
    Each module DLL must implement a function CreateMModule of this
    type which will be called to initialise it. That function must
    @param version_major major version number of Mahogany 
    @param version_minor minor version number of Mahogany 
    @param version_release release version number of Mahogany 
    @return NULL if it could not initialise the module.
*/
extern "C"
{
   typedef class MModule * (* CreateModuleFuncType) (int version_major,
                                                     int version_minor,
                                                     int version_release);
};

/** Function to resolve main program symbols from modules.
 */
extern "C"
{
   void * MModule_GetSymbol(const char *name)
   {
      wxDllType prog = wxDllLoader::GetProgramHandle();
      return (void *) wxDllLoader::GetSymbol(prog, name);
   }
}
   

/**@name Two macros to make testing for compatible versions easier. */
//@{
/** Test whether version_xxx is identical to the one a module is
    compiled for.
    The two macros require the version parameters to be names as in
    the prototype for CreateMModule.
*/
#define MMODULE_SAME_VERSION() (version_major == M_VERSION_MAJOR && \
                                version_minor == M_VERSION_MINOR && \
                                version_release == M_VERSION_RELEASE)
/** Test whether version_xxx is compatible with the one the module is compiled for.
*/
#define MMODULE_VERSION_COMPATIBLE() (version_major == M_VERSION_MAJOR && \
                                      version_minor >= M_VERSION_MINOR)

//@}
//@}
#endif // MMODULE_H
