///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/MFolder.cpp - folder related classes
// Purpose:     manage all folders used by the application
// Author:      Vadim Zeitlin
// Modified by:
// Created:     18.09.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#   pragma implementation "MFolder.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"

#  include "Profile.h"
#  include "MApplication.h"
#  include "strutil.h"

#  include <wx/confbase.h>    // for wxSplitPath
#  include <wx/dynarray.h>
#endif // USE_PCH

#include "Mdefaults.h"

#include "MFolder.h"

#include <wx/dir.h>

// ----------------------------------------------------------------------------
// template classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MFolder *, wxArrayFolder);

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDER_COMMENT;
extern const MOption MP_FOLDER_FILTERS;
extern const MOption MP_FOLDER_ICON;
extern const MOption MP_FOLDER_ICON_D;
extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_TREEINDEX;
extern const MOption MP_FOLDER_TREEINDEX_D;
extern const MOption MP_FOLDER_TRY_CREATE;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_IMAPHOST;
extern const MOption MP_NNTPHOST;
extern const MOption MP_POPHOST;
extern const MOption MP_PROFILE_TYPE;
extern const MOption MP_USERNAME;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// invalid value for children count
static const size_t INVALID_CHILDREN_COUNT = (size_t)-1;

// ----------------------------------------------------------------------------
// MFolder implementations
// ----------------------------------------------------------------------------

// MTempFolder is a temporary object which stores tyhe info about folder, but is
// not persistent
class MTempFolder : public MFolder
{
public:
   MTempFolder(const String& fullname,
               FolderType type,
               int flags,
               const String& path,
               const String& server,
               const String& login,
               const String& password)
      : m_type(type),
        m_flags(flags),
        m_fullname(fullname),
        m_path(path),
        m_server(server), m_login(login), m_password(password)
        {
        }

   // trivial implementation of base class pure virtuals
   virtual String GetPath() const { return m_path; }
   virtual void SetPath(const String& path) { m_path = path; }
   virtual String GetServer() const { return m_server; }
   virtual String GetLogin() const { return m_login; }
   virtual String GetPassword() const {return m_password; }
   virtual String GetName() const { return m_fullname.AfterLast('/'); }
   virtual wxString GetFullName() const { return m_fullname; }
   virtual FolderType GetType() const { return m_type; }
   virtual bool NeedsNetwork(void) const { return FALSE; }
   virtual int GetIcon() const { return -1; }
   virtual void SetIcon(int /* icon */) { }
   virtual String GetComment() const { return ""; }
   virtual void SetComment(const String& /* comment */) { }
   virtual int GetFlags() const { return m_flags; }
   virtual void SetFlags(int flags) { m_flags = flags; }

   virtual Profile *GetProfile() const
   {
      Profile *profile = mApplication->GetProfile();
      profile->IncRef();
      return profile;
   }

   virtual wxArrayString GetFilters() const { return wxArrayString(); }
   virtual void SetFilters(const wxArrayString& /* filters */) { }
   virtual void AddFilter(const String& /* filter */) { }
   virtual void RemoveFilter(const String& /* filter */) { }

   // we're outside the tree, so none of these functions make sense for us
   virtual size_t GetSubfolderCount() const { return 0; }
   virtual MFolder *GetSubfolder(size_t) const { return NULL; }
   virtual MFolder *GetSubfolder(const String&) const { return NULL; }
   virtual MFolder *GetParent() const { return NULL; }
   virtual MFolder *CreateSubfolder(const String&, FolderType) { return NULL; }
   virtual void Delete() { FAIL_MSG("doesn't make sense for MTempFolder"); }
   virtual bool Rename(const String&)
    { FAIL_MSG("doesn't make sense for MTempFolder"); return FALSE; }

private:
   FolderType m_type;
   int m_flags;
   String m_fullname,
          m_path,
          m_server,
          m_login,
          m_password;
};

