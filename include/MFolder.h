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

#ifndef  USE_PCH
#  include "FolderType.h"
#  include "Profile.h"
#endif // USE_PCH

#include "MObject.h"         // for MObjectRC

#include "lists.h"

class Profile;

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


    There is also another kind of folders, which don't appear in the tree at
    all: they're called "temporary" folders because they're not stored in
    profile and so only exist during one session. Temporary folders are used
    for things like showing an attachment of type message/rfc822 in a
    standalone viewer (we create a temp mail folder for it then and so need an
    associated MFolder) or for virtual folders such as the search results one.
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

   /** @name static functions

     These methods are used to access an existing MFolder object by name (only
     returns "permanent" folders) or creating a new MFolder object -- Create()
     creates a new permanent MFolder while CreateTemp() a new temporary one
    */
   //@{

   /**
     get folder object by name, NULL is returned if it doesn't exist. The
     root folder is returned if the name is empty.

     @param fullname the full folder name without the leading slash
     @return folder (should be DecRef()'d by caller) or NULL on error
   */
   static MFolder *Get(const String& fullname);

   /**
     create a new folder of specified type, it's an error to call it with
     the folder name which already exists (NULL will be returned)

     The parameter tryCreateOnOpen tells us to try to create the physical
     folder (e.g. file, IMAP mailbox, ...) when the folder is opened for the
     first time. It is harmless to specify this parameter even for the folder
     types which can't be created (e.g. NNTP or POP).

     @param fullname the full folder name without the leading slash
     @param type the type of the new folder
     @param tryCreateOnOpen if true, create physical folder later
     @return folder (should be DecRef()'d by caller) or NULL on error
   */
   static MFolder *Create(const String& fullname,
                          MFolderType type,
                          bool tryCreateOnOpen = true);

   /**
     create a temporary folder with the specified name, type and class (all the
     other parameters may be set later)

     @param kind the class of the folder ("cclient", "virtual", ...)
     @param fullname the name of the folder, not used for much currently
     @param type the type of the new folder
     @param profile associated with this folder, use global one if NULL
     @return folder (should be DecRef()'d by caller) or NULL on error
    */
   static MFolder *CreateTemp(const String& fullname,
                              MFolderType type,
                              Profile *profile = NULL);

   /**
     Create a temp folder representing a file.

     NB: with the default value the file specified by path parameter will be
         physically deleted when this folder object is deleted!

     @param fullname the name of the folder, not used for much currently
     @param path the path to the file
     @param flags for the folder (only MF_FLAGS_TEMPORARY makes sense here)
     @return folder (should be DecRef()'d by caller) or NULL on error
   */
   static MFolder *CreateTempFile(const String& fullname,
                                  const String& path,
                                  int flags = MF_FLAGS_TEMPORARY);

   //@}

   /**@name misc accessors */
   //@{
      /**
        get the folder path (i.e. something by which it's identified by the
        mail subsystem)
      */
   virtual String GetPath() const = 0;

      /// change the folder path
   virtual void SetPath(const String& path) = 0;

      /// get the server for the folder:
   virtual String GetServer() const = 0;

      /// set the server for the folder
   virtual void SetServer(const String& server) = 0;

      /// get the login for the folder:
   virtual String GetLogin() const = 0;

      /// get the password for the folder:
   virtual String GetPassword() const = 0;

      /// save the login and password (will be encrypted here) for this folder
   virtual void SetAuthInfo(const String& login, const String& password) = 0;

      /// the folder name must be unique among its siblings
   virtual String GetName() const = 0;

      /// full folder name (has the same form as a full path name)
   virtual wxString GetFullName() const = 0;

      /// folder type (can't be changed once it's created)
   virtual MFolderType GetType() const = 0;

      /// folder class: cclient or virtual currently, default "" == cclient
   virtual String GetClass() const = 0;

      /// set the folder file format (only for MF_FILE folders)
   virtual void SetFileMboxFormat(FileMailboxFormat format) = 0;

      /// get the folder file format (only for MF_FILE folders)
   virtual FileMailboxFormat GetFileMboxFormat() const = 0;

      /**
        the icon index for this folder or -1 if there is no specific icon
        associated to it (the icon index should be used to pass it to
        GetFolderIconName())
       */
   virtual int GetIcon() const = 0;
      /// set the icon
   virtual void SetIcon(int icon) = 0;

      /// folder may have an arbitrary comment associated with it - get it
   virtual String GetComment() const = 0;
      /// change the comment
   virtual void SetComment(const String& comment) = 0;

      /// a folder may have a fixed position (by default it doesn't have any)
   virtual int GetTreeIndex() const { return -1; }
      /// set the position in the tree (used by wxFolderTree only)
   virtual void SetTreeIndex(int /* pos */) { }

   //@}

   /** @name Profile stuff */
   //@{

   /**
     Get the profile associated with this folder: it will never be NULL (as
     we fall back to the application profile if we don't have our own) and
     can be used to read the other (than the ones we have explicit functions
     for accessing them) settings associated with this folder.

     @return profile object to be DecRef()d by caller (never NULL)
    */
   virtual Profile *GetProfile() const = 0;
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

      /// can we open the folder of this type and flags?
   bool CanOpen() const { return CanOpenFolder(GetType(), GetFlags()); }

      /// can we delete this folder?
   bool CanDelete() const { return CanDeleteFolderOfType(GetType()); }

      /// can we delete messages from this folder?
   bool CanDeleteMsgs() const { return CanDeleteMessagesInFolder(GetType()); }

      /// can this folder have a login/password?
   bool MayHaveLogin() const { return FolderTypeHasUserName(GetType()); }

      /// does this folder need login/password to be opened?
   bool NeedsLogin() const
      { return MayHaveLogin() && !(GetFlags() & MF_FLAGS_ANON); }

      /// does this folder need net connection to be accessed?
   virtual bool NeedsNetwork() const
      { return FolderNeedsNetwork(GetType(), GetFlags()); }

      /// SSL mode for this folder (this is a NOP for temp folders)
   virtual SSLSupport GetSSL(SSLCert *acceptUnsigned = NULL) const = 0;
   virtual void SetSSL(SSLSupport ssl, SSLCert cert) = 0;

      /**
         Get the value of "try to create" flag.

         If we had never tried to access this folder we normally try to create
         it when we first do it -- this allows to delay the folder creation
         until it is really needed.
       */
   virtual bool ShouldTryToCreate() const = 0;

      /// after first access the try to create flag must be reset, this
      /// function allows to do it
   virtual void DontTryToCreate() = 0;

   //@}

   /**@name Filters */
   //@{
      /// get the array of filters to use for this folder
   virtual wxArrayString GetFilters() const = 0;
      /// set the filters (this replaces the old value)
   virtual void SetFilters(const wxArrayString& filters) = 0;

      /// adds a filter to the start of the filter list
   virtual void PrependFilter(const String& filter) = 0;
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
   virtual MFolder *CreateSubfolder(const String& name,
                                    MFolderType type,
                                    bool tryCreateOnOpen = true) = 0;
      /// delete this folder (does not delete the C++ object!)
   virtual void Delete() = 0;
      /// rename this folder: FALSE returned if it failed
   virtual bool Rename(const String& name) = 0;
      /// move this folder: FALSE returned if it failed
   virtual bool Move(MFolder *newParent) = 0;
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

