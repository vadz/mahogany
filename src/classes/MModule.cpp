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
#ifdef __GNUG__
#   pragma implementation "MModule.h"
#endif

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
// Actual MModule code
// ----------------------------------------------------------------------------

#define MMD_SIGNATURE "Mahogany-Module-Definition"


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
#endif
};

/// A list of all loaded modules.
KBLIST_DEFINE(MModuleList, MModuleListEntry);

/// The actual list of all loaded modules.
static MModuleList *gs_MModuleList = NULL;

static
MModuleList *GetMModuleList(void)
{
   if(! gs_MModuleList) gs_MModuleList = new MModuleList;
   return gs_MModuleList;
}

/* When a module gets deleted it must make sure that it is no longer
   in the module list. */
void MAppBase::RemoveModule(MModuleCommon *module)
{
   if(gs_MModuleList)
   {
      MModuleList::iterator i;
      for(i = gs_MModuleList->begin();
          i != gs_MModuleList->end();
          i++)
         if( (**i).m_Module == module )
            gs_MModuleList->erase(i); // remove our entry
   }
}

typedef MModule *MModulePtr;
KBLIST_DEFINE(MModulePtrList, MModulePtr);

extern
void MModule_Cleanup(void)
{
   if(gs_MModuleList)
   {
      MModulePtrList modules;
      /* For statically linked modules, we must decrement them here to
         avoid memory leaks. The refcount gets raised to 1 before
         anyone uses them, so it should be back to 1 now so that
         DecRef() causes them to be freed.

         We cannot decref them straight from the list, as this would
         remove them from the list and the iterator would get
         corrupted. Ugly, but we need to remember them and clean them
         outside the loop.
      */
      MModuleList::iterator i;
      for(i = gs_MModuleList->begin();
          i != gs_MModuleList->end();
          i++)
         if( (**i).m_Module )
            modules.push_back( new MModulePtr((**i).m_Module) );

      for(MModulePtrList::iterator j = modules.begin();
          j != modules.end();
          j++)
         (**j)->DecRef();

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
         // on the fly initialisation for static modules:
         if( (**i).m_Module == NULL )
         {
            // initialise the module:
            (**i).m_Module = (*((**i).m_InitFunc))(
               M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE,
               &gs_MInterface, &errorCode);
         }
#endif
         (**i).m_Module->SafeIncRef();

         return (**i).m_Module;
      }
   return NULL; // not found
}


#ifdef USE_MODULES_STATIC
extern
int MModule_AddStaticModule(const char *Name,
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
   return 1;
}
#else
static
MModule *LoadModuleInternal(const String & name, const String &pathname)
{
   bool success = false;
   wxDllType dll = wxDllLoader::LoadLibrary(pathname, &success);
   if(! success) return NULL;

   MModule_InitModuleFuncType initFunc =
      (MModule_InitModuleFuncType)
      wxDllLoader::GetSymbol(dll, MMODULE_INITMODULE_FUNCTION);


   int errorCode = 255;
   MModule *module = NULL;
   if(initFunc)
   {
      module = (*initFunc)(
      M_VERSION_MAJOR, M_VERSION_MINOR, M_VERSION_RELEASE,
      &gs_MInterface, &errorCode);
   }

   if(module)
   {
      MModuleListEntry *me = new MModuleListEntry;
      me->m_Name = name;
      me->m_Interface = module->GetInterface();
      me->m_Module = module;
      GetMModuleList()->push_back(me);
   }
   else
   {
      String msg;
      msg.Printf(_("Cannot initialise module '%s', error code %d."),
                 pathname.c_str(), errorCode);
      MDialog_ErrorMessage(msg);
      wxDllLoader::UnloadLibrary(dll);
   }
   return module;
}
#endif


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
   else
   {
#ifndef USE_MODULES_STATIC
      wxArrayString dirs = BuildListOfModulesDirs();
      size_t nDirs = dirs.GetCount();


      const wxString moduleExt = wxDllLoader::GetDllExt();

      wxString pathname;
      for ( size_t i = 0; i < nDirs && ! module; i++ )
      {
         pathname = dirs[i];
         pathname << name << moduleExt;
         if(wxFileExists(pathname))
            module = LoadModuleInternal(name, pathname);
      }
      return module;
#else // USE_MODULES_STATIC
      return NULL;
#endif // !USE_MODULES_STATIC/USE_MODULES_STATIC
   }
}

