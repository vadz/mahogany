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
#  include "Mdefaults.h"

#  include <wx/utils.h>      // for wxRemoveFile
#  include <wx/dynarray.h>      // for WX_DEFINE_ARRAY
#endif // USE_PCH

#include "MFolder.h"
#include "MFilter.h"
#include "MFCache.h"
#include "MFStatus.h"

#include <wx/dir.h>
#include <wx/confbase.h>    // for wxSplitPath

// ----------------------------------------------------------------------------
// template classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MFolder *, wxArrayFolder);

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_FOLDER_CLASS;
extern const MOption MP_FOLDER_COMMENT;
extern const MOption MP_FOLDER_FILE_DRIVER;
extern const MOption MP_FOLDER_FILTERS;
extern const MOption MP_FOLDER_ICON;
extern const MOption MP_FOLDER_ICON_D;
extern const MOption MP_FOLDER_LOGIN;
extern const MOption MP_FOLDER_PASSWORD;
extern const MOption MP_FOLDER_PATH;
extern const MOption MP_FOLDER_TREEINDEX;
extern const MOption MP_FOLDER_TRY_CREATE;
extern const MOption MP_FOLDER_TYPE;
extern const MOption MP_IMAPHOST;
extern const MOption MP_NNTPHOST;
extern const MOption MP_POPHOST;
extern const MOption MP_PROFILE_TYPE;
extern const MOption MP_USERNAME;
extern const MOption MP_USE_SSL;
extern const MOption MP_USE_SSL_UNSIGNED;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// invalid value for children count
static const size_t INVALID_CHILDREN_COUNT = (size_t)-1;

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

// return the folder class name from its type: currently we don't store the
// class sepaartely simply because we have only one non default class and only
// for the folders of one type
static String GetClassForType(MFolderType type)
{
   return type == MF_VIRTUAL ? _T("virtual") : _T("cclient");
}

// ----------------------------------------------------------------------------
// MFolder implementations
// ----------------------------------------------------------------------------

// MTempFolder is a temporary object which stores tyhe info about folder, but
// is not persistent
class MTempFolder : public MFolder
{
public:
   MTempFolder(const String& fullname,
               MFolderType type,
               Profile *profile)
      : m_fullname(fullname)
   {
      m_type = type;
      m_flags = 0;

      m_profile = profile ? profile : mApplication->GetProfile();
      m_profile->IncRef();

      m_format = (FileMailboxFormat)(long)
                  READ_CONFIG(m_profile, MP_FOLDER_FILE_DRIVER);

      m_ssl = SSLSupport_None;
      m_cert = SSLCert_SignedOnly;

      // by default, try to create the new folder
      m_tryToCreate = true;
   }

   ~MTempFolder()
   {
      if ( m_flags & MF_FLAGS_TEMPORARY )
      {
         // it was a temp folder created just to view an embedded message,
         // delete it now as we don't need it any longer
         wxRemoveFile(m_path);
      }

      m_profile->DecRef();
   }

   // trivial implementation of base class pure virtuals
   virtual String GetPath() const { return m_path; }
   virtual void SetPath(const String& path)
   {
      m_path = path;

      // just as in MFolderFromProfile::GetPath() we need to ensure that
      // we don't have any slashes in the file names under Windows
#ifdef OS_WIN
      if ( m_type == MF_FILE || m_type == MF_MH )
      {
         m_path.Replace("/", "\\");
      }
#endif // Windows
   }

   virtual String GetServer() const { return m_server; }
   virtual void SetServer(const String& server) { m_server = server; }

   virtual String GetLogin() const { return m_login; }
   virtual String GetPassword() const { return m_password; }
   virtual void SetAuthInfo(const String& login, const String& password)
      { m_login = login; m_password = password; }

   virtual void SetFileMboxFormat(FileMailboxFormat format)
   {
      m_format = format;
   }
   virtual FileMailboxFormat GetFileMboxFormat() const { return m_format; }

   virtual String GetName() const { return m_fullname.AfterLast('/'); }
   virtual wxString GetFullName() const { return m_fullname; }
   virtual MFolderType GetType() const { return m_type; }
   virtual String GetClass() const { return GetClassForType(m_type); }

