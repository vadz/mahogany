/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)              *
 *                                                                  *
 * $Id$
 *******************************************************************/

/*
 * This code loads modules dynamically at runtime, or, if linked
 * statically, initialises them at runtime. The refcount of the
 * modules is then kept at >1, so they will never get unloaded.
 */

/* see the comment in the header
#ifdef __GNUG__
#   pragma implementation "MModule.h"
#endif
*/

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef  USE_PCH
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "strutil.h"
#   include "kbList.h"

#   include "Mdefaults.h"
#endif // USE_PCH

#include "modules/Filters.h"    // for FilterRule

#include "MModule.h"

#include <wx/dynlib.h>
#include <wx/textfile.h>        // for wxTextFile

// ----------------------------------------------------------------------------
// Implementation of the MInterface
// ----------------------------------------------------------------------------

#include "MInterface.h"
#include "MInterface.cpp"

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_MODULES_DONT_LOAD;

// ----------------------------------------------------------------------------
// function prototypes
// ----------------------------------------------------------------------------

#ifndef USE_MODULES_STATIC

// return the array of paths to search for modules
static wxArrayString BuildListOfModulesDirs();

#endif // USE_MODULES_STATIC

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define DLL_EXTENSION wxDynamicLibrary::GetDllExt()

// the trace mask used by module loading code
#define M_TRACE_MODULES _T("mmodule")

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/** Structure for holding information about a loaded module. */
struct MModuleListEntry
{
   /// Name under which it was loaded.
   String m_Name;
   /// interface provided
   String m_Interface;
   /// Module pointer.
   MModule *m_Module;
#ifdef USE_MODULES_STATIC
   /// Description
   String m_Description;
   /// Version
   String m_Version;
   /// The init function to call.
   MModule_InitModuleFuncType m_InitFunc;
#endif // USE_MODULES_STATIC
};

/// A list of all loaded modules.
KBLIST_DEFINE(MModuleList, MModuleListEntry);

/// The actual list of all loaded modules.
static MModuleList *gs_MModuleList = NULL;

// ============================================================================
// implementation
// ============================================================================

static
MModuleList *GetMModuleList(void)
{
   if ( !gs_MModuleList )
   {
      gs_MModuleList = new MModuleList;
   }

   return gs_MModuleList;
}

void MAppBase::UnloadDLLs()
{
   while ( !m_dllsToUnload.empty() )
   {
      // FIXME: if we do unload the library, M crashes because modules are
      //        unloaded too soon -- they should really remain in memory for as
      //        long as they're used
#if wxCHECK_VERSION(2, 5, 0)
      (*m_dllsToUnload.begin())->Detach(); // prevent DLL from being unloaded

      delete *m_dllsToUnload.begin();
#endif // wxGTK 2.5.0+

      m_dllsToUnload.pop_front();
   }
}

/* When a module gets deleted it must make sure that it is no longer
   in the module list. */
void MAppBase::RemoveModule(MModuleCommon *module)
{
   CHECK_RET( module, _T("NULL module in MApp::RemoveModule()?") );

   wxLogTrace(M_TRACE_MODULES, _T("Unloading module %p"), module);

#ifndef USE_MODULES_STATIC
   // we can't unload the DLL right now because we're still executing in the
   // modules dtor which is in this DLL (and so doing it would result in
   // immediate crash) -- instead put it in the list of the DLLs to unload
   // which will be processed in wxMApp::OnIdle()
   m_dllsToUnload.push_back(module->GetDLL());
#endif // USE_MODULES_STATIC

   if(gs_MModuleList)
   {
      MModuleList::iterator i;
      for ( i = gs_MModuleList->begin(); i != gs_MModuleList->end(); )
      {
         MModuleListEntry *entry = *i;
         if( entry->m_Module == module )
            i = gs_MModuleList->erase(i);
         else
            ++i;
      }
   }
}

