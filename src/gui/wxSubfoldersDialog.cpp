///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   wxSubfoldersDialog.cpp - implementation of functions from
//              MFolderDialogs.h
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"
#  include "strutil.h"
#endif

#include "MFolder.h"

#include "ASMailFolder.h"
#include "MailFolder.h"
#include "MailFolderCC.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// public interface
// ----------------------------------------------------------------------------

bool ShowFolderSubfoldersDialog(MFolder *folder, wxWindow *parent)
{
   Profile_obj profile(folder->GetFullName());
   CHECK( profile, FALSE, "can't create profile" );

   String name;
   MailFolderCC::CanonicalizeMHPath(&name);
   name += READ_CONFIG(profile, MP_FOLDER_PATH);
   CHECK( !!name, FALSE, "empty folder path?" );

   while ( name.Last() == '/' )
   {
      // shouldn't end with backslash because otherwise mail_list won't return
      // the root folder itself (but we want it)
      name.Truncate(name.Length() - 1);
   }

   MailFolder *mfDummy = MailFolder::OpenFolder(MF_FILE, "");
   ASMailFolder *asmf = ASMailFolder::Create(mfDummy);

   UserData ud = NULL; // what should this be?
   Ticket ticket = asmf->ListFolders
                         (
                          "",       // host
                          MF_MH,    // folder type
                          name,     // start folder name
                          "*",      // everything recursively
                          FALSE,    // subscribed only?
                          "",       // reference (what's this?)
                          ud
                         );

   // how can I build this folderListing now?

#if 0
   size_t nEntries = folderListing->CountEntries();
   fprintf(stderr, "Folder '%s' has %u subfolders\n",
           folder->GetFullName().c_str(), nEntries);

   FolderListing::iterator i;
   for ( const FolderListingEntry *entry = folderListing->GetFirstEntry(i);
         entry;
         entry = folderListing->GetNextEntry(i) )
   {
      fprintf(stderr, "\tSubfolder: %s\n", entry->GetName().c_str());
   }
#endif // 0

   mfDummy->DecRef();
   asmf->DecRef();

   return FALSE;
}

