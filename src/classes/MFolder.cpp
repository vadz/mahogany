///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/MFolder.cpp - older related classes
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
#endif // USE_PCH

#include "MFolder.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this class adds functions to save/restore itself to/from config file.
//
// the way we store the data is pretty straightforward and quite ineffcient: the
// folders correspond to the config groups and the data is stored as the named
// values.
//
// as, of course, all this would be too nice without some dirty hacks, here is
// one: this class shouldn't have any member vars because we cast MFolder
// objects into MStorableFolders all the time - in fact, we could just as well
// define all these functions in the base (MFolder) class, but doing it here
// spares other source modules a recompilation if we change some implementation
// detail.
class MStorableFolder : public MFolder
{
public:
   // ctor & dtor
   MStorableFolder(MFolder *parent, const String& name, Type type = Invalid)
      : MFolder(parent, name, type) { }

   // save/restore folder info (the current path in config object associated to
   // the profile should be the path of our group)
   void Save(ProfileBase *profile);
   void Load(ProfileBase *profile);

   // get the path to our group (top groups path is "")
   String GetPath() const;

protected:
   // override some base class virtuals
   virtual void OnDelete();
   virtual void OnRename(const String& name);

   // helpers

   // our info only, without children
   void SaveSelf(ProfileBase *profile);
   void LoadSelf(ProfileBase *profile);

   // the children
   void SaveChildren(ProfileBase *profile);
   void LoadChildren(ProfileBase *profile);

   // the given subfolder only
   void SaveSubfolder(ProfileBase *profile, size_t n);
   void LoadSubfolder(ProfileBase *profile, const String& name);

private:
   // the value names we use to store our info
   static const char *ms_keyType;
   static const char *ms_keyFlags;
};

// this class loads everything from config when created and writes it back
// there when deleted
class MRootFolder : public MStorableFolder
{
public:
   // ctor & dtor
   MRootFolder();
   virtual ~MRootFolder();

   // operations
      // delete (recursively!) a group from config file
   void DeleteGroup(const String& path);

private:
   // path in the global profile where we store our data
   static const char *ms_foldersPath;
};

// ----------------------------------------------------------------------------
// private globals
// ----------------------------------------------------------------------------
static MRootFolder *gs_rootFolder = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MFolder
// ----------------------------------------------------------------------------

MFolder::MFolder(MFolder *parent, const String& name, Type type)
       : m_name(name)
{
   m_type = type;
   m_flags = 0;
   m_parent = parent;
}

MFolder::~MFolder()
{
   // delete all children
   size_t nCount = m_subfolders.Count();
   for ( size_t n = 0; n < nCount; n++ )
   {
      delete m_subfolders[n];
   }
}

int MFolder::GetSubfolder(const String& name) const
{
   // stupid linear search
   size_t nCount = m_subfolders.Count();
   for ( size_t n = 0; n < nCount; n++ )
   {
      if ( m_subfolders[n]->GetName().CmpNoCase(name) == 0 )
      {
         return n;
      }
   }

   return -1;
}

MFolder *MFolder::CreateSubfolder(const String& name, Type type)
{
   // first of all, check if the name is valid
   if ( GetSubfolder(name) != -1 )
   {
      wxLogError(_("Can't create subfolder '%s': folder with this name "
                   "already exists."), name.c_str());

      return NULL;
   }

   // ok, it is: do create it
   MFolder *folder = new MFolder(this, name, type);
   m_subfolders.Add(folder);

   return folder;
}

void MFolder::Delete()
{
   CHECK_RET( m_parent != NULL, "can't delete the root pseudo-folder" );

   // it's really the parent who deletes us
   int index = m_parent->GetSubfolder(m_name);
   CHECK_RET( index != -1, "we're not a subfolder of our parent??" );

   OnDelete();

   m_parent->m_subfolders.Remove((size_t)index);

   // bye-bye
   delete this;
}

void MFolder::Rename(const String& name)
{
   CHECK_RET( m_parent != NULL, "can't rename the root pseudo-folder" );

   if ( m_parent->GetSubfolder(name) != -1 )
   {
      wxLogError(_("Can't rename folder '%s' to '%s': the folder with "
                   "the new name already exists."),
                   m_name.c_str(), name.c_str());
      return;
   }

   OnRename(name);

   m_name = name;
}

// ----------------------------------------------------------------------------
// MStorableFolder
// ----------------------------------------------------------------------------

const char *MStorableFolder::ms_keyType = "Type";
const char *MStorableFolder::ms_keyFlags = "Flags";