extern
void MModule_Cleanup(void)
{
   if(gs_MModuleList)
   {
      /* For statically linked modules, we must decrement them here to
         avoid memory leaks. The refcount gets raised to 1 before
         anyone uses them, so it should be back to 1 now so that
         DecRef() causes them to be freed.

         We cannot decref them directly under the cursor, as this
         would remove them from the list and the iterator would get
         corrupted, so the iterating cursor points to the next element.
      */
      for (MModuleList::iterator j = gs_MModuleList->begin();
          j != gs_MModuleList->end();)
      {
         MModuleList::iterator i = j;
         ++j;
         if( (**i).m_Module )
            (**i).m_Module->DecRef();
      }

      delete gs_MModuleList;
      gs_MModuleList = NULL;
   }
}


static
MModule *FindModule(const String & name)
{
   MModuleList::iterator i;

   for(i = GetMModuleList()->begin();
       i != GetMModuleList()->end();
       i++)
      if( (**i).m_Name == name )
      {
#ifdef USE_MODULES_STATIC
         int errorCode = 0; //for now we ignore it
         // on-the-fly initialisation for static modules:
         if( (**i).m_Module == NULL )
         {
            // initialise the module:
            (**i).m_Module = (*((**i).m_InitFunc))(
               M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE,
               &gs_MInterface, &errorCode);
         }
#endif // USE_MODULES_STATIC
         if( (**i).m_Module )
            (**i).m_Module->IncRef();

         return (**i).m_Module;
      }
   return NULL; // not found
}


#ifdef USE_MODULES_STATIC
extern
void MModule_AddStaticModule(const wxChar *Name,
                             const wxChar *Interface,
                             const wxChar *Description,
                             const wxChar *Version,
                            MModule_InitModuleFuncType initFunc)
{
   MModuleListEntry *me = new MModuleListEntry;
   me->m_Name = Name;
   me->m_Version = Version;
   me->m_Description = Description;
   me->m_Interface = Interface;
   me->m_Module = NULL;
   me->m_InitFunc = initFunc;
   GetMModuleList()->push_back(me);
}
#else // !USE_MODULES_STATIC
static
MModule *LoadModuleInternal(const String & name, const String &pathname)
{
   wxDynamicLibrary *dll = new wxDynamicLibrary(pathname);
   if ( !dll )
   {
      wxLogTrace(M_TRACE_MODULES, _T("Failed to load module '%s' from '%s'."),
                 name.c_str(), pathname.c_str());

      return NULL;
   }

   wxLogTrace(M_TRACE_MODULES, _T("Successfully loaded module '%s' from '%s'."),
              name.c_str(), pathname.c_str());

   MModule_InitModuleFuncType initFunc =
      (MModule_InitModuleFuncType)dll->GetSymbol(MMODULE_INITMODULE_FUNCTION);


   int errorCode = 255;
   MModule *module = NULL;
   if(initFunc)
   {
      module = (*initFunc)
               (
                  M_VERSION_MAJOR,
                  M_VERSION_MINOR,
                  M_VERSION_RELEASE,
                  &gs_MInterface,
                  &errorCode
               );
   }

   if(module)
   {
      // the module will delete the DLL when it's going to be unloaded
      module->SetDLL(dll);

      wxLogTrace(M_TRACE_MODULES, _T("Loaded module %p"), module);

      MModuleListEntry *me = new MModuleListEntry;
      me->m_Name = name;
      me->m_Module = module;
      MModule_GetModulePropFuncType propFunc =
         (MModule_GetModulePropFuncType)
         dll->GetSymbol(MMODULE_GETPROPERTY_FUNCTION);
      if(propFunc)
         me->m_Interface = GetMModuleProperty((*propFunc)(),
                                              MMODULE_INTERFACE_PROP);
      else
         me->m_Interface = name;
      GetMModuleList()->push_back(me);
   }
   else
   {
      delete dll;

      String msg;
      msg.Printf(_("Cannot initialise module '%s', error code %d."),
                 pathname.c_str(), errorCode);
      MDialog_ErrorMessage(msg);
   }

   return module;
}
#endif // USE_MODULES_STATIC/!USE_MODULES_STATIC

