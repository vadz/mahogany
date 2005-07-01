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

#include <wx/setup.h>   // for wxSIZE_T_IS_ULONG/UINT

// ----------------------------------------------------------------------------
// Type of a mail folder
// ----------------------------------------------------------------------------

// use this with AND to obtain pure type from an int which also contains
// the folder flags (see FolderFlags enum)
#define MF_TYPEMASK 0x00ff

enum MFolderType
{
   // the MF_XXX constants have the same values as c-client folder types
   MF_ILLEGAL = 0xff,            // illegal (don't use -1 because of bitmask)

   // real folder types
   MF_INBOX = 0,                 // system inbox
   MF_FILE = 1,                  // mbox file
   MF_POP = 2,                   // pop3
   MF_IMAP = 3,                  // imap
   MF_NNTP = 4,                  // newsgroup
   MF_NEWS = 5,                  // newsgroup in local newsspool
   MF_MH = 6,                    // MH folder (directory/files)

   // these two are experimental
   MF_MFILE = 7,                 // the Mahogany file type
   MF_MDIR = 8,                  // the Mahogany dir type

   MF_PROFILE = 10,              // read type etc from profile
   MF_VIRTUAL,                   // virtual folder

   // pseudo types
   MF_GROUP = 0x20,              // only used for grouping other folders
   MF_ROOT = 0xfe                // this is the special root pseudo-folder
};

/// supported formats for the local file mailboxes (hence MH not counted)
enum FileMailboxFormat
{
   /// the default format, must be 0!
   FileMbox_MBX,

   /// traditional Unix one
   FileMbox_MBOX,

   /// SCO default format
   FileMbox_MMDF,

   /// MM-compatible fast format
   FileMbox_TNX,

   /// end of enum marker
   FileMbox_Max
};

/// kinds of SSL support
///
/// NB: these values shouldn't be changed, they're stored in config
enum SSLSupport
{
   /// never use TLS nor SSL
   SSLSupport_None,

   /// use TLS if available, fall back to plain text if not
   SSLSupport_TLSIfAvailable,

   /// force using TLS, fail if it's not available
   SSLSupport_TLS,

   /// use SSL
   SSLSupport_SSL,

   /// end of enum marker
   SSLSupport_Max
};

/// accept unsigned SSL certificates?
///
/// NB: these values must be false and true
enum SSLCert
{
   /// don't trust self-signed certificates
   SSLCert_SignedOnly,

   /// accept self-signed certificates
   SSLCert_AcceptUnsigned
};

// ----------------------------------------------------------------------------
// Flags of a mail folder
// ----------------------------------------------------------------------------

// mask to AND with an int to obtain pure flags
static const int MF_FLAGSMASK = 0xffffff00;

// the flags of a mail folder
//
// Notice the difference between MF_FLAGS_UNACCESSIBLE, MF_FLAGS_GROUP and
// MF_FLAGS_NOSELECT: UNACCESSIBLE means that, although the folder is just a
// "normal" folder, we failed to open it when we tried to do it the last time
// (for example, because of a network problem). OTOH, MF_FLAGS_NOSELECT means
// that the folder can't be opened at all: for example, it is a NNTP folder
// "comp.programming" which is really a news hierarchy and not a newsgroup.
// The name comes from cclient which marks such folders with \Noselect flag.
//
// Finally, MF_FLAGS_GROUP now doesn't anything at all about whether the
// folder can or can't be opened: it was used instead of MF_FLAGS_NOSELECT
// before as for some kinds of folders (notable NNTP) they are (usually, but
// even here not always) related, however for the IMAP folders they are really
// independent and MF_FLAGS_GROUP just means that this folder may have sub
// folders. It is always TRUE for MH folders and always false for MBOX and
// POP3. NNTP and IMAP folders may have it or not.
enum FolderFlags
{
   MF_FLAGS_DEFAULT       = 0x00000000, // empty flags
   MF_FLAGS_ANON          = 0x00000100, // use anonymous access
   MF_FLAGS_INCOMING      = 0x00000200, // collect all new mail from it
   MF_FLAGS_UNACCESSIBLE  = 0x00000400, // folder couldn't be opened when we
                                        // tried the last time
   MF_FLAGS_MODIFIED      = 0x00000800, // [essential] folder settings have been
                                        // modified (invalidates "unaccessible"
                                        // flag) since the last attempt to open it
   MF_FLAGS_NEWMAILFOLDER = 0x00001000, // the central new mail folder
   MF_FLAGS_DONTDELETE    = 0x00002000, // forbid deletion of this folder
   MF_FLAGS_KEEPOPEN      = 0x00004000, // keep this folder open at all times
   // MF_FLAGS_REOPENONPING  = 0x00008000, // -- value unused any more --
   MF_FLAGS_ISLOCAL       = 0x00010000, // can be accessed even without network
   MF_FLAGS_HIDDEN        = 0x00020000, // don't show in the folder tree
   MF_FLAGS_GROUP         = 0x00040000, // contains subfolders
   // MF_FLAGS_SSLAUTH       = 0x00080000, // -- value unused any more --
   MF_FLAGS_NOSELECT      = 0x00100000, // folder can't be opened
   // MF_FLAGS_SSLUNSIGNED   = 0x00200000, // -- value unused any more --
   MF_FLAGS_MONITOR       = 0x00400000, // poll this folder periodically
   MF_FLAGS_TEMPORARY     = 0x00800000  // temp folder: delete file on close
};

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
// Message::Send() and SendMessage flags
// ----------------------------------------------------------------------------

