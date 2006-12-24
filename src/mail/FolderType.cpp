///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/FolderType.cpp: functions working with folder types
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-07-01 (extracted from FolderType.h)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2005 Vadim Zeitlin <vadim@zeitlins.org>
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
#endif // USE_PCH

#include "FolderType.h"

// ============================================================================
// implementation
// ============================================================================

bool CanHaveSubfolders(MFolderType folderType,
                       int flags,
                       MFolderType *subtype)
{
   switch ( folderType )
   {
      case MF_FILE:  // we emulate subfolders support for file folders
      case MF_MH:
         if ( subtype )
            *subtype = folderType;
         return true;

      case MF_NEWS:
      case MF_NNTP:
      case MF_IMAP:
         if ( !(flags & MF_FLAGS_GROUP) )
            return false;

         if ( subtype )
            *subtype = folderType;

         return true;

      case MF_GROUP:
      case MF_ROOT:
         if ( subtype )
         {
            // can contain any subfolders at all
            *subtype = MF_ILLEGAL;
         }
         return true;

      default:
         FAIL_MSG( _T("unexpected folder type") );
         // fall through

      case MF_INBOX:
      case MF_POP:
      case MF_VIRTUAL:     // currently doesn't implement "list subfolders"
         return false;
   }
}

bool CanCreateMessagesInFolder(MFolderType folderType)
{
   ASSERT(GetFolderType(folderType) == folderType);

   switch ( folderType )
   {
      case MF_NNTP:
      case MF_NEWS:
      case MF_GROUP:
      case MF_POP:
      case MF_ROOT:
      case MF_VIRTUAL:     // so far we don't support this, maybe later
         return false;

      case MF_ILLEGAL:
      case MF_PROFILE:
         FAIL_MSG(_T("this is not supposed to be called for this type"));
         // fall through nevertheless

         // don't use "default:" - like this, the compiler will warn us if we
         // add a new type to the MFolderType enum and forget to add it here
      case MF_INBOX:
      case MF_FILE:
      case MF_MH:
      case MF_IMAP:
      case MF_MFILE:
      case MF_MDIR:
         ; // don't put return false here to avoid VC++ warnings
   }

   return true;
}

bool CanOpenFolder(MFolderType folderType, int folderFlags)
{
   ASSERT(GetFolderType(folderType) == folderType);

   switch ( folderType )
   {
      case MF_NNTP:
      case MF_NEWS:
      case MF_IMAP:
         if ( !(folderFlags & MF_FLAGS_NOSELECT) )
         {
            // can open
            break;
         }
         //else: fall through

      case MF_GROUP:
      case MF_ROOT:
         return false;

      case MF_ILLEGAL:
      case MF_PROFILE:
         FAIL_MSG(_T("this is not supposed to be called for this type"));
         // fall through nevertheless

         // don't use "default:" - like this, the compiler will warn us if we
         // add a new type to the MFolderType enum and forget to add it here
      case MF_INBOX:
      case MF_FILE:
      case MF_MH:
      case MF_POP:
      case MF_MFILE:
      case MF_MDIR:
      case MF_VIRTUAL:
         ; // don't put return here to avoid VC++ warnings
   }

   return true;
}

