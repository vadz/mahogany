/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)              *
 *                                                                  *
 * $Id$
 *******************************************************************/

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
// Actual MModule code
// ----------------------------------------------------------------------------

#define MMD_SIGNATURE "Mahogany-Module-Definition"

/** Structure for holding information about a loaded module. */
struct ModuleEntry
{
   /// Name under which it was loaded.
   String m_Name;
   /// interface provided
   String m_Interface;
   /// Module pointer.
   MModule *m_Module;
};

/// A list of all loaded modules.
KBLIST_DEFINE(ModuleList, ModuleEntry);

/// The actual list of all loaded modules.
static ModuleList *gs_ModuleList = NULL;

static
ModuleList *GetModuleList(void)
{
   if(! gs_ModuleList) gs_ModuleList = new ModuleList;
   return gs_ModuleList;
}

extern
void MModule_Cleanup(void)
{
   if(gs_ModuleList)
      delete gs_ModuleList;
   gs_ModuleList = NULL;
}

class MModuleImpl : public MModule
{
public:
#ifdef USE_MODULES_STATIC
   /// Create an MModuleImpl.
   static MModule *Create(MModule_InitModuleFuncType init,
                          MModule_GetNameFuncType gn,
                          MModule_GetInterfaceFuncType gi,
                          MModule_GetDescriptionFuncType gs,
                          MModule_GetVersionFuncType gv,
                          MModule_GetMVersionFuncType gmv,
                          MModule_UnLoadModuleFuncType unl)
      { ASSERT(init); return new MModuleImpl(init,gn, gi, gs, gv, gmv, unl); }
   bool Init(void)
      {
         if(m_Init)
         {
            bool rc = (*m_Init)(M_VERSION_MAJOR,
                                M_VERSION_MINOR,
                                M_VERSION_RELEASE,
                                &gs_MInterface) == MMODULE_ERR_NONE;
            m_Init = NULL;
            return rc;
         }
         else
            return true;
      }
#else
   /// Create an MModuleImpl from a Dll handle
   static MModule *Create(wxDllType);
#endif
   
   /// Returns the Module's name as used in LoadModule().
   virtual const char * GetName(void)
      { return m_GetName ? (*m_GetName)() : _("unknown"); }
   virtual const char * GetDescription(void)
      { return m_GetDescription ? (*m_GetDescription)() : _("no description"); }
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void)
      { return m_GetVersion ? (*m_GetVersion)() : "0"; }
   virtual const char * GetInterface(void)
      { return m_GetInterface ? (*m_GetInterface)() : _("unknown"); }
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release)
      {
         if(m_GetMVersion)
            (*m_GetMVersion)(version_major, version_minor,
                             version_release);
         else
         {
            *version_major = M_VERSION_MAJOR;
            *version_minor = M_VERSION_MINOR;
            *version_release = M_VERSION_RELEASE;
         }
      }

private:
   MModule_GetNameFuncType m_GetName;
   MModule_GetInterfaceFuncType m_GetInterface;
   MModule_GetDescriptionFuncType m_GetDescription;
   MModule_GetVersionFuncType m_GetVersion;
   MModule_GetMVersionFuncType m_GetMVersion;

   ~MModuleImpl();

#ifdef USE_MODULES_STATIC
   MModule_InitModuleFuncType m_Init;
   MModule_UnLoadModuleFuncType m_Unload;
   
   MModuleImpl(MModule_InitModuleFuncType init,
               MModule_GetNameFuncType gn,
               MModule_GetInterfaceFuncType gi,
               MModule_GetDescriptionFuncType gd,
               MModule_GetVersionFuncType gv,
               MModule_GetMVersionFuncType gmv,
               MModule_UnLoadModuleFuncType unl)
      {
         m_Init = init;
         m_GetName = gn; m_GetInterface = gi;
         m_GetDescription = gd; m_GetVersion = gv;
         m_GetMVersion = gmv; m_Unload = unl;
      }
#else
   MModuleImpl(wxDllType dll);
   wxDllType m_Dll;
#endif

   GCC_DTOR_WARN_OFF();
};

#ifndef USE_MODULES_STATIC
MModule *
MModuleImpl::Create(wxDllType dll)
{
   MModule_InitModuleFuncType
      initModuleFunc = (MModule_InitModuleFuncType)
      wxDllLoader::GetSymbol(dll, "InitMModule"); 
   if(! initModuleFunc)
   {
      wxDllLoader::UnloadLibrary(dll);
      return NULL;
   }
   if( (*initModuleFunc)(M_VERSION_MAJOR,
                         M_VERSION_MINOR,
                         M_VERSION_RELEASE,
                         &gs_MInterface) == MMODULE_ERR_NONE)
   {
      return new MModuleImpl(dll);
   }
   else
      return NULL;
}