// this class implements MFolder pure virtuals by reading/writing the data
// to/from (main application) profile.
//
// the way we store the data is pretty straightforward and quite ineffcient: the
// folders correspond to the config groups and the data is stored as the named
// values.
class MFolderFromProfile : public MFolder
{
public:
   // ctor & dtor
      // the folderName is the full profile path which should exist: it may be
      // created with Create() if Exists() returns FALSE
   MFolderFromProfile(const String& folderName)
      : m_folderName(folderName)
   {
      m_nChildren = INVALID_CHILDREN_COUNT;

      m_profile = Profile::CreateFolderProfile(m_folderName);
      if ( !m_profile )
      {
         // this should never happen
         FAIL_MSG( "invalid MFolderFromProfile name!" );

         // to avoid crashes later
         m_profile = mApplication->GetProfile();
         m_profile->IncRef();
      }
   }

      // dtor frees hte profile and removes us from cache
   virtual ~MFolderFromProfile();

   // static functions
      // return TRUE if the folder with given name exists
   static bool Exists(const String& fullname);
      // create a folder in the profile, shouldn't be called if it already
      // exists - will return FALSE in this case.
   static bool Create(const String& fullname);

   // implement base class pure virtuals
   virtual String GetPath() const;
   virtual void SetPath(const String& path);
   virtual String GetServer() const;
   virtual String GetLogin() const;
   virtual String GetPassword() const;

   virtual String GetName() const;
   virtual wxString GetFullName() const { return m_folderName; }

   virtual FolderType GetType() const;
   virtual bool NeedsNetwork() const;

   virtual int GetIcon() const;
   virtual void SetIcon(int icon);

   virtual String GetComment() const;
   virtual void SetComment(const String& comment);

   virtual int GetTreeIndex() const;
   virtual void SetTreeIndex(int pos);

   virtual int GetFlags() const;
   virtual void SetFlags(int flags);

   virtual Profile *GetProfile() const
   {
      m_profile->IncRef();
      return m_profile;
   }

   virtual wxArrayString GetFilters() const;
   virtual void SetFilters(const wxArrayString& filters);
   virtual void AddFilter(const String& filter);
   virtual void RemoveFilter(const String& filter);

   virtual size_t GetSubfolderCount() const;
   virtual MFolder *GetSubfolder(size_t n) const;
   virtual MFolder *GetSubfolder(const String& name) const;
   virtual MFolder *GetParent() const;

   virtual MFolder *CreateSubfolder(const String& name, FolderType type);
   virtual void Delete();
   virtual bool Rename(const String& newName);

protected:
   // helpers
      // common part of Create() and Exists(): returns true if group exists
   static bool GroupExists(Profile *profile, const String& fullname);

   /** Get the full name of the subfolder.
       As all folder names are relative profile names, we need to
       check for m_folderName not being empty before appending a slash
       and subfolder name.
       @param name relative subfolder name
       @return the complete relative folder name of the subfolder
   */
   wxString GetSubFolderFullName(const String& name) const
   {
      String fullname;
      if(m_folderName.Length())
         fullname << m_folderName << '/';
      fullname << name;

      return fullname;
   }

   // update m_nChildren when a subfolder is deleted
   void OnSubfolderDelete()
   {
      if ( m_nChildren != INVALID_CHILDREN_COUNT )
      {
         m_nChildren--;
      }
   }

private:
   // the full folder name
   String m_folderName;

   // this should never be NULL
   Profile *m_profile;

   // cached the number of children (initially INVALID_CHILDREN_COUNT)
   size_t m_nChildren;

   // we allow it to directly change our name
   friend class MFolderCache;
};

// a special case of MFolderFromProfile: the root folder
class MRootFolderFromProfile : public MFolderFromProfile
{
public:
   // ctor
   MRootFolderFromProfile() : MFolderFromProfile("")
   {
   }

   // dtor
   virtual ~MRootFolderFromProfile() { }

   // implement base class pure virtuals (some of them don't make sense to us)
   virtual FolderType GetType() const { return MF_ROOT; }
   virtual bool NeedsNetwork(void) const { return FALSE; }

   virtual String GetComment() const { return ""; }
   virtual void SetComment(const String& /* comment */)
      { FAIL_MSG("can not set root folder attributes."); }