M_LIST_RC_ABSTRACT(MFolderList, MFolder);

// ----------------------------------------------------------------------------
// A smart pointer to MFolder: not only it takes care of the ref count itself,
// but it also allows to create in the same way a folder for arbitrary file
// name (if the absolute path is given) or for the folder tree entry
// (otherwise)
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
         String name = profile->GetFolderName();
         if ( !name.empty() )
         {
            Init(name);
         }
         else
         {
            wxFAIL_MSG( _T("attempt to create MFolder from non folder profile") );

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

   // get the pointer we store
   MFolder *Get() const { return m_folder; }

   // replace the pointer we store with another one, taking ownership of it
   void Set(MFolder *folder)
   {
      SafeDecRef(m_folder);

      m_folder = folder;
   }

   // explicitly test if object is valid
   bool IsOk() const { return m_folder != NULL; }

private:
   // workaround for g++ bug: see BEGIN_DECLARE_AUTOPTR() definition in
   // MObject.h for details
#ifndef NO_PRIVATE_COPY
   // no copy ctor/assignment operator
   MFolder_obj(const MFolder_obj&);
   MFolder_obj& operator=(const MFolder_obj&);
#endif // !NO_PRIVATE_COPY

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
   /// how to traverse the tree?
   enum RecurseMode
   {
      /// don't recurse at all
      Recurse_No,

      /// depth first post order traversal (first children, then parent)
      Recurse_ChildrenFirst,

      /// pre order traversal (first parent, then children)
      Recurse_ParentFirst
   };

   /// traverse starting from the root folder (i.e. everything)
   MFolderTraversal();

   /// give the folder from which to start to the ctor
   MFolderTraversal(const MFolder& folderStart);

   /// traverse the tree, OnVisitFolder() will be called for each subfolder
   bool Traverse(RecurseMode mode = Recurse_ChildrenFirst)
      { return DoTraverse(m_folderName, mode); }

   /// return FALSE from here to stop tree traversal, TRUE to continue
   virtual bool OnVisitFolder(const wxString& folderName) = 0;

   /// virtual dtor for the base class
   virtual ~MFolderTraversal() { }

   // this overload is old and deprecated, for backwards compatibility only
   bool Traverse(bool recurse)
      { return Traverse(recurse ? Recurse_ChildrenFirst : Recurse_No); }

private:
   /// recursive function used by Traverse()
   bool DoTraverse(const wxString& start, RecurseMode mode);

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
                                      MFolderType folderType,
                                      long folderFlags,
                                      const String& path,
                                      bool notify = TRUE);

/**
   Add all subfolders of the given folder to the tree.
 */
extern size_t AddAllSubfoldersToTree(MFolder *parent,
                                     class ASMailFolder *mailFolder);

/**
   Creates entries for all files and directories under the given directory.
   The MBOX folders are created for files and GROUP folders for subdirs.

   @returns number of created folders
 */

extern size_t CreateMboxSubtree(MFolder *parent,
                                const String& rootMailDir);

#endif // _MFOLDER_H