MModuleImpl::MModuleImpl(wxDllType dll)
{
   m_Dll = dll;
   m_GetName = (MModule_GetNameFuncType)
      wxDllLoader::GetSymbol(dll, "GetName"); 
   m_GetInterface = (MModule_GetInterfaceFuncType)
      wxDllLoader::GetSymbol(dll, "GetInterface"); 
   m_GetDescription  = (MModule_GetDescriptionFuncType)
      wxDllLoader::GetSymbol(dll, "GetDescription"); 
   m_GetVersion = (MModule_GetVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetModuleVersion"); 
   m_GetMVersion = (MModule_GetMVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetMVersion"); 
}
#endif

MModuleImpl::~MModuleImpl()
{
#ifdef USE_MODULES_STATIC
   MModule_UnLoadModuleFuncType unLoadModuleFunc = m_Unload;
#else
   MModule_UnLoadModuleFuncType
      unLoadModuleFunc = (MModule_UnLoadModuleFuncType)
      wxDllLoader::GetSymbol(m_Dll, "UnLoadMModule"); 
#endif
   ASSERT(unLoadModuleFunc);
   if(! unLoadModuleFunc)
      return; // be careful

   // Remove our entry in the list:
#ifdef DEBUG
   bool found = false;
#endif
   ModuleList::iterator i;
   for(i = GetModuleList()->begin();
       i != GetModuleList()->end();
       i++)
      if( (**i).m_Name == GetName() )
      {
         GetModuleList()->erase(i);
#ifdef DEBUG
         found = true;
#endif  
         break;
      }
   ASSERT(found == true);

#ifndef USE_MODULES_STATIC
   if(unLoadModuleFunc()) // check if we can safely unload it
      wxDllLoader::UnloadLibrary(m_Dll);
#endif
}

static
MModule *FindModule(const String & name)
{
   ModuleList::iterator i;
   for(i = GetModuleList()->begin();
       i != GetModuleList()->end();
       i++)
      if( (**i).m_Name == name )
      {
#ifndef USE_MODULES_STATIC
         return (**i).m_Module;
#else
         // on the fly initialisation for static modules:
         if( ((MModuleImpl *)(**i).m_Module)->Init())
            return (**i).m_Module;
         else
            return NULL;
      }
#endif
            
   return NULL; // not found
}


#ifndef USE_MODULES_STATIC
static
MModule *LoadModuleInternal(const String & name, const String &pathname)
{
   // No, load it:
   bool success = false;
   wxDllType dll = wxDllLoader::LoadLibrary(pathname, &success);
   if(! success) return NULL;

   MModule *module = MModuleImpl::Create(dll);
   if(module)
   {
      ModuleEntry *me = new ModuleEntry;
      me->m_Name = name;
      me->m_Interface = module->GetInterface();
      me->m_Module = module;
      GetModuleList()->push_back(me);
   }
   else
      wxDllLoader::UnloadLibrary(dll);
   return module;
}
#else
extern
int MModule_AddStaticModule(MModule_InitModuleFuncType init,
                            MModule_GetNameFuncType gn,
                            MModule_GetInterfaceFuncType gi,
                            MModule_GetDescriptionFuncType gd,
                            MModule_GetVersionFuncType gv,
                            MModule_GetMVersionFuncType gmv,
                            MModule_UnLoadModuleFuncType unl)
{
  MModule *module = MModuleImpl::Create(init,gn,gi,gd,gv,gmv,unl);
  if(module)
  {
     ModuleEntry *me = new ModuleEntry;
     me->m_Name = module->GetName();
     me->m_Interface = module->GetInterface();
     me->m_Module = module;
     GetModuleList()->push_back(me);
  }
  return 1;
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
      // Yes, just return it:
      module->IncRef();
      return module;
   }
   else
   {
#ifndef USE_MODULES_STATIC
      const int nDirs = 3;
      wxString
         pathname,
         dirs[nDirs];

      /* We search for modules in three places:
      Global directory:
      prefix/share/Mahogany/CANONICAL_HOST/modules/
      Local:
      $HOME/.M/CANONICAL_HOST/modules/
      $HOME/.M/modules/
   */
      dirs[0] = mApplication->GetGlobalDir();
      dirs[0] << DIR_SEPARATOR << M_CANONICAL_HOST
              << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
      dirs[1] = mApplication->GetLocalDir();
      dirs[1] << DIR_SEPARATOR << M_CANONICAL_HOST
              << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
      dirs[2] = mApplication->GetLocalDir();
      dirs[2] << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;

#if defined( OS_WIN )
      const wxString moduleExt = ".dll";
#elif defined( OS_UNIX )
      const wxString moduleExt = ".so" ;
#else
#   error   "No DLL extension known on this platform."
#endif

      for(int i = 0; i < nDirs && ! module; i++)
      {
         pathname = dirs[i];
         pathname << name << moduleExt;
         if(wxFileExists(pathname))
            module = LoadModuleInternal(name, pathname);
      }
      return module;
#else
      return NULL;
#endif
   }
}

/* static */
MModule *
MModule::GetProvider(const wxString &interfaceName)
{
   ModuleList::iterator i;
   for(i = GetModuleList()->begin();
       i != GetModuleList()->end();
       i++)
      if( (**i).m_Interface == interfaceName )
      {
         (**i).m_Module->IncRef();
         return (**i).m_Module;
      }
   return NULL; // not found
}




#if 0
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
#endif



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

   MModuleListingEntryImpl(const String &name = "",
                           const String &interfaceName = "",
                           const String &shortdesc = "",
                           const String &desc = "",
                           const String &version = "",
                           const String &author = "")
      {
         m_Name = name;
         m_Interface = interfaceName;
         m_ShortDesc = shortdesc;
         m_Desc = desc;
         m_Version = version;
         m_Author = author;
         strutil_delwhitespace(m_Name);
         strutil_delwhitespace(m_Interface);
         strutil_delwhitespace(m_Desc);
         strutil_delwhitespace(m_ShortDesc);
         strutil_delwhitespace(m_Version);
         strutil_delwhitespace(m_Author);
      }
private:
   String m_Name, m_Interface, m_ShortDesc, m_Desc, m_Version, m_Author;
   GCC_DTOR_WARN_OFF();
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
   GCC_DTOR_WARN_OFF();
};

