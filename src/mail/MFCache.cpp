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

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include <wx/wxchar.h>
#endif // USE_PCH

#include <wx/textfile.h>

#include "MEvent.h"
#include "MFolder.h"
#include "MFCache.h"
#include "MFStatus.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// location of the cache file
#define CACHE_FILENAME _T("status")

// the delimiter in the text file lines
#define CACHE_DELIMITER _T(":")      // string, not char, to allow concatenating
#define CACHE_DELIMITER_CH (CACHE_DELIMITER[0])

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
   Load();

   // register for folder rename events
   m_evtmanHandle = MEventManager::Register(*this, MEventId_FolderTreeChange);

   // no changes yet
   m_isDirty =
   m_hasFailedToSave = false;
}

MfStatusCache::~MfStatusCache()
{
   if ( m_evtmanHandle )
   {
      MEventManager::Deregister(m_evtmanHandle);
   }

   Save();

   // delete the elements too
   WX_CLEAR_ARRAY(m_folderData);
}

// ----------------------------------------------------------------------------
// MfStatusCache event processing
// ----------------------------------------------------------------------------

bool MfStatusCache::OnMEvent(MEventData& ev)
{
   MEventFolderTreeChangeData& event = (MEventFolderTreeChangeData&)ev;
   if ( event.GetChangeKind() == MEventFolderTreeChangeData::Rename )
   {
      // keep the status cache entry for the old folder
      int index = m_folderNames.Index(event.GetFolderFullName());
      if ( index != wxNOT_FOUND )
      {
         m_folderNames[index] = event.GetNewFolderName();
      }
   }

   // continue with the event processing
   return true;
}

// ----------------------------------------------------------------------------
// MfStatusCache data access
// ----------------------------------------------------------------------------

bool MfStatusCache::GetStatus(const String& folderName,
                              MailFolderStatus *status)
{
   int n = m_folderNames.Index(folderName);
   if ( n == wxNOT_FOUND || !m_folderData[(size_t)n]->IsValid() )
   {
      // no status or at least no valid status
      return false;
   }

   if ( status )
   {
      *status = *m_folderData[(size_t)n];
   }

   return true;
}

void MfStatusCache::UpdateStatus(const String& folderName,
                                 const MailFolderStatus& status)
{
   CHECK_RET( status.IsValid(), _T("invalid status in MfStatusCache") );

   int n = m_folderNames.Index(folderName);
   if ( n == wxNOT_FOUND )
   {
      wxLogTrace(M_TRACE_MFSTATUS,
                 _T("Added status for '%s' (%lu total, %lu unread)"),
                 folderName.c_str(), status.total, status.unread);

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

      wxLogTrace(M_TRACE_MFSTATUS,
                 _T("Changed status for '%s' (%lu total, %lu unread)"),
                 folderName.c_str(), status.total, status.unread);
   }

   // update
   *m_folderData[(size_t)n] = status;
   m_isDirty = true;

   // and tell everyone about it
   MEventManager::Send(new MEventFolderStatusData(folderName));
}

void MfStatusCache::InvalidateStatus(const String& folderName)
{
   wxLogTrace(M_TRACE_MFSTATUS, _T("Invalidated status for '%s'"),
              folderName.c_str());

   int n = m_folderNames.Index(folderName);
   if ( n != wxNOT_FOUND )
   {
      // don't remove it because chances are that an UpdateStatus() for the
      // same folder will follow soon (as we typically invalidate the status
      // when noticing new mail in the folder and then update it as soon as we
      // know how many new messages we have got) but just invalidate for now
      m_folderData[(size_t)n]->total = UID_ILLEGAL;

      m_isDirty = true;
   }

   // if anybody has the status info for this folder, it must be updated
   MEventManager::Send(new MEventFolderStatusData(folderName));
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
   filename << GetCacheDirName() << DIR_SEPARATOR << CACHE_FILENAME;

   return filename;
}

String MfStatusCache::GetFileHeader() const
{
   return _T("Mahogany Folder Status Cache File (version %d.%d)");
}

int MfStatusCache::GetFormatVersion() const
{
   return BuildVersion(1, 1);
}

