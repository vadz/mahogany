///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Profile.cpp
// Purpose:     implementation of Profile &c
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin at 26.08.03 to use AllConfigSources
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2003 M-Team
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
   #pragma implementation "Profile.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Profile.h"
   #include "strutil.h"
   #include "MApplication.h"
   #include "Mdefaults.h"
   #include <wx/fileconf.h>               // for wxFileConfig
#endif // USE_PCH

#include <wx/confbase.h>

#include "lists.h"
#include "pointers.h"

#include "ConfigSourcesAll.h"
#include "ConfigPrivate.h"

#ifdef DEBUG
   #define   PCHECK() MOcheck()
#else
   #define   PCHECK()
#endif

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_PROFILE_IDENTITY;
extern const MOption MP_PROFILE_TYPE;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// a name for the empty profile, like this it is invalid for wxConfig, so it
///  will never conflict with a real profile name
#define   PROFILE_EMPTY_NAME _T("EMPTYPROFILE?(*[]{}")

/** Name for the subgroup level used for suspended profiles. Must
    never appear as part of a profile path name. */
#define SUSPEND_PATH _T("__suspended__")

/// flags for readEntry
enum
{
   /// use identity, if any
   Lookup_Identity = 1,
   /// look under parent profile if not found here
   Lookup_Parent   = 2,
   /// honour suspended status
   Lookup_Suspended = 4,
   /// default: look everywhere
   Lookup_All = 7
};

// ============================================================================
// private classes
// ============================================================================

/**
   ProfileImpl class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
   If no corresponding profile file is found, the profile will not
   have a wxConfig object associated with it but only refer to its
   parent or the global config.
   @see Profile
   @see wxConfig
*/

class ProfileImpl : public Profile
{
public:
   /// creates a normal ProfileImpl
   static Profile * CreateProfile(const String & ipathname,
                                      Profile const *Parent);
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
      /// Read a character entry.
   String readEntry(const String & key,
                    const String & defaultvalue = (const wxChar *)NULL,
                    ReadResult * found = NULL) const;
   /// Read an integer value.
   long readEntry(const String & key,
                  long defaultvalue,
                  ReadResult * found = NULL) const;
   /// read entry without recursing upwards
   virtual int readEntryFromHere(const String& key, int defvalue) const;

   /// Read anything, return true if found, falsee otherwise
   virtual bool readEntry(LookupData &ld,
                          int flags = Lookup_All) const;

   virtual bool writeEntry(const String& key, const String& value);
   virtual bool writeEntry(const String & key, long value);
   virtual bool writeEntryIfNeeded(const String& key, long value, long def);

   //@}

   virtual bool GetFirstGroup(String& s, EnumData& cookie) const;
   virtual bool GetNextGroup(String& s, EnumData& cookie) const;
   virtual bool GetFirstEntry(String& s, EnumData& cookie) const;
   virtual bool GetNextEntry(String& s, EnumData& cookie) const;

      /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const;

   virtual bool HasEntry(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual bool DeleteEntry(const String& key);
   virtual bool DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);

   virtual const String& GetName(void) const { return m_ProfileName; }

   virtual wxConfigBase *GetConfig() const { return wxConfig::Get(); }

   virtual void Suspend(void)
      {
         PCHECK();

         m_Suspended++;

         ms_suspendCount++;

         m_wroteSuspended = false;
      }

   /// Commit changes from suspended mode.
   virtual void Commit(void);
   /// Discard changes from suspended mode.
   virtual void Discard(void);
   /// Is the profile currently suspended?
   virtual bool IsSuspended(void) const { return m_Suspended != 0; }

   /** This temporarily overloads this profile with another Identity,
       i.e. the name of an Identity profile. */
   virtual void SetIdentity(const String & idName);
   virtual void ClearIdentity(void);
   virtual String GetIdentity(void) const;

   virtual bool IsAncestor(Profile *profile) const;

   String GetRootPath(void) const
   {
      return GetProfileSection();
   }

   virtual String GetFolderName() const;

protected:
   ProfileImpl()
      {
         m_Suspended = 0;
         m_Identity = NULL;
      }

   /// Destructor, writes back those entries that got changed.
   ~ProfileImpl();

   virtual const wxChar * GetProfileSection(void) const
      {
         return M_PROFILE_CONFIG_SECTION;
      }

   /// Name of this profile == path in wxConfig
   String   m_ProfileName;

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param Parent the parent profile
   */
   ProfileImpl(const String & iClassName, Profile const *Parent);