/* static */
MModuleListing *MModule::GetListing(void)
{
#ifdef USE_MODULES_STATIC

   MModuleListingImpl *listing = MModuleListingImpl::Create(GetModuleList()->size());
   size_t count = 0;
   ModuleList::iterator i;
   for(i = GetModuleList()->begin();
       i != GetModuleList()->end();
       i++)
   {
      MModuleListingEntryImpl entry(
         (**i).m_Module->GetName(), // module name
         (**i).m_Module->GetInterface(),
         (**i).m_Module->GetDescription(),
         "", // long description
         String((**i).m_Module->GetVersion())+_(" (builtin)"),
         "m-developers@groups.com");
      (*listing)[count++] = entry;
   }
   return listing;
#else
   kbStringList modules;
   
   const int nDirs = 3;
   wxString
      pathname,
      filename,
      dirs[nDirs];

   dirs[0] = mApplication->GetGlobalDir();
   dirs[0] << DIR_SEPARATOR << M_CANONICAL_HOST
           << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
   dirs[1] = mApplication->GetLocalDir();
   dirs[1] << DIR_SEPARATOR << M_CANONICAL_HOST
           << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
   dirs[2] = mApplication->GetLocalDir();
   dirs[2] << DIR_SEPARATOR << "modules" << DIR_SEPARATOR;
   
   /// First, build list of all .mmd files:
   for(int i = 0; i < nDirs ; i++)
   {
         pathname = dirs[i];
         if(wxDirExists(pathname))
         {
            pathname << "*.mmd";
            filename = wxFindFirstFile(pathname);
            while(filename.Length())
            {
               modules.push_back(new // without ".mmd" :
                                 String(filename.Mid(0,filename.Length()-4))); 
               filename = wxFindNextFile();
            }
         }
   }
   /// Second: load list info:
   MModuleListingImpl *listing = MModuleListingImpl::Create(modules.size());
   kbStringList::iterator it;
   bool errorflag;
   size_t count = 0;
   for(it = modules.begin(); it != modules.end(); it ++)
   {
      filename = **it;
      filename << ".mmd";
      errorflag = false;
      wxTextFile tf(filename);
      if(! tf.Open())
         errorflag = true;
      /*
        We need at least the following lines:
        Mahogany-Module-Definition"
        Name:
        Interface:
        Version:
        Author:
      */
      else
      {
         if(tf.GetLineCount() < 5)
            errorflag = true;
         else
         {
            String first = tf[0].Mid(0,strlen(MMD_SIGNATURE));
            if(first != MMD_SIGNATURE ||
               tf[1].Mid(0,strlen("Name:")) != "Name:" ||
               tf[2].Mid(0,strlen("Interface:")) != "Interface:" ||
               tf[3].Mid(0,strlen("Version:")) != "Version:" ||
               tf[4].Mid(0,strlen("Author:")) != "Author:")
                         errorflag = true;
            else
            {
               String description;
               for(size_t l = 6; l < tf.GetLineCount(); l++)
                  description << tf[l] << '\n';
               String name = (**it).AfterLast(DIR_SEPARATOR);
               name = name.Mid(0, filename.Length()-strlen(".mmd"));
               String tmp = tf[1].Mid(strlen("Name:")); // == short description
               MModuleListingEntryImpl entry(
                  name, // module name

                  tf[2].Mid(strlen("Interface:")),
                  tmp,
                  description,
                  tf[3].Mid(strlen("Version:")),
                  tf[4].Mid(strlen("Author:"))
                  );
               (*listing)[count++] = entry;
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
#endif
}
