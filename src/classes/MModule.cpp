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
      
      CreateModuleFuncType
         createModuleFunc = (CreateModuleFuncType)
         wxDllLoader::GetSymbol(dll, "CreateMModule"); 
      if(! createModuleFunc)
      {
         //FIXME unload the DLL
         return NULL;
      }
      
      module = createModuleFunc(M_VERSION_MAJOR,
                                M_VERSION_MINOR,
                                M_VERSION_RELEASE);
      if(module)
      {
         ModuleEntry *me = new ModuleEntry;
         me->m_Name = name;
         me->m_Module = module;
         gs_ModuleList.push_back(module);
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
