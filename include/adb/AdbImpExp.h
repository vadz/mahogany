///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   adb/AdbImpExp.h - importing address books
// Purpose:     functions to import or export ADB contents (the functions live
//              in a small separate header because, like this, it isn't
//              necessary to recompile any client code when AdbImporter or
//              AdbExporter classes change)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADBIMPEXP_H
#define _ADBIMPEXP_H

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class AdbImporter;
class AdbExporter;
class AdbEntryGroup;

// ----------------------------------------------------------------------------
// public interface of ADB import and export classes
// ----------------------------------------------------------------------------

// try to import the data from the given file using the specified AdbImporter
// or the first one which is capable of doing it if importer == NULL. return
// TRUE if succeeded or FALSE if failed.
//
// if the function succeeds, it transfers the data to the native ADB called
// adbname (which may have existed or not before the call to this function: if
// it did exist, it won't be deleted even if import fails).
extern bool AdbImport(const String& filename,
                      const String& adbname,
                      const String& username,
                      AdbImporter *importer = NULL);

// just as AdbImport() but imports the data into an existing group of an
// existing address book
extern bool AdbImport(const String& filename,
                      AdbEntryGroup *group,
                      AdbImporter *importer = NULL);

// export the given ADB group (recursively) using the specified exporter,
// returns TRUE on success
extern bool AdbExport(AdbEntryGroup& group,
                      AdbExporter& exporter);

#endif // _ADBIMPEXP_H
