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

/** A class representing a folder used by M. The folders are organized in a tree
    structure with an artificial "root" folder on the top of it. It is the only
    one whose parent is NULL and its type is MFolder::Root.

    A folder is identified by its full name which has the form
    /folder1/.../folderN/folder.
    The name "" corresponds to a root pseudo-folder which always exists and
    can't be recreated or changed.

    This class is refcounted (deriving from MObjectRC) so the usual rules of
    dealing with refcounted objects apply.

    NB: this class doesn't know anything about how this folder is actually used
        (that's what MailFolder class is for), nor anything about its GUI
        representation (wxFolderTree control), it's just an abstraction to allow
        the application to conveniently manage (create, delete) it's folders.

    Actually, the folder information is stored in a profile, but this class
    hides this implementation detail so in the future we may store folder info
    in a database or on the network and still use the same interface.
*/
class MFolder : public MObjectRC
{
public:
   /// constants
   enum Flags
   {
      OpenOnStartup   = 0x0001,  /// auto open on startup?
      OpenInMainFrame = 0x0002   /// or in a separate window?
   };

   /**@name static functions */
   //@{
   /** get folder object by name, NULL is returned if it doesn't exist. The
       root folder is returned if the name is empty.
   */
   static MFolder *Get(const String& fullname);
   /** create a new folder of specified type, it's an error to call it with
       the folder name which already exists (NULL will be returned)
   */
   static MFolder *Create(const String& fullname, FolderType type);
   /** create a temp object containing folder data
   */
   static MFolder *CreateTemp(const String& fullname,
                              FolderType type,
                              int flags,
                              const String& path,
                              const String& server,
                              const String& login,
                              const String& password);
   //@}

   /**@name misc accessors */
   //@{
   /** get the folder path (i.e. something by which it's identified by the
       mail subsystem)
   */
   virtual String GetPath() const = 0;
   /// get the server for the folder:
   virtual String GetServer() const = 0;
   /// get the login for the folder:
   virtual String GetLogin() const = 0;
   /// get the password for the folder:
   virtual String GetPassword() const = 0;

   /// the folder name must be unique among its siblings
   virtual String GetName() const = 0;

   /// full folder name (has the same form as a full path name)
   virtual wxString GetFullName() const = 0;

   /// folder type can't be changed once it's created
   virtual FolderType GetType() const = 0;

   /// Does the folder need a working network to be accessed?
   virtual bool NeedsNetwork(void) const = 0;

      /// the icon index for this folder or -1 if there is no specific icon
      /// associated to it (the icon index should be used to pass it to
      /// GetFolderIconName())
   virtual int GetIcon() const = 0;
      /// set the icon
   virtual void SetIcon(int icon) = 0;

      /// folder may have an arbitrary comment associated with it - get it
   virtual String GetComment() const = 0;
      /// change the comment
   virtual void SetComment(const String& comment) = 0;
   //@}
   /**@name flags */
   //@{
       /// get the folder flags (see Flags enum)
   virtual int GetFlags() const = 0;
      /// set the flags (this replaces the old value of flags)
   virtual void SetFlags(int flags) = 0;

      /// set the specified flags (this adds new flags to the old value)
   void AddFlags(int flags) { SetFlags(GetFlags() | flags); }
      /// clear the specified flags
   void ResetFlags(int flags) { SetFlags(GetFlags() & ~flags); }
   //@}

   /**@name flags */
   //@{
      /// get the array of filters to use for this folder
   virtual wxArrayString GetFilters() const = 0;
      /// set the filters (this replaces the old value)
   virtual void SetFilters(const wxArrayString& filters) = 0;

      /// adds a filter without changing the other ones
   virtual void AddFilter(const String& filter) = 0;
      /// remove a filter without changing the other ones
   virtual void RemoveFilter(const String& filter) = 0;
   //@}

   /**@name sub folders access */
   //@{
   /// get the number of subfolders
   virtual size_t GetSubfolderCount() const = 0;
      /// get the given subfolder by index (or NULL if index is invalid)
   virtual MFolder *GetSubfolder(size_t n) const = 0;
      /// get the given subfolder by name (or NULL if not found)
   virtual MFolder *GetSubfolder(const String& name) const = 0;
      /// get the parent of this folder (NULL only for the top level one)
   virtual MFolder *GetParent() const = 0;
   //@}

   /**@name operations */
   //@{
   /// create a new subfolder
   virtual MFolder *CreateSubfolder(const String& name, FolderType type) = 0;
      /// delete this folder (does not delete the C++ object!)
   virtual void Delete() = 0;
      /// rename this folder: FALSE returned if it failed
   virtual bool Rename(const String& name) = 0;
   //@}

   MOBJECT_DEBUG(MFolder)

protected:
   /// ctor and dtor are private because the user code doesn't create nor deletes
   /// these objects directly
   MFolder() { }
   virtual ~MFolder() { }

   /// no assignment operator/copy ctor
   MFolder(const MFolder&);
   MFolder& operator=(const MFolder&);
};

// ----------------------------------------------------------------------------
// A smart pointer to MFolder
// ----------------------------------------------------------------------------

class MFolder_obj
{
public:
   // ctor & dtor
      // creates a new folder
   MFolder_obj(const String& name) { Init(name); }
      // creates a folder corresponding to the profile
   MFolder_obj(const Profile *profile)
      {
         String name;
         if ( profile->GetName().
               StartsWith(String(M_PROFILE_CONFIG_SECTION) + '/', &name) )
         {
            Init(name);
         }
         else
         {
            wxFAIL_MSG( "attempt to create MFolder from non folder profile" );

            m_folder = NULL;
         }
      }
      // takes ownership of the existing object
   MFolder_obj(MFolder *folder) { m_folder = folder; }
      // release folder
  ~MFolder_obj() { SafeDecRef(m_folder); }

   // provide access to the real thing via operator->
   MFolder *operator->() const { return m_folder; }

   // implicit conversion to the real pointer (dangerous, but necessary for
   // backwards compatibility)
   operator MFolder *() const { return m_folder; }

   // implicit conversion to bool to allow testing like in "if ( folder )"
   operator bool() const { return m_folder != NULL; }

   // explicitly test if object is valid
   bool IsOk() const { return m_folder != NULL; }

private:
   // no copy ctor/assignment operator
   MFolder_obj(const MFolder_obj&);
   MFolder_obj& operator=(const MFolder_obj&);

   // create folder by name
   void Init(const String& name) { m_folder = MFolder::Get(name); }

   MFolder *m_folder;
};

// ----------------------------------------------------------------------------
// A simple class which allows to traverse all subfolders of the given folder:
// any useful action should be done in OnVisitFolder() virtual function
// ----------------------------------------------------------------------------

// NB: this class currently works only with folders stored in the profiles
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

// ----------------------------------------------------------------------------
// functions to work with folder tree
// ----------------------------------------------------------------------------

/**
   Creates a new folder in the folder tree under parent (if given or at the
   top level otherwise) with the given name (this is what is shown to the
   user) and the specified type, flags and path (folder properties). Unless
   notify parameter is FALSE, a notification message about the new folder
   creation will be sent via MEventManager.
 */
extern MFolder *CreateFolderTreeEntry(MFolder *parent,
                                      const String& name,
                                      FolderType folderType,
                                      long folderFlags,
                                      const String& path,
                                      bool notify = TRUE);
#endif // _MFOLDER_H
