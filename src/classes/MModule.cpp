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
#include "Mconfig.h"
#include "Mcommon.h"
#include "MModule.h"

#include "Mversion.h"
#include "kbList.h"

#include <wx/dynlib.h>

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
   /// Returns a brief description of the module.
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
private:
   MModule_GetNameFuncType m_GetName;
   MModule_GetDescriptionFuncType m_GetDescription;
   MModule_GetVersionFuncType m_GetVersion;
   MModule_GetMVersionFuncType m_GetMVersion;
};


MModule *
MModuleImpl::Create(wxDllType dll)
{
   MModule_InitModuleFuncType
      initModuleFunc = (MModule_InitModuleFuncType)
      wxDllLoader::GetSymbol(dll, "InitMModule"); 
   if(! initModuleFunc)
   {
      //FIXME unload the DLL
      return NULL;
   }
   if( (*initModuleFunc)(M_VERSION_MAJOR,
                         M_VERSION_MINOR,
                         M_VERSION_RELEASE) == MMODULE_ERR_NONE)
   {
      return new MModuleImpl(dll);
   }
   else
      return NULL;
}

MModuleImpl::MModuleImpl(wxDllType dll)
{
   m_GetName = (MModule_GetNameFuncType)
      wxDllLoader::GetSymbol(dll, "GetName"); ;
   m_GetDescription  = (MModule_GetDescriptionFuncType)
      wxDllLoader::GetSymbol(dll, "GetDescription"); 
   m_GetVersion = (MModule_GetVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetVersion"); 
   m_GetMVersion = (MModule_GetMVersionFuncType)
      wxDllLoader::GetSymbol(dll, "GetMVersion"); 
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
      // No, load it:
      bool success = false;
      wxDllType dll = wxDllLoader::LoadLibrary(name, &success);
      if(! success) return NULL;

      module = MModuleImpl::Create(dll);
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
