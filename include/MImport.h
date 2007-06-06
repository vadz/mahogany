///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MImport.h - declaration of MImporter interface used to import
//              various settings from other MUA (typically during the
//              installation)
// Purpose:     MImporter is the interface implemented by the "import" modules
//              which will be found automatically by M during installation and
//              used
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.05.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MIMPORT_H_
#define _MIMPORT_H_

#ifdef __GNUG__
   #pragma interface "MImport.h"
#endif

#include "MModule.h"    // the base class

class MFolder;
class WXDLLEXPORT wxWindow;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the importer interface name
#define M_IMPORTER_INTERFACE _T("Importer")

// the name of the property containing the name of the program we work with
#define M_IMPORTER_PROG_NAME _T("progname")

// ----------------------------------------------------------------------------
// MImporter: class for importing everything
// ----------------------------------------------------------------------------

class MImporter : public MModule
{
public:
   // the flags telling what this importer knows to do and hence which of
   // ImportXXX() functions may be called
   enum
   {
      Import_ADB      = 0x0001,
      Import_Folders  = 0x0002,
      Import_Settings = 0x0004,
      Import_Filters  = 0x0008
   };

   // the flags for ImportFolders()
   enum
   {
      ImportFolder_SystemImport    = 0x0001, // import system folders
      ImportFolder_SystemUseParent = 0x0003, // put them under parent
      ImportFolder_AllUseParent    = 0x0006  // put all folders under parent
   };

   // return TRUE if the program this importer works with is/was installed
   virtual bool Applies() const = 0;

   // return the combination of Import_XXX flags defined above
   virtual int GetFeatures() const = 0;

   // import the address books
   virtual bool ImportADB() = 0;

   // import the folders creating them under the given parent folder
   // (note: it won't DecRef() folderParent)
   virtual bool ImportFolders(MFolder *folderParent,
                              int flags = ImportFolder_SystemUseParent) = 0;

   // import the program settings
   virtual bool ImportSettings() = 0;

   // import the filters
   virtual bool ImportFilters() = 0;

   // return the name of the program we work with (generated during
   // DECLARE_M_IMPORTER and IMPLEMENT_M_IMPORTER macro expansion)
   virtual const wxChar *GetProgName() const = 0;
};

// ----------------------------------------------------------------------------
// functions to work with importers
// ----------------------------------------------------------------------------

// return TRUE if at least one importer can import something, FALSE otherwise
extern bool HasImporters();

// functions to show import dialog for all importers or the specified one
extern bool ShowImportDialog(MImporter& importer, wxWindow *parent = NULL);
extern bool ShowImportDialog(wxWindow *parent = NULL);

// ----------------------------------------------------------------------------
// macros for importers declaration/implementation
// ----------------------------------------------------------------------------

// this macro must be used inside the declaration of the importer class
#define DECLARE_M_IMPORTER()                                                   \
   virtual const wxChar *GetProgName() const;                                  \
   MMODULE_DEFINE();                                                           \
   DEFAULT_ENTRY_FUNC                                                          \

// these macros are used to define the importer and its properties, you may put
// lines of the form MMODULE_PROP(name, value) between them to add other
// properties such as description or whatever
#define MIMPORTER_BEGIN_IMPLEMENT(cname, progname, desc)                       \
   MMODULE_BEGIN_IMPLEMENT(cname, _T(#cname), M_IMPORTER_INTERFACE,            \
                           _T(""), _T("1.00"))                                 \
      MMODULE_PROP(M_IMPORTER_PROG_NAME, progname)                             \
      MMODULE_PROP(MMODULE_DESCRIPTION_PROP, desc)

#define MIMPORTER_END_IMPLEMENT(cname)                                         \
   MMODULE_END_IMPLEMENT(cname)                                                \
   const wxChar *cname::GetProgName() const                                    \
   {                                                                           \
      return GetMModuleProperty(ms_properties, M_IMPORTER_PROG_NAME);          \
   }                                                                           \
   MModule *cname::Init(int maj, int min, int rel, MInterface *, int *err)     \
   {                                                                           \
      return new cname();                                                      \
   }

// this macro replaces BEGIN/END pair if you don't need any additional
// properties
#define IMPLEMENT_M_IMPORTER(cname, progname, desc)                            \
   MIMPORTER_BEGIN_IMPLEMENT(cname, progname, desc)                            \
   MIMPORTER_END_IMPLEMENT(cname)

#endif // _MIMPORT_H_
