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

// undef this to statically link all ADB importers into the executable (old
// behaviour, will be r
#define USE_ADB_MODULES

// include the base class declaration
#ifdef USE_ADB_MODULES
   #include "MModule.h"

   typedef MModule AdbImporterBase;
#else // !USE_ADB_MODULES
   #include "MObject.h"

   typedef MObjectRC AdbImporterBase;
#endif // USE_ADB_MODULES/!USE_ADB_MODULES

// some forward declarations
class wxArrayString;
class AdbEntry;

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

class AdbImporter : public AdbImporterBase
{
public:
   // dynamic creation helpers
#ifdef USE_ADB_MODULES
   AdbImporter(MInterface *minterface) : AdbImporterBase(minterface) { }
#else // !USE_ADB_MODULES
   typedef AdbImporter *(*Constructor)();
#endif // USE_ADB_MODULES/!USE_ADB_MODULES

   // list holding information about all ADB importers we have
   struct AdbImporterInfo
   {
      const char *name;    // internal name
      const char *desc;    // descriptive name (shown to the user)

      AdbImporterInfo *next;  // next node in the linked list

#ifdef USE_ADB_MODULES
      AdbImporter *CreateImporter() const;

      AdbImporterInfo(const char *name, const char *desc);
#else // !USE_ADB_MODULES
      Constructor CreateImporter;   // creator function

      // ctor for the struct
      AdbImporterInfo(const char *name,
                      Constructor ctor,
                      const char *desc);
#endif // USE_ADB_MODULES/!USE_ADB_MODULES
   };

   // access the linked list of AdbImporterInfo, you must call
   // FreeAdbImporterInfo() after you have used the list
   static AdbImporterInfo *GetAdbImpporterInfo();
   static void FreeAdbImporterInfo(AdbImporterInfo *info);

   // enumerate all available importers: return the names (to make it possible
   // to create import objects) and the descriptions (to show to the user);
   // returns the number of different importers
   static size_t EnumImporters(wxArrayString& names, wxArrayString& descs);

   // get importer by name (it's a new pointer, caller must DecRef() it)
   static AdbImporter *GetImporterByName(const String& name);

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

   // get the name and description (shown to the user) of the format imported
   // by this class (these functions are automatically generated during
   // IMPLEMENT_ADB_IMPORTER macro expansion
   virtual const char *GetName() const = 0;
   virtual const char *GetFormatDesc() const = 0;
   virtual const char *GetDescription() const = 0;

private:
   friend AdbImporterInfo; // give it access to ms_listImporters
   static AdbImporterInfo *ms_listImporters;
};

// ----------------------------------------------------------------------------
// dynamic object creation macros
// ----------------------------------------------------------------------------

#ifdef USE_ADB_MODULES

#define DECLARE_ADB_IMPORTER()                                             \
   const char *GetFormatDesc() const;                                      \
   MMODULE_DEFINE();                                                       \
   DEFAULT_ENTRY_FUNC

#define IMPLEMENT_ADB_IMPORTER(cname, desc, format, Author)                \
   MMODULE_BEGIN_IMPLEMENT(cname, _(desc), "AdbImporter", _(desc), "1.00") \
      MMODULE_PROP(author, Author)                                         \
      MMODULE_PROP(adbformat, format)                                      \
   MMODULE_END_IMPLEMENT(cname)                                            \
   const char *cname::GetFormatDesc() const                                \
   {                                                                       \
      return GetMModuleProperty("adbformat");                              \
   }                                                                       \
   MModule *cname::Init(int version_major, int version_minor,              \
                        int version_release, MInterface *minterface,       \
                        int *errorCode)                                    \
   {                                                                       \
      return new cname(minterface);                                        \
   }

#else // !USE_ADB_MODULES

#define DECLARE_ADB_IMPORTER()                                             \
   const char *GetName() const;                                            \
   const char *GetFormatDesc() const;                                      \
   const char *GetDescription() const;                                     \
   static AdbImporterInfo ms_info
#define IMPLEMENT_ADB_IMPORTER(name, desc, format, author)                 \
   const char *name::GetName() const { return #name; }                     \
   const char *name::GetFormatDesc() const { return _(format); }           \
   const char *name::GetDescription() const { return _(desc); }            \
   AdbImporter *ConstructorFor##name() { return new name; }                \
   AdbImporter::AdbImporterInfo name::ms_info(#name, ConstructorFor##name, \
                                             desc)

#endif // USE_ADB_MODULES/!USE_ADB_MODULES

#endif // _ADBIMPORT_H
