///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   CacheFile.h: base class for various cache files we have
// Purpose:     currently combines common code of MFCache.cpp and Pop3.cpp
// Author:      Vadim Zeitlin
// Modified by:
// Created:     15.10.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "CacheFile.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "MApplication.h"
#endif // USE_PCH

#include <wx/log.h>           // for wxLogNull
#include <wx/filefn.h>        // for wxMkdir
#include <wx/file.h>
#include <wx/textfile.h>

#include "CacheFile.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

/* static */
String CacheFile::GetCacheDirName()
{
   String dirname;
   dirname << mApplication->GetLocalDir() << DIR_SEPARATOR << _T("cache");

   return dirname;
}

// ----------------------------------------------------------------------------
// version checking
// ----------------------------------------------------------------------------

/* static */
void CacheFile::SplitVersion(int version, int& verMaj, int& verMin)
{
   verMaj = (version & 0xff00) >> 8;
   verMin = version & 0xff;
}

/* static */
int CacheFile::BuildVersion(int verMaj, int verMin)
{
   return (verMaj << 8) | verMin;
}

int CacheFile::CheckFormatVersion(const String& header, int *version) const
{
   int verMaj, verMin;
   if ( wxSscanf(header, GetFileHeader(), &verMaj, &verMin) == 2 )
   {
      int verMajCurrent, verMinCurrent;
      SplitVersion(GetFormatVersion(), verMajCurrent, verMinCurrent);

      if ( verMaj > verMajCurrent ||
            (verMaj == verMajCurrent && verMin > verMinCurrent) )
      {
         // I don't know what should we really do (refuse to overwrite it?
         // this would be stupid, it's just a cache file...), but at least
         // tell the user about it
         wxLogWarning(_("Your mail folder status cache file (%s) was "
                        "created by a newer version of Mahogany but "
                        "will be overwritten when the program exits "
                        "in older format."), GetFileName().c_str());

         // don't try to read it
         return -1;
      }
      else
      {
         *version = BuildVersion(verMaj, verMin);
      }
   }
   else
   {
      wxLogError(_("Missing header at line 1."));

      return false;
   }

   return true;
}

// ----------------------------------------------------------------------------
// saving/loading
// ----------------------------------------------------------------------------

bool CacheFile::Save()
{
   String filename = GetFileName();

   String dirname;
   wxSplitPath(filename, &dirname, NULL, NULL);

   if ( !wxPathExists(dirname) )
   {
      if ( !wxMkdir(dirname) )
      {
         static String s_dirFailedCreate;

         // remember if we had already given the message about this directory
         // and don't do any more - as we're called perdiodically, this would
         // result in a flood of messages if the user went away from the
         // terminal
         if ( dirname != s_dirFailedCreate )
         {
            s_dirFailedCreate = dirname;

            wxLogError(_("Failed to create directory for cache files."));
         }

         return false;
      }
   }

   wxTempFile file;
   bool ok = file.Open(filename);

   // write header
   if ( ok )
   {
      int verMaj, verMin;
      SplitVersion(GetFormatVersion(), verMaj, verMin);

      String header;
      header.Printf(GetFileHeader(), verMaj, verMin);

      header += '\n';
      ok = file.Write(header);
   }

   if ( ok )
   {
      // now write the data to the file
      ok = DoSave(file);
   }

   if ( ok )
   {
      // ensure that data has been flushed to disk
      ok = file.Commit();
   }

   if ( !ok )
   {
      wxLogMessage(_("Some non vital information could be lost, please "
                     "try to correct the problem and restart the program."));

      wxLogError(_("Failed to write cache file."));

      return false;
   }

   return true;
}

bool CacheFile::Load()
{
   wxTextFile file;

   String filename = GetFileName();

   {
      wxLogNull noLog;
      if ( !file.Open(filename) )
      {
         // cache file can perfectly well not exist, it's not an error
         return true;
      }
   }

   // check the header
   size_t count = file.GetLineCount();

   // first line is the header but we never write just the header
   bool ok = count > 1;

   int version = 0; // unneeded but suppresses the compiler warning
   if ( ok )
   {
      int rc = CheckFormatVersion(file[0u], &version);
      if ( rc == -1 )
      {
         // future version detected, skip loading
         return true;
      }

      ok = rc != 0;
   }

   if ( ok )
   {
      ok = DoLoad(file, version);

      if ( !ok )
      {
         wxLogWarning(_("Failed to load cache file '%s'."),
                      GetFileName().c_str());
      }
   }

   return ok;
}