/* static */
MModule *
MModule::LoadModule(const String & name)
{
   // Check if it's already loaded:
   MModule *module = FindModule(name);
   if(module)
   {
      // Yes, just return it (incref happens in FindModule()):
      return module;
   }
#ifdef USE_MODULES_STATIC
   return NULL;
#else // !USE_MODULES_STATIC

   wxArrayString dirs = BuildListOfModulesDirs();
   size_t nDirs = dirs.GetCount();

#ifdef DEBUG
   String path;
   for ( size_t n = 0; n < nDirs; n++ )
   {
      if ( !path.empty() )
         path += ':';
      path += dirs[n];
   }

   wxLogTrace(M_TRACE_MODULES, _T("Looking for module '%s' in the path '%s'."),
              name.c_str(), path.c_str());
#endif // DEBUG

   const wxString moduleExt = DLL_EXTENSION;

   for ( size_t i = 0; i < nDirs; ++i )
   {
      wxString pathname = dirs[i];
      pathname << name << moduleExt;
      if(wxFileExists(pathname))
      {
         if ((module = LoadModuleInternal(name, pathname)) != NULL)
            break;
      }
   }
   return module;
#endif // !USE_MODULES_STATIC/USE_MODULES_STATIC
}

/* static */
MModule *
MModule::GetProvider(const wxString &interfaceName)
{
   RefCounter<MModuleListing> listing(ListAvailableModules(interfaceName));
   if ( !listing )
   {
      wxLogWarning(_("No modules implementing \"%s\" interface found."),
                   interfaceName.c_str());
      return NULL;
   }

   if ( listing->Count() > 1 )
   {
      wxLogWarning(_("Several modules implement \"%s\" interface, you "
                     "should probably disable all but one of them using the "
                     "\"Edit|Modules...\" menu command."),
                   interfaceName.c_str());

      // still return something
   }

   return LoadModule((*listing)[0].GetName());
}

class MModuleListingEntryImpl : public MModuleListingEntry
{
public:
   virtual const String &GetName(void) const
      { return m_Name; }
   virtual const String &GetInterface(void) const
      { return m_Interface; }
   virtual const String &GetShortDescription(void) const
      { return m_ShortDesc; }
   virtual const String &GetDescription(void) const
      { return m_Desc; }
   virtual const String &GetVersion(void) const
      { return m_Version; }
   virtual const String &GetAuthor(void) const
      { return m_Author; }
   virtual MModule *GetModule(void) const
      {
         if(m_Module) m_Module->IncRef();
         return m_Module;
      }
   MModuleListingEntryImpl(const String &name = _T(""),
                           const String &interfaceName = _T(""),
                           const String &shortdesc = _T(""),
                           const String &desc = _T(""),
                           const String &version = _T(""),
                           const String &author = _T(""),
                           MModule *module = NULL)
      {
         m_Name = name;
         m_Interface = interfaceName;
         m_ShortDesc = shortdesc;
         m_Desc = desc;
         m_Version = version;
         m_Author = author;
         m_Module = module;
         if(m_Module) m_Module->IncRef();
         strutil_delwhitespace(m_Name);
         strutil_delwhitespace(m_Interface);
         strutil_delwhitespace(m_Desc);
         strutil_delwhitespace(m_ShortDesc);
         strutil_delwhitespace(m_Version);
         strutil_delwhitespace(m_Author);
      }
   ~MModuleListingEntryImpl()
      {
         if(m_Module) m_Module->DecRef();
      }
   /// must be implemented to handle refcount of MModule pointer:
   MModuleListingEntryImpl & operator=(
      const MModuleListingEntryImpl & newval)
      {
         if(m_Module) m_Module->DecRef();
         m_Name = newval.m_Name; m_Interface = newval.m_Interface;
         m_ShortDesc = newval.m_ShortDesc;
         m_Desc = newval.m_Desc; m_Author = newval.m_Author;
         m_Version = newval.m_Version;
         m_Module = newval.m_Module;
         if(m_Module) m_Module->IncRef();
         return *this;
      }
private:
   /// forbidden:
   MModuleListingEntryImpl(const MModuleListingEntryImpl &);