bool MfStatusCache::DoLoad(const wxTextFile& file, int version)
{
   bool isFmtOk = true;

   CacheFileFormat fmt;
   if ( version == BuildVersion(1, 1) )
   {
      fmt = CacheFile_1_1;
   }
   else if ( version == BuildVersion(1, 0) )
   {
      fmt = CacheFile_1_0;
   }
   else
   {
      fmt = CacheFile_Max;
   }

   // read the data
   wxString str, name;
   str.Alloc(1024);     // avoid extra memory allocations
   name.Alloc(1024);

   MailFolderStatus status;

   size_t count = file.GetLineCount();
   for ( size_t n = 1; n < count; n++ )
   {
      str = file[n];

      // first get the end of the full folder name knowing that we should
      // skip all "::" as they could have only resulted from quoting a ':'
      // in the folder name and so the loop below looks for the first ':'
      // not followed by another ':'
      const wxChar *p = wxStrchr(str, CACHE_DELIMITER_CH);
      while ( p && p[1] == CACHE_DELIMITER_CH )
      {
         p = wxStrchr(p + 2, CACHE_DELIMITER_CH);
      }

      if ( !p )
      {
         wxLogError(_("Missing '%c' at line %d."), CACHE_DELIMITER_CH, n + 1);

         isFmtOk = false;

         break;
      }

      name = wxString(str.c_str(), p);

      // now unquote ':' which were doubled by Save()
      name.Replace(CACHE_DELIMITER CACHE_DELIMITER, CACHE_DELIMITER);

      // get the rest
      status.Init();
      switch ( fmt )
      {
         case CacheFile_1_0:
            isFmtOk = wxSscanf(p + 1,
                             _T("%lu") CACHE_DELIMITER
                             _T("%lu") CACHE_DELIMITER
                             _T("%lu"),
                             &status.total,
                             &status.unread,
                             &status.flagged) == 3;
            break;

         default:
            FAIL_MSG( _T("unknown cache file format") );
            // fall through nevertheless

         case CacheFile_1_1:
            isFmtOk = wxSscanf(p + 1,
                             _T("%lu") CACHE_DELIMITER
                             _T("%lu") CACHE_DELIMITER 
                             _T("%lu") CACHE_DELIMITER
                             _T("%lu"),
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

      // ignore the folders which were deleted during the last program run
      MFolder *folder = MFolder::Get(name);
      if ( folder )
      {
         folder->DecRef();

         // do add the entry to the cache
         size_t entry = m_folderNames.Add(name);
         m_folderData.Insert(new MailFolderStatus(status), entry);
      }
      else
      {
         wxLogDebug(_T("Removing deleted folder '%s' from status cache."),
                    name.c_str());
      }
   }

   if ( !isFmtOk )
   {
      wxLogWarning(_("Your mail folder status cache file (%s) was corrupted."),
                   file.GetName());

      return false;
   }

   return true;
}

bool MfStatusCache::Save()
{
   // avoid doing anything if we don't have anything to cache
   if ( !m_folderNames.IsEmpty() )
   {
      if ( !CacheFile::Save() )
      {
         // set a flag to indicate that we shouldn't be called any more by
         // Flush() -- but we'll still be called from our dtor for one last
         // attempt to save our contents
         m_hasFailedToSave = true;

         return false;
      }
   }

   // reset the dirty flag - we're saved now
   m_isDirty = false;

   return true;
}

bool MfStatusCache::DoSave(wxTempFile& file)
{
   wxString str, name;
   str.reserve(1024);
   name.reserve(512);

   // write data
   size_t count = m_folderNames.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      const MailFolderStatus *status = m_folderData[n];

      // double all delimiters in the folder name
      name = m_folderNames[n];
      name.Replace(CACHE_DELIMITER, CACHE_DELIMITER CACHE_DELIMITER);

      // and write info to file: note that we don't remember the number of
      // recent messages because they won't be recent the next time we run
      // anyhow nor the number of messages matching the search criteria as
      // this is hardly ever useful
      str.Printf(_T("%s") CACHE_DELIMITER
                 _T("%lu") CACHE_DELIMITER
                 _T("%lu") CACHE_DELIMITER
                 _T("%lu") CACHE_DELIMITER
                 _T("%lu\n"),
                 name.c_str(),
                 status->total,
                 status->newmsgs,
                 status->unread,
                 status->flagged);

      if ( !file.Write(str) )
      {
         return false;
      }
   }

   return true;
}

/* static */
void MfStatusCache::Flush()
{
   // don't flush the cache repeatedly if we failed to do it, this leads to a
   // endless stream of annoying message boxes without any good effect
   if ( gs_mfStatusCache &&
         gs_mfStatusCache->IsDirty() &&
           !gs_mfStatusCache->HasFailedToSave() )
   {
      (void)gs_mfStatusCache->Save();
   }
}