   virtual int GetFlags() const { return 0u; }
   virtual void SetFlags(int /* flags */)
      { FAIL_MSG("can not set root folder attributes."); }

   virtual MFolder *GetParent() const { return NULL; }

   virtual void Delete()
      { FAIL_MSG("can not delete root folder."); }
   virtual bool Rename(const String& /* newName */)
      { FAIL_MSG("can not rename root folder."); return FALSE; }
};

// ----------------------------------------------------------------------------
// helper classes
// ----------------------------------------------------------------------------

// a folder tree iterator which just counts the items it traverses
class CountTraversal : public MFolderTraversal
{
public:
  CountTraversal(const MFolderFromProfile *folder) : MFolderTraversal(*folder)
    { m_count = 0; }

  virtual bool OnVisitFolder(const wxString& /* folderName */)
    { m_count++; return TRUE; }

  size_t GetCount() const { return m_count; }

private:
  size_t m_count;
};

// a folder tree iterator which finds the item with given index
class IndexTraversal : public MFolderTraversal
{
public:
   IndexTraversal(const MFolderFromProfile *folder, size_t count)
      : MFolderTraversal(*folder) { m_count = count; }

   virtual bool OnVisitFolder(const wxString& folderName)
   {
      if ( m_count-- == 0 )
      {
         // found, stop traversing the tree
         m_folderName = folderName;

         return FALSE;
      }

      return TRUE;
   }

   const wxString& GetName() const { return m_folderName; }

private:
   size_t   m_count;
   wxString m_folderName;
};

// maintain the list of already created MFolder objects (the cache)
//
// there should be only one object of this class (gs_cache defined in this
// file) and it's only needed to check in its dtor that nothing is left in the
// cache on program termination, otherwise this class has just static
// functions.
class MFolderCache
{
public:
   // default ctor and dtor
   MFolderCache() { }

   // functions to work with the cache
      // returns NULL is the object is not in the cache
   static MFolder *Get(const String& name);
      // adds a new folder to the cache (must be really new!)
   static void Add(MFolder *folder);
      // removes an object from the cache (must be there!)
   static void Remove(MFolder *folder);
      // rename all cached folders with this name
   static void RenameAll(const String& oldName, const String& newName);

private:
   // no copy ctor/assignment operator
   MFolderCache(const MFolderCache&);
   MFolderCache& operator=(const MFolderCache&);

   // consistency check
   static void Check()
   {
      ASSERT_MSG( ms_aFolderNames.GetCount() == ms_aFolders.GetCount(),
                  "folder cache corrupted" );
   }

   static wxArrayString ms_aFolderNames;
   static wxArrayFolder ms_aFolders;
};

// -----------------------------------------------------------------------------
// global variables
// -----------------------------------------------------------------------------

// the global cache of already created folders
static MFolderCache gs_cache;

// =============================================================================
// implementation
// =============================================================================

// -----------------------------------------------------------------------------
// MFolder
// -----------------------------------------------------------------------------

MFolder *MFolder::Get(const String& fullname)
{
   wxString name = fullname;

   // remove the trailing backslash if any
   size_t len = fullname.Len();
   if ( len != 0 && fullname[len - 1] == '/' )
   {
      name.Truncate(len - 1);
   }

   // first see if we already have it in the cache
   MFolder *folder = MFolderCache::Get(name);
   if ( !folder )
   {
      // the first case catches the root folder - it always exists
      if ( name.empty() )
      {
         folder = new MRootFolderFromProfile();
         MFolderCache::Add(folder);
      }
      else if ( MFolderFromProfile::Exists(name) )
      {
         // do create a new folder object
         folder = new MFolderFromProfile(name);
         MFolderCache::Add(folder);
      }
      //else: folder doesn't exist, NULL will be returned
   }
   else
   {
      // in the cache
      folder->IncRef();
   }

   return folder;
}