   String m_Name,
          m_Interface,
          m_ShortDesc,
          m_Desc,
          m_Version,
          m_Author;

   MModule *m_Module;

   GCC_DTOR_WARN_OFF
};

class MModuleListingImpl : public MModuleListing
{
public:
   /// returns the number of entries
   virtual size_t Count(void) const
      { return m_count; }
   /// returns the n-th entry
   virtual const MModuleListingEntry & operator[] (size_t n) const
      { ASSERT(n <= m_count); return m_entries[n]; }
   /// returns the n-th entry
   MModuleListingEntryImpl & operator[] (size_t n)
      { ASSERT(n <= m_count); return m_entries[n]; }
   static MModuleListingImpl *Create(size_t n)
      { return new MModuleListingImpl(n); }
   void SetCount(size_t n)
      { ASSERT(n <= m_count); m_count = n; }
protected:
   MModuleListingImpl(size_t n)
      {
         m_count = n;

         // avoid allocating 0 sized array
         m_entries = m_count > 0 ? new MModuleListingEntryImpl[m_count] : NULL;
      }
   ~MModuleListingImpl()
      { delete [] m_entries; }
private:
   MModuleListingEntryImpl *m_entries;
   size_t m_count;
   GCC_DTOR_WARN_OFF
};

// this function can list all loaded modules (default) or do other things as
// well depending on the parameters values, so the name is a bit unfortunate
static MModuleListing *
DoListLoadedModules(bool listall = false, const String& interfaceName = _T(""))
{
#ifdef USE_MODULES_STATIC
   wxArrayString modulesNot;

   // only exclude modules to ignore if we're looking for modules implementing
   // some interface, not if we want (really) all modules
   if ( listall && !interfaceName.empty() )
   {
      const String modulesDontLoad = READ_APPCONFIG(MP_MODULES_DONT_LOAD);
      modulesNot = strutil_restore_array(modulesDontLoad);
   }
#else
   // this function only works for loaded modules in dynamic case
   ASSERT_MSG( !listall, _T("this mode is not supported with dynamic modules") );
#endif // USE_MODULES_STATIC

   MModuleListingImpl *listing =
      MModuleListingImpl::Create(GetMModuleList()->size());

   size_t count = 0;
   for ( MModuleList::iterator i = GetMModuleList()->begin();
         i != GetMModuleList()->end();
         i++ )
#ifdef USE_MODULES_STATIC
   {
      MModuleListEntry *me = *i;

      // we have unloaded modules in the list, ignore them unless listall is
      // TRUE
      if ( (listall || me->m_Module) &&
           (interfaceName.empty() || me->m_Interface == interfaceName) )
      {
         // also ignore the modules excluded by user
         if ( !listall || modulesNot.Index(me->m_Name) == wxNOT_FOUND )
         {
            MModuleListingEntryImpl
               entry
               (
                  me->m_Name, // module name
                  me->m_Interface,
                  me->m_Description,
                  _T(""), // long description
                  String(me->m_Version) + _(" (builtin)"),
                  _T("mahogany-developers@lists.sourceforge.net"),
                  me->m_Module
               );

            (*listing)[count++] = entry;
         }
      }
   }
#else // !USE_MODULES_STATIC
   {
      MModule *m = (**i).m_Module;
      if ( !m )
      {
         FAIL_MSG( _T("module should be loaded") );

         continue;
      }

      MModuleListingEntryImpl entry
                              (
                                 m->GetName(), // module name
                                 m->GetInterface(),
                                 m->GetDescription(),
                                 _T(""), // long description
                                 m->GetVersion(),
                                 _T(""), // author
                                 m
                              );
      (*listing)[count++] = entry;
   }
#endif // USE_MODULES_STATIC/!USE_MODULES_STATIC

   // we might have less than we thought at first
   listing->SetCount(count);

   return listing;
}

/* static */
MModuleListing *
MModule::ListLoadedModules(void)
{
   return DoListLoadedModules();
}

