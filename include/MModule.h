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

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/**@name Mahogany Module management classes. */
//@{

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

class MModule;


/**
   This is an ABC that every MModule must implement.

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

//@}
#endif // MMODULE_H
