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

#include <wx/dynlib.h>
// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/**@name Mahogany Module management classes. */
//@{

/// Error values returned when loading
enum MModule_Errors
{
   /// no error
   MMODULE_ERR_NONE = 0,
   /// incompatible versions
   MMODULE_ERR_INCOMPATIBLE_VERSIONS
};

class MModuleListingEntry
{
public:
   virtual const String &GetName(void) const = 0;
   virtual const String &GetDescription(void) const = 0;
   virtual const String &GetShortDescription(void) const = 0;
   virtual const String &GetVersion(void) const = 0;
   virtual const String &GetAuthor(void) const = 0;
   virtual ~MModuleListingEntry() {}
};

class MModuleListing : public MObjectRC
{
public:
   /// returns the number of entries
   virtual size_t Count(void) const = 0;
   /// returns the n-th entry
   virtual const MModuleListingEntry & operator[] (size_t n) const = 0;
};

/**
   This is the interface for Mahogany extension modules.
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
   virtual const char * GetName(void) = 0;
   /// Returns a brief description of the module.
   virtual const char * GetDescription(void) = 0;
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void) = 0;
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release) = 0;

/** Returns a pointer to a listing of available modules. Must be
    DecRef()'d by the caller. */
   static MModuleListing * GetListing(void);
};


/** @name This is the API that each MModule shared lib must
    implement. */
//@{
extern "C"
{
   /** Function type for InitModule() function.
       Each module DLL must implement a function CreateMModule of this
       type which will be called to initialise it. That function must
       @param version_major major version number of Mahogany 
       @param version_minor minor version number of Mahogany 
       @param version_release release version number of Mahogany
       @param interface pointer to dummy class providing the interface 
       to Mahogany
       @return 0 if it initialise the module, errno otherwise
   */
   typedef int (* MModule_InitModuleFuncType) (int version_major,
                                               int version_minor,
                                               int version_release,
                                               class MInterface *Minterface);
   /** Function type for GetName() function.
       @return pointer to a static buffer with the module name
   */
   typedef const char  * (* MModule_GetNameFuncType ) (void);
   /** Function type for GetDescription() function.
       @return pointer to a static buffer with the module description
   */
   typedef const char  * (* MModule_GetDescriptionFuncType ) (void);
   /** Function type for GetVersion() function.
       @return pointer to a static buffer with the module description
   */
   typedef const char  * (* MModule_GetVersionFuncType ) (void);
   /** Function type for GetMVersion() function.
       @return pointer to a static buffer with the module description
   */
   typedef const char  * (* MModule_GetMVersionFuncType )
      (int *version_major, int *version_minor,
       int *version_release);
}
//@}

/** Function to resolve main program symbols from modules.
 */
extern "C"
{
   extern void * MModule_GetSymbol(const char *name);
}
   

/**@name Two macros to make testing for compatible versions easier. */
//@{
/** Test whether version_xxx is identical to the one a module is
    compiled for.
    The two macros require the version parameters to be names as in
    the prototype for InitMModule.
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
