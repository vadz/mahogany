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

#include "Mdefaults.h"

#ifndef USE_PCH
#   include "Profile.h"
#   include "MailFolder.h"
#   include "MObject.h"
#   include "FolderType.h"
#endif

// ----------------------------------------------------------------------------
// A class representing a folder used by M. The folders are organized in a tree
// structure with an artificial "root" folder on the top of it. It is the only
// one whose parent is NULL and its type is MFolder::Root.
//
// A folder is identified by its full name which has the form
//                   /folder1/.../folderN/folder.
// The name "" corresponds to a root pseudo-folder which always exists and
// can't be recreated or changed.
//
// This class is refcounted (deriving from MObjectRC) so the usual rules of
// dealing with refcounted objects apply.
//
// NB: this class doesn't know anything about how this folder is actually used
//     (that's what MailFolder class is for), nor anything about its GUI
//     representation (wxFolderTree control), it's just an abstraction to allow
//     the application to conveniently manage (create, delete) it's folders.
//
// Actually, the folder information is stored in a profile, but this class
// hides this implementation detail so in the future we may store folder info
// in a database or on the network and still use the same interface.
// ----------------------------------------------------------------------------
class MFolder : public MObjectRC
{
public:
   // compatibility
   typedef FolderType Type;

   // constants
   enum Flags
   {
      OpenOnStartup   = 0x0001,  // auto open on startup?
      OpenInMainFrame = 0x0002   // or in a separate window?
   };

   // static functions
      // get folder object by name, NULL is returned if it doesn't exist. The
      // root folder is returned if the name is empty.
   static MFolder *Get(const String& fullname);
      // create a new folder of specified type, it's an error to call it with
      // the folder name which already exists (NULL will be returned)
   static MFolder *Create(const String& fullname, Type type);

   // misc accessors
      // the folder name must be unique among its siblings
   virtual String GetName() const = 0;

      // full folder name (has the same form as a full path name)
   virtual wxString GetFullName() const = 0;

      // folder type can't be changed once it's created
   virtual Type GetType() const = 0;

      // folder may have an arbitrary comment associated with it - get it
   virtual String GetComment() const = 0;
      // change the comment
   virtual void SetComment(const String& comment) = 0;

      // get the folder flags (see Flags enum)
   virtual unsigned int GetFlags() const = 0;
      // set the flags (no consitency checks made here!)
   virtual void SetFlags(unsigned int flags) = 0;

   // sub folders access
      // get the number of subfolders
   virtual size_t GetSubfolderCount() const = 0;
      // get the given subfolder by index (or NULL if index is invalid)
   virtual MFolder *GetSubfolder(size_t n) const = 0;
      // get the given subfolder by name (or NULL if not found)
   virtual MFolder *GetSubfolder(const String& name) const = 0;
      // get the parent of this folder (NULL only for the top level one)
   virtual MFolder *GetParent() const = 0;

   // operations
      // create a new subfolder
   virtual MFolder *CreateSubfolder(const String& name, Type type) = 0;
      // delete this folder (does not delete the C++ object!)
   virtual void Delete() = 0;
      // rename this folder: FALSE returned if it failed
   virtual bool Rename(const String& name) = 0;

   MOBJECT_DEBUG(MFolder)

protected:
   // ctor and dtor are private because the user code doesn't create nor deletes
   // these objects directly
   MFolder() { }
   virtual ~MFolder() { }

   // no assignment operator/copy ctor
   MFolder(const MFolder&);
   MFolder& operator=(const MFolder&);
};

// ----------------------------------------------------------------------------
// A simple class which allows to traverse all subfolders of the given folder:
// any useful action should be done in OnVisitFolder() virtual function
// ----------------------------------------------------------------------------

// FIXME this class currently works only with folders stored in the profiles
class MFolderTraversal
{
public:
   /// give the folder from which to start to the ctor
   MFolderTraversal(const MFolder& folderStart);

   /// traverse the tree, OnVisitFolder() will be called for each subfolder
   bool Traverse(bool recurse = TRUE);

   /// return FALSE from here to stop tree traversal, TRUE to continue
   virtual bool OnVisitFolder(const wxString& folderName) = 0;

   /// virtual dtor for the base class
   virtual ~MFolderTraversal() { }

private:
   /// recursive function used by Traverse()
   bool DoTraverse(const wxString& start, bool recurse);

   /// the (full) name of the start folder
   wxString m_folderName;
};

#endif // _MFOLDER_H