MFolder *MFolder::Create(const String& fullname, FolderType type)
{
   MFolder *folder = Get(fullname);
   if ( folder )
   {
      wxLogError(_("Cannot create a folder '%s' which already exists."),
                 fullname.c_str());

      folder->DecRef();

      return NULL;
   }

   if ( !MFolderFromProfile::Create(fullname) )
   {
      // error message already given
      return NULL;
   }

   folder = Get(fullname);

   CHECK( folder, NULL, "Get() must succeed if Create() succeeded!" );

   Profile_obj profile(folder->GetFullName());
   CHECK( profile, NULL, "panic in MFolder: no profile" );

   profile->writeEntry(MP_FOLDER_TYPE, type);

   // the physical folder might not exist yet, we will try to create it when
   // opening fails the next time
   if ( folder->CanOpen() )
   {
      profile->writeEntry(MP_FOLDER_TRY_CREATE, 1);
   }

   return folder;
}

/* static */
MFolder *MFolder::CreateTemp(const String& fullname,
                             FolderType type,
                             int flags,
                             const String& path,
                             const String& server,
                             const String& login,
                             const String& password)
{
   return new MTempFolder(fullname, type, flags, path, server, login, password);
}

#ifdef DEBUG

String MFolder::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << "name '" << GetFullName() << '\'';

   return str;
}

#endif // DEBUG

// -----------------------------------------------------------------------------
// MFolderFromProfile
// -----------------------------------------------------------------------------

MFolderFromProfile::~MFolderFromProfile()
{
   m_profile->DecRef();

   // remove the object being deleted (us) from the cache
   MFolderCache::Remove(this);
}

bool
MFolderFromProfile::GroupExists(Profile *profile, const String& fullname)
{
   // split path into '/' separated components
   wxArrayString components;
   wxSplitPath(components, fullname);

   //PFIXME FolderPathChanger changePath(profile, "");

   size_t n, count = components.GetCount();
   for ( n = 0; n < count; n++ )
   {
      if ( !profile->HasGroup(components[n]) )
      {
         break;
      }

      // go down
      profile->SetPath(components[n]);
   }

   // group only exists if we exited from the loop normally (not via break)
   return n == count;
}

bool MFolderFromProfile::Exists(const String& fullname)
{
   Profile_obj profile("");

   return profile->HasGroup(fullname);
}

bool MFolderFromProfile::Create(const String& fullname)
{
   Profile_obj profile(fullname);
   CHECK( profile, FALSE, "panic in MFolder: no profile" );

   // VZ: let me decide about whether this check has to be done - but surely
   //     not here...
#if 0
   // check that the name is valid
   if (
        fullname.Find('/') != wxNOT_FOUND
#ifdef OS_WIN
        || fullname.Find('\\' != wxNOT_FOUND )
#endif
      )
   {
      FAIL_MSG("invalid characters in the folder name (callers bug)");

      return FALSE;
   }
#endif // 0

   //PFIXME
   if ( GroupExists(profile, fullname) )
   {
      wxLogError(_("Cannot create profile folder '%s' because a group "
                   "with this name already exists."), fullname.c_str());

      return FALSE;
   }

   return TRUE;
}

String MFolderFromProfile::GetName() const
{
   return m_folderName.AfterLast('/');
}

String MFolderFromProfile::GetPath() const
{
   return READ_CONFIG(m_profile, MP_FOLDER_PATH);
}

void MFolderFromProfile::SetPath(const String& path)
{
   m_profile->writeEntry(MP_FOLDER_PATH, path);
}

String MFolderFromProfile::GetServer() const
{
   String server;
   switch( GetType() )
   {
      case MF_NNTP:
         server = READ_CONFIG_TEXT(m_profile, MP_NNTPHOST);
         break;

      case MF_IMAP:
         server = READ_CONFIG_TEXT(m_profile, MP_IMAPHOST);
         break;

      case MF_POP:
         server = READ_CONFIG_TEXT(m_profile, MP_POPHOST);
         break;

      default:
         ; // do nothing
   }

   return server;
}

String MFolderFromProfile::GetLogin() const
{
   String login = READ_CONFIG(m_profile, MP_FOLDER_LOGIN);

   // if no login setting, fall back to username:
   if ( login.empty() && FolderTypeHasUserName(GetType()) )
   {
      login = READ_CONFIG_TEXT(m_profile, MP_USERNAME);
   }

   return login;
}