   virtual bool NeedsNetwork(void) const { return true; }
   virtual SSLSupport GetSSL(SSLCert *cert) const
   {
      if ( cert )
         *cert = m_cert;

      return m_ssl;
   }

   virtual void SetSSL(SSLSupport ssl, SSLCert cert)
   {
      m_ssl = ssl;
      m_cert = cert;
   }

   virtual bool ShouldTryToCreate() const { return m_tryToCreate; }
   virtual void DontTryToCreate() { m_tryToCreate = false; }

   virtual int GetIcon() const { return -1; }
   virtual void SetIcon(int /* icon */) { }

   virtual String GetComment() const { return _T(""); }
   virtual void SetComment(const String& /* comment */) { }

   virtual int GetFlags() const { return m_flags; }
   virtual void SetFlags(int flags) { m_flags = flags; }

   virtual Profile *GetProfile() const
   {
      m_profile->IncRef();
      return m_profile;
   }

   virtual wxArrayString GetFilters() const { return wxArrayString(); }
   virtual void SetFilters(const wxArrayString& /* filters */) { }
   virtual void PrependFilter(const String& /* filter */) { }
   virtual void AddFilter(const String& /* filter */) { }
   virtual void RemoveFilter(const String& /* filter */) { }

   // we're outside the tree, so none of these functions make sense for us
   virtual size_t GetSubfolderCount() const { return 0; }
   virtual MFolder *GetSubfolder(size_t) const { return NULL; }
   virtual MFolder *GetSubfolder(const String&) const { return NULL; }
   virtual MFolder *GetParent() const { return NULL; }
   virtual MFolder *CreateSubfolder(const String&,
                                    MFolderType, bool) { return NULL; }
   virtual void Delete() { FAIL_MSG(_T("doesn't make sense for MTempFolder")); }
   virtual bool Rename(const String&)
      { FAIL_MSG(_T("doesn't make sense for MTempFolder")); return false; }
   virtual bool Move(MFolder*)
      { FAIL_MSG(_T("doesn't make sense for MTempFolder")); return false; }

private:
   String m_fullname,
          m_path,
          m_server,
          m_login,
          m_password;

   MFolderType m_type;

   int m_flags;

   FileMailboxFormat m_format;

   SSLSupport m_ssl;
   SSLCert m_cert;

   bool m_tryToCreate;

   Profile *m_profile;
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
      // created with Create() if Exists() returns false
   MFolderFromProfile(const String& folderName)
      : m_folderName(folderName)
   {
      m_nChildren = INVALID_CHILDREN_COUNT;

      m_profile = Profile::CreateFolderProfile(m_folderName);
      if ( !m_profile )
      {
         // this should never happen
         FAIL_MSG( _T("invalid MFolderFromProfile name!") );

         // to avoid crashes later
         m_profile = mApplication->GetProfile();
         m_profile->IncRef();
      }
   }

      // dtor frees hte profile and removes us from cache
   virtual ~MFolderFromProfile();

   // static functions
      // return true if the folder with given name exists
   static bool Exists(const String& fullname);
      // create a folder in the profile, shouldn't be called if it already
      // exists - will return false in this case.
   static bool Create(const String& fullname);

   // implement base class pure virtuals
   virtual String GetPath() const;
   virtual void SetPath(const String& path);
   virtual String GetServer() const;
   virtual void SetServer(const String& server);
   virtual String GetLogin() const;
   virtual String GetPassword() const;
   virtual void SetAuthInfo(const String& login, const String& password);

   virtual void SetFileMboxFormat(FileMailboxFormat format);
   virtual FileMailboxFormat GetFileMboxFormat() const;

   virtual String GetName() const;
   virtual wxString GetFullName() const { return m_folderName; }

   virtual MFolderType GetType() const;
   virtual String GetClass() const;
   virtual bool NeedsNetwork() const;
   virtual SSLSupport GetSSL(SSLCert *acceptUnsigned) const;
   virtual void SetSSL(SSLSupport ssl, SSLCert cert);

