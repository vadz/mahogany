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
/** The UIdArray define is a class which is an integer array. It needs
    to provide a int Count() method to return the number of elements
    and an int operator[int] to access them.
    We use wxArrayInt for this.
    @deffunc UIdArray
*/
class UIdArray;

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
   MF_MH = 6,                    // MH folder (directory/files)

   // these two are still experimental and not always compiled in:
   MF_MFILE = 7,                 // the Mahogany file type
   MF_MDIR = 8,                  // the Mahogany dir type

   MF_PROFILE_OR_FILE,           // profile, if it doesn't work, file
   MF_PROFILE = 10,              // read type etc from profile

   // real folder types
   Inbox = MF_INBOX,     // system inbox
   File  = MF_FILE,      // local file (MBOX format)
   POP   = MF_POP,       // POP3 server
   IMAP  = MF_IMAP,      // IMAP4 server
   Nntp  = MF_NNTP,      // NNTP server
   News  = MF_NEWS,

   // pseudo types
   MF_GROUP = 0x20,            // only used for grouping other folders
   MF_ROOT = 0xfe,             // this is the the special root pseudo-folder

   FolderInvalid = MF_ILLEGAL  // folder not initialized properly
};

// ----------------------------------------------------------------------------
// Flags of a mail folder
// ----------------------------------------------------------------------------

// mask to AND with an int to obtain pure flags
static const int MF_FLAGSMASK = 0xffffff00;

// the flags of a mail folder
enum FolderFlags
{
   MF_FLAGS_DEFAULT       = 0x00000000, // empty flags
   MF_FLAGS_ANON          = 0x00000100, // use anonymous access
   MF_FLAGS_INCOMING      = 0x00000200, // collect all new mail from it
   MF_FLAGS_UNACCESSIBLE  = 0x00000400, // folder can't be opened
   MF_FLAGS_MODIFIED      = 0x00000800, // [essential] folder settings have been
                                        // modified (invalidates "unaccessible"
                                        // flag) since the last attempt to open it
   MF_FLAGS_NEWMAILFOLDER = 0x00001000, // the central new mail folder
   MF_FLAGS_DONTDELETE    = 0x00002000, // forbid deletion of this folder
   MF_FLAGS_KEEPOPEN      = 0x00004000, // keep this folder open at all times
   MF_FLAGS_REOPENONPING  = 0x00008000, // force a close and re-open on a ping
   MF_FLAGS_ISLOCAL       = 0x00010000, // can be accessed even without network
   MF_FLAGS_HIDDEN        = 0x00020000, // don't show in the folder tree
   MF_FLAGS_GROUP         = 0x00040000, // can be only half opened, not opened
   MF_FLAGS_SSLAUTH       = 0x00080000  // use SSL authentication/encryption
};

/** SendMessageCC supports two different protocols:
 */
enum Protocol { Prot_Illegal, Prot_SMTP, Prot_NNTP, Prot_Default = Prot_SMTP };

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


#ifdef USE_SSL
/// is this a folder type for which username/password make sense?
inline bool FolderTypeSupportsSSL(FolderType type)
{
   ASSERT(GetFolderType(type) == type);
   switch(type)
   {
   case MF_POP:
   case MF_IMAP:
   case MF_NNTP:
      return true;
   default:
      return false;
   }
}
#endif

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
   case MF_MFILE:
   case MF_MDIR:
      ; // don't put return false here to avoid VC++ warnings
   }

   return false;
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
inline bool CanHaveSubfolders(FolderType folderType,
                              int flags,
                              FolderType *subtype = NULL)
{
   switch ( folderType )
   {
      case MF_MH:
         if ( subtype )
         {
            // MH folder can only have MH subfolders
            *subtype = MF_MH;
         }
         return TRUE;

      case MF_NEWS:
      case MF_NNTP:
         if ( flags & MF_FLAGS_GROUP )
         {
            if ( subtype )
            {
               *subtype = folderType;
            }

            return TRUE;
         }
         else
         {
            return FALSE;
         }

      case MF_IMAP:
         if ( flags & MF_FLAGS_GROUP )
         {
            if ( subtype )
            {
               *subtype = MF_IMAP;
            }

            return TRUE;
         }
         else
         {
            return FALSE;
         }

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

/// can a folder of this type be (physically) deleted by the user?
inline bool CanDeleteFolderOfType(FolderType folderType)
{
   return folderType == MF_FILE ||
          folderType == MF_MH ||
          // folderType == MF_POP || -- can it?
          folderType == MF_IMAP;
}

/// is it a file or directory local folder
inline bool IsFileOrDirFolder(FolderType folderType)
{
   FolderType ft = GetFolderType(folderType);
   return ft == MF_FILE || ft == MF_MH
      || ft == MF_MFILE || ft == MF_MDIR
      ;
}

/// can the messages in this folder be deleted by user?
inline bool CanDeleteMessagesInFolder(FolderType folderType)
{
   return folderType != MF_NNTP && folderType != MF_NEWS;
}

/// can we copy messages to this folder?
inline bool CanCreateMessagesInFolder(FolderType folderType)
{
   switch ( folderType )
   {
      case MF_NNTP:
      case MF_NEWS:
      case MF_GROUP:
      case MF_ROOT:
         return false;

      case MF_ILLEGAL:
      case MF_PROFILE_OR_FILE:
      case MF_PROFILE:
         FAIL_MSG("this is not supposed to be called for this type");
         // fall through nevertheless

         // don't use "default:" - like this, the compiler will warn us if we
         // add a new type to the FolderType enum and forget to add it here
      case MF_INBOX:
      case MF_FILE:
      case MF_MH:
      case MF_IMAP:
      case MF_POP:
      case MF_MFILE:
      case MF_MDIR:
         ; // don't put return false here to avoid VC++ warnings
   }

   return true;
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