String MFolderFromProfile::GetPassword() const
{
   return strutil_decrypt(READ_CONFIG(m_profile, MP_FOLDER_PASSWORD));
}

FolderType MFolderFromProfile::GetType() const
{
   return GetFolderType(READ_CONFIG(m_profile, MP_FOLDER_TYPE));
}

bool MFolderFromProfile::NeedsNetwork() const
{
   return FolderNeedsNetwork(GetType(), GetFlags());
}

int MFolderFromProfile::GetIcon() const
{
   // it doesn't make sense to inherit the icon from parent profile (does it?)
   return m_profile->readEntryFromHere(GetOptionName(MP_FOLDER_ICON),
                                       GetNumericDefault(MP_FOLDER_ICON));
}

void MFolderFromProfile::SetIcon(int icon)
{
   m_profile->writeEntry(MP_FOLDER_ICON, icon);
}

String MFolderFromProfile::GetComment() const
{
   return READ_CONFIG(m_profile, MP_FOLDER_COMMENT);
}

void MFolderFromProfile::SetComment(const String& comment)
{
   m_profile->writeEntry(MP_FOLDER_COMMENT, comment);
}

int MFolderFromProfile::GetFlags() const
{
   return GetFolderFlags(READ_CONFIG(m_profile, MP_FOLDER_TYPE));
}

void MFolderFromProfile::SetFlags(int flags)
{
   int typeAndFlags = READ_CONFIG(m_profile, MP_FOLDER_TYPE);
   typeAndFlags = CombineFolderTypeAndFlags(GetFolderType(typeAndFlags),
                                            flags);
   m_profile->writeEntry(MP_FOLDER_TYPE, typeAndFlags);
}

int MFolderFromProfile::GetTreeIndex() const
{
   // it doesn't make sense to inherit the order from parent profile
   return m_profile->readEntryFromHere(GetOptionName(MP_FOLDER_TREEINDEX),
                                       GetNumericDefault(MP_FOLDER_TREEINDEX));
}

void MFolderFromProfile::SetTreeIndex(int pos)
{
   m_profile->writeEntry(MP_FOLDER_TREEINDEX, (long)pos);
}

// ----------------------------------------------------------------------------
// filters stuff
// ----------------------------------------------------------------------------

wxArrayString MFolderFromProfile::GetFilters() const
{
   return strutil_restore_array(':', READ_CONFIG(m_profile, MP_FOLDER_FILTERS));
}

void MFolderFromProfile::SetFilters(const wxArrayString& filters)
{
   m_profile->writeEntry(MP_FOLDER_FILTERS, strutil_flatten_array(filters, ':'));
}

void MFolderFromProfile::AddFilter(const String& filter)
{
   String filters = READ_CONFIG(m_profile, MP_FOLDER_FILTERS);
   if ( !filters.empty() )
      filters += ':';
   filters += filter;

   m_profile->writeEntry(MP_FOLDER_FILTERS, filters);
}

void MFolderFromProfile::RemoveFilter(const String& filter)
{
   String filters = READ_CONFIG(m_profile, MP_FOLDER_FILTERS);

   if ( filters == filter )
   {
      // we don't have any other filters
      filters.clear();
   }
   else // something will be left
   {
      String others;
      if ( filters.StartsWith(filter + ':', &others) )
      {
         filters = others;
      }
      else
      {
         const char *start = strstr(filters, ':' + filter);
         if ( !start )
         {
            // we don't have such filter
            return;
         }

         filters.erase(start - filter.c_str(), filter.length() + 1);
      }
   }

   m_profile->writeEntry(MP_FOLDER_FILTERS, filters);
}

// ----------------------------------------------------------------------------
// subfolders
// ----------------------------------------------------------------------------

size_t MFolderFromProfile::GetSubfolderCount() const
{
   if ( m_nChildren == INVALID_CHILDREN_COUNT )
   {
      CountTraversal count(this);

      // traverse but don't recurse into subfolders
      count.Traverse(FALSE);

      // const_cast needed
      ((MFolderFromProfile *)this)->m_nChildren = count.GetCount();
   }

   return m_nChildren;
}