/* static */
MModule *
MModule::GetProvider(const wxString &interfaceName)
{
   MModuleList::iterator i;
   for(i = GetMModuleList()->begin();
       i != GetMModuleList()->end();
       i++)
      if( (**i).m_Interface == interfaceName )
      {
#ifdef USE_MODULES_STATIC
         if( (**i).m_Module == NULL ) // static case not loaded yet
            (**i).m_Module = FindModule( (**i).m_Name ); // this will
         // initialise it
         else
#endif
            (**i).m_Module->IncRef();
         return (**i).m_Module;
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
         m_Module->SafeIncRef();
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
         m_Module->SafeIncRef();
         strutil_delwhitespace(m_Name);
         strutil_delwhitespace(m_Interface);
         strutil_delwhitespace(m_Desc);
         strutil_delwhitespace(m_ShortDesc);
         strutil_delwhitespace(m_Version);
         strutil_delwhitespace(m_Author);
      }
   ~MModuleListingEntryImpl()
      {
         m_Module->SafeDecRef();
      }
   /// must be implemented to handle refcount of MModule pointer:
   MModuleListingEntryImpl & operator=(
      const MModuleListingEntryImpl & newval)
      {
         m_Module->SafeDecRef();
         m_Name = newval.m_Name; m_Interface = newval.m_Interface;
         m_ShortDesc = newval.m_ShortDesc;
         m_Desc = newval.m_Desc; m_Author = newval.m_Author;
         m_Version = newval.m_Version;
         m_Module = newval.m_Module;
         m_Module->SafeIncRef();
         return *this;
      }
private:
   /// forbidden:
   MModuleListingEntryImpl(const MModuleListingEntryImpl &);

   String m_Name, m_Interface, m_ShortDesc,
         m_Desc, m_Version, m_Author;
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

#ifdef USE_MODULES_STATIC
static MModuleListing * DoListLoadedModules(bool listall = false)
#else
static MModuleListing * DoListLoadedModules(void)
#endif
{
   MModuleListingImpl *listing = MModuleListingImpl::Create(GetMModuleList()->size());
   size_t count = 0;
   MModuleList::iterator i;
   for(i = GetMModuleList()->begin();
       i != GetMModuleList()->end();
       i++)
#ifdef USE_MODULES_STATIC
   {
      // we have unloaded modules in the list, ignore them:
      if((**i).m_Module || listall)
      {
         MModuleListingEntryImpl entry(
            (**i).m_Name, // module name
            (**i).m_Interface,
            (**i).m_Description,
            "", // long description
            String((**i).m_Version)+ _(" (builtin)"),
            "m-developers@groups.com",
            (**i).m_Module);
         (*listing)[count++] = entry;
      }
   }
   listing->SetCount(count); // we might have less than we thought at first
#else
   {
      MModule *m = (**i).m_Module;
      ASSERT(m);
      MModuleListingEntryImpl entry(
         m->GetName(), // module name
         m->GetInterface(),
         m->GetDescription(),
         "", // long description
         m->GetVersion(),
         "", m);
      (*listing)[count++] = entry;
   }
#endif
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
   return DoListLoadedModules(true);
#else // !USE_MODULES_STATIC
   kbStringList modules;

   wxString pathname, filename;
   wxArrayString dirs = BuildListOfModulesDirs();
   size_t nDirs = dirs.GetCount();

   /// First, build list of all .mmd files:
   for( size_t i = 0; i < nDirs ; i++ )
   {
         pathname = dirs[i];
         if(wxDirExists(pathname))
         {
            pathname << "*.mmd";
            filename = wxFindFirstFile(pathname);
            while(filename.Length())
            {
               modules.push_back(new wxString(filename));
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
               "forgot to update the constants describing MMD format" );

   // Second: load list info:
   MModuleListingImpl *listing = MModuleListingImpl::Create(modules.size());
   kbStringList::iterator it;
   bool errorflag;
   size_t count = 0;
   for(it = modules.begin(); it != modules.end(); it ++)
   {
      filename = **it;
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
         String msg;
         msg.Printf(_("Cannot parse MMD file for module '%s'."),
                    filename.c_str());
         MDialog_ErrorMessage(msg);
      }
   }
   listing->SetCount(count);
   return listing;
#endif // USE_MODULES_STATIC/!USE_MODULES_STATIC
}

#ifndef USE_MODULES_STATIC

static wxArrayString BuildListOfModulesDirs()
{
   // look under extra M_CANONICAL_HOST directory under Unix, but not for other
   // platforms (doesn't make much sense under Windows)

   wxString path1, path2;
   wxArrayString dirs;

   path1 << mApplication->GetGlobalDir()
#ifdef OS_UNIX
         << DIR_SEPARATOR << M_CANONICAL_HOST
#endif // Unix
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;

   path2 << mApplication->GetLocalDir()
#ifdef OS_UNIX
         << DIR_SEPARATOR << M_CANONICAL_HOST
#endif // Unix
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;

   dirs.Add(path1);

   // under Windows, the global and local dirs might be the same
   if ( path2 != path1 )
   {
      dirs.Add(path2);
   }

   // under Windows this owuld be the same as (1)
#ifdef OS_UNIX
   path1 << mApplication->GetLocalDir()
         << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
   dirs.Add(path1);
#endif // Unix

   // make it possible to use modules without installing them in debug builds
#ifdef DEBUG
   dirs.Add("./modules/");
#endif // DEBUG

   return dirs;
}

#endif // !USE_MODULES_STATIC
