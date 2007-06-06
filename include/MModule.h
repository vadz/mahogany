/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MMODULE_H
#define MMODULE_H

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "MObject.h"

class WXDLLEXPORT wxDynamicLibrary;

class MInterface;

#include "Mversion.h"      // for M_VERSION_MAJOR &c

#include "MAtExit.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// this is compiler dependent
#ifdef USE_MODULES_STATIC
#  define MDLLEXPORT static
#else
#  ifdef OS_WIN
#     define MDLLEXPORT WXEXPORT
#  else
#     define MDLLEXPORT extern
#  endif
#endif

// ----------------------------------------------------------------------------
// the standard module properties
// ----------------------------------------------------------------------------

// the name of the module
#define MMODULE_NAME_PROP _T("name")

// the short description of the module shown in the folder dialog (module won't
// be shown there if it is empty)
#define MMODULE_DESC_PROP _T("desc")

// the long (multiline) description of the module
#define MMODULE_DESCRIPTION_PROP _T("description")

// the interface the module implements (may be empty)
#define MMODULE_INTERFACE_PROP _T("interface")

// the version of the module
#define MMODULE_VERSION_PROP _T("version")

// author/copyright string
#define MMODULE_AUTHOR_PROP _T("author")

/**@name Mahogany Module management classes. */
//@{

/// Name of the function which initialised each DLL
#define MMODULE_INITMODULE_FUNCTION _T("InitMModule")

/// Name of the function used to retrieve info about the module
#define MMODULE_GETPROPERTY_FUNCTION _T("GetMModuleProperties")

/// Name of the function called to do module cleanup
#define MMODULE_CLEANUP_FUNCTION _T("CleanupMModule")

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

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

/**@name Function definitions as used by Entry() */
//@{
enum MMOD_FUNC
{
   /// GetFlags(): Returns bitwise or of the MMOD_FLAG_ values
   MMOD_FUNC_GETFLAGS,
   /// Main(): Run the main module functionality
   MMOD_FUNC_MAIN,
   /// Config(): Run the module configuration function
   MMOD_FUNC_CONFIG,
   /** ProcessMenuId(): Process the menu id passed as additional
       argument. Return true if it processed it.
   */
   MMOD_FUNC_MENUEVENT,
   /// perform module initialization, called after main frame creation
   MMOD_FUNC_INIT,
   /** below this number are reserved, from this one on available for
       module specific functions: */
   MMOD_FUNC_USER = 0x100
};
//@}

/// The name of the interface of modules which should be loaded on startup
#define STARTUP_INTERFACE _T("Startup")

// ----------------------------------------------------------------------------
// classes used for browsing/listing the available modules
// ----------------------------------------------------------------------------

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
   MOBJECT_NAME(MModuleListing)
};

// ----------------------------------------------------------------------------
// ModuleProperty holds a module property which is just a named string value
// ----------------------------------------------------------------------------

struct ModuleProperty
{
   const wxChar *name;
   const wxChar *value;
};

// ----------------------------------------------------------------------------
// MModule class
// ----------------------------------------------------------------------------

/**
   Small class to ensure proper deregistration of modules on
   deletion and to make them ref counted.

   NB: this class can't derive from MObjectRC because the implementation of the
       latter is in the main program and so the module DLLs wouldn't link then!
 */
class MModuleCommon
{
public:
   /// ctor sets the ref count to 1 to make the object alive
   MModuleCommon(MInterface *minterface = NULL)
   {
      m_nRef = 1;
      m_MInterface = minterface;
#ifndef USE_MODULES_STATIC
      m_dll = NULL;
#endif // !USE_MODULES_STATIC
   }

   /// must be used if not specified in the ctor
   void SetMInterface(MInterface *minterface) { m_MInterface = minterface; }

#ifndef USE_MODULES_STATIC
   /// must be used to let the module know about the DLL that it must unload
   void SetDLL(wxDynamicLibrary *dll) { m_dll = dll; }

   /// for MAppBase::RemoveModule() only, in fact
   wxDynamicLibrary *GetDLL() const { return m_dll; }
#endif // !USE_MODULES_STATIC

   virtual MInterface *GetMInterface() { return m_MInterface; }

   virtual void IncRef() { m_nRef++; }
   virtual bool DecRef() { if ( --m_nRef ) return TRUE; delete this; return FALSE; }

protected:
   /// Removes the module from the global list
   virtual ~MModuleCommon();

