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

#  include <wx/confbase.h>    // for wxSplitPath
#  include <wx/dynarray.h>
#endif // USE_PCH

#include "Mdefaults.h"

#include "MFolder.h"

// ----------------------------------------------------------------------------
// template classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(MFolder *, wxArrayFolder);

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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
   MFolderFromProfile(const String& folderName) : MFolder()
      { m_folderName = folderName; }
      // dtor is trivial
   virtual ~MFolderFromProfile();

   // static functions
      // return TRUE if the folder with given name exists
   static bool Exists(const String& fullname);
      // create a folder in the profile, shouldn't be called if it already
      // exists - will return FALSE in this case.
   static bool Create(const String& fullname);

   // implement base class pure virtuals
   virtual String GetName() const;
   virtual wxString GetFullName() const { return m_folderName; }

   virtual Type GetType() const;

   virtual String GetComment() const;
   virtual void SetComment(const String& comment);

   virtual unsigned int GetFlags() const;
   virtual void SetFlags(unsigned int flags);

   virtual size_t GetSubfolderCount() const;
   virtual MFolder *GetSubfolder(size_t n) const;
   virtual MFolder *GetSubfolder(const String& name) const;
   virtual MFolder *GetParent() const;

   virtual MFolder *CreateSubfolder(const String& name, Type type);
   virtual void Delete();
   virtual bool Rename(const String& newName);

protected:
   // helpers
      // common part of Create() and Exists()
   static bool GroupExists(ProfileBase *profile, const String& fullname);
      // get the full name of the subfolder
   wxString GetSubFolderFullName(const String& name) const
   {
      String fullname;
      fullname << m_folderName << '/' << name;

      return fullname;
   }

private:
   String m_folderName;    // the full folder name
};

// a special case of MFolderFromProfile: the root folder
class MRootFolderFromProfile : public MFolderFromProfile
{
public:
   // ctor
   MRootFolderFromProfile() : MFolderFromProfile("") { }

   // implement base class pure virtuals (some of them don't make sense to us)
   virtual Type GetType() const { return MFolder::Root; }

   virtual String GetComment() const { return ""; }
   virtual void SetComment(const String& /* comment */)
      { FAIL_MSG("can not set root folder attributes."); }

   virtual unsigned int GetFlags() const { return 0u; }
   virtual void SetFlags(unsigned int /* flags */)
      { FAIL_MSG("can not set root folder attributes."); }

   virtual MFolder *GetParent() const { return NULL; }

   virtual void Delete()
      { FAIL_MSG("can not delete root folder."); }
   virtual bool Rename(const String& /* newName */)
      { FAIL_MSG("can not rename root folder."); return FALSE; }
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
      if ( name.IsEmpty() )
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

MFolder *MFolder::Create(const String& fullname, Type type)
{
   MFolder *folder = Get(fullname);
   if ( folder )
   {
      wxLogError(_("Can not create a folder '%s' which already exists."),
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

   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, NULL, "panic in MFolder: no app profile" );

   FolderPathChanger pathChanger(profile, folder->GetFullName());
   profile->writeEntry(MP_FOLDER_TYPE, type);

   return folder;
}

#ifdef DEBUG

String MFolder::Dump() const
{
   String str;
   str.Printf("MFolder: name = %s, %d refs", GetFullName().c_str(), m_nRef);

   return str;
}

#endif // DEBUG

// -----------------------------------------------------------------------------
// MFolderFromProfile
// -----------------------------------------------------------------------------

MFolderFromProfile::~MFolderFromProfile()
{
   // remove the object being deleted (us) from the cache
   MFolderCache::Remove(this);
}

bool
MFolderFromProfile::GroupExists(ProfileBase *profile, const String& fullname)
{
   // split path into '/' separated components
   wxArrayString components;
   wxSplitPath(components, fullname);

   FolderPathChanger changePath(profile, "");

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
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   if ( !GroupExists(profile, fullname) )
      return FALSE;

   // the profile type field must be set for the folder profile group
   FolderPathChanger changePath(profile, fullname);

   return READ_CONFIG(profile, MP_PROFILE_TYPE) == ProfileBase::PT_FolderProfile;
}

bool MFolderFromProfile::Create(const String& fullname)
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   if ( GroupExists(profile, fullname) )
   {
      wxLogError(_("Can not create profile folder '%s' because a group "
                   "with this name already exists."), fullname.c_str());

      return FALSE;
   }

   // this will create everything automatically
   FolderPathChanger changePath(profile, fullname);
   profile->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);

   return TRUE;
}

String MFolderFromProfile::GetName() const
{
   return m_folderName.AfterLast('/');
}

MFolder::Type MFolderFromProfile::GetType() const
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, MFolder::Invalid, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);
   return (Type)READ_CONFIG(profile, MP_FOLDER_TYPE);
}

