/*-*- c++ -*-********************************************************
 * MModule - a pluggable module architecture for Mahogany           *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
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

//static MInterfaceImpl gs_MInterface;

// ----------------------------------------------------------------------------
// Actual MModule code
// ----------------------------------------------------------------------------

#define MMD_SIGNATURE "Mahogany-Module-Definition"

/** Structure for holding information about a loaded module. */
struct ModuleEntry
{
   /// Name under which it was loaded.
   String m_Name;
   /// Module pointer.
   MModule *m_Module;
};



class MModuleImpl : public MModule
{
public:
   /// Create an MModuleImpl from a Dll handle
   static MModule *Create(wxDllType);
   
   /// Returns the Module's name as used in LoadModule().
   virtual const char * GetName(void)
      { return (*m_GetName)(); }
   virtual const char * GetDescription(void)
      { return (*m_GetDescription)(); }
   /// Returns a textual representation of the particular version of the module.
   virtual const char * GetVersion(void)
      { return (*m_GetVersion)(); }
   /// Returns the Mahogany version this module was compiled for.
   virtual void GetMVersion(int *version_major, int *version_minor,
                            int *version_release)
      {
         (*m_GetMVersion)(version_major, version_minor, version_release);
      }

private:
   MModuleImpl(wxDllType dll);
   ~MModuleImpl();
   MModule_GetNameFuncType m_GetName;
   MModule_GetDescriptionFuncType m_GetDescription;
   MModule_GetVersionFuncType m_GetVersion;
   MModule_GetMVersionFuncType m_GetMVersion;
   wxDllType m_Dll;

   GCC_DTOR_WARN_OFF();
};


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
      wxDllLoader::GetSymbol(dll, "GetName"); ;
   m_GetDescription  = (MModule_GetDescriptionFuncType)
      wxDllLoader::GetSymbol(dll, "GetDescription"); 
   m_GetVersion = (MModule_GetVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetModuleVersion"); 
   m_GetMVersion = (MModule_GetMVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetMVersion"); 
}

MModuleImpl::~MModuleImpl()
{
   MModule_UnLoadModuleFuncType
      unLoadModuleFunc = (MModule_UnLoadModuleFuncType)
      wxDllLoader::GetSymbol(m_Dll, "UnLoadMModule"); 
   ASSERT(unLoadModuleFunc);
   if(! unLoadModuleFunc)
      return; // be careful
   if(unLoadModuleFunc()) // check if we can safely unload it
      wxDllLoader::UnloadLibrary(m_Dll);
}


/// A list of all loaded modules.
KBLIST_DEFINE(ModuleList, ModuleEntry);

/// The actual list of all loaded modules.
static ModuleList gs_ModuleList;

static
MModule *FindModule(const String & name)
{
   ModuleList::iterator i;
   for(i = gs_ModuleList.begin();
       i != gs_ModuleList.end();
       i++)
      if( (**i).m_Name == name )
         return (**i).m_Module;
   return NULL; // not found
}


static
MModule *LoadModuleInternal(const String & name)
{
   // No, load it:
   bool success = false;
   wxDllType dll = wxDllLoader::LoadLibrary(name, &success);
   if(! success) return NULL;

   MModule *module = MModuleImpl::Create(dll);
   if(module)
   {
      ModuleEntry *me = new ModuleEntry;
      me->m_Name = name;
      me->m_Module = module;
      gs_ModuleList.push_back(module);
   }
   else
      wxDllLoader::UnloadLibrary(dll);
   return module;
}


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
            module = LoadModuleInternal(pathname);
      }
      return module;
   }
}


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



class MModuleListingEntryImpl : public MModuleListingEntry
{
public:
   virtual const String &GetName(void) const
      { return m_Name; }
   virtual const String &GetShortDescription(void) const
      { return m_ShortDesc; }
   virtual const String &GetDescription(void) const
      { return m_Desc; }
   virtual const String &GetVersion(void) const
      { return m_Version; }
   virtual const String &GetAuthor(void) const
      { return m_Author; }

   MModuleListingEntryImpl(const String &name = "",
                           const String &shortdesc = "",
                           const String &desc = "",
                           const String &version = "",
                           const String &author = "")
      {
         m_Name = name; m_ShortDesc = shortdesc; m_Desc = desc;
         m_Version = version; m_Author = author;
      }
private:
   String m_Name, m_ShortDesc, m_Desc, m_Version, m_Author;
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
        Version:
        Author:
      */
      else
      {
         if(tf.GetLineCount() < 4)
            errorflag = true;
         else
         {
            String first = tf[0].Mid(0,strlen(MMD_SIGNATURE));
            if(first != MMD_SIGNATURE ||
               tf[1].Mid(0,strlen("Name:")) != "Name:" ||
               tf[2].Mid(0,strlen("Version:")) != "Version:" ||
               tf[3].Mid(0,strlen("Author:")) != "Author:")
                         errorflag = true;
            else
            {
               String description;
               for(size_t l = 5; l < tf.GetLineCount(); l++)
                  description << tf[l] << '\n';
               String name = (**it).AfterLast(DIR_SEPARATOR);
               name = name.Mid(0, filename.Length()-strlen(".mmd"));
               String tmp = tf[1].Mid(strlen("Name:")); // == short description
               MModuleListingEntryImpl entry(
                  name, // module name   
                  tmp,
                  description,
                  tf[2].Mid(strlen("Version:")),
                  tf[3].Mid(strlen("Author:"))
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
}
