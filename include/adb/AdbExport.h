///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbExport.h - exporting address books
// Purpose:     generic interface for an AdbExporter - a class which is capable
//              to export data from our ADB format to some other one
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADBEXPORT_H
#define _ADBEXPORT_H

#ifdef __GNUG__
#   pragma interface "AdbExport.h"
#endif

#include "MObject.h"    // the base class declaration

class AdbEntryGroup;

// ----------------------------------------------------------------------------
// AdbExporter is a very simple interface: all it has to do is to implement
// Export() method.
// ----------------------------------------------------------------------------

class AdbExporter : public MObjectRC
{
public:
   // dynamic creation helpers
   typedef AdbExporter *(*Constructor)();

   // list holding information about all ADB exporters we have
   static struct AdbExporterInfo
   {
      const char *name;    // internal name
      const char *desc;    // descriptive name (shown to the user)

      Constructor CreateExporter;   // creator function

      AdbExporterInfo *next;  // next node in the linked list

      // ctor for the struct
      AdbExporterInfo(const char *name,
                      Constructor ctor,
                      const char *desc);
   } *ms_listExporters;

   // enumerate all available exporters: return the names (to make it possible
   // to create export objects) and the descriptions (to show to the user);
   // returns the number of different exporters
   static size_t EnumExporters(wxArrayString& names, wxArrayString& descs);

   // get exporter by name (it's a new pointer, caller must DecRef() it)
   static AdbExporter *GetExporterByName(const String& name);

   // do export the given address group (it may be an address book or may be
   // not - this can be tested for)
   virtual bool Export(const AdbEntryGroup& group) = 0;

   // get the name and description (shown to the user) of the format exported
   // by this class (these functions are automatically generated during
   // EXPLEMENT_ADB_EXPORTER macro expansion
   virtual String GetDescription() const = 0;
   virtual String GetName() const = 0;
};

// ----------------------------------------------------------------------------
// dynamic object creation macros
// ----------------------------------------------------------------------------

#define DECLARE_ADB_EXPORTER()                                             \
   String GetName() const;                                                 \
   String GetDescription() const;                                          \
   static AdbExporterInfo ms_info

#define IMPLEMENT_ADB_EXPORTER(name, desc)                                 \
   String name::GetName() const { return #name; }                          \
   String name::GetDescription() const { return _(desc); }                 \
   AdbExporter *ConstructorFor##name() { return new name; }                \
   AdbExporter::AdbExporterInfo name::ms_info(#name, ConstructorFor##name, \
                                             desc)

#endif // _ADBEXPORT_H
