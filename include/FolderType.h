///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   FolderType.h - constants for folder types
// Purpose:     collect all folder types and flags in one place
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.02.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef  _FOLDERTYPE_H
#define  _FOLDERTYPE_H

// ----------------------------------------------------------------------------
// Type of a mail folder
// ----------------------------------------------------------------------------

// use this with AND to obtain pure type from an int which also contains
// the folder flags (see FolderFlags enum)
static const int MF_TYPEMASK = 0x00ff;

enum FolderType
{
   // the MF_XXX constants have the same values as c-client folder types
   MF_ILLEGAL = 0xff,            // illegal type - cannot use -1 because of bitmask
   MF_INBOX = 0,                 // system inbox
   MF_FILE = 1,                  // mbox file
   MF_POP = 2,                   // pop3
   MF_IMAP = 3,                  // imap
   MF_NNTP = 4,                  // newsgroup
   MF_NEWS = 5,                  // newsgroup in local newsspool
   MF_PROFILE = 10,              // read type etc from profile
   MF_MH,                        // MH folder (directory/files)
   MF_PROFILE_OR_FILE,           // profile, if it doesn't work, file

   // real folder types
   Inbox = MF_INBOX,     // system inbox
   File  = MF_FILE,      // local file (MBOX format)
   POP   = MF_POP,       // POP3 server
   IMAP  = MF_IMAP,      // IMAP4 server
   Nntp  = MF_NNTP,      // NNTP server
   News  = MF_NEWS,

   // pseudo types
   FolderInvalid = MF_ILLEGAL, // folder not initialized properly
   FolderRoot = 999            // this is the the special pseudo-folder
};

// ----------------------------------------------------------------------------
// Flags of a mail folder
// ----------------------------------------------------------------------------

// mask to AND with an int to obtain pure flags
static const int MF_FLAGSMASK = 0xff00;

// the flags of a mail folder
enum FolderFlags
{
   MF_FLAGS_ANON = 0x100,      // use anonymous access
   MF_FLAGS_INCOMING = 0x200   // collect all new mail from it
};

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/// get the type from an int
inline FolderType GetFolderType(int typeAndFlags)
{
   return (FolderType)(typeAndFlags & MF_TYPEMASK);
}

/// get the flags from an int
inline static int GetFolderFlags(int typeAndFlags)
{
   return typeAndFlags & MF_FLAGSMASK;
}

/// is this a folder type for which username/password make sense?
inline bool FolderTypeHasUserName(FolderType type)
{
   if ( type == POP || type == IMAP || type == News )
      return true;
   else
      return false;
}

/// combine type and flags into one int
inline int CombineFolderTypeAndFlags(FolderType type, int flags)
{
   ASSERT_MSG( !(flags & MF_TYPEMASK), "flags shouldn't contain type" );

   return type | flags;
}

#endif //  _FOLDERTYPE_H
