///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFCache.cpp - implements classes from MFCache.h
// Purpose:     cache often accessed info to improve MailFolder speed
// Author:      Vadim Zeitlin
// Modified by:
// Created:     02.04.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

#ifdef __GNUG__
   #pragma implementation "MFCache.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "MApplication.h"
   #include "MEvent.h"
#endif // USE_PCH

#include <wx/log.h>           // for wxLogNull
#include <wx/filefn.h>        // for wxMkdir
#include <wx/file.h>
#include <wx/textfile.h>

#include "MFCache.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// location of the cache file
#define CACHE_DIRNAME "cache"
#define CACHE_FILENAME "status"

// the cache file header format
#define CACHE_HEADER "Mahogany Folder Status Cache File (version %d.%d)\n"

// the cache file format version
#define CACHE_VERSION_MAJOR 1
#define CACHE_VERSION_MINOR 1

// the delimiter in the text file lines
#define CACHE_DELIMITER ":"      // yes, it's a string, not a char

// the versions of the file format we know about
enum CacheFileFormat
{
   CacheFile_1_0,    // name:total:unread:flagged
   CacheFile_1_1,    // name:total:new:unread:flagged
   CacheFile_Current = CacheFile_1_1,
   CacheFile_Max
};

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

static MfStatusCache *gs_mfStatusCache = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MfStatusCache construction/destruction
// ----------------------------------------------------------------------------

/* static */
MfStatusCache *MfStatusCache::Get()
{
   if ( !gs_mfStatusCache )
   {
      gs_mfStatusCache = new MfStatusCache;
   }

   return gs_mfStatusCache;
}

/* static */
void MfStatusCache::CleanUp()
{
   if ( gs_mfStatusCache )
   {
      delete gs_mfStatusCache;
      gs_mfStatusCache = NULL;
   }
}

MfStatusCache::MfStatusCache()
             : m_folderNames(TRUE /* auto sorted array */)
{
   Load(GetFileName());

   // no changes yet
   m_isDirty = false;
}

MfStatusCache::~MfStatusCache()
{
   Save(GetFileName());

   // delete the elements too
   WX_CLEAR_ARRAY(m_folderData);
}

// ----------------------------------------------------------------------------
// MfStatusCache data access
// ----------------------------------------------------------------------------

bool MfStatusCache::GetStatus(const String& folderName,
                              MailFolderStatus *status)
{
   CHECK( status, false, "NULL pointer in MfStatusCache::GetStatus" );

   int n = m_folderNames.Index(folderName);
   if ( n == wxNOT_FOUND )
      return false;

   *status = *m_folderData[(size_t)n];

   return true;
}

void MfStatusCache::UpdateStatus(const String& folderName,
                                 const MailFolderStatus& status)
{
   CHECK_RET( status.IsValid(), "invalid status in MfStatusCache" );

   int n = m_folderNames.Index(folderName);
   if ( n == wxNOT_FOUND )
   {
      // add it
      n = m_folderNames.Add(folderName);
      m_folderData.Insert(new MailFolderStatus, (size_t)n);
   }
   else // already have it
   {
      // did it really change?
      if ( *m_folderData[(size_t)n] == status )
      {
         // no, avoid sending the event below
         return;
      }
   }

   // update
   *m_folderData[(size_t)n] = status;
   m_isDirty = true;

   // and tell everyone about it
   MEventManager::Send(new MEventFolderStatusData(folderName));
}

void MfStatusCache::InvalidateStatus(const String& folderName)
{
   int n = m_folderNames.Index(folderName);
   if ( n != wxNOT_FOUND )
   {
      m_folderNames.RemoveAt((size_t)n);
      m_folderData.RemoveAt((size_t)n);

      m_isDirty = true;
   }
}

// ----------------------------------------------------------------------------
// MfStatusCache loading/saving
// ----------------------------------------------------------------------------

/*
   We use a simple text file for cache right now - if this proves to be too
   slow or disk space consuming, we can always switch to something more
   efficient here, the only methods to change are those below.
 */

String MfStatusCache::GetFileName() const
{
   String filename;
   filename << mApplication->GetLocalDir() << DIR_SEPARATOR
            << CACHE_DIRNAME << DIR_SEPARATOR
            << CACHE_FILENAME;

   return filename;
}

