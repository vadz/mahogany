//-*- c++ -*-//////////////////////////////////////////////////////////////////
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

#include "Mcommon.h"
#include <limits.h>

// ----------------------------------------------------------------------------
// Type of a mail folder
// ----------------------------------------------------------------------------
/** The INTARRAY define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.
    We use wxArrayInt for this.
*/
#define INTARRAY wxArrayInt
class INTARRAY;

/// A Type used for message UIds
typedef unsigned long UIdType;
/// An illegal, never happening UId number:
#define UID_ILLEGAL   ULONG_MAX

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
   MF_MH = 6,                    // MH folder (directory/files)
   MF_PROFILE_OR_FILE,           // profile, if it doesn't work, file

   // real folder types
   Inbox = MF_INBOX,     // system inbox
   File  = MF_FILE,      // local file (MBOX format)
   POP   = MF_POP,       // POP3 server
   IMAP  = MF_IMAP,      // IMAP4 server
   Nntp  = MF_NNTP,      // NNTP server
   News  = MF_NEWS,

   // pseudo types
   MF_GROUP = 50,
   FolderGroup = MF_GROUP,     // doesn't contain mail, but (any) other folders
   MF_GROUP_NEWS,              // a news hierarchy
   MF_GROUP_IMAP,              // a directory on an IMAP server

   FolderRoot,                 // this is the the special root pseudo-folder
   MF_ROOT = FolderRoot,

   FolderInvalid = MF_ILLEGAL  // folder not initialized properly
};

// ----------------------------------------------------------------------------
// Flags of a mail folder
// ----------------------------------------------------------------------------

// mask to AND with an int to obtain pure flags
static const int MF_FLAGSMASK = 0xff00;

// the flags of a mail folder
enum FolderFlags
{
   MF_FLAGS_ANON          = 0x0100, // use anonymous access
   MF_FLAGS_INCOMING      = 0x0200, // collect all new mail from it
   MF_FLAGS_UNACCESSIBLE  = 0x0400, // folder can't be opened
   MF_FLAGS_MODIFIED      = 0x0800, // [essential] folder settings have been
                                    // modified (invalidates "unaccessible"
                                    // flag) since the last attempt to open it
   MF_FLAGS_NEWMAILFOLDER = 0x1000, // the central new mail folder
   MF_FLAGS_DONTDELETE    = 0x2000, // forbid deletion of this folder
   MF_FLAGS_KEEPOPEN      = 0x4000, // keep this folder open at all times
   MF_FLAGS_REOPENONPING  = 0x8000  // force a close and re-open on a ping
};

/** SendMessageCC supports two different protocols:
 */
enum Protocol { Prot_SMTP, Prot_NNTP, Prot_Default = Prot_SMTP };
   

// ----------------------------------------------------------------------------
// For asynchronous operations:
// ----------------------------------------------------------------------------

/** Each operation returns a unique number, to identify it. */
typedef int Ticket;

/// A ticket number that never appears.
#define ILLEGAL_TICKET   -1

/** Each operation can carry some user data. */
typedef void * UserData;

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
   ASSERT(GetFolderType(type) == type);

   switch ( type )
   {
      case MF_POP:
      case MF_IMAP:
      case MF_NNTP:
      case MF_GROUP:
      case MF_GROUP_IMAP:
      case MF_GROUP_NEWS:
         return true;

      // don't use "default:" - like this, the compiler will warn us if we add
      // a new type to the FolderType enum and forget to add it here
      case MF_ROOT:
      case MF_ILLEGAL:
      case MF_PROFILE_OR_FILE:
      case MF_PROFILE:
         FAIL_MSG("this is not supposed to be called for this type");
         // fall through nevertheless

      case MF_INBOX:
      case MF_FILE:
      case MF_MH:
      case MF_NEWS:
         ;
   }

   return false;
}

/// is this folder a group?
inline bool FolderTypeIsGroup(FolderType type)
{
   return
      type == MF_GROUP 
      || type == MF_GROUP_NEWS 
      || type == MF_GROUP_IMAP ;

}
/// is this a folder type for which server field makes sense?
inline bool FolderTypeHasServer(FolderType type)
{
   // currently it's the same as FolderTypeHasUserName(), but it's not
   // impossible that there are some protocols which don't have
   // authentification, yet may have the server name associated with them -
   // this will have to be changed then
   return FolderTypeHasUserName(type);
}

/// combine type and flags into one int
inline int CombineFolderTypeAndFlags(FolderType type, int flags)
{
   ASSERT_MSG( !(flags & MF_TYPEMASK), "flags shouldn't contain type" );

   return type | flags;
}

/// is this folder a local, file based one?
inline bool IsLocalQuickFolder(FolderType type)
{
   return type == MF_FILE || type == MF_MH || type == MF_NEWS;
}

/// can this folder contain other subfolders? if so, of which type?
inline bool CanHaveSubfolders(FolderType type, FolderType *subtype = NULL)
{
   switch ( type )
   {
      case MF_MH:
         if ( subtype )
         {
            // MH folder can only have MH subfolders
            *subtype = MF_MH;
         }
         return TRUE;

      case MF_GROUP_NEWS:
         if ( subtype )
         {
            // FIXME they may also contain MF_NEWS
            *subtype = MF_NNTP;
         }
         return TRUE;

      case MF_GROUP_IMAP:
         if ( subtype )
         {
            *subtype = MF_IMAP;
         }
         return TRUE;

      case MF_GROUP:
      case MF_ROOT:
         if ( subtype )
         {
            // can contain any subfolders at all
            *subtype = MF_ILLEGAL;
         }
         return TRUE;

      default:
         return FALSE;
   }
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