   MInterface *m_MInterface;

private:
   /// ref count of the module, when it reaches 0, the module is unloaded
   size_t m_nRef;

#ifndef USE_MODULES_STATIC
   /// DLL from which the module was loaded, it is deleted when it's unloaded
   wxDynamicLibrary *m_dll;
#endif // !USE_MODULES_STATIC
};

// for "compatibility" with MObjectRC
inline void SafeIncRef(MModuleCommon *p) { if ( p != NULL ) p->IncRef(); }
inline void SafeDecRef(MModuleCommon *p) { if ( p != NULL ) p->DecRef(); }

/**
   This is the interface for Mahogany extension modules.
   Only simple types like const char * (FIXME Nerijus wxChar * ?) are used to keep modules as
   simple as possible and reduce dependencies on other libraries
   (e.g. wxWindows or libstc++).
*/
class MModule : public MModuleCommon
{
public:
   MModule(MInterface *minterface = NULL) : MModuleCommon(minterface) { }

   /** MModule interface, this needs to be implemented by the actual modules. */
   //@{
   /// Returns the Module's name as used in LoadModule().
   virtual const wxChar * GetName(void) const = 0;
   /// Returns the name of the interface ABC that this module implements.
   virtual const wxChar * GetInterface(void) const = 0;
   /// Returns a brief description of the module.
   virtual const wxChar * GetDescription(void) const = 0;
   /// Returns a textual representation of the particular version of the module.
   virtual const wxChar * GetVersion(void) const = 0;

   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release) const = 0;

   /// Set arg to a function number and call the function if it exists.
   virtual int Entry(int /* MMOD_FUNC */ arg, ... ) = 0;
   //@}

   /** These static functions handle the loading of modules. */
   //@{
   /** This will load the module of a given name if not already loaded.
       @param name name of the module.
       @return pointer to the freshly loaded MModule or NULL.
   */
   static MModule * LoadModule(const String & name);

   /** Returns a pointer to a listing of all loaded modules. Must be
       DecRef()'d by the caller. */
   static MModuleListing * ListLoadedModules(void);

   /** Returns a pointer to a listing of available modules. Must be
       DecRef()'d by the caller. Does not check if modules are loaded,
       i.e. GetModule() from these entries will return NULL.

       @param iface if not empty, will only return modules implementing
                    the given interface, otherwise returns all modules
   */
   static MModuleListing * ListAvailableModules(const String& iface = wxEmptyString);

   /** Finds the first module which provides the given interface. Only
       searches already loaded modules.
       @param interface name of the interface
       @return an inc-ref'd module or NULL
   */
   static MModule *GetProvider(const wxString &interfaceName);
   //@}

   MOBJECT_NAME(MModule)
};

/** @name This is the API that each MModule shared lib must implement. */
//@{
extern "C"
{
   /** Function type for InitModule() function.
       Each module DLL must implement a function CreateMModule of this
       type which will be called to initialise it. That function must
       return the new object representing this module.

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

   /** Function returning information about this module.

       The function should return the name, description and version of the
       module as static strings (they are not freed by the called)
   */
   typedef const ModuleProperty *(* MModule_GetModulePropFuncType)(void);

   /**
      Function called to do module cleanup.

      A module may not have this function in which case nothing is done when it
      is unloaded.
    */
   typedef void (*MModule_CleanUpFuncType)();
}
//@}

// ----------------------------------------------------------------------------
// macros for module declaration/implementation
// ----------------------------------------------------------------------------

#ifdef USE_MODULES_STATIC
/** Used by modules to register themselves statically. */
extern
void MModule_AddStaticModule(const wxChar *Name,
                             const wxChar *Interface,
                             const wxChar *Description,
                             const wxChar *Version,
                             MModule_InitModuleFuncType init);

#   define MMODULE_INITIALISE(ClassName, Name, Interface, Description, Version) \
    struct MStaticModuleInitializerFor##ClassName \
    { \
     MStaticModuleInitializerFor##ClassName() \
     { \
        MModule_AddStaticModule(Name, Interface, \
                                Description, Version, InitMModule); \
     } \
    } gs_moduleInitializerFor##ClassName;

#  define MMODULE_CLEANUP(func) MRunFunctionAtExit moduleCleanup(func);
#else // !USE_MODULES_STATIC
#  define MMODULE_INITIALISE(ClassName, Name, Interface, Description, Version)
#  define MMODULE_CLEANUP(func) \
   extern "C" \
   { \
      MDLLEXPORT void CleanupMModule() { (func)(); } \
   }