bool MfStatusCache::Load(const String& filename)
{
   wxTextFile file;
   {
      wxLogNull noLog;
      if ( !file.Open(filename) )
      {
         // cache file can perfectly well not exist, it's not an error
         return true;
      }
   }

   // check the headers
   size_t count = file.GetLineCount();

   // first line is the header but we never write just the header
   bool isFmtOk = count > 1;

   // get the version
   CacheFileFormat fmt = CacheFile_Current;
   if ( isFmtOk )
   {
      int verMaj, verMin;
      isFmtOk = sscanf(file[0u], CACHE_HEADER, &verMaj, &verMin) == 2;

      if ( isFmtOk )
      {
         if ( verMaj > CACHE_VERSION_MAJOR ||
               (verMaj == CACHE_VERSION_MAJOR && verMin > CACHE_VERSION_MINOR) )
         {
            // I don't know what should we really do (refuse to overwrite it?
            // this would be stupid, it's just a cache file...), but at least
            // tell the user about it
            wxLogWarning(_("Your mail folder status cache file (%s) was "
                           "created by a newer version of Mahogany but "
                           "will be overwritten when the program exits "
                           "in older format."), filename.c_str());

            // don't try to read it
            return true;
         }
         else if ( verMaj < CACHE_VERSION_MAJOR ||
                     (verMaj == CACHE_VERSION_MAJOR &&
                        verMin < CACHE_VERSION_MINOR) )
         {
            // read the old format
            fmt = CacheFile_1_0;
         }
      }
      else
      {
         wxLogError(_("Missing header at line 1."));
      }
   }

   // now read the data
   if ( isFmtOk )
   {
      wxString str, name;
      str.Alloc(1024);     // avoid extra memory allocations
      name.Alloc(1024);

      MailFolderStatus status;

      // almost a constant...
      static const char chDelim = CACHE_DELIMITER[0];

      for ( size_t n = 1; n < count; n++ )
      {
         str = file[n];

         // we have to unquote CACHE_DELIMITER which could occur in the folder
         // name - see Save()
         //
         // this is safe to do as we can only have 2 consecutive delimiters in
         // the folder name
         str.Replace(CACHE_DELIMITER CACHE_DELIMITER, CACHE_DELIMITER);

         // now tokenize it (can't use wxStringTokenizer because we can still
         // have consecutive delimiters if the folder name contained double
         // colon (unlikely but not impossible) and we'd have to code around
         // it - better do it manually)

         // first get the full folder name
         const char *p = strchr(str, chDelim);
         while ( p && p[0] && p[1] == chDelim )
         {
            p = strchr(p, chDelim);
         }

         if ( !p || !p[0] )
         {
            wxLogError(_("Missing '%c' at line %d."), chDelim, n + 1);

            isFmtOk = false;

            break;
         }

         name = wxString(str.c_str(), p);

         // get the rest
         status.Init();
         switch ( fmt )
         {
            case CacheFile_1_0:
               isFmtOk = sscanf(p + 1,
                                "%lu" CACHE_DELIMITER
                                "%lu" CACHE_DELIMITER
                                "%lu",
                                &status.total,
                                &status.unread,
                                &status.flagged) == 3;
               break;

            default:
               FAIL_MSG( "unknown cache file format" );
               // fall through nevertheless

            case CacheFile_1_1:
               isFmtOk = sscanf(p + 1,
                                "%lu" CACHE_DELIMITER
                                "%lu" CACHE_DELIMITER 
                                "%lu" CACHE_DELIMITER
                                "%lu",
                                &status.total,
                                &status.newmsgs,
                                &status.unread,
                                &status.flagged) == 4;
         }

         if ( !isFmtOk )
         {
            wxLogError(_("Missing field(s) at line %d."), n + 1);

            break;
         }

         // do add the entry to the cache
         size_t entry = m_folderNames.Add(name);
         m_folderData.Insert(new MailFolderStatus, entry);
         *m_folderData[entry] = status;
      }
   }

   if ( !isFmtOk )
   {
      wxLogWarning(_("Your mail folder status cache file (%s) was corrupted."),
                   filename.c_str());

      return false;
   }

   return true;
}

bool MfStatusCache::Save(const String& filename)
{
   // avoid doing anything if we don't have anything to cache
   size_t count = m_folderNames.GetCount();
   if ( !count )
   {
      m_isDirty = false;

      return true;
   }

   // create the directory for the file if needed
   String dirname;
   wxSplitPath(filename, &dirname, NULL, NULL);

   if ( !wxDirExists(dirname) )
   {
      if ( !wxMkdir(dirname) )
      {
         wxLogError(_("Failed to create directory for cache files."));

         return false;
      }
   }

   wxTempFile file;
   bool ok = file.Open(filename);

   wxString str;
   str.Alloc(1024);     // avoid extra memory allocations

   // write header
   if ( ok )
   {
      str.Printf(CACHE_HEADER, CACHE_VERSION_MAJOR, CACHE_VERSION_MINOR);

      ok = file.Write(str);
   }

   if ( ok )
   {
      wxString name;
      name.Alloc(512);

      // write data
      for ( size_t n = 0; ok && (n < count); n++ )
      {
         MailFolderStatus *status = m_folderData[n];

         // double all delimiters in the folder name
         name = m_folderNames[n];
         name.Replace(CACHE_DELIMITER, CACHE_DELIMITER CACHE_DELIMITER);

         // and write info to file: note that we don't remember the number of
         // recent messages because they won't be recent the next time we run
         // anyhow nor the number of messages matching the search criteria as
         // this is hardly ever useful
         str.Printf("%s" CACHE_DELIMITER
                    "%lu" CACHE_DELIMITER
                    "%lu" CACHE_DELIMITER
                    "%lu" CACHE_DELIMITER
                    "%lu\n",
                    name.c_str(),
                    status->total,
                    status->newmsgs,
                    status->unread,
                    status->flagged);

         ok = file.Write(str);
      }
   }

   if ( ok )
   {
      // ensure that data has been flushed to disk
      ok = file.Commit();
   }

   if ( !ok )
   {
      wxLogError(_("Failed to write cache file."));

      return false;
   }

   // reset the dirty flag - we're saved now
   m_isDirty = false;

   return true;
}

/* static */
void MfStatusCache::Flush()
{
   if ( gs_mfStatusCache && gs_mfStatusCache->IsDirty() )
   {
      (void)gs_mfStatusCache->Save(gs_mfStatusCache->GetFileName());
   }
}