String MStorableFolder::GetPath() const
{
   String str;
   if ( m_parent != NULL )
   {
      str << ((MStorableFolder *)m_parent)->GetPath() << '/' << m_name;
   }

   return str;
}

void MStorableFolder::OnDelete()
{
   // delete our group
   ((MRootFolder *)GetRootFolder())->DeleteGroup(GetPath());
}

void MStorableFolder::OnRename(const String& /* name */)
{
   // delete the old data - the new data will be written when we're saved anyhow
   OnDelete();
}

void MStorableFolder::Save(ProfileBase *profile)
{
   SaveSelf(profile);
   SaveChildren(profile);
}

void MStorableFolder::Load(ProfileBase *profile)
{
   LoadSelf(profile);
   LoadChildren(profile);
}

void MStorableFolder::SaveSelf(ProfileBase *profile)
{
   profile->writeEntry(ms_keyType, (long)m_type);
   profile->writeEntry(ms_keyFlags, (long)m_flags);
}

void MStorableFolder::LoadSelf(ProfileBase *profile)
{
   m_type = (MFolder::Type)profile->readEntry(ms_keyType, 0l);
   m_flags = (unsigned int)profile->readEntry(ms_keyFlags, 0l);
}

void MStorableFolder::SaveChildren(ProfileBase *profile)
{
   size_t nCount = m_subfolders.Count();
   for ( size_t n = 0; n < nCount; n++ )
   {
      SaveSubfolder(profile, n);
   }
}

void MStorableFolder::LoadChildren(ProfileBase *profile)
{
   // enum all the children and load them
   wxArrayString names;
   String folderName;
   long dummy;

   bool cont = profile->GetFirstGroup(folderName, dummy);
   while ( cont )
   {
      if ( names.Index(folderName) == NOT_FOUND )
      {
         names.Add(folderName);
         LoadSubfolder(profile, folderName);
      }
      else
      {
         // we already have such folder!
         wxLogWarning(_("Folder '%s': duplicate subfolder name '%s', "
                        "only the first subfolder loaded."),
                      m_name.c_str(), folderName.c_str());
      }

      cont = profile->GetNextGroup(folderName, dummy);
   }

   // save some memory
   m_subfolders.Shrink();
}

void MStorableFolder::SaveSubfolder(ProfileBase *profile, size_t n)
{
   // attention to the cast: the object is not really of this type, but we can
   // nevertheless do it in order to call MStorableFolders methods on it.
   MStorableFolder *folder = (MStorableFolder *)m_subfolders[n];

   // go down into the subgroup (path will be restored on scope exit)
   ProfilePathChanger changePath(profile, folder->GetName());
   folder->Save(profile);
}

void MStorableFolder::LoadSubfolder(ProfileBase *profile, const String& name)
{
   // go down into the subgroup (path will be restored on scope exit)
   ProfilePathChanger changePath(profile, name);

   MStorableFolder *folder = new MStorableFolder(this, name);
   folder->Load(profile);

   m_subfolders.Add(folder);
}

// ----------------------------------------------------------------------------
// MRootFolder
// ----------------------------------------------------------------------------

const char *MRootFolder::ms_foldersPath = "/Profiles/";

MRootFolder::MRootFolder() : MStorableFolder(NULL, "", MFolder::Root)
{
   ProfileBase *profile = mApplication->GetProfile();

   CHECK_RET( profile != NULL, "no profile to load folders from" );

   ProfilePathChanger changePath(profile, ms_foldersPath);

   LoadChildren(profile);
}

MRootFolder::~MRootFolder()
{
   ProfileBase *profile = mApplication->GetProfile();

   CHECK_RET( profile != NULL, "no profile to save folders to" );

   ProfilePathChanger changePath(profile, ms_foldersPath);

   SaveChildren(profile);
}

void MRootFolder::DeleteGroup(const String& path)
{
   ProfileBase *profile = mApplication->GetProfile();

   CHECK_RET( profile != NULL, "can't delete group - no app profile" );

   ProfilePathChanger changePath(profile, ms_foldersPath);
   profile->DeleteGroup(path);
}

// ----------------------------------------------------------------------------
// our public interface
// ----------------------------------------------------------------------------
MFolder *GetRootFolder()
{
   // we don't want to create it more than once (just think about what would
   // happen when we'd write it back)
   if ( gs_rootFolder == NULL )
   {
      gs_rootFolder = new MRootFolder;
   }

   return gs_rootFolder;
}

void DeleteRootFolder()
{
   delete gs_rootFolder;
}