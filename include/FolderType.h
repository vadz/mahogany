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
   FolderGroup,                // doesn't contain mail, but other folders
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
   MF_FLAGS_ANON = 0x100,           // use anonymous access
   MF_FLAGS_INCOMING = 0x200,       // collect all new mail from it
   MF_FLAGS_UNACCESSIBLE = 0x400,   // folder can't be opened
   MF_FLAGS_MODIFIED = 0x800,       // [essential] folder settings have been
                                    // modified: invalidates "unaccessible"
                                    // flag
   MF_FLAGS_NEWMAILFOLDER = 0x1000, // the central new mail folder
   MF_FLAGS_DONTDELETE    = 0x2000, // forbid deletion of this folder
   MF_FLAGS_KEEPOPEN      = 0x4000, // keep this folder open at all times
   MF_FLAGS_REOPENONPING  = 0x8000  // force a close and re-open on a ping
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
   if ( type == POP || type == IMAP || type == Nntp )
      return true;
   else
      return false;
}

/// is this a folder type for which server field makes sense?
inline bool FolderTypeHasServer(FolderType type)
{
   // currently it's the same as FolderTypeHasUserName(), but it's not
   // impossible that there are some protocols which don't have
   // authentification, yet may have the server name associated with them
   if ( type == POP || type == IMAP || type == Nntp )
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

// ----------------------------------------------------------------------------
// Icon functions: the associated icon for the folder is shown in the folder
// tree control, folder options dialog &c
//
// NB: these functions are implemented for now in wxFolderTree.cpp
// ----------------------------------------------------------------------------

/// get the number of icons from which we may choose folder icon from
extern size_t GetNumberOfFolderIcons();

/// get the name of the folder icon with given index
extern String GetFolderIconName(size_t n);

/// get the icon for this folder or default icon for this folder type (or -1)
int GetFolderIconForDisplay(const class MFolder* folder);

/// get the default icon for folders of this type
int GetDefaultFolderTypeIcon(FolderType folderType);

#endif //  _FOLDERTYPE_H