/* static */
MModuleListing *
MModule::ListAvailableModules(const String& interfaceName)
{
#ifdef USE_MODULES_STATIC
   return DoListLoadedModules(true, interfaceName);
#else // !USE_MODULES_STATIC
   kbStringList modules;

   wxArrayString dirs = BuildListOfModulesDirs();
   size_t nDirs = dirs.GetCount();

#ifdef DEBUG
   String path;
   for ( size_t n = 0; n < nDirs; n++ )
   {
      if ( !path.empty() )
         path += ':';
      path += dirs[n];
   }

   wxLogTrace(M_TRACE_MODULES,
              _T("Looking for modules of type \"%s\" in the path '%s'."),
              interfaceName.c_str(), path.c_str());
#endif // DEBUG

   // First, build list of all .dll/.so files in module directories
   wxString extDll = DLL_EXTENSION;

   // but take care to only take each of the modules once - loading different
   // modules with the same name is a recipe for disaster, so remember the
   // base names of the modules we had already seen in this array
   wxArrayString moduleNames;

   wxString pathname, filename, basename;
   for( size_t i = 0; i < nDirs ; i++ )
   {
      pathname = dirs[i];
      if ( wxPathExists(pathname) )
      {
         pathname = dirs[i];
         pathname << "*" << extDll;
         filename = wxFindFirstFile(pathname);
         while ( filename.length() )
         {
            wxSplitPath(filename, NULL, &basename, NULL);
            if ( moduleNames.Index(basename) == wxNOT_FOUND )
            {
               moduleNames.Add(basename);

               modules.push_back(new wxString(filename));
            }

            filename = wxFindNextFile();
         }
      }
   }

   // Second: load list info:
   const String modulesDontLoad = READ_APPCONFIG(MP_MODULES_DONT_LOAD);
   const wxArrayString modulesNot = strutil_restore_array(modulesDontLoad);

   MModuleListingImpl *listing = MModuleListingImpl::Create(modules.size());
   kbStringList::iterator it;
   size_t count = 0;
   for(it = modules.begin(); it != modules.end(); it ++)
   {
      filename = **it;

      wxDynamicLibrary dll(filename);
      MModule_GetModulePropFuncType
         getProps = dll.IsLoaded() ?
            (MModule_GetModulePropFuncType)
            dll.GetSymbol(MMODULE_GETPROPERTY_FUNCTION) : NULL;

      if ( !getProps )
      {
         // this is not our module
         wxLogWarning(_("Shared library '%s' is not a Mahogany module."),
                      filename.c_str());

         continue;
      }

      const ModuleProperty *props = (*getProps)();

      // does it have the right interface?
      String interfaceModule;

      if ( props )
      {
         interfaceModule = GetMModuleProperty(props,
                                              MMODULE_INTERFACE_PROP);

         if ( !interfaceName.empty() )
         {
            if ( interfaceName != interfaceModule )
            {
               // wrong interface, we're not interested in this one
               continue;
            }

            // note that this check is only done for a specific interface, if
            // all modules are requested, then return really all of them
            const String name = GetMModuleProperty(props, MMODULE_NAME_PROP);
            if ( modulesNot.Index(name) != wxNOT_FOUND )
            {
               // this module was excluded by user
               continue;
            }
         }

         // use the file name, not MMODULE_NAME_PROP, so that we can
         // LoadModule() it later
         String name;
         wxSplitPath(filename, NULL, &name, NULL),
         MModuleListingEntryImpl entry(
            name,
            interfaceModule,
            GetMModuleProperty(props, MMODULE_DESC_PROP),
            GetMModuleProperty(props, MMODULE_DESCRIPTION_PROP),
            GetMModuleProperty(props, MMODULE_VERSION_PROP),
            GetMModuleProperty(props, MMODULE_AUTHOR_PROP)
         );

         (*listing)[count++] = entry;
      }
      else // no properties in this module??
      {
         wxLogWarning(_("Mahogany module '%s' is probably corrupted"),
                      filename.c_str());
      }
   }

   wxLogTrace(M_TRACE_MODULES, _T("\t%lu modules found."),
              (unsigned long)count);

   listing->SetCount(count);

   return listing;
#endif // USE_MODULES_STATIC/!USE_MODULES_STATIC
}

