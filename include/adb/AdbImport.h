///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbImport.h - importing address books
// Purpose:     generic interface for an AdbImporter - a class which is capable
//              to import data from foreign ADB format into the native one
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADBIMPORT_H
#define _ADBIMPORT_H

#ifdef __GNUG__
   #pragma interface "AdbImport.h"
#endif

#include "adb/AdbModule.h"     // the base class

// some forward declarations
class wxArrayString;
class AdbEntry;

// the interface we implement
#define ADB_IMPORTER_INTERFACE _T("AdbImporter")

// ----------------------------------------------------------------------------
// AdbImporter - this class imports data from ADBs in foreign formats.
//
// TODO: currently we only know about file-based address books, no attempt
//       was made to take into account global naming services or anything else
//       more fancy than files - but we should think about it...
//
// The objects of this class (or of classes derived from it) are created by
// AdbImport function (see AdbImport.h) dynamically, i.e. the function does
// not know the name of the class whose object it creates but just uses the
// first importer which can import the given address book or the one which was
// chosen by the user.
//
// To allow this "dynamic" creation DECLARE and IMPLEMENT_ADB_IMPORTER macros
// (see below) should be used by any derived classes.
//
// This class inherits from MModule and so uses reference counting
// (see MObject.h for basic rules or dealing with ref counted objects).
// ----------------------------------------------------------------------------

class AdbImporter : public AdbModule
{
public:
   // enumerate all available importers: return the names (to make it possible
   // to create import objects) and the descriptions (to show to the user);
   // returns the number of different importers
   static size_t EnumImporters(wxArrayString& names, wxArrayString& descs)
   {
      return EnumModules(ADB_IMPORTER_INTERFACE, names, descs);
   }

   // get importer by name (it's a new pointer, caller must DecRef() it)
   static AdbImporter *GetImporterByName(const String& name)
   {
      return (AdbImporter *)GetModuleByName(ADB_IMPORTER_INTERFACE, name);
   }

   // test address book in the filename - can we import it?
   virtual bool CanImport(const String& filename) = 0;

   // get the name of the "default" address book file for this importer:
   // sometimes, it makes sense (Unix .mailrc files always live in the home
   // directory, so returning $HOME/.mailrc makes sense for mailrc importer),
   // sometimes it doesn't - in this case, just return an empty string.
   //
   // NB: if this function decides to search for the file (e.g., it knows its
   //     name but not the location), it should provide the necessary feedback
   //     to the user.
   virtual String GetDefaultFilename() const = 0;

   // start importing the file: GetEntry/GroupNames and ImportEntry can only be
   // called after a call to this function
   virtual bool StartImport(const String& filename) = 0;

   // we have a concept of a path which is needed to work with tree-like ADB
   // structures: it's just the sequence of the "nodes" separated by '/'. The
   // root node corresponds to the empty path (or also to the path "/")

   // enumerate all entries under the given path, return their number
   virtual size_t GetEntryNames(const String& path,
                                wxArrayString& entries) const = 0;

   // enumerate all subgroups under the given path, return their number
   virtual size_t GetGroupNames(const String& path,
                                wxArrayString& groups) const = 0;

   // copy data from one entry under the given path to AdbEntry (the index
   // corresponds to the entry index in the array returned by GetEntryNames)
   virtual bool ImportEntry(const String& path,
                            size_t index,
                            AdbEntry *entry) = 0;

   virtual ~AdbImporter() = 0;
};

// ----------------------------------------------------------------------------
// dynamic object creation macros
// ----------------------------------------------------------------------------

#define DECLARE_ADB_IMPORTER() DECLARE_ADB_MODULE()
#define IMPLEMENT_ADB_IMPORTER(cname, desc, format, author) \
   IMPLEMENT_ADB_MODULE(ADB_IMPORTER_INTERFACE, cname, desc, format, author)

#endif // _ADBIMPORT_H