#endif // USE_MODULES_STATIC/!USE_MODULES_STATIC

/// helper of MMODULE_BEGIN_IMPLEMENT
#ifdef USE_MODULES_STATIC
   #define MMODULE_DEFINE_GET_PROPERTIES(ClassName)
#else
   #define MMODULE_DEFINE_GET_PROPERTIES(ClassName)                           \
      extern "C"                                                              \
      {                                                                       \
      MDLLEXPORT const ModuleProperty *GetMModuleProperties()                 \
         { return ClassName::ms_properties; }                                 \
      }
#endif

/// this macro must be used inside the class declaration for any module class
#define MMODULE_DEFINE() \
public: \
   virtual const wxChar * GetName(void) const; \
   virtual const wxChar * GetInterface(void) const; \
   virtual const wxChar * GetDescription(void) const; \
   virtual const wxChar * GetVersion(void) const; \
   virtual void GetMVersion(int *version_major, \
                            int *version_minor, \
                            int *version_release) const; \
   static  MModule *Init(int, int, int, MInterface *, int *); \
   static const ModuleProperty ms_properties[]

/// this macro may be used for modules which don't do anything in their Entry()
#define DEFAULT_ENTRY_FUNC   virtual int Entry(int /* arg */, ...) { return 0; }

/// these macros must be used in the .cpp file implementing the module class
#define MMODULE_BEGIN_IMPLEMENT(ClassName, Name, Interface, Description, Version) \
void ClassName::GetMVersion(int *version_major, \
                            int *version_minor, \
                            int *version_release) const \
{ \
   if(version_major)   *version_major = M_VERSION_MAJOR; \
   if(version_minor)   *version_major = M_VERSION_MINOR; \
   if(version_release) *version_major = M_VERSION_RELEASE; \
} \
\
extern "C" \
{ \
   MDLLEXPORT MModule *InitMModule(int version_major,\
                                   int version_minor,\
                                   int version_release,\
                                   MInterface * minterface,\
                                   int *errorCode)\
   {\
      if ( (version_major < M_VERSION_MAJOR) || \
           ((version_major == M_VERSION_MAJOR) && \
            (version_minor < M_VERSION_MINOR)) ) \
      {\
         *errorCode = MMODULE_ERR_INCOMPATIBLE_VERSIONS; \
         return NULL; \
      } \
      \
      MModule *module = ClassName::Init(version_major,  version_minor, \
                             version_release, \
                             minterface , errorCode);\
      if ( module ) \
         module->SetMInterface(minterface); \
      return module; \
   }\
} \
MMODULE_INITIALISE(ClassName, Name, Interface, Description, Version) \
\
const ModuleProperty ClassName::ms_properties[] = \
{ \
   { MMODULE_NAME_PROP, Name }, \
   { MMODULE_DESC_PROP, Description }, \
   { MMODULE_VERSION_PROP, Version }, \
   { MMODULE_INTERFACE_PROP, Interface },

#define MMODULE_PROP(name, value) { name, value },

#define MMODULE_END_IMPLEMENT(ClassName) \
   { NULL, NULL }, \
   }; \
\
MMODULE_DEFINE_GET_PROPERTIES(ClassName) \
\
const wxChar * ClassName::GetName(void) const \
   { return GetMModuleProperty(ms_properties, MMODULE_NAME_PROP); } \
const wxChar * ClassName::GetInterface(void) const \
   { return GetMModuleProperty(ms_properties, MMODULE_INTERFACE_PROP); } \
const wxChar * ClassName::GetDescription(void) const \
   { return GetMModuleProperty(ms_properties, MMODULE_DESC_PROP); } \
const wxChar * ClassName::GetVersion(void) const \
   { return GetMModuleProperty(ms_properties, MMODULE_VERSION_PROP); }

// ----------------------------------------------------------------------------
// helper functions and macros
// ----------------------------------------------------------------------------

/** Get a module property from the properties table */
extern
const wxChar *GetMModuleProperty(const ModuleProperty *table, const wxChar *name);

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
/** Test whether version_xxx is compatible with the one the module is compiled
    for.
*/
#define MMODULE_VERSION_COMPATIBLE(a,b) (a_major == M_VERSION_MAJOR && \
                                         b_minor >= M_VERSION_MINOR)

//@}



//@}
#endif // MMODULE_H