   virtual bool ShouldTryToCreate() const;
   virtual void DontTryToCreate();

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
   virtual void PrependFilter(const String& filter);
   virtual void AddFilter(const String& filter);
   virtual void RemoveFilter(const String& filter);

   virtual size_t GetSubfolderCount() const;
   virtual MFolder *GetSubfolder(size_t n) const;
   virtual MFolder *GetSubfolder(const String& name) const;
   virtual MFolder *GetParent() const;

   virtual MFolder *CreateSubfolder(const String& name,
                                    MFolderType type,
                                    bool tryCreateLater);
   virtual void Delete();
   virtual bool Rename(const String& newName);
   virtual bool Move(MFolder* newParent);

protected:
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
   MRootFolderFromProfile() : MFolderFromProfile(_T(""))
   {
   }

   // dtor
   virtual ~MRootFolderFromProfile() { }

   // implement base class pure virtuals (some of them don't make sense to us)
   virtual MFolderType GetType() const { return MF_ROOT; }
   virtual bool NeedsNetwork(void) const { return false; }
   virtual SSLSupport GetSSL(SSLCert *) const { return SSLSupport_None; }
   virtual void SetSSL(SSLSupport, SSLCert) { }

   virtual bool ShouldTryToCreate() const
      { FAIL_MSG(_T("doesn't make sense for root folder")); return false; }
   virtual void DontTryToCreate()
      { FAIL_MSG(_T("doesn't make sense for root folder")); }

   virtual String GetComment() const { return _T(""); }
   virtual void SetComment(const String& /* comment */)
      { FAIL_MSG(_T("can not set root folder attributes.")); }

   virtual int GetFlags() const { return 0u; }
   virtual void SetFlags(int /* flags */)
      { FAIL_MSG(_T("can not set root folder attributes.")); }

   virtual MFolder *GetParent() const { return NULL; }

   virtual void Delete()
      { FAIL_MSG(_T("can not delete root folder.")); }
   virtual bool Rename(const String& /* newName */)
      { FAIL_MSG(_T("can not rename root folder.")); return false; }
   virtual bool Move(const MFolder* /* newParent */)
      { FAIL_MSG(_T("can not move root folder.")); return false; }
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
    { m_count++; return true; }

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

         return false;
      }

      return true;
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
                  _T("folder cache corrupted") );
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

MFolder *
MFolder::Create(const String& fullname, MFolderType type, bool tryCreateLater)
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

   CHECK( folder, NULL, _T("Get() must succeed if Create() succeeded!") );

   Profile_obj profile(folder->GetFullName());
   CHECK( profile, NULL, _T("panic in MFolder: no profile") );

   profile->writeEntry(MP_FOLDER_TYPE, type);

   // the physical folder might not exist yet, we will try to create it when
   // opening fails the next time
   //
   // NB: if we CanDeleteFolderOfType(), then we can create them, too and, vice
   //     versa, if we can't delete folders of this type there is no sense in
   //     trying to create them neither
   if ( tryCreateLater && CanDeleteFolderOfType(type) && folder->CanOpen() )
   {
      profile->writeEntry(MP_FOLDER_TRY_CREATE, 1);
   }

   return folder;
}

/* static */
MFolder *
MFolder::CreateTemp(const String& fullname,
                    MFolderType type,
                    Profile *profile)
{
   return new MTempFolder(fullname, type, profile);
}

/* static */
MFolder *
MFolder::CreateTempFile(const String& fullname, const String& path, int flags)
{
   MFolder *folder = CreateTemp(fullname, MF_FILE);
   if ( folder )
   {
      folder->SetPath(path);
      folder->SetFlags(flags);
   }

   return folder;
}

#ifdef DEBUG

String MFolder::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << _T("name '") << GetFullName() << _T('\'');

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

bool MFolderFromProfile::Exists(const String& fullname)
{
   Profile_obj profile(_T(""));

   return profile->HasGroup(fullname);
}