/**
   All supported protocols for sending mail
 */
enum Protocol
{
   /// invalid value
   Prot_Illegal,

   /// use SMTP
   Prot_SMTP,

   /// use NNTP
   Prot_NNTP,

   /// use local sendmail agent
   Prot_Sendmail,

   /// default mail delivery mode, i.e. either Prot_SMTP or Prot_Sendmail
   Prot_Default
};

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/// get the type from an int
inline MFolderType GetFolderType(int typeAndFlags)
{
   return (MFolderType)(typeAndFlags & MF_TYPEMASK);
}

/// get the flags from an int
inline static int GetFolderFlags(int typeAndFlags)
{
   return typeAndFlags & MF_FLAGSMASK;
}

/// is this a folder type for which we [may] need authentication?
inline bool FolderTypeHasAuth(MFolderType type)
{
   ASSERT(GetFolderType(type) == type);

   return type == MF_POP || type == MF_IMAP || type == MF_NNTP;
}

#ifdef USE_SSL

/// can we use SSL for folders of this type?
inline bool FolderTypeSupportsSSL(MFolderType type)
{
   return FolderTypeHasAuth(type);
}

#endif // USE_SSL

/// is this a folder type for which username/password make sense?
inline bool FolderTypeHasUserName(MFolderType type)
{
   return FolderTypeHasAuth(type) || type == MF_GROUP;
}

/// is this a folder type for which server field makes sense?
inline bool FolderTypeHasServer(MFolderType type)
{
   // currently it's the same as FolderTypeHasUserName(), but it's not
   // impossible that there are some protocols which don't have
   // authentication, yet may have the server name associated with them -
   // this will have to be changed then
   return FolderTypeHasUserName(type);
}

/// combine type and flags into one int
inline int CombineFolderTypeAndFlags(MFolderType type, int flags)
{
   ASSERT_MSG( !(flags & MF_TYPEMASK), _T("flags shouldn't contain type") );

   return type | flags;
}

/// is this folder a local, file based one?
inline bool IsLocalQuickFolder(MFolderType type)
{
   return type == MF_FILE || type == MF_MH || type == MF_NEWS;
}

/**
   Can this folder contain other subfolders and, if so, of which type?

   @param folderType the type of this folder
   @param flagsof this folder
   @param subtype if non NULL and function returns true, filled with the type
                  of our subfolders
   @param true if folders of this type can have subfolders, false otherwise
 */
extern bool CanHaveSubfolders(MFolderType folderType,
                              int flags,
                              MFolderType *subtype = NULL);

/// can a folder of this type be (physically) deleted by the user?
inline bool CanDeleteFolderOfType(MFolderType folderType)
{
   ASSERT(GetFolderType(folderType) == folderType);

   return folderType == MF_FILE || folderType == MF_MH || folderType == MF_IMAP;
}

/// is it a file or directory local folder
inline bool IsFileOrDirFolder(MFolderType folderType)
{
   MFolderType ft = GetFolderType(folderType);

   return ft == MF_FILE || ft == MF_MH || ft == MF_MFILE || ft == MF_MDIR;
}

/// can the messages in this folder be deleted by user?
inline bool CanDeleteMessagesInFolder(MFolderType folderType)
{
   return folderType != MF_NNTP && folderType != MF_NEWS;
}

/// can we copy messages to this folder?
extern bool CanCreateMessagesInFolder(MFolderType folderType);

extern bool CanOpenFolder(MFolderType folderType, int folderFlags);

/// does this folder require network to be up?
inline bool FolderNeedsNetwork(MFolderType type, int flags)
{
   return FolderTypeHasAuth(type) && !(flags & MF_FLAGS_ISLOCAL);
}

/**
   @name Icon functions.
   
   The associated icon for the folder is shown in the folder tree control,
   folder options dialog &c.

   NB: these functions are implemented for now in wxFolderTree.cpp
 */
//@{

/// get the number of icons from which we may choose folder icon from
extern size_t GetNumberOfFolderIcons();

/// get the name of the folder icon with given index
extern String GetFolderIconName(size_t n);

/// get the icon for this folder or default icon for this folder type (or -1)
int GetFolderIconForDisplay(const class MFolder* folder);

/// get the default icon for folders of this type
int GetDefaultFolderTypeIcon(MFolderType folderType);

//@}

#endif //  _FOLDERTYPE_H
