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
// this is compiler dependent
#ifdef USE_MODULES_STATIC
#   define MDLLEXPORT static
#else
#   ifdef OS_WIN
#      ifdef _MSC_VER
#            define MDLLEXPORT __declspec( dllexport )
#   else
#         error "don't know how export functions from DLL with this compiler"
#      endif
#   else
#      define MDLLEXPORT extern
#   endif
#endif


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
   virtual const String &GetInterface(void) const = 0;
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
   /// Returns the name of the interface ABC that this module implements.
   virtual const char * GetInterface(void) = 0;
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
   /** Finds a module which provides the given interface. Only
       searches already loaded modules.
       @param interface name of the interface
       @return an inc-ref'd module or NULL
   */
   static MModule *GetProvider(const wxString &interfaceName);
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
   /** Function type for GetInterface() function.
       @return pointer to a static buffer with the module name
   */
   typedef const char  * (* MModule_GetInterfaceFuncType ) (void);
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
   typedef void (* MModule_GetMVersionFuncType )
      (int *version_major, int *version_minor,
       int *version_release);
   /** Function called just before DLL gets unloaded, i.e. when the
       module gets deleted.
       @return 0 if unloading the DLL is not permitted.
    */
   typedef int (* MModule_UnLoadModuleFuncType) (void);
}
//@}

#if 0
/** Function to resolve main program symbols from modules.
 */
extern "C"
{
   extern void * MModule_GetSymbol(const char *name);
}
#endif


#ifdef USE_MODULES_STATIC
/** Used by modules to register themselves statically.
    Return code is always 1. */
extern
int MModule_AddStaticModule(MModule_InitModuleFuncType init,
                            MModule_GetNameFuncType gn,
                            MModule_GetInterfaceFuncType gi,
                            MModule_GetDescriptionFuncType gs,
                            MModule_GetVersionFuncType gv,
                            MModule_GetMVersionFuncType gmv,
                            MModule_UnLoadModuleFuncType unl);
#endif

/** Call this before application exit. */
extern
void MModule_Cleanup(void);

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


/** Used by modules to register themselves statically. */
/*    Return code is always 1. */
#ifdef USE_MODULES_STATIC 
#define MMODULE_INITIALISE static int dummy = \
MModule_AddStaticModule(InitMModule,GetName, GetInterface, \
                        GetDescription, GetModuleVersion, GetMVersion, UnLoadMModule);
#else
#   define MMODULE_INITIALISE
#endif   

/** This macro does all the module definition, greatly simplifying
    most modules' source code.
    @param ModuleName name of the module file
    @param InterfaceName name of the module interface
    @param ShortDescription short description string for module
    @param InitFunction name of the function to initialise module
    @param codeForUnload name of a function for unloadint the module
*/
#define MMODULE_DEFINE_MODULE(ModuleName, InterfaceName, \
                              ShortDescription, ModuleVersion, \
                              InitFunction, codeForUnload) \
extern "C" \
{ \
   /* Gets called to initialise module: */ \
   MDLLEXPORT int InitMModule(int version_major, \
                              int version_minor, \
                              int version_release, \
                              MInterface *minterface) \
   { \
      /* This test is done here rather than in the MModule.cpp loading \
         code, as a module can be more flexible in accepting certain\
         versions as compatible than Mahogany.\
         I.e. the module knows Mahogany, but Mahogany doesn't\\
         necesarily know the module.\
      */\
   if(! MMODULE_VERSION_COMPATIBLE() )\
       return MMODULE_ERR_INCOMPATIBLE_VERSIONS;\
      return InitFunction(minterface);\
   }\
   /* Gets called before module gets unloaded, return 0 to veto dll unloading:*/\
   MDLLEXPORT int UnLoadMModule(void){ return codeForUnload(); }\
\
   /* Return module name, must be identical to filename without extension:*/\
   MDLLEXPORT const char * GetName(void) { return ModuleName; }\
\
   /* Return name of the interface implemented. */\
   MDLLEXPORT const char * GetInterface(void){ return InterfaceName; }\
\
   /* Return a short description (one line):*/\
   MDLLEXPORT const char *\
   GetDescription(void)   { return ShortDescription; }\
\
/* Return module version as a string:*/ \
   MDLLEXPORT const char * GetModuleVersion(void)   { return ModuleVersion; }\
\
/* Return the Mahogany version this module was compiled for: */\
   MDLLEXPORT void \
   GetMVersion(int *version_major, int *version_minor,\
               int *version_release)\
   {\
      ASSERT(version_major);\
      ASSERT(version_minor);\
      ASSERT(version_release);\
      *version_major = M_VERSION_MAJOR;\
      *version_minor = M_VERSION_MINOR;\
      *version_release = M_VERSION_RELEASE;\
   }\
} /* extern "C" */ \
MMODULE_INITIALISE




//@}
#endif // MMODULE_H