#ifndef USE_MODULES_STATIC

static wxArrayString BuildListOfModulesDirs()
{
   wxArrayString dirs;

   // 1. look in user directory

   const String localDir = mApplication->GetLocalDir();

   String pathUser;

#ifdef OS_UNIX
   // look first under extra M_CANONICAL_HOST directory under Unix, this
   // allows to have arch-dependent modules which is handy when your home
   // directory is shared between multiple machines (via NFS for example)

   pathUser << localDir
            << DIR_SEPARATOR << M_CANONICAL_HOST
            << DIR_SEPARATOR << _T("modules") << DIR_SEPARATOR;

   dirs.Add(pathUser);

   pathUser.clear();
#endif // Unix

   pathUser << localDir << DIR_SEPARATOR << _T("modules") << DIR_SEPARATOR;

   dirs.Add(pathUser);

   // 2. look in global directory

   // look in $prefix/lib/mahogany modules but just in modules under the
   // program/bundle directory elsewhere (not that it mattered anyhow as
   // dynamic modules are not supported neither under Windows nor under Mac
   // right now...)
   const String globalDir = M_PREFIX;
   if ( globalDir != localDir )
   {
      String pathSystem;
      pathSystem << globalDir << DIR_SEPARATOR
#ifdef OS_UNIX
                 << _T("lib/mahogany/")
#endif // Unix
                 << _T("modules") << DIR_SEPARATOR;

      dirs.Add(pathSystem);
   }

   // 3. finally, also make it possible to use modules without installing them
   //    so look in the source tree
#ifdef M_TOP_BUILDDIR
   wxString pathSrcTree;
   pathSrcTree << M_TOP_BUILDDIR
               << DIR_SEPARATOR << _T("src")
               << DIR_SEPARATOR << _T("modules") << DIR_SEPARATOR;
   dirs.Add(pathSrcTree);

   pathSrcTree.clear();
   pathSrcTree << M_TOP_BUILDDIR
               << DIR_SEPARATOR << _T("src")
               << DIR_SEPARATOR << _T("adb") << DIR_SEPARATOR;
   dirs.Add(pathSrcTree);

   pathSrcTree.clear();
   pathSrcTree << M_TOP_BUILDDIR
               << DIR_SEPARATOR << _T("src")
               << DIR_SEPARATOR << _T("modules")
               << DIR_SEPARATOR << _T("crypt") << DIR_SEPARATOR;
   dirs.Add(pathSrcTree);

   pathSrcTree.clear();
   pathSrcTree << M_TOP_BUILDDIR
               << DIR_SEPARATOR << _T("src")
               << DIR_SEPARATOR << _T("modules")
               << DIR_SEPARATOR << _T("spam") << DIR_SEPARATOR;
   dirs.Add(pathSrcTree);

   pathSrcTree.clear();
   pathSrcTree << M_TOP_BUILDDIR
               << DIR_SEPARATOR << _T("src")
               << DIR_SEPARATOR << _T("modules")
               << DIR_SEPARATOR << _T("viewflt") << DIR_SEPARATOR;
   dirs.Add(pathSrcTree);
#endif // Unix

   return dirs;
}

#endif // !USE_MODULES_STATIC

// ----------------------------------------------------------------------------
// working with module properties
// ----------------------------------------------------------------------------

const wxChar *
GetMModuleProperty(const ModuleProperty *table, const wxChar *name)
{
   while ( table->name )
   {
      if ( wxStrcmp(table->name, name) == 0 )
         return table->value;

      table++;
   }

   return _T("");
}

MModuleCommon::~MModuleCommon()
{
   if ( m_MInterface )
      m_MInterface->RemoveModule(this);
}

// Unfortunately most interfaces don't have supporting *.cpp file
// We have to put their RefCounter functions here
DEFINE_REF_COUNTER(FilterRule)