bool MFolderFromProfile::Create(const String& fullname)
{
   // update the count of children in the immediate parent of the folder we're
   // going to create

   // split path into '/' separated components
   wxArrayString components;
   wxSplitPath(components, fullname);

   bool updatedCount = false;

   String path, component;

   Profile *profile = Profile::CreateFolderProfile(_T(""));

   size_t n,
          count = components.GetCount();
   for ( n = 0; n < count; n++ )
   {
      CHECK( profile, false, _T("failed to create profile?") );

      component = components[n];

      if ( !updatedCount )
      {
         if ( profile->readEntryFromHere(component + _T('/') + MP_FOLDER_TYPE,
                                         MF_ILLEGAL) == MF_ILLEGAL )
         {
            MFolderFromProfile *folder = (MFolderFromProfile *)MFolder::Get(path);
            if ( !folder )
            {
               FAIL_MSG( _T("this folder must already exist!") );
            }
            else
            {
               // we have to invalidate it - unfortunately we can't just
               // increase it because we may have a situation like this:
               //
               //    1. Create("a/b/c")
               //    2. Create("a/b")
               //    3. Create("a")
               //
               // and then, assuming 'a' hadn't existed before, the children
               // count of the root folder would be incremented thrice even
               // though only one new children was created
               folder->m_nChildren = INVALID_CHILDREN_COUNT;

               folder->DecRef();
            }

            // this is the the last existing folder for which we have anything to
            // update
            updatedCount = true;
         }
      }

      // go down
      if ( !path.empty() )
         path += _T('/');
      path += components[n];

      profile->DecRef();
      profile = Profile::CreateFolderProfile(path);
   }

   // we need to write something to this group to really create it
   profile->writeEntry(MP_FOLDER_TYPE, MF_ILLEGAL);

   profile->DecRef();

   return true;
}

String MFolderFromProfile::GetName() const
{
   return m_folderName.AfterLast(_T('/'));
}

