///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFolder.h - non GUI and non mail folder related classes
// Purpose:     manage all folders used by the application
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18.09.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef  _MFOLDER_H
#define  _MFOLDER_H

#ifdef __GNUG__
#   pragma interface "MFolder.h"
#endif

#include <wx/dynarray.h>

// ----------------------------------------------------------------------------
// declare an array of our folders
// ----------------------------------------------------------------------------
class MFolder;
WX_DEFINE_ARRAY(MFolder *, ArrayMFolders);

// ----------------------------------------------------------------------------
// global functions to get/free root pseudo-folder object
// ----------------------------------------------------------------------------

// get the root pseudo-folder object: this function will create it and all its
// subfolders (i.e. all folders).
extern MFolder *GetRootFolder();

// must be called at the end of the program (will do nothing if GetRootFolder
// was never called, otherwise will delete the root folder object)
extern void DeleteRootFolder();

// ----------------------------------------------------------------------------
// A class representing a folder used by M. The folders are organized in a tree
// structure with an artificial "root" folder on the top of it. It is the only
// one whose parent is NULL and its type is MFolder::Root. This object is
// created on program startup and loads all existing folders from disk, so after
// its creation they're all accessible via GetSubfolder() method.
//
// The objects of these class are not ref counted, but should be never deleted
// nevertheless: their lifetime is the lifetime of the program and they're
// deleted when the root pseudo-folder is, i.e. at the very end of the program
// execution.
//
// NB: this class doesn't know anything about how this folder is actually used
//     (that's what MailFolder class is for), nor anything about its GUI
//     representation (wxFolderTree control), it's just an abstraction to allow
//     the application to conveniently manage (create, delete) it's folders.
// ----------------------------------------------------------------------------
class MFolder
{
public:
   // constants
   enum Type
   {
      // real folder types
      Inbox,     // system inbox
      File,      // local file (MBOX format)
      POP,       // POP3 server
      IMAP,      // IMAP4 server
      News,      // NNTP server

      // pseudo types
      Invalid,   // folder not initialized properly
      Root,      // this is the the special pseudo-folder
      Max        // end of enum marker
   };

   enum Flags
   {
      OpenOnStartup   = 0x0001,  // auto open on startup?
      OpenInMainFrame = 0x0002   // or in a separate window?
   };

   // misc accessors
      // the folder name must be unique among its siblings
   const String& GetName() const { return m_name; }

      // full folder name has the same form as a full path name
   wxString GetFullName() const;

      // folder type can't be changed once it's created
   Type GetType() const { return m_type; }

      // folder may have an arbitrary comment associated with it - get it
   const String& GetComment() const { return m_comment; }
      // chaneg the comment
   void SetComment(const String& comment) { m_comment = comment; }

      // get the folder flags (see Flags enum)
   unsigned int GetFlags() const { return m_flags; }
      // set the flags (no consitency checks made here!)
   void SetFlags(unsigned int flags) { m_flags = flags; }

   // sub folders access
      // get the number of subfolders
   size_t GetSubfolderCount() const { return m_subfolders.Count(); }
      // get the given subfolder by index
   MFolder *GetSubfolder(size_t n) const { return m_subfolders[n]; }
      // get the index of given subfolder by name (or -1 if not found)
   int GetSubfolder(const String& name) const;
      // get the parent of this folder (NULL only for the top level one)
   MFolder *GetParent() const { return m_parent; }

   // operations
      // create a new subfolder
   MFolder *CreateSubfolder(const String& name, Type type);
      // delete this folder (can't use this pointer after this!)
   void Delete();
      // rename this folder
   void Rename(const String& name);

protected:
   // ctor and dtor are private because the user code doesn't create nor deletes
   // these objects directly
   MFolder(MFolder *parent, const String& name, Type type = Invalid);
   virtual ~MFolder();

   // no assignment operator/copy ctor
   MFolder(const MFolder&);
   MFolder& operator=(const MFolder&);

   // notification for the derived classes
      // called just before we're deleted in Delete()
   virtual void OnDelete() { }
      // called just before Rename() changes the m_name variable
   virtual void OnRename(const String& /* name */) { }

   // folder characterstics
   String          m_name,
                   m_comment;
   Type            m_type;
   unsigned int    m_flags;

   // hierarchical structure
   MFolder        *m_parent;
   ArrayMFolders   m_subfolders;
};

#endif // _MFOLDER_H
