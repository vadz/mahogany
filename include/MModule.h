/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
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

/// Name of the function which initialised each DLL
#define MMODULE_INITMODULE_FUNCTION "InitMModule"

/// Error values returned when loading
enum MModule_Errors
{
   /// no error
   MMODULE_ERR_NONE = 0,
   /// incompatible versions
   MMODULE_ERR_INCOMPATIBLE_VERSIONS
};

/**@name Module flags */
//@{
/// Module has main work function
#define MMOD_FLAG_HASMAIN   0x0001
/// Module has config function
#define MMOD_FLAG_HASCONFIG  0x0002
//@}


/** This class is used as an entry in the MModuleListing and describes 
    a single module. */
class MModuleListingEntry
{
public:
   virtual const String & GetName(void) const = 0;
   virtual const String & GetDescription(void) const = 0;
   virtual const String & GetShortDescription(void) const = 0;
   virtual const String & GetVersion(void) const = 0;
   virtual const String & GetAuthor(void) const = 0;
   virtual const String & GetInterface(void) const = 0;
   /// this one returns NULL for non-loaded modules
   virtual class MModule * GetModule(void) const = 0;
   virtual ~MModuleListingEntry() {}
};

/** This list of available modules gets returned from
    MModule::ListAvailableModules(). */
class MModuleListing : public MObjectRC
{
public:
   /// returns the number of entries
   virtual size_t Count(void) const = 0;
   /// returns the n-th entry
   virtual const MModuleListingEntry & operator[] (size_t n) const = 0;
   MOBJECT_NAME(MModuleListing);
};


/**@name Function definitions as used by Entry() */
//@{
/// GetFlags(): Returns bitwise or of the MMOD_FLAG_ values
#define MMOD_FUNC_GETFLAGS   0
/// Main(): Run the main module functionality
#define MMOD_FUNC_MAIN       1
/// Config(): Run the module configuration function
#define MMOD_FUNC_CONFIG     2
/** ProcessMenuId(): Process the menu id passed as additional
    argument. Return true if it processed it.
*/
#define MMOD_FUNC_MENUEVENT  3
/** below this number are reserved, from this one on available for
    module specific functions: */
#define MMOD_FUNC_USER     0x100
//@}

/**
   Small class to ensure proper deregistration of modules on
   deletion.
 */
class MModuleCommon : public MObjectRC
{
protected:
   /// Removes the module from the global list
   virtual ~MModuleCommon() { }
};

/**
   This is the interface for Mahogany extension modules.
   Only simple types like const char * are used to keep modules as
   simple as possible and reduce dependencies on other libraries
   (e.g. wxWindows or libstc++).
*/
class MModule : public MModuleCommon
{
public:
   /** MModule interface, this needs to be implemented by the actual
       modules. */
   //@{
   /// Returns the Module's name as used in LoadModule().
   virtual const char * GetName(void) const = 0;
   /// Returns the name of the interface ABC that this module implements.
   virtual const char * GetInterface(void) const = 0;
   /// Returns a brief description of the module.
   virtual const char * GetDescription(void) const = 0;
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void) const = 0;
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release) const = 0;
   /// Set arg to a function number and call the function if it exists.
   virtual int Entry(int arg, ... ) = 0;
   //@}
   
   /** These static functions handle the loading of modules. */
   //@{
   /** This will load the module of a given name if not already
       loaded.
       @param name name of the module.
       @return pointer to the freshly loaded MModule or NULL.
   */
   static MModule * LoadModule(const String & name);
   /** Returns a pointer to a listing of all loaded modules. Must be
       DecRef()´d by the caller. */
   static MModuleListing * ListLoadedModules(void);
   /** Returns a pointer to a listing of available modules. Must be
       DecRef()'d by the caller. Does not check if modules are loaded, 
       i.e. GetModule() from these entries will return NULL.
       If interfaceName is not empty, will only return modules implementing
       the given interface.
   */
   static MModuleListing * ListAvailableModules(const String& interfaceName);
   /** Finds a module which provides the given interface. Only
       searches already loaded modules.
       @param interface name of the interface
       @return an inc-ref'd module or NULL
   */
   static MModule *GetProvider(const wxString &interfaceName);
   //@}

   MOBJECT_NAME(MModule)
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
       @param errorCode set this to 0 if no error
       @return a pointer to the module or NULL in case of error
   */
   typedef MModule *(* MModule_InitModuleFuncType) (int version_major,
                                                    int version_minor,
                                                    int version_release,
                                                    class MInterface *minterface,
                                                    int *errorCode);
}
//@}

#ifdef USE_MODULES_STATIC
/** Used by modules to register themselves statically.
    Return code is always 1. */
extern
int MModule_AddStaticModule(const char *Name,
                             const char *Interface,
                             const char *Description,
                             const char *Version,
                            MModule_InitModuleFuncType init);

/** Used by modules to register themselves statically. */
#   define MMODULE_INITIALISE(Name, Interface, Description, Version) \
    static int dummy = MModule_AddStaticModule(Name, Interface, \
                                               Description, Version, InitMModule);
#else
#   define MMODULE_INITIALISE(a,b,c,d)
#endif   


#define MMODULE_DEFINE_CMN(ClassName) \
public: \
virtual const char * GetName(void) const; \
virtual const char * GetDescription(void) const; \
virtual const char * GetInterface(void) const; \
virtual const char * GetVersion(void) const; \
virtual void GetMVersion(int *version_major, \
                         int *version_minor, \
                         int *version_release) const; \
virtual ~ClassName(); \
static  MModule *Init(int, int, int, MInterface *, int *); 

#define DEFAULT_ENTRY_FUNC   virtual int Entry(int /* arg */, ...) { return 0; }

#ifdef DEBUG
#   define MMODULE_DEFINE(ClassName) \
      MMODULE_DEFINE_CMN(ClassName) \
      virtual const char *DebugGetClassName() const { return #ClassName; }
#else
#   define MMODULE_DEFINE(ClassName)      MMODULE_DEFINE_CMN(ClassName)
#endif


#define MMODULE_IMPLEMENT(ClassName, Name, Interface, Description, Version) \
const char * ClassName::GetName(void) const { return Name; } \
const char * ClassName::GetDescription(void) const { return Description; } \
const char * ClassName::GetInterface(void) const { return Interface; } \
const char * ClassName::GetVersion(void) const { return Version; } \
void ClassName::GetMVersion(int *version_major, int *version_minor, \
                            int *version_release) const \
{ \
   if(version_major)   *version_major = M_VERSION_MAJOR; \
   if(version_minor)   *version_major = M_VERSION_MINOR; \
   if(version_release) *version_major = M_VERSION_RELEASE; \
} \
extern "C" \
{\
   MDLLEXPORT MModule *InitMModule(int version_major,\
                           int version_minor,\
                           int version_release,\
                           class MInterface * minterface,\
                           int *errorCode)\
   {\
      return ClassName::Init(version_major,  version_minor, \
                             version_release, \
                             minterface , errorCode);\
   }\
} \
MMODULE_INITIALISE(Name, Interface, Description, Version)

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
#define MMODULE_SAME_VERSION(a,b,c) (a == M_VERSION_MAJOR && \
                                   b == M_VERSION_MINOR && \
                                   c == M_VERSION_RELEASE)
/** Test whether version_xxx is compatible with the one the module is compiled for.
*/
#define MMODULE_VERSION_COMPATIBLE(a,b) (a_major == M_VERSION_MAJOR && \
                                         b_minor >= M_VERSION_MINOR)

//@}



//@}
#endif // MMODULE_H
