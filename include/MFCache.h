///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFCache.h - some helper classes for caching folder related info
// Purpose:     cache often accessed info to improve MailFolder speed
// Author:      Vadim Zeitlin
// Modified by:
// Created:     02.04.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef  _MFCACHE_H_
#define  _MFCACHE_H_

#ifndef   USE_PCH
#  include <wx/dynarray.h>        // for WX_DEFINE_ARRAY
#endif // USE_PCH

#include "CacheFile.h"           // base class

#include "MEvent.h"
#include "MFStatus.h"

WX_DEFINE_ARRAY(MailFolderStatus *, MfStatusArray);

// trace mask for logging MfStatusCache methods and other mailfolder
// status-related activity
#define M_TRACE_MFSTATUS _T("mfstatus")

// ----------------------------------------------------------------------------
// MfStatusCache: this class caches MailFolderStatus info for each folder,
// this allows to preserve the status across sessions and avoids unnecessary
// rebuilding of the entire folder listing if we're only interested in its
// status (which we often are, for example when updating the status in the
// tree)
// ----------------------------------------------------------------------------

class MfStatusCache : public CacheFile,
                      public MEventReceiver
{
public:
   // this is a singleton class and this function is the only way to access it
   static MfStatusCache *Get();

   // MfStatusCache is not ref counted, instead a single object is kept alive
   // all the time and someone must call this method exactly once before the
   // program termination to delete it (it's ok to call it even if Get() had
   // been never called)
   static void CleanUp();

   // flush the current status to disk (always safe to call, even if there is
   // no MfStatusCache in use currently)
   static void Flush();

   // query the status info: return true and fill the provided pointer with
   // info if we have it (and the pointer is not NULL), return false otherwise
   bool GetStatus(const String& folderName, MailFolderStatus *status = NULL);

   // update the status info 
   void UpdateStatus(const String& folderName, const MailFolderStatus& status);

   // forget the status info for the given folder
   void InvalidateStatus(const String& folderName);

protected:
   // protected ctor for CreateStatusCache()
   MfStatusCache();

   // and protected dtor - CleanUp() should be called instead
   virtual ~MfStatusCache();

   // implement MEventReceiver pure virtual to process folder rename events
   virtual bool OnMEvent(MEventData& event);

   // do we need to be saved at all?
   bool IsDirty() const { return m_isDirty; }

   // have we failed to save our contents the last time Save() was called?
   bool HasFailedToSave() const { return m_hasFailedToSave; }

   // override some CacheFile methods

   virtual bool Save();

   // implement CacheFile pure virtuals

   virtual String GetFileName() const;
   virtual String GetFileHeader() const;
   virtual int GetFormatVersion() const;

   virtual bool DoLoad(const wxTextFile& file, int version);
   virtual bool DoSave(wxTempFile& file);

private:
   // the names of the folders we have cached status for
   wxArrayString m_folderNames;

   // the data for the folders above
   MfStatusArray m_folderData;

   // the MEventManager cookie
   void *m_evtmanHandle;

   // the dirty flag, i.e. has anything changed since we were last saved?
   bool m_isDirty;

   // this flag is set if our Save() failed and is used to prevent calling it
   // again, and again, and again...
   bool m_hasFailedToSave;

   DECLARE_NO_COPY_CLASS(MfStatusCache)
};

#endif // _MFCACHE_H_
