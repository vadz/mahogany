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

   virtual FolderType GetType() const;

   virtual String GetComment() const;
   virtual void SetComment(const String& comment);

   virtual int GetFlags() const;
   virtual void SetFlags(int flags);

   virtual size_t GetSubfolderCount() const;
   virtual MFolder *GetSubfolder(size_t n) const;
   virtual MFolder *GetSubfolder(const String& name) const;
   virtual MFolder *GetParent() const;

   virtual MFolder *CreateSubfolder(const String& name, FolderType type);
   virtual void Delete();
   virtual bool Rename(const String& newName);

protected:
   // helpers
      // common part of Create() and Exists()
   static bool GroupExists(ProfileBase *profile, const String& fullname);
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
   virtual FolderType GetType() const { return FolderRoot; }

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

   return folder;
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
   // remove the object being deleted (us) from the cache
   MFolderCache::Remove(this);
}

bool
MFolderFromProfile::GroupExists(ProfileBase *profile, const String& fullname)
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
   Profile_obj profile(fullname);
   CHECK( profile, FALSE, "panic in MFolder: no profile" );

   bool exists = (READ_CONFIG(profile, MP_PROFILE_TYPE) ==
                  ProfileBase::PT_FolderProfile);

   return exists;
}

bool MFolderFromProfile::Create(const String& fullname)
{
   Profile_obj profile(fullname);
   CHECK( profile, FALSE, "panic in MFolder: no profile" );

   //PFIXME
   if ( GroupExists(profile, fullname) )
   {
      wxLogError(_("Cannot create profile folder '%s' because a group "
                   "with this name already exists."), fullname.c_str());

      return FALSE;
   }

   profile->writeEntry(MP_PROFILE_TYPE, ProfileBase::PT_FolderProfile);

   return TRUE;
}

String MFolderFromProfile::GetName() const
{
   return m_folderName.AfterLast('/');
}

FolderType MFolderFromProfile::GetType() const
{
   Profile_obj profile(m_folderName);
   CHECK( profile, FolderInvalid, "panic in MFolder: no profile" );

   FolderType t = GetFolderType(READ_CONFIG(profile, MP_FOLDER_TYPE));

   return t;
}

String MFolderFromProfile::GetComment() const
{
   Profile_obj profile(m_folderName);
   CHECK( profile, "", "panic in MFolder: no profile" );

   String str = READ_CONFIG(profile, MP_FOLDER_COMMENT);

   return str;
}

void MFolderFromProfile::SetComment(const String& comment)
{
   Profile_obj profile(m_folderName);
   CHECK_RET( profile, "panic in MFolder: no profile" );

   profile->writeEntry(MP_FOLDER_COMMENT, comment);
}

int MFolderFromProfile::GetFlags() const
{
   Profile_obj profile(m_folderName);
   CHECK( profile, FolderInvalid, "panic in MFolder: no profile" );

   int f = GetFolderFlags(READ_CONFIG(profile, MP_FOLDER_TYPE));

   return f;
}

void MFolderFromProfile::SetFlags(int flags)
{
   Profile_obj profile(m_folderName);
   CHECK_RET( profile, "panic in MFolder: no profile" );

   int typeAndFlags = READ_CONFIG(profile, MP_FOLDER_TYPE);
   typeAndFlags = CombineFolderTypeAndFlags(GetFolderType(typeAndFlags),
                                            flags);
   profile->writeEntry(MP_FOLDER_TYPE, typeAndFlags);
}

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

size_t MFolderFromProfile::GetSubfolderCount() const
{

  CountTraversal  count(this);
  // traverse but don't recurse into subfolders
  count.Traverse(FALSE);

  return count.GetCount();
}

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

MFolder *MFolderFromProfile::GetSubfolder(size_t n) const
{
   IndexTraversal  index(this, n);
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
   return MFolder::Create(GetSubFolderFullName(name), type);
}

void MFolderFromProfile::Delete()
{
   CHECK_RET( !m_folderName.IsEmpty(), "can't delete the root pseudo-folder" );

   // Get parent profile:
   Profile_obj profile(m_folderName.BeforeLast('/'));
   CHECK_RET( profile, "panic in MFolder: no profile" );

   profile->DeleteGroup(GetName());

   // notify everybody about the disappearance of the folder
   MEventManager::Send(
      new MEventFolderTreeChangeData (GetFullName(),
                                      MEventFolderTreeChangeData::Delete)
      );
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
      wxLogError(_("Cannot rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_folderName.c_str(), newName.c_str());

      return FALSE;
   }


   Profile_obj profile(path);
   CHECK( profile, FALSE, "panic in MFolder: no profile" );
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
   if ( !rootName.IsEmpty() )
      rootName += '/';
   //else: there should be no leading slash

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
         }
      }

      cont = profile->GetNextGroup(name, dummy);
   }

   return TRUE;
}