String MFolderFromProfile::GetPath() const
{
   String path = READ_CONFIG(m_profile, MP_FOLDER_PATH);

#ifdef OS_WIN
   // c-client doesn't accept the files with slashes in names under Windows, so
   // make sure we replace all of them with the proper backslashes before using
   // the filename (it is also better to show it like to the user in this form)
   if ( GetType() == MF_FILE )
      path.Replace(_T("/"), _T("\\"));
#endif // Windows

   return path;
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

void MFolderFromProfile::SetServer(const String& /* server */)
{
   // it's not used now...
   FAIL_MSG( _T("not implemented") );
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

void
MFolderFromProfile::SetAuthInfo(const String& login, const String& password)
{
   m_profile->writeEntry(MP_FOLDER_LOGIN, login);
   m_profile->writeEntry(MP_FOLDER_PASSWORD, strutil_encrypt(password));
}

void MFolderFromProfile::SetFileMboxFormat(FileMailboxFormat format)
{
   m_profile->writeEntry(MP_FOLDER_FILE_DRIVER, format);
}

FileMailboxFormat MFolderFromProfile::GetFileMboxFormat() const
{
   return (FileMailboxFormat)(long)READ_CONFIG(m_profile, MP_FOLDER_FILE_DRIVER);
}

MFolderType MFolderFromProfile::GetType() const
{
   return GetFolderType(READ_CONFIG(m_profile, MP_FOLDER_TYPE));
}

String MFolderFromProfile::GetClass() const
{
   return GetClassForType(GetType());
}

bool MFolderFromProfile::NeedsNetwork() const
{
   return FolderNeedsNetwork(GetType(), GetFlags());
}

SSLSupport MFolderFromProfile::GetSSL(SSLCert *acceptUnsigned) const
{
   SSLSupport ssl = (SSLSupport)(long)READ_CONFIG(m_profile, MP_USE_SSL);

   if ( acceptUnsigned )
   {
      if ( ssl != SSLSupport_None )
      {
         *acceptUnsigned = READ_CONFIG_BOOL(m_profile, MP_USE_SSL_UNSIGNED)
                           ? SSLCert_AcceptUnsigned : SSLCert_SignedOnly;
      }
      else // we don't use SSL certs at all
      {
         *acceptUnsigned = SSLCert_SignedOnly;
      }
   }

   return ssl;
}

void MFolderFromProfile::SetSSL(SSLSupport ssl, SSLCert cert)
{
   m_profile->writeEntry(GetOptionName(MP_USE_SSL), (long)ssl);
   if ( ssl != SSLSupport_None )
   {
      m_profile->writeEntry(GetOptionName(MP_USE_SSL_UNSIGNED),
                            cert == SSLCert_AcceptUnsigned);
   }
   //else: no need to write it there
}

bool MFolderFromProfile::ShouldTryToCreate() const
{
   return m_profile->readEntryFromHere(MP_FOLDER_TRY_CREATE, false) != 0;
}

void MFolderFromProfile::DontTryToCreate()
{
   if ( m_profile->HasEntry(MP_FOLDER_TRY_CREATE) )
   {
      m_profile->DeleteEntry(MP_FOLDER_TRY_CREATE);
   }
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
   return strutil_restore_array(READ_CONFIG(m_profile, MP_FOLDER_FILTERS));
}

void MFolderFromProfile::SetFilters(const wxArrayString& filters)
{
   m_profile->writeEntry(MP_FOLDER_FILTERS, strutil_flatten_array(filters));
}

void MFolderFromProfile::PrependFilter(const String& filter)
{
   wxArrayString filters = GetFilters();
   filters.Insert(filter, 0);
   SetFilters(filters);
}

void MFolderFromProfile::AddFilter(const String& filter)
{
   wxArrayString filters = GetFilters();
   filters.Add(filter);
   SetFilters(filters);
}

void MFolderFromProfile::RemoveFilter(const String& filter)
{
   wxArrayString filters = GetFilters();
   int n = filters.Index(filter);
   if ( n != wxNOT_FOUND )
   {
      filters.RemoveAt(n);

      SetFilters(filters);
   }
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
      count.Traverse(false);

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
   if ( index.Traverse(false) )
   {
      FAIL_MSG( _T("invalid index in MFolderFromProfile::GetSubfolder()") );

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
   String path = m_folderName.BeforeLast(_T('/'));
   return Get(path);
}

MFolder *MFolderFromProfile::CreateSubfolder(const String& name,
                                             MFolderType type,
                                             bool tryCreateLater)
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
   MFolder *subfolder = MFolder::Create(GetSubFolderFullName(name),
                                        type, tryCreateLater);

   // is it our immediate child?
   if ( subfolder && name.find(_T('/')) == String::npos )
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
   CHECK_RET( !m_folderName.empty(), _T("can't delete the root pseudo-folder") );

   // delete this folder from the parent profile
   String parentName = m_folderName.BeforeLast('/');
   Profile_obj profile(parentName);
   CHECK_RET( profile, _T("panic in MFolder: no profile") );

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
      FAIL_MSG( _T("no parent for deleted folder?") );
   }

   // notify everybody about the disappearance of the folder
   MEventManager::Send(
      new MEventFolderTreeChangeData (GetFullName(),
                                      MEventFolderTreeChangeData::Delete)
      );
}

bool MFolderFromProfile::Rename(const String& newName)
{
   CHECK( !m_folderName.empty(), false, _T("can't rename the root pseudo-folder") );

   String path = m_folderName.BeforeLast(_T('/')),
          name = m_folderName.AfterLast(_T('/'));

   String newFullName = path;
   if ( !path.empty() )
      newFullName += _T('/');
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

      return false;
   }
#endif // 0

   Profile_obj profile(path);
   CHECK( profile, false, _T("panic in MFolder: no profile") );
   if ( !profile->Rename(name, newName) )
   {
      wxLogError(_("Cannot rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_folderName.c_str(), newName.c_str());

      return false;
   }

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

   return true;
}

bool MFolderFromProfile::Move(MFolder *newParent)
{
   CHECK( newParent, false, _T("no new parent in MFolder::Move()") );

   // This does not really 'move' the folder, but it creates a new one with the
   // correct parent and name, and copies all the profile information from the
   // old one to the new one. It then calls Delete on itself, so that the old
   // folder is removed from the tree. Last thing is to notify everyone that a
   // new folder has been created.

   // There are things that do not make sense at all
   CHECK( GetFolderType(GetType()) != MF_ILLEGAL, false,
            _T("How did you manage to try to move an MF_ILLEGAL folder ?") );
   CHECK( GetFolderType(GetType()) != MF_NEWS, false,
            _T("can't move News folders") );
   CHECK( GetFolderType(GetType()) != MF_INBOX, false,
            _T("can't move system Inbox") );
   CHECK( GetFolderType(GetType()) != MF_ROOT, false,
            _T("can't move the root pseudo-folder") );

   // And there are things we can't do yet.
   CHECK( GetSubfolderCount() == 0, false, _T("can't move a folder with sub-folders (yet)") );
   CHECK( GetFolderType(GetType()) != MF_IMAP, false, _T("can't move IMAP folders (yet)") );

   if ( GetFolderType(GetType()) == MF_IMAP )
   {
      // IMAP folders have one more check: we must make sure that they stay on
      // the same server, so that we can simply send it a RENAME command.
      CHECK( false, false, _T("Same server check not yet implemented") );
   }
   
   // Compute the name of the folder to create
   String path = newParent->GetFullName();
   String name = m_folderName.AfterLast('/');
   String newFullName = path;
   if ( !path.empty() )
      newFullName += _T('/');
   newFullName += name;

   // Create a new folder
   MFolder_obj newSubfolder(newParent->CreateSubfolder(name, MF_ILLEGAL));
   if ( !newSubfolder )
   {
      wxLogError(_("Could not create subfolder '%s' in '%s'."),
                   name.c_str(), path.c_str());
      return false;
   }

   wxString oldProfilePath, newProfilePath;
   oldProfilePath << Profile::GetProfilePath() << '/' << m_folderName;
   newProfilePath << Profile::GetProfilePath() << '/' << newFullName;
   if ( CopyEntries(mApplication->GetProfile()->GetConfig(), oldProfilePath, newProfilePath, false) == -1 )
   {
      wxLogError(_("Could not copy profile."));
      return false;
   }
   bool rc = true;

   if ( GetFolderType(GetType()) == MF_IMAP )
   {
      // IMAP folders need one last specific thing: send a RENAME
      // command to the server, unless we are moving the root folder
      // for this server (in this case, nothing changes on the server
      // and no RENAME command should be issued).
      CHECK( false, false, _T("RENAME command to server not yet implemented") );
   }

   // We should update the cache, but I (XNO) did not find a way to do it correctly.
   //MFolderCache::Remove(this);
   //MFolderCache::Add(newSubfolder);
   
   // Iterate over all the filters to change the name of the folder where
   // it appears.
   wxArrayString allFilterNames = MFilter::GetAllFilters();
   for ( size_t i = 0; i < allFilterNames.GetCount(); ++i )
   {
      wxString filterName = allFilterNames[i];
      MFilter *filter = MFilter::CreateFromProfile(filterName);
      MFilterDesc filterDesc = filter->GetDesc();
      if ( filterDesc.IsSimple() )
      {
         MFDialogSettings *dialogSettings = filterDesc.GetSettings();
         wxString argument = dialogSettings->GetActionArgument();
         size_t nbReplacements = argument.Replace(m_folderName, newFullName);
         if ( nbReplacements > 0 )
         {
            dialogSettings->SetAction(dialogSettings->GetAction(), argument);
            filterDesc.Set(dialogSettings);
            filter->Set(filterDesc);
            wxLogStatus(_("Filter '%s' has been updated."), filterName.c_str());
         }
         else
         {
            dialogSettings->DecRef();
         }
      }
      else
      {
         // XNOTODO: Find out how to update this filter anyway
         wxLogError(_("Filter '%s' is not \"simple\" and has not been updated."), filterName.c_str());
      }
      filter->DecRef();
   }

   /*
   // notify everybody about the disappearance of the old folder
   MEventManager::Send(
      new MEventFolderTreeChangeData (m_folderName,
                                      MEventFolderTreeChangeData::Delete)
      );
      */
   // notify everybody about the creation of the new folder
   MEventManager::Send(
      new MEventFolderTreeChangeData(newFullName,
                                     MEventFolderTreeChangeData::Create)
      );

   // Now, we can delete the old folder from the hierarchy
   Delete();

   struct MailFolderStatus status;
   MfStatusCache* cache = MfStatusCache::Get();
   rc = cache->GetStatus(m_folderName, &status);
   cache->UpdateStatus(newFullName, status);

   return true;
}

// -----------------------------------------------------------------------------
// MFolderCache implementation
// -----------------------------------------------------------------------------

wxArrayString MFolderCache::ms_aFolderNames(true /* auto sort */);
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
               _T("can't add the folder to the cache - it's already there") );

   size_t index = ms_aFolderNames.Add(folder->GetFullName());
   ms_aFolders.Insert(folder, index);
}