   /// get the full path of our entry/subgroup with this name in config
   String GetFullPath(const String& name) const
   {
      ASSERT_MSG( !name.empty(), _T("empty name in Profile::GetFullPath()?") );

      return GetName() + _T('/') + name;
   }

   /// common part of all writeEntry() overloads
   bool DoWriteEntry(const LookupData& data);


   /// suspend count: if positive, we're in suspend mode
   int m_Suspended;

   /// did we actually write any settings while suspended (have to undo them?)?
   bool m_wroteSuspended;

   /// Is this profile using a different Identity at present?
   ProfileImpl *m_Identity;

   /// the count of all suspended profiles: if 0, nothing is suspended
   static size_t ms_suspendCount;

   MOBJECT_DEBUG(ProfileImpl)

   DECLARE_NO_COPY_CLASS(ProfileImpl)

   GCC_DTOR_WARN_OFF
};

size_t ProfileImpl::ms_suspendCount = 0;

//@}


/**
   Identity class which is a Profile representing a single identity
   setting.
   @see Profile
   @see wxConfig
*/

class Identity : public ProfileImpl
{
public:
   static Identity * Create(const String &name)
      { return new Identity(name); }

   virtual const wxChar * GetProfileSection(void) const
      {
         return M_IDENTITY_CONFIG_SECTION;
      }
private:
   Identity(const String & name)
      {
         m_ProfileName << GetRootPath() << _T('/') << name;
      }

   DECLARE_NO_COPY_CLASS(Identity)
};

/**
   Filter profile class which is a Profile representing a single
   filter rule.
   @see Profile
   @see wxConfig
*/

class FilterProfile : public ProfileImpl
{
public:
   static FilterProfile * Create(const String &name)
      { return new FilterProfile(name); }

   virtual const wxChar * GetProfileSection(void) const
      {
         return M_FILTERS_CONFIG_SECTION;
      }
private:
   FilterProfile(const String & name)
      {
         m_ProfileName << GetRootPath() << _T('/') << name;
      }

   DECLARE_NO_COPY_CLASS(FilterProfile)
};

/// Same as Identity and FilterProfile but for "/Modules" branch
class ModuleProfile : public ProfileImpl
{
public:
   static ModuleProfile *Create(const String& name)
   {
      return new ModuleProfile(name);
   }

   virtual const wxChar *GetProfileSection() const
   {
      return _T("/Modules");
   }

private:
   ModuleProfile(const String& name)
   {
      m_ProfileName << GetRootPath() << _T('/') << name;
   }

   DECLARE_NO_COPY_CLASS(ModuleProfile)
};

/**
   Temporary profile.

   This class is a subtree of its parent profile and it deletes this subtree
   from config when it is itself deleted.
 */
class TempProfile : public ProfileImpl
{
public:
   TempProfile(Profile *parent)
   {
      if ( !parent )
         parent = mApplication->GetProfile();

      // find the first unused temporary name
      for( int n = 0; ; n++ )
      {
         m_name.Printf(_T("__Temp%d__"), n);
         if ( !parent->HasGroup(m_name) )
            break;
      }

      m_parent = parent;
      m_parent->IncRef();

      m_ProfileName << m_parent->GetName() << _T('/') << m_name;
   }

   ~TempProfile()
   {
      m_parent->DeleteGroup(m_name);
      m_parent->DecRef();
   }

private:
   Profile *m_parent;

   String m_name;
};

// ----------------------------------------------------------------------------
// module globals
// ----------------------------------------------------------------------------

// the unique AllConfigSources object
static AllConfigSources *gs_allConfigSources = NULL;


// ============================================================================
// Profile::EnumData and ProfileEnumDataImpl implementation
// ============================================================================

// ----------------------------------------------------------------------------
// EnumData
// ----------------------------------------------------------------------------

Profile::EnumData::EnumData()
{
   m_impl = new ProfileEnumDataImpl;
}

Profile::EnumData::~EnumData()
{
   delete m_impl;
}

// ----------------------------------------------------------------------------
// ProfileEnumDataImpl implementation
// ----------------------------------------------------------------------------