MFolder *MFolderFromProfile::GetSubfolder(size_t n) const
{
   // IndexTraversal is not used any more because it quite inefficient and this
   // function is called a *lot* of times
   IndexTraversal index(this, n);

   // don't recurse into subfolders
   if ( index.Traverse(FALSE) )
   {
      FAIL_MSG( "invalid index in MFolderFromProfile::GetSubfolder()" );

      return NULL;
   }
   //else: not all folders traversed, i.e. the right one found

   return Get(index.GetName());
}

MFolder *MFolderFromProfile::GetSubfolder(const String& name) const
{
   return Get(GetSubFolderFullName(name));
}

MFolder *MFolderFromProfile::GetParent() const
{
   String path = m_folderName.BeforeLast('/');
   return Get(path);
}

MFolder *MFolderFromProfile::CreateSubfolder(const String& name, FolderType type)
{
   // first of all, check if the name is valid
   MFolder *folder = GetSubfolder(name);
   if ( folder )
   {
      wxLogError(_("Cannot create subfolder '%s': folder with this name "
                   "already exists."), name.c_str());

      folder->DecRef();

      return NULL;
   }

   // ok, it is: do create it
   MFolder *subfolder = MFolder::Create(GetSubFolderFullName(name), type);

   // is it our immediate child?
   if ( subfolder && name.find('/') == String::npos )
   {
      // we must update the children count if we had already calculated it
      if ( m_nChildren != INVALID_CHILDREN_COUNT )
      {
         m_nChildren++;
      }
   }

   return subfolder;
}

// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------

void MFolderFromProfile::Delete()
{
   CHECK_RET( !m_folderName.empty(), "can't delete the root pseudo-folder" );

   // delete this folder from the parent profile
   String parentName = m_folderName.BeforeLast('/');
   Profile_obj profile(parentName);
   CHECK_RET( profile, "panic in MFolder: no profile" );

   profile->DeleteGroup(GetName());

   // let the parent now that its number of children changed
   MFolderFromProfile *parent = (MFolderFromProfile *)Get(parentName);
   if ( parent )
   {
      parent->OnSubfolderDelete();

      parent->DecRef();
   }
   else
   {
      // either we have managed to delete the root folder (bad) or something is
      // seriously wrong (even worse)
      FAIL_MSG( "no parent for deleted folder?" );
   }

   // notify everybody about the disappearance of the folder
   MEventManager::Send(
      new MEventFolderTreeChangeData (GetFullName(),
                                      MEventFolderTreeChangeData::Delete)
      );
}