void MFolderCache::Remove(MFolder *folder)
{
   Check();

   // don't use name here - the folder might have been renamed
   int index = ms_aFolders.Index(folder);
   CHECK_RET( index != wxNOT_FOUND,
              _T("can't remove folder from cache because it's not in it") );

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
   int index = ms_aFolderNames.Index(oldName);
   if ( index != wxNOT_FOUND )
   {
      size_t n = (size_t)index;

      ms_aFolderNames[n] = newName;
      MFolderFromProfile *folder = (MFolderFromProfile *)ms_aFolders[n];

      folder->m_folderName = newName;
      SafeDecRef(folder->m_profile);
      folder->m_profile = Profile::CreateFolderProfile(newName);
   }
}

// ----------------------------------------------------------------------------
// MFolderTraversal
// ----------------------------------------------------------------------------

MFolderTraversal::MFolderTraversal()
{
   // leave m_folderName empty for the root folder
}

MFolderTraversal::MFolderTraversal(const MFolder& folderStart)
{
   m_folderName = folderStart.GetFullName();
}

// returns true if all folders were visited or false if traversal was aborted
bool MFolderTraversal::DoTraverse(const wxString& start, RecurseMode mode)
{
   Profile_obj profile(start);
   CHECK( profile, false, _T("panic in MFolderTraversal: no profile") );

   // enumerate all groups
   String name;
   Profile::EnumData cookie;

   wxString rootName(start);
   if ( !rootName.empty() )
      rootName += _T('/');
   //else: there should be no leading slash

   for ( bool cont = profile->GetFirstGroup(name, cookie);
         cont;
         cont = profile->GetNextGroup(name, cookie) )
   {
      wxString fullname(rootName);
      fullname += name;

      // traverse the children before/after the parent or not at all depending
      // on the mode
      if ( mode != Recurse_ChildrenFirst )
      {
         // we traverse the parent first or only the parent
         if ( !OnVisitFolder(fullname) )
            return false;

         if ( mode == Recurse_No )
            continue;
      }

      if ( !DoTraverse(fullname, mode) )
         return false;

      if ( mode == Recurse_ChildrenFirst )
      {
         if ( !OnVisitFolder(fullname) )
            return false;
      }
   }

   return true;
}

// ----------------------------------------------------------------------------
// functions to create new folders in the tree
// ----------------------------------------------------------------------------

extern MFolder *CreateFolderTreeEntry(MFolder *parent,
                                      const String& name,
                                      MFolderType folderType,
                                      long folderFlags,
                                      const String& path,
                                      bool notify)
{
   // construct the full name by prepending the parent name (if we have parent
   // and its name is non empty - only root entry is empty)
   String fullname;
   if ( parent && parent->GetType() != MF_ROOT )
   {
      fullname << parent->GetFullName() << _T('/');
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
      MFolder_obj folderParent(folder->GetParent());

      // are we're the child of the IMAP server entry?
      if ( folderParent &&
           folderParent->GetType() == MF_IMAP &&
           folderParent->GetPath().empty() )
      {
         // yes, now check if we're INBOX
         if ( path.empty() || !wxStricmp(path, _T("INBOX")) )
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

      return false;
   }

   // create folders for all files in this dir
   wxString filename;
   bool cont = dir.GetFirst(&filename, _T(""), wxDIR_FILES);
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
                             false // don't send notify event
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
   cont = dir.GetFirst(&dirname, _T(""), wxDIR_DIRS);
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
                                 false // don't send notify event
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

   return true;
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

