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

#include <wx/dynlib.h>


/* static */
MModule *
MModule::LoadModule(const String & name)
{
   bool success = false;
   wxDllType dll = wxDllLoader::LoadLibrary(name, &success);
   if(! success) return NULL;

   CreateModuleFuncType
      createModuleFunc = wxDllLoader::GetSymbol(dll, "CreateModule");
   if(! createModuleFunc)
   {
      //FIXME unload the DLL
      return NULL;
   }
   return createModuleFunc();
}

