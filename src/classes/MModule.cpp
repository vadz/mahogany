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
#   include "Mconfig.h"
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   include "kbList.h"
#   include "strutil.h"
#endif

#include "MDialogs.h"

#include "Mversion.h"
#include "MModule.h"

#include <wx/dynlib.h>
#include <wx/utils.h>
#include <wx/textfile.h>

// ----------------------------------------------------------------------------
// Implementation of the MInterface
// ----------------------------------------------------------------------------

#include "MInterface.h"
#include "MInterface.cpp"

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

#define MMD_SIGNATURE "Mahogany-Module-Definition"

// the trace mask used by module loading code
#define M_TRACE_MODULES "mmodule"

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
      m_dllsToUnload.begin()->Detach(); // prevent the DLL from being unloaded

      delete *m_dllsToUnload.begin();
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
void MModule_AddStaticModule(const char *Name,
                             const char *Interface,
                             const char *Description,
                             const char *Version,
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
   MModuleList::iterator i;
   for(i = GetMModuleList()->begin();
       i != GetMModuleList()->end();
       i++)
   {
      MModuleListEntry *me = *i;
      if( me->m_Interface == interfaceName )
      {
#if 0 // creating them on the fly is wrong!
#ifdef USE_MODULES_STATIC
         if( !me->m_Module )
         {
            // create the module on the fly
            int errorCode;
            me->m_Module = (*(me->m_InitFunc))(
               M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE,
               &gs_MInterface, &errorCode);
         }
#endif
#endif
         if ( me->m_Module )
         {
            me->m_Module->IncRef();
            return me->m_Module;
         }
      }
   }

   return NULL; // not found
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
   MModuleListingEntryImpl(const String &name = "",
                           const String &interfaceName = "",
                           const String &shortdesc = "",
                           const String &desc = "",
                           const String &version = "",
                           const String &author = "",
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
      { if ( m_count ) delete [] m_entries; }
private:
   MModuleListingEntryImpl *m_entries;
   size_t m_count;
   GCC_DTOR_WARN_OFF
};

// this function can list all loaded modules (default) or do other things as
// well depending on the parameters values, so the name is a bit unfortunate
static MModuleListing * DoListLoadedModules(bool listall = false,
                                            const String& interfaceName = "",
                                            bool loadableOnly = false)
{
#ifndef USE_MODULES_STATIC
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
         // check that the module is "loadable" if requested
         String desc = me->m_Description;
         if ( !loadableOnly || !desc.empty() )
         {
            MModuleListingEntryImpl entry
                                    (
                                       me->m_Name, // module name
                                       me->m_Interface,
                                       desc,
                                       "", // long description
                                       String(me->m_Version) + _(" (builtin)"),
                                       "mahogany-developers@lists.sourceforge.net",
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

      // check that the module is "loadable" if requested
      String desc = m->GetDescription();
      if ( !loadableOnly || !desc.empty() )
      {
         MModuleListingEntryImpl entry
                                 (
                                    m->GetName(), // module name
                                    m->GetInterface(),
                                    desc,
                                    "", // long description
                                    m->GetVersion(),
                                    "", // author
                                    m
                                 );
         (*listing)[count++] = entry;
      }
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
MModule::ListLoadableModules()
{
   return ListAvailableModules("", true /* loadable only */);
}

/* static */
MModuleListing *
MModule::ListAvailableModules(const String& interfaceName, bool loadableOnly)
{
#ifdef USE_MODULES_STATIC
   return DoListLoadedModules(true, interfaceName, loadableOnly);
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

   // First, build list of all .mmd and .so files in module directories
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
         // first look for MMDs
         pathname << "*.mmd";
         filename = wxFindFirstFile(pathname);
         while( filename.length() )
         {
            wxSplitPath(filename, NULL, &basename, NULL);
            if ( moduleNames.Index(basename) == wxNOT_FOUND )
            {
               moduleNames.Add(basename);

               modules.push_back(new wxString(filename));
            }

            filename = wxFindNextFile();
         }

         // now restart with the DLLs
         pathname = dirs[i];
         pathname << "*" << extDll;
         filename = wxFindFirstFile(pathname);
         while ( filename.length() )
         {
            // only add if we hadn't found a matching .mmd for it
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

   // the components of MMD file format
   enum
   {
      MMD_LINE_SIGNATURE,
      MMD_LINE_NAME,
      MMD_LINE_INTERFACE,
      MMD_LINE_VERSION,
      MMD_LINE_AUTHOR,
      MMD_LINE_LAST
   };

   static const char *MMD_HEADERS[] =
   {
      MMD_SIGNATURE,
      "Name:",
      "Interface:",
      "Version:",
      "Author:",
   };

   ASSERT_MSG( WXSIZEOF(MMD_HEADERS) == MMD_LINE_LAST,
               _T("forgot to update the constants describing MMD format") );

   // Second: load list info:
   MModuleListingImpl *listing = MModuleListingImpl::Create(modules.size());
   kbStringList::iterator it;
   bool errorflag;
   size_t count = 0;
   for(it = modules.begin(); it != modules.end(); it ++)
   {
      filename = **it;

      // it's either a .mmd file in which case we just read its contents or a
      // .so file in which case we load it and call its GetMModuleInfo()
      if ( filename.Right(4) == ".mmd" )
      {
         errorflag = false;
         wxTextFile tf(filename);
         if(! tf.Open())
         {
            errorflag = true;
         }
         else
         {
            // check that we have all required lines
            if(tf.GetLineCount() < MMD_LINE_LAST)
            {
               errorflag = true;
            }
            else
            {
               // check that the first MMD_LINE_LAST lines are correct and get
               // the values of the fields too
               wxString headerValues[MMD_LINE_LAST];
               for ( size_t line = 0; !errorflag && (line < MMD_LINE_LAST); line++ )
               {
                  if ( !tf[line].StartsWith(MMD_HEADERS[line], &headerValues[line]) )
                     errorflag = TRUE;
               }

               if ( !errorflag )
               {
                  String interfaceModule = headerValues[MMD_LINE_INTERFACE];

                  // take all modules if interfaceName is empty, otherwise only
                  // take those which implement the given interface
                  if ( !interfaceName || interfaceName == interfaceModule )
                  {
                     String description;
                     for(size_t l = MMD_LINE_LAST + 1; l < tf.GetLineCount(); l++)
                        description << tf[l] << '\n';

                     String name;
                     wxSplitPath((**it), NULL, &name, NULL);
                     MModuleListingEntryImpl entry(
                        name, // module name

                        headerValues[MMD_LINE_INTERFACE],
                        headerValues[MMD_LINE_NAME],
                        description,
                        headerValues[MMD_LINE_VERSION],
                        headerValues[MMD_LINE_AUTHOR]);
                     (*listing)[count++] = entry;
                  }
               }
            }
         }
         if(errorflag)
         {
            wxLogWarning(_("Cannot parse MMD file for module '%s'."),
                         filename.c_str());
         }
      }
      else // it's not an .mmd, get info from .so/.dll directly
      {
         errorflag = true;

         wxDynamicLibrary dll(filename);
         if ( dll.IsLoaded() )
         {
            MModule_GetModulePropFuncType getProps =
               (MModule_GetModulePropFuncType)
               dll.GetSymbol(MMODULE_GETPROPERTY_FUNCTION);

            if ( getProps )
            {
               const ModuleProperty *props = (*getProps)();

               // does it have the right interface?
               String interfaceModule;

               if ( props )
               {
                  interfaceModule = GetMModuleProperty(props,
                                                       MMODULE_INTERFACE_PROP);

                  if ( !interfaceName || interfaceName == interfaceModule )
                  {
                     // check if it is loadable if necessary
                     String desc = GetMModuleProperty(props, MMODULE_DESC_PROP);
                     if ( !loadableOnly || !desc.empty() )
                     {
                        String name;
                        wxSplitPath(filename, NULL, &name, NULL);
                        MModuleListingEntryImpl entry(
                           name,
                           interfaceModule,
                           desc,
                           GetMModuleProperty(props, MMODULE_DESCRIPTION_PROP),
                           GetMModuleProperty(props, MMODULE_VERSION_PROP),
                           GetMModuleProperty(props, MMODULE_AUTHOR_PROP));
                        (*listing)[count++] = entry;
                     }
                  }

                  errorflag = false;
               }
               else // no properties in this module??
               {
                  wxLogWarning(_("Mahogany module '%s' is probably corrupted"),
                               filename.c_str());

                  errorflag = true;
               }
            }
         }

         if ( errorflag )
         {
            wxLogWarning(_("Shared library '%s' is not a Mahogany module."),
                         filename.c_str());
         }
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

   // make it possible to use modules without installing them
#ifdef M_TOP_BUILDDIR
   wxString path0;
   path0 << M_TOP_BUILDDIR
         << DIR_SEPARATOR << "src"
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
   dirs.Add(path0);

   path0.clear();
   path0 << M_TOP_BUILDDIR
         << DIR_SEPARATOR << "src"
         << DIR_SEPARATOR << "adb" << DIR_SEPARATOR;
   dirs.Add(path0);

   path0.clear();
   path0 << M_TOP_BUILDDIR
         << DIR_SEPARATOR << "src"
         << DIR_SEPARATOR << "modules"
         << DIR_SEPARATOR << "crypt" << DIR_SEPARATOR;
   dirs.Add(path0);

   path0.clear();
   path0 << M_TOP_BUILDDIR
         << DIR_SEPARATOR << "src"
         << DIR_SEPARATOR << "modules"
         << DIR_SEPARATOR << "viewflt" << DIR_SEPARATOR;
   dirs.Add(path0);
#endif // Unix

   // look under extra M_CANONICAL_HOST directory under Unix, but not for other
   // platforms (doesn't make much sense under Windows)

   wxString path1;
   path1 << mApplication->GetGlobalDir()
#ifdef OS_UNIX
         << DIR_SEPARATOR << M_CANONICAL_HOST
#endif // Unix
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;

   dirs.Add(path1);

   wxString path2;
   path2 << mApplication->GetLocalDir()
#ifdef OS_UNIX
         << DIR_SEPARATOR << M_CANONICAL_HOST
#endif // Unix
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;

   // under Windows, the global and local dirs might be the same
   if ( path2 != path1 )
   {
      dirs.Add(path2);
   }

   return dirs;
}

#endif // !USE_MODULES_STATIC

// ----------------------------------------------------------------------------
// working with module properties
// ----------------------------------------------------------------------------

const char *GetMModuleProperty(const ModuleProperty *table, const char *name)
{
   while ( table->name )
   {
      if ( strcmp(table->name, name) == 0 )
         return table->value;

      table++;
   }

   return "";
}