bool ProfileEnumDataImpl::DoGetNext(String& s, What what)
{
   for ( ;; )
   {
      if ( m_current == m_end )
      {
         // no more config sources to iterate over
         return false;
      }

      // try to get "what" from the current config source
      bool rc;
      if ( !m_started )
      {
         rc = what == Group ? m_current->GetFirstGroup(m_path, s, m_cookie)
                            : m_current->GetFirstEntry(m_path, s, m_cookie);
         m_started = true;
      }
      else // GetFirst() already called, now do GetNext()
      {
         rc = what == Group ? m_current->GetNextGroup(s, m_cookie)
                            : m_current->GetNextEntry(s, m_cookie);
      }

      if ( !rc )
      {
         // no more "what"s in this config source, pass to the next one
         ++m_current;
         m_started = false;
      }
      else // we did get something
      {
         // skip special subgroups created internally by Profile and those
         // which we had already seen before (presumably in another config
         // source)
         if ( s != SUSPEND_PATH &&
               m_namesSeen.Index(s) == wxNOT_FOUND )
         {
            m_namesSeen.Add(s);

            break;
         }
         //else: skip this one
      }
   }

   return true;
}

// ============================================================================
// Profile implementation
// ============================================================================

DEFINE_REF_COUNTER(Profile)

// ----------------------------------------------------------------------------
// Profile creation
// ----------------------------------------------------------------------------

/** This function sets profile parameters and is applied
    to all profiles directly after creation.
*/
static inline
void EnforcePolicy(Profile * /* p */)
{
   // currently we don't do anything here
}