bool MFolderFromProfile::Rename(const String& newName)
{
   CHECK( !m_folderName.empty(), FALSE, "can't rename the root pseudo-folder" );

   String path = m_folderName.BeforeLast('/'),
          name = m_folderName.AfterLast('/');

   String newFullName = path;
   if ( !!path )
      newFullName += '/';
   newFullName += newName;

   // we can't use Exists() here as it tries to read a value from the config
   // group newFullName and, as a side effect of this, creates this group, so
   // Profile::Rename() below will then fail!
#if 0
   if ( Exists(newFullName) )
   {
      wxLogError(_("Cannot rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_folderName.c_str(), newName.c_str());

      return FALSE;
   }
#endif // 0

   Profile_obj profile(path);
   CHECK( profile, FALSE, "panic in MFolder: no profile" );
   if ( profile->Rename(name, newName) )
   {
      String oldName = m_folderName;
      m_folderName = newFullName;

      // TODO: MFolderCache should just subscribe to "Rename" events...
      MFolderCache::RenameAll(oldName, newFullName);

      // notify everybody about the change of the folder name
      MEventManager::Send(
         new MEventFolderTreeChangeData(oldName,
                                        MEventFolderTreeChangeData::Rename,
                                        newFullName)
         );

      return TRUE;
   }
   else
   {
      wxLogError(_("Cannot rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_folderName.c_str(), newName.c_str());

      return FALSE;
   }
}

// -----------------------------------------------------------------------------
// MFolderCache implementation
// -----------------------------------------------------------------------------

wxArrayString MFolderCache::ms_aFolderNames(TRUE /* auto sort */);
wxArrayFolder MFolderCache::ms_aFolders;

MFolder *MFolderCache::Get(const String& name)
{
   Check();
   int index = ms_aFolderNames.Index(name);
   return index == wxNOT_FOUND ? NULL : ms_aFolders[(size_t)index];
}

void MFolderCache::Add(MFolder *folder)
{
   Check();

   // the caller should verify that it's not already in the cache
   ASSERT_MSG( ms_aFolders.Index(folder) == wxNOT_FOUND,
               "can't add the folder to the cache - it's already there" );

   size_t index = ms_aFolderNames.Add(folder->GetFullName());
   ms_aFolders.Insert(folder, index);
}

void MFolderCache::Remove(MFolder *folder)
{
   Check();

   // don't use name here - the folder might have been renamed
   int index = ms_aFolders.Index(folder);
   CHECK_RET( index != wxNOT_FOUND,
              "can't remove folder from cache because it's not in it" );

#if wxCHECK_VERSION(2, 2, 8)
   ms_aFolderNames.RemoveAt((size_t)index);
#else
   ms_aFolderNames.Remove((size_t)index);
#endif

   ms_aFolders.RemoveAt((size_t)index);
}

/* static */
void MFolderCache::RenameAll(const String& oldName, const String& newName)
{
   MFolderFromProfile *folder = (MFolderFromProfile *)Get(oldName);
   if ( folder )
   {
      folder->m_folderName = newName;
      SafeDecRef(folder->m_profile);
      folder->m_profile = Profile::CreateFolderProfile(newName);
   }
}

// ----------------------------------------------------------------------------
// MFolderTraversal
// ----------------------------------------------------------------------------

MFolderTraversal::MFolderTraversal(const MFolder& folderStart)
{
   m_folderName = folderStart.GetFullName();
}

bool MFolderTraversal::Traverse(bool recurse)
{
   return DoTraverse(m_folderName, recurse);
}

// returns TRUE if all folders were visited or FALSE if traversal was aborted
bool MFolderTraversal::DoTraverse(const wxString& start, bool recurse)
{
   Profile_obj profile(start);
   CHECK( profile, FALSE, "panic in MFolderTraversal: no profile" );

   // enumerate all groups
   String name;
   long dummy;

   wxString rootName(start);
   if ( !rootName.empty() )
      rootName += '/';
   //else: there should be no leading slash

   bool cont = profile->GetFirstGroup(name, dummy);
   while ( cont )
   {
      wxString fullname(rootName);
      fullname += name;

      // if this folder has children recurse into them (if we should)
      if ( recurse )
      {
         if ( !DoTraverse(fullname, TRUE) )
            return FALSE;
      }

      if ( !OnVisitFolder(fullname) )
      {
         return FALSE;
      }

      cont = profile->GetNextGroup(name, dummy);
   }

   return TRUE;
}

// ----------------------------------------------------------------------------
// functions to create new folders in the tree
// ----------------------------------------------------------------------------

extern MFolder *CreateFolderTreeEntry(MFolder *parent,
                                      const String& name,
                                      FolderType folderType,
                                      long folderFlags,
                                      const String& path,
                                      bool notify)
{
   // construct the full name by prepending the parent name (if we have parent
   // and its name is non empty - only root entry is empty)
   String fullname;
   if ( parent && parent->GetType() != MF_ROOT )
   {
      fullname << parent->GetFullName() << '/';
   }
   fullname << name;

   MFolder *folder = MFolder::Create(fullname, folderType);

   if ( folder == NULL )
   {
      wxLogError(_("Cannot create a folder '%s'.\n"
                   "Maybe a folder of this name already exists?"),
                 fullname.c_str());

      return NULL;
   }

   Profile_obj profile(folder->GetFullName());
   profile->writeEntry(MP_FOLDER_PATH, path);

   // copy folder flags from its parent
   folder->SetFlags(folderFlags);

   // special IMAP hack: in IMAP, the entry with empty folder name and the
   // special folder INBOX are equivalent (i.e. if you don't specify any
   // folder name you get INBOX) so it doesn't make sense to have both an
   // entry for IMAP server (path == "") and INBOX on it
   //
   // the solution we adopt is to disable the IMAP server folder when IMAP
   // Inbox is created to clear the confusion between the two
   //
   // of course, if the user really wants to open it he can always clear the
   // "can't be opened" flag in the folder properties dialog...
   if ( folderType == MF_IMAP )
   {
      MFolder_obj folderParent = folder->GetParent();

      // are we're the child of the IMAP server entry?
      if ( folderParent &&
           folderParent->GetType() == MF_IMAP &&
           folderParent->GetPath().empty() )
      {
         // yes, now check if we're INBOX
         if ( path.empty() || !wxStricmp(path, "INBOX") )
         {
            // INBOX created, so disable the parent
            folderParent->AddFlags(MF_FLAGS_NOSELECT);

            // should we do MailFolder::CloseFolder(folderParent)?
         }
      }
   }

   // notify the others about new folder creation unless expliticly disabled
   if ( notify )
   {
      MEventManager::Send(
         new MEventFolderTreeChangeData(fullname,
                                        MEventFolderTreeChangeData::Create)
         );
   }

   return folder;
}

bool CreateMboxSubtreeHelper(MFolder *parent,
                             const String& rootMailDir,
                             size_t *count)
{
   wxDir dir(rootMailDir);
   if ( !dir.IsOpened() )
   {
      wxLogError(_("Directory doesn't exist - no MBOX folders found."));

      return FALSE;
   }

   // create folders for all files in this dir
   wxString filename;
   bool cont = dir.GetFirst(&filename, "", wxDIR_FILES);
   while ( cont )
   {
      wxString fullname;
      fullname << rootMailDir << wxFILE_SEP_PATH << filename;
      MFolder *folderMbox = CreateFolderTreeEntry
                            (
                             parent,
                             filename,
                             MF_FILE,
                             0,
                             fullname,
                             FALSE // don't send notify event
                            );
      if ( folderMbox )
      {
         (*count)++;

         folderMbox->DecRef();
      }
      else
      {
         wxLogWarning(_("Failed to create folder '%s'"), fullname.c_str());
      }

      cont = dir.GetNext(&filename);
   }

   // and recursively call us for each subdir
   wxString dirname;
   cont = dir.GetFirst(&dirname, "", wxDIR_DIRS);
   while ( cont )
   {
      wxString subdir;
      subdir << rootMailDir << wxFILE_SEP_PATH << dirname;
      MFolder *folderSubgroup = CreateFolderTreeEntry
                                (
                                 parent,
                                 dirname,
                                 MF_GROUP,
                                 0,
                                 subdir,
                                 FALSE // don't send notify event
                                );

      if ( folderSubgroup )
      {
         CreateMboxSubtreeHelper(folderSubgroup, subdir, count);

         folderSubgroup->DecRef();
      }
      else
      {
         wxLogWarning(_("Failed to create folder group '%s'"), dirname.c_str());
      }

      cont = dir.GetNext(&dirname);
   }

   return TRUE;
}

size_t CreateMboxSubtree(MFolder *parent, const String& rootMailDir)
{
   size_t count = 0;
   if ( !CreateMboxSubtreeHelper(parent, rootMailDir, &count) )
   {
      wxLogError(_("Failed to import MBOX folders."));

      return 0;
   }

   // notify everyone about folder creation
   MEventManager::Send(
      new MEventFolderTreeChangeData(parent->GetPath(),
                                     MEventFolderTreeChangeData::CreateUnder));

   return count;
}

// ----------------------------------------------------------------------------
// MFolder_obj
// ----------------------------------------------------------------------------

void MFolder_obj::Init(const String& name)
{
   if ( !name.empty() && IsAbsPath(name) )
   {
      // called with a filename, create a temp folder to access it
      m_folder = MFolder::CreateTemp(name, MF_FILE, 0, name);
   }
   else
   {
      // called with a folder name, create a folder for the tree entry
      m_folder = MFolder::Get(name);
   }
}
