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

#ifdef __GNUG__
#   pragma interface "MFCache.h"
#endif

#include "MailFolder.h"         // for MailFolderStatus

#include <wx/dynarray.h>

WX_DEFINE_ARRAY(MailFolderStatus *, MfStatusArray);

// ----------------------------------------------------------------------------
// MfStatusCache: this class caches MailFolderStatus info for each folder,
// this allows to preserve the status across sessions and avoids unnecessary
// rebuilding of the entire folder listing if we're only interested in its
// status (which we often are, for example when updating the status in the
// tree)
// ----------------------------------------------------------------------------

class MfStatusCache
{
public:
   // this is a singleton class and this function is the only way to access it
   static MfStatusCache *Create();

   // but it can be deleted directly
   ~MfStatusCache();

   // query the status info: return true and fill the provided pointer with
   // info if we have it, return false otherwise
   bool GetStatus(const String& folderName, MailFolderStatus *status);

   // update the status info 
   void UpdateStatus(const String& folderName, const MailFolderStatus& status);

protected:
   // protected ctor for CreateStatusCache()
   MfStatusCache();

   // get the full cache file name
   String GetFileName() const;

   // load cache file
   bool Load(const String& filename);

   // save cache file
   bool Save(const String& filename);

private:
   // the names of the folders we have cached status for
   wxArrayString m_folderNames;

   // the data for the folders above
   MfStatusArray m_folderData;
};

#endif // _MFCACHE_H_