Profile *
Profile::CreateProfile(const String & classname,
                       Profile const *parent)
{
   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != '/'));
   Profile *p =  ProfileImpl::CreateProfile(classname, parent);

   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateIdentity(const String & idName)
{
   ASSERT(idName.Length() == 0 ||  // only relative paths allowed
          (idName[0u] != '.' && idName[0u] != '/'));
   Identity *p =  Identity::Create(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateFilterProfile(const String & idName)
{
   ASSERT(idName.Length() == 0 ||  // only relative paths allowed
          (idName[0u] != '.' && idName[0u] != '/'));
   Profile *p =  FilterProfile::Create(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateModuleProfile(const String & classname, Profile const *parent)
{
   ASSERT_MSG( !parent,
               _T("CreateModuleProfile(): !NULL parent not supported") );

   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != _T('/')));
   Profile *p =  ModuleProfile::Create(classname);

   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateFolderProfile(const String& foldername)
{
   return CreateProfile(foldername);
}

Profile *
Profile::CreateTemp(Profile *parent)
{
   return new TempProfile(parent);
}

Profile *
Profile::CreateGlobalConfig(const String& filename)
{
   ASSERT_MSG( !gs_allConfigSources, _T("recreating the configs?") );

   gs_allConfigSources = AllConfigSources::Init(filename);

   Profile *p = ProfileImpl::CreateProfile(_T(""),NULL);
   EnforcePolicy(p);
   return p;
}

void
Profile::DeleteGlobalConfig()
{
   if ( gs_allConfigSources )
   {
      AllConfigSources::Cleanup();
      gs_allConfigSources = NULL;
   }
}

// ----------------------------------------------------------------------------
// Profile accessors to all existing filters/identities/...
// ----------------------------------------------------------------------------

wxArrayString Profile::GetAllFilters()
{
   return GetAllGroupsUnder(GetFiltersPath());
}

wxArrayString Profile::GetAllIdentities()
{
   return GetAllGroupsUnder(GetIdentityPath());
}

wxArrayString Profile::GetAllGroupsUnder(const String& path)
{
   wxArrayString groups;

   CHECK( gs_allConfigSources, groups,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   ProfileEnumDataImpl cookie;
   String s;
   for ( bool cont = gs_allConfigSources->GetFirstGroup(path, s, cookie);
         cont;
         cont = cookie.GetNextGroup(s) )
   {
      groups.Add(s);
   }

   return groups;
}

// ----------------------------------------------------------------------------
// other Profile methods
// ----------------------------------------------------------------------------

bool Profile::FlushAll()
{
   return gs_allConfigSources ? gs_allConfigSources->FlushAll() : true;
}

String Profile::ExpandEnvVarsIfNeeded(const String& val) const
{
   String valExp = val;
   if ( IsExpandingEnvVars() )
      valExp = wxExpandEnvVars(val);
   return valExp;
}

String
Profile::readEntry(const String& key,
                   const wxChar *defaultvalue,
                   ReadResult *found) const
{
   PCHECK();

   return readEntry(key, String(defaultvalue), found);
}


// ============================================================================
// ProfileImpl implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileImpl creation
// ----------------------------------------------------------------------------

/**
   ProfileImpl class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/
ProfileImpl::ProfileImpl(const String & iName, Profile const *Parent)
{
   // expand env vars in string values by default
   SetExpandEnvVars(true);

   m_ProfileName = ( Parent && Parent->GetName().Length())
                     ? ( Parent->GetName() + _T('/') )
                     : String(GetRootPath());
   if(iName.Length())
      m_ProfileName << _T('/') << iName;
   m_Suspended = 0;
   m_Identity = NULL;

   String id = readEntry(GetOptionName(MP_PROFILE_IDENTITY),
                         GetStringDefault(MP_PROFILE_IDENTITY));
   if ( !id && mApplication->GetProfile() )
      id = READ_APPCONFIG_TEXT(MP_CURRENT_IDENTITY);
   if ( !!id )
      SetIdentity(id);
}


Profile *
ProfileImpl::CreateProfile(const String & iClassName,
                       Profile const *parent)
{
   return new ProfileImpl(iClassName, parent);
}

Profile *
ProfileImpl::GetParent(void) const
{
   return CreateProfile(GetName().BeforeLast(_T('/')), NULL);
}

ProfileImpl::~ProfileImpl()
{
   PCHECK();

   if ( m_Suspended )
   {
      FAIL_MSG( _T("deleting a suspended profile") );

      Discard(); // but we tidy up, no big deal
   }

   if(m_Identity)
      m_Identity->DecRef();
}

// ----------------------------------------------------------------------------
// ProfileImpl entries/groups enumeration
// ----------------------------------------------------------------------------

bool ProfileImpl::GetFirstGroup(String& s, EnumData& cookie) const
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->GetFirstGroup(GetName(), s, GetEnumData(cookie));
}

bool ProfileImpl::GetNextGroup(String& s, EnumData& cookie) const
{
   PCHECK();

   return GetEnumData(cookie).GetNextGroup(s);
}

bool ProfileImpl::GetFirstEntry(String& s, EnumData& cookie) const
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->GetFirstEntry(GetName(), s, GetEnumData(cookie));
}

bool ProfileImpl::GetNextEntry(String& s, EnumData& cookie) const
{
   PCHECK();

   return GetEnumData(cookie).GetNextEntry(s);;
}

bool ProfileImpl::HasGroup(const String& name) const
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->HasGroup(GetFullPath(name));
}

bool ProfileImpl::HasEntry(const String& entry) const
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->HasEntry(GetFullPath(entry));
}

// ----------------------------------------------------------------------------
// ProfileImpl identity handling
// ----------------------------------------------------------------------------

void
ProfileImpl::SetIdentity(const String & idName)
{
   PCHECK();

   if ( m_Identity )
      ClearIdentity();

   if ( !idName.empty() )
      m_Identity = Identity::Create(idName);
}

void
ProfileImpl::ClearIdentity(void)
{
   PCHECK();

   if ( m_Identity )
   {
      m_Identity->DecRef();
      m_Identity = NULL;
   }
}

String
ProfileImpl::GetIdentity(void) const
{
   PCHECK();

   return m_Identity ? m_Identity->GetName() : String(_T(""));
}

// ----------------------------------------------------------------------------
// ProfileImpl other operations on entries
// ----------------------------------------------------------------------------

bool
ProfileImpl::Rename(const String& oldName, const String& newName)
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->Rename(GetFullPath(oldName), newName);
}

bool
ProfileImpl::DeleteEntry(const String& key)
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->DeleteEntry(GetFullPath(key));
}

bool
ProfileImpl::DeleteGroup(const String & path)
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   return gs_allConfigSources->DeleteGroup(GetFullPath(path));
}

// ----------------------------------------------------------------------------
// ProfileImpl reading data from config sources
// ----------------------------------------------------------------------------

String
ProfileImpl::readEntry(const String & key,
                       const String & def,
                       ReadResult * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found)
      *found = ld.GetFound();

   return ExpandEnvVarsIfNeeded(ld.GetString());
}

long
ProfileImpl::readEntry(const String & key, long def, ReadResult * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found)
      *found = ld.GetFound();

   return ld.GetLong();
}

int ProfileImpl::readEntryFromHere(const String& key, int defvalue) const
{
   LookupData ld(key, defvalue);
   readEntry(ld, Lookup_All & ~Lookup_Parent);

   return (int)ld.GetLong();
}

// the worker function which does all the work for both long and string data
bool
ProfileImpl::readEntry(LookupData &ld, int flags) const
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
               _T("can't call Profile methods before CreateGlobalConfig()") );

   // did we find the requested data in this profile?
   bool foundHere = false;


   // step 1: check if we havee an identity, this overrides our own settings
   if ( (flags & Lookup_Identity) && m_Identity )
   {
      foundHere = m_Identity->readEntry(ld, flags & ~Lookup_Identity);
   }


   // step 2: look in this profile itself
   if ( !foundHere )
   {
      // first look under SUSPEND_PATH: if the profile is not suspended, the
      // entry there simply doesn't exist, so it doesn't hurt, and if it is it
      // should override the "normal" entry. This is *necessary* for all
      // parent profiles because they may be suspended without us knowing
      // about it.
      //
      // there is also a micro-optimization here: now we look under
      // SUSPEND_PATH only if at least some profile is suspended, this allows
      // us to gain a lot of time during 99% of normal operation when no
      // profiles are suspended which is quite nice according to the profiling
      // (no pun intended) results.
      if ( ms_suspendCount && (flags & Lookup_Suspended) )
      {
         foundHere = gs_allConfigSources->Read(GetFullPath(SUSPEND_PATH), ld);
      }

      // now look in the usual place
      if ( !foundHere )
         foundHere = gs_allConfigSources->Read(GetName(), ld);
   }


   // step 3: recursively look in the parent profiles

   // we could recursively call ourselves but doing it "manually" is much more
   // efficient than creating a bunch of new ProfileImpl objects
   bool foundAnywhere = foundHere;
   if ( !foundHere && (flags & Lookup_Parent) )
   {
      // micro-optimization: we should stop once the path becomes equal to the
      // root path, but comparing their lengths is faster than comparing
      // strings and leads to the samee result (knowing that the root path is
      // the prefix of our path)
      const size_t lenRoot = GetRootPath().length();

      String path = GetName();
      do
      {
         path = path.BeforeLast(_T('/'));

         // we don't check parents identity as I don't think it should
         // propagate to us -- should it?


         // try suspended parent profile first: we must do this because
         // otherwise when the user edits the parent folders options and
         // applies them, the child folders wouldn't see the changes
         if ( ms_suspendCount ) // (same optimization as above)
         {
            foundAnywhere = gs_allConfigSources->
                              Read(path + _T('/') + SUSPEND_PATH, ld);
         }

         // then try the normal location
         if ( !foundAnywhere )
         {
            foundAnywhere = gs_allConfigSources->Read(path, ld);
         }
      }
      while ( !foundAnywhere && (path.length() > lenRoot) );
   }

   // did we find it at all?
   if ( !foundAnywhere )
      return false;

   // we did, also indicate where
   ld.SetFound(foundHere ? Profile::Read_FromHere : Profile::Read_FromParent);

   return true;
}

bool
ProfileImpl::DoWriteEntry(const LookupData& data)
{
   PCHECK();

   CHECK( gs_allConfigSources, false,
            _T("can't call Profile methods before CreateGlobalConfig()") );

   String path = GetName();
   if ( m_Suspended )
   {
      // set the flag telling us that we did write some suspended entries
      m_wroteSuspended = true;

      path << _T('/') << SUSPEND_PATH;
   }

   return gs_allConfigSources->Write(path, data, GetConfigSourceForWriting());
}

bool
ProfileImpl::writeEntry(const String& key, const String& value)
{
   LookupData ld(key, value);

   return DoWriteEntry(ld);
}

bool
ProfileImpl::writeEntry(const String& key, long value)
{
   LookupData ld(key, value);

   return DoWriteEntry(ld);
}

bool
ProfileImpl::writeEntryIfNeeded(const String& key, long value, long defvalue)
{
   if ( readEntry(key, defvalue) == value )
   {
      // nothing to do
      return true;
   }

   return writeEntry(key, value);
}

// ----------------------------------------------------------------------------
// ProfileImpl suspended mode support
// ----------------------------------------------------------------------------

void
ProfileImpl::Commit(void)
{
   PCHECK();

   ASSERT_MSG( m_Suspended, _T("calling Commit() without matching Suspend()") );

   if ( m_Suspended > 1 )
   {
      // don't commit yet, we remain suspended
      m_Suspended--;
      return;
   }

   if ( m_wroteSuspended )
   {
      CHECK_RET( gs_allConfigSources,
                  _T("can't call Profile methods before CreateGlobalConfig()") );

      // copy the suspended settings to their normal locations
      if ( !gs_allConfigSources->CopyGroup(GetFullPath(SUSPEND_PATH),
                                           GetName()) )
      {
         // TODO: what can we do to recover here?
         wxLogError(_("Saving previously applied settings failed."));
      }
   }
   //else: nothing to copy anyhow

   // now we just forget the suspended subgroup
   Discard();
}

void
ProfileImpl::Discard(void)
{
   PCHECK();

   CHECK_RET( m_Suspended, _T("calling Discard() without matching Suspend()") );

   if ( !--m_Suspended )
   {
      if ( m_wroteSuspended )
      {
         CHECK_RET( gs_allConfigSources,
                     _T("can't call Profile methods before CreateGlobalConfig()") );

         if ( !gs_allConfigSources->DeleteGroup(GetFullPath(SUSPEND_PATH)) )
         {
            FAIL_MSG( _T("failed to delete suspended settings") );
         }
      }
      //else: nothing to do
   }

   ASSERT_MSG( ms_suspendCount > 0, _T("suspend count broken") );

   ms_suspendCount--;
}

#ifdef DEBUG

String
ProfileImpl::DebugDump() const
{
   PCHECK();

   return String::Format(_T("%s; name = \"%s\""),
                         MObjectRC::DebugDump().c_str(), m_ProfileName.c_str());
}

#endif // DEBUG


// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(wxConfigBase *conf,
               const wxArrayString& astr,
               const String& key)
{
   // save all array entries
   conf->DeleteGroup(key);    // remove all old entries

   String path;
   path << key << _T('/');

   size_t nCount = astr.Count();
   String strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf(_T("%lu"), (unsigned long)n);
      conf->Write(path + strkey, astr[n]);
   }
}

// restores array saved by SaveArray
void RestoreArray(wxConfigBase *conf, wxArrayString& astr, const String& key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   String path;
   path << key << _T('/');

   String strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf(_T("%lu"), (unsigned long)n);
      if ( !conf->HasEntry(path+strkey) )
         break;

      strVal = conf->Read(path+strkey, _T(""));
      astr.Add(strVal);
   }
}

void SaveArray(Profile *profile, const wxArrayString& astr, const String& key)
{
   wxConfigBase *conf = profile->GetConfig();
   CHECK_RET( conf, _T("can't be used with non config based profile!") );

   conf->SetPath(profile->GetName());

   SaveArray(conf, astr, key);
}

void RestoreArray(Profile *profile, wxArrayString& astr, const String& key)
{
   wxConfigBase *conf = profile->GetConfig();
   CHECK_RET( conf, _T("can't be used with non config based profile!") );

   conf->SetPath(profile->GetName());

   RestoreArray(conf, astr, key);
}

// ----------------------------------------------------------------------------
// miscellaneous other Profile functions
// ----------------------------------------------------------------------------

// some characters are invalid in the profile name, replace them
String
Profile::FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const wxChar *aValidChars = _T("_-.");   // NOT '/' and '\\'!

   String filteredName;
   size_t len = profileName.Len();
   filteredName.Alloc(len);
   for ( size_t n = 0; n < len; n++ )
   {
      wxChar ch = profileName[n];
      if ( wxIsalnum(ch) || wxStrchr(aValidChars, ch) )
      {
         filteredName << ch;
      }
      else
      {
         // replace it -- hopefully the name will stay unique (FIXME)
         filteredName << _T('_');
      }
   }

   return filteredName;
}

bool ProfileImpl::IsAncestor(Profile *profile) const
{
   if ( !profile )
      return false;

   // can't compare as strings because '/' are sometimes duplicated...
   wxArrayString aMyComponents, aOtherComponents;
   wxSplitPath(aMyComponents, m_ProfileName);
   wxSplitPath(aOtherComponents, profile->GetName());

   if ( aOtherComponents.GetCount() < aMyComponents.GetCount() )
      return false;

   size_t count = aMyComponents.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( aOtherComponents[n] != aMyComponents[n] )
         return false;
   }

   return true;
}

String ProfileImpl::GetFolderName() const
{
   String folderName;
   if ( GetName().StartsWith(GetProfilePath(), &folderName) )
   {
      const wxChar *p = folderName.c_str();

      if ( *p )
      {
         ASSERT_MSG( *p == _T('/'), _T("profile path must start with slash") );

         // erase the slash
         folderName.erase(0, 1);
      }
      //else: leave empty
   }

   return folderName;
}
