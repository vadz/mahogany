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

#include "AdbModule.h"    // the base class declaration

class AdbEntry;
class AdbEntryGroup;

// the interface we implement
#define ADB_EXPORTER_INTERFACE "AdbExporter"

// ----------------------------------------------------------------------------
// AdbExporter is a very simple interface: all it has to do is to implement
// Export() method.
// ----------------------------------------------------------------------------

class AdbExporter : public AdbModule
{
public:
   // enumerate all available exporters: return the names (to make it possible
   // to create export objects) and the descriptions (to show to the user);
   // returns the number of different exporters
   static size_t EnumExporters(wxArrayString& names, wxArrayString& descs)
   {
      return EnumModules(ADB_EXPORTER_INTERFACE, names, descs);
   }

   // get exporter by name (it's a new pointer, caller must DecRef() it)
   static AdbExporter *GetExporterByName(const String& name)
   {
      return (AdbExporter *)GetModuleByName(ADB_EXPORTER_INTERFACE, name);
   }

   // do export the given address group (it may be an address book or may be
   // not - this can be tested for) to the specified "destination" which can bea
   // filename, directory name or whatever else (if empty, the exporter should
   // ask the user)
   virtual bool Export(AdbEntryGroup& group, const String& dest) = 0;

   // export one entry only
   virtual bool Export(const AdbEntry& entry, const String& dest) = 0;
};

// ----------------------------------------------------------------------------
// dynamic object creation macros
// ----------------------------------------------------------------------------

#define DECLARE_ADB_EXPORTER() DECLARE_ADB_MODULE()
#define IMPLEMENT_ADB_EXPORTER(cname, desc, format, author) \
   IMPLEMENT_ADB_MODULE(ADB_EXPORTER_INTERFACE, cname, desc, format, author)

#endif // _ADBEXPORT_H