String MFolderFromProfile::GetComment() const
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, "", "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);
   return READ_CONFIG(profile, MP_FOLDER_COMMENT);
}

void MFolderFromProfile::SetComment(const String& comment)
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK_RET( profile != NULL, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);
   profile->writeEntry(MP_FOLDER_COMMENT, comment);
}

unsigned int MFolderFromProfile::GetFlags() const
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);
   return (unsigned int)READ_CONFIG(profile, MP_FOLDER_FLAGS);
}

void MFolderFromProfile::SetFlags(unsigned int flags)
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK_RET( profile != NULL, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);
   profile->writeEntry(MP_FOLDER_FLAGS, flags);
}

size_t MFolderFromProfile::GetSubfolderCount() const
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);

   // enumerate all groups
   String name;
   long dummy;
   size_t count = 0;

   bool cont = profile->GetFirstGroup(name, dummy);
   while ( cont )
   {
      // check that it's a folder - this is the reason why we can't call
      // GetGroupCount() directly
      {
         ProfilePathChanger changePath2(profile, name);
         if ( READ_CONFIG(profile, MP_PROFILE_TYPE) ==
                  ProfileBase::PT_FolderProfile )
         {
            count++;
         }
      }

      cont = profile->GetNextGroup(name, dummy);
   }

   return count;
}

MFolder *MFolderFromProfile::GetSubfolder(size_t n) const
{
   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);

   // count until the group we need by enumerating them
   wxString name;
   long dummy;
   bool cont = profile->GetFirstGroup(name, dummy);
   for ( cont = profile->GetFirstGroup(name, dummy);
         cont;
         cont = profile->GetNextGroup(name, dummy) )
   {
      // check that it's a folder - this is the reason why we can't call
      // GetGroupCount() directly
      {
         ProfilePathChanger changePath2(profile, name);
         if ( READ_CONFIG(profile, MP_PROFILE_TYPE) !=
                  ProfileBase::PT_FolderProfile )
         {
            // skip "n--" - it doesn't count as a folder
            continue;
         }
      }

      if ( n-- == 0 )
         break;
   }

   CHECK( cont, NULL, "invalid subfolder index" );

   return Get(GetSubFolderFullName(name));
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

MFolder *MFolderFromProfile::CreateSubfolder(const String& name, Type type)
{
   // first of all, check if the name is valid
   MFolder *folder = GetSubfolder(name);
   if ( folder )
   {
      wxLogError(_("Can't create subfolder '%s': folder with this name "
                   "already exists."), name.c_str());

      folder->DecRef();

      return NULL;
   }

   // ok, it is: do create it
   return MFolder::Create(GetSubFolderFullName(name), type);
}

void MFolderFromProfile::Delete()
{
   CHECK_RET( !m_folderName.IsEmpty(), "can't delete the root pseudo-folder" );

   ProfileBase *profile = mApplication->GetProfile();
   CHECK_RET( profile != NULL, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, m_folderName);

   profile->SetPath("..");
   profile->DeleteGroup(GetName());

   // notify everybody about the disappearance of the folder
   EventFolderTreeChangeData event(GetFullName(),
                                   EventFolderTreeChangeData::Delete);
   EventManager::Send(event);
}

bool MFolderFromProfile::Rename(const String& newName)
{
   CHECK( !m_folderName.IsEmpty(), FALSE, "can't rename the root pseudo-folder" );

   if ( GetType() == Inbox )
   {
      wxLogError(_("INBOX folder is a special folder used by the mail "
                   "system and can not be renamed."));

      return FALSE;
   }

   String path = m_folderName.BeforeLast('/'),
          name = m_folderName.AfterLast('/');

   String newFullName;
   newFullName << path << '/' << newName;

   if ( Exists(newFullName) )
   {
      wxLogError(_("Can't rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_folderName.c_str(), newName.c_str());

      return FALSE;
   }

   ProfileBase *profile = mApplication->GetProfile();
   CHECK( profile != NULL, FALSE, "panic in MFolder: no app profile" );

   FolderPathChanger changePath(profile, path);
   if ( profile->Rename(name, newName) )
   {
      m_folderName = newFullName;

      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

// -----------------------------------------------------------------------------
// MFolderCache implementation
// -----------------------------------------------------------------------------

wxArrayString MFolderCache::ms_aFolderNames;
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

   ms_aFolderNames.Add(folder->GetFullName());
   ms_aFolders.Add(folder);
}

void MFolderCache::Remove(MFolder *folder)
{
   Check();

   // don't use name here - the folder might have been renamed
   int index = ms_aFolders.Index(folder);
   CHECK_RET( index != wxNOT_FOUND,
              "can't remove folder from cache because it's not in it" );

   ms_aFolderNames.Remove((size_t)index);
   ms_aFolders.Remove((size_t)index);
}
