/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "Profile.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"

#   include "Profile.h"
#   include "strutil.h"
#   include "PathFinder.h"
#   include "MApplication.h"
#   ifdef  OS_WIN
#      include <wx/msw/regconf.h>
#   else
#      include <wx/confbase.h>
#      include <wx/file.h>
#      include <wx/textfile.h>
#      include <wx/fileconf.h>
#   endif
#   include   <wx/config.h>
#endif

#include "Mdefaults.h"

#include <wx/fileconf.h>

#include <ctype.h>

#ifdef OS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// a name for the empty profile, like this it is invalid for wxConfig, so it
///  will never conflict with a real profile name
#define   PROFILE_EMPTY_NAME "EMPTYPROFILE?(*[]{}"

/** Name for the subgroup level used for suspended profiles. Must
    never appear as part of a profile path name. */
#define SUSPEND_PATH "__suspended__"

#ifdef DEBUG
   // there are too many of profile trace messages - filter them. They won't
   // appear by default, if you do want them change the wxLog::ms_ulTraceMask
   // to include wxTraceProfileClass bit
   static const int wxTraceProfileClass = 0x200;
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/// a small struct holding key and value
class LookupData
{
public:
   LookupData(const String &key, const String &def)
      {
         m_Type = LD_STRING;
         m_Str = def;
         m_Key = key;
         m_Found = false;
      }
   LookupData(const String &key, long def)
      {
         m_Type = LD_LONG;
         m_Key = key;
         m_Long = def;
         m_Found = false;
      }

   enum Type { LD_LONG, LD_STRING };

   Type GetType(void) const { return m_Type; }
   const String & GetKey(void) const { return m_Key; }
   const String & GetString(void) const
      { ASSERT(m_Type == LD_STRING); return m_Str; }
   long GetLong(void) const
      { ASSERT(m_Type == LD_LONG); return m_Long; }
   bool GetFound(void) const
      {
         return m_Found;
      }
   void SetResult(const String &str)
      {
         ASSERT(m_Type == LD_STRING);
         m_Str = str;
      }
   void SetResult(long l)
      {
         ASSERT(m_Type == LD_LONG);
         m_Long = l;
      }
   void SetFound(bool found = TRUE)
      {
         m_Found = found;
      }
private:
   Type m_Type;
   String m_Key;
   String m_Str;
   long   m_Long;
   bool   m_Found;
};

#ifdef DEBUG
#   define   PCHECK() MOcheck(); ASSERT(ms_GlobalConfig)
#else
#   define   PCHECK()
#endif


/// A dummy Profile just inheriting from the top level
class EmptyProfile : public Profile
{
public:
   /// Create a dummy Profile just inheriting from the top level
   static Profile * CreateEmptyProfile(const Profile *parent = NULL)
      { return new EmptyProfile(parent); }
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(const String & key,
                            const String & defaultvalue = (const char*)NULL,
                            bool *found = NULL) const
      {
         if(found) *found = FALSE;
         return m_Parent ?
            m_Parent->readEntry(key, defaultvalue, found)
            : defaultvalue;
      }
   /// Read an integer value.
   virtual long readEntry(const String & key,
                          long defaultvalue,
                          bool *found = NULL) const
      {
         if(found) *found = FALSE;
         return m_Parent ?
            m_Parent->readEntry(key, defaultvalue, found)
            : defaultvalue;
      }

   /// Write back the character value.
   virtual bool writeEntry(const String & key, const String & Value)
      { return FALSE ; }
   /// Write back the int value.
   virtual bool writeEntry(const String & key, long Value)
      { return FALSE; }
   //@}

   /// return true if the entry is defined
   virtual bool HasEntry(const String & key) const
      { return FALSE; }
   /// return true if the group exists
   virtual bool HasGroup(const String & name) const
      { return FALSE; }
   /// delete the entry specified by path
   virtual bool DeleteEntry(const String& key)
      { return FALSE; }
   /// delete the entry group specified by path
   virtual bool DeleteGroup(const String & path)
      { return FALSE; }
   /// rename a group
   virtual bool Rename(const String& oldName, const String& newName)
      { return FALSE; }
   /// return the name of the profile
   virtual const String GetName(void) const
      { return String(""); }

   /** @name Enumerating groups/entries
       again, this is just directly forwarded to wxConfig
   */
   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const{ return FALSE; }
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const{ return FALSE; }
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const{ return FALSE; }
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const{ return FALSE; }

   /// Returns a unique, not yet existing sub-group name: //MT!!
   virtual String GetUniqueGroupName(void) const
      { return "NOSUCHGROUP"; }

   /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const
      { return m_Parent; }
   virtual void SetPath(const String & /*path*/ ) {}
   virtual void ResetPath(void) {} ;

   /// Set temporary/suspended operation.
   virtual void Suspend(void) { };
   /// Commit changes from suspended mode.
   virtual void Commit(void) { };
   /// Discard changes from suspended mode.
   virtual void Discard(void) { };
   /// Is the profile currently suspended?
   virtual bool IsSuspended(void) const { return false; }

   /// Set the identity to be used for this profile
   virtual void SetIdentity(const String & /*idName*/) { };
   /// Unset the identity set by SetIdentity
   virtual void ClearIdentity(void) { };
   // Get the currently used identity
   virtual String GetIdentity(void) const { return ""; };

   /// is this profile a (grand) parent of the given one?
   virtual bool IsAncestor(Profile *profile) const
      {
         // if our  parent is one, then so are we
         return m_Parent ? m_Parent->IsAncestor(profile) : FALSE;
      };
private:
   Profile *m_Parent;

   EmptyProfile(const Profile *parent)
      {
         m_Parent = (Profile *) parent;
         if(m_Parent) m_Parent->IncRef();
      }
   ~EmptyProfile()
      {
         if(m_Parent) m_Parent->DecRef();
      }

   GCC_DTOR_WARN_OFF
};


const char * gs_RootPath_Identity = M_IDENTITY_CONFIG_SECTION;
const char * gs_RootPath_Profile = M_PROFILE_CONFIG_SECTION;
const char * gs_RootPath_FilterProfile = M_FILTERS_CONFIG_SECTION;

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
                    const String & defaultvalue = (const char *)NULL,
                    bool * found = NULL) const;
   /// Read an integer value.
   long readEntry(const String & key,
                  long defaultvalue,
                  bool * found = NULL) const;
   /// Read string or integer
   virtual void readEntry(LookupData &ld) const;

   /// Write back the character value.
   bool writeEntry(const String & key,
                   const String & Value);
   /// Write back the int value.
   bool writeEntry(const String & key,
                   long Value);
   //@}

   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const;
   /// Returns a unique, not yet existing sub-group name:
   virtual String GetUniqueGroupName(void) const;

      /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const;

   virtual bool HasEntry(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual bool DeleteEntry(const String& key);
   virtual bool DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);

   virtual const String GetName(void) const { return m_ProfileName;}

   virtual void SetPath(const String &path)
      {
         PCHECK();

         ASSERT_MSG( path.Length() == 0 || path[0u] != '/',
                     "only relative paths allowed" );

         m_ProfilePath = path;
      }
   virtual void ResetPath(void)
      {
         PCHECK();
         m_ProfilePath = "";
      }

   /// Set temporary/suspended operation.
   virtual void Suspend(void)
      {
         PCHECK();

         m_Suspended++;
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

   MOBJECT_DEBUG(ProfileImpl)

   virtual bool IsAncestor(Profile *profile) const;

   virtual const char * GetRootPath(void) const
      {
         return gs_RootPath_Profile;
      }

protected:
   ProfileImpl()
      {
         m_Suspended = 0;
         m_Identity = NULL;
      }
   /// Destructor, writes back those entries that got changed.
   ~ProfileImpl();

   /// Name of this profile == path in wxConfig
   String   m_ProfileName;

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   ProfileImpl(const String & iClassName, Profile const *Parent);
   /// If not empty, temporarily modified path for this profile.
   String   m_ProfilePath;
   GCC_DTOR_WARN_OFF
   /// suspend count: if positive, we're in suspend mode
   int m_Suspended;
   /// Is this profile using a different Identity at present?
   Profile * m_Identity;
};
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
   static Profile * CreateIdentity(const String &name)
      { return new Identity(name); }

   virtual const char * GetRootPath(void) const
      {
         return gs_RootPath_Identity;
      }
private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   Identity(const String & name)
      {
         m_ProfileName = GetRootPath();
         m_ProfileName << '/' << name;
      }
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
   static FilterProfile * CreateFilterProfile(const String &name)
      { return new FilterProfile(name); }

   virtual const char * GetRootPath(void) const
      {
         return gs_RootPath_FilterProfile;
      }
private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   FilterProfile(const String & name)
      {
         m_ProfileName = GetRootPath();
         m_ProfileName << '/' << name;
      }
};




// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

void Profile::FlushAll()
{
   ASSERT(ms_GlobalConfig);

   ms_GlobalConfig->Flush();
}

bool Profile::IsExpandingEnvVars() const
{
   PCHECK();
   return ms_GlobalConfig->IsExpandingEnvVars();
}

void Profile::SetExpandEnvVars(bool bDoIt)
{
   PCHECK();
   ms_GlobalConfig->SetExpandEnvVars(bDoIt);
}

/// does a profile/config group with this name exist?
/* static */
bool
Profile::ProfileExists(const String &iName)
{
   ms_GlobalConfig->SetPath("");
   String name = M_PROFILE_CONFIG_SECTION;
   name << '/' << iName;
   return ms_GlobalConfig->HasGroup(name);
}

/** List all profiles of a given type or all profiles in total.
    @param type Type of profile to list or PT_Any for all.
    @return a pointer to kbStringList of profile names to be freed by caller.
*/
static void
ListProfilesHelper(wxConfigBase *config,
                   kbStringList *list,
                   int type,
                   String const &path)
{
   wxString oldpath = config->GetPath();
   config->SetPath(path);

   int ptype;

   // these variables will be filled by GetFirstGroup/GetNextGroup
   long index = 0;
   wxString name;

   bool ok = config->GetFirstGroup(name, index);
   while ( ok )
   {
      config->Read(name + '/' + MP_PROFILE_TYPE, &ptype, MP_PROFILE_TYPE_D);
      wxString pathname = path;
      pathname << '/' << name;
      if(type == Profile::PT_Any || ptype == type)
         list->push_back(new String(pathname));
      ListProfilesHelper(config, list, type, pathname);

      ok = config->GetNextGroup (name, index);
   }

   // restore path
   config->SetPath(oldpath);
}

kbStringList *
Profile::ListProfiles(int type)
{
   ASSERT(ms_GlobalConfig);
   kbStringList *list = new kbStringList;
   if( ms_GlobalConfig )
   {
      // may be give a (debug) warning here?
      ListProfilesHelper(ms_GlobalConfig, list, type, "");
   }

   return list;
}


//FIXME:MT calling wxWindows from possibly non-GUI code
bool ProfileImpl::GetFirstGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   bool success = ms_GlobalConfig->GetFirstGroup(s, l);
   if(success && s == SUSPEND_PATH)
      return GetNextGroup(s,l);
   else
      return success;
}

bool ProfileImpl::GetNextGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   bool success = ms_GlobalConfig->GetNextGroup(s, l);
   while(success && s == SUSPEND_PATH)
      success = ms_GlobalConfig->GetNextGroup(s, l);
   return success;
}

String ProfileImpl::GetUniqueGroupName(void) const
{
   PCHECK();
   String name; // We use hex numbers
   unsigned long number = 0;
   for(;;)
   {
      name.Printf("%lX", number);
      if(HasGroup(name))
         number++;
      else
         return name;
   }
   wxASSERT(0); // must not happen!
   return "--outOfBounds--";
}


bool ProfileImpl::GetFirstEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetFirstEntry(s, l);
}

bool ProfileImpl::GetNextEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetNextEntry(s, l);
}


void
ProfileImpl::SetIdentity(const String & idName)
{
   PCHECK();
   if(m_Identity) ClearIdentity();
   m_Identity = Profile::CreateIdentity(idName);
}
void
ProfileImpl::ClearIdentity(void)
{
   PCHECK();
   if(m_Identity) m_Identity->DecRef();
   m_Identity = NULL;
}

String
ProfileImpl::GetIdentity(void) const
{
   PCHECK();
   return m_Identity ? m_Identity->GetName() : String("");
}

wxArrayString Profile::GetAllFilters()
{
   return GetAllGroupsUnder(M_FILTERS_CONFIG_SECTION);
}

wxArrayString Profile::GetAllIdentities()
{
   return GetAllGroupsUnder(M_IDENTITY_CONFIG_SECTION);
}

wxArrayString Profile::GetAllGroupsUnder(const String& path)
{
   wxArrayString identities;

   wxString oldpath = ms_GlobalConfig->GetPath();
   ms_GlobalConfig->SetPath(path);

   long index;
   wxString name;
   bool cont = ms_GlobalConfig->GetFirstGroup(name, index);
   while ( cont )
   {
      if ( name != SUSPEND_PATH )
      {
         identities.Add(name);
      }

      cont = ms_GlobalConfig->GetNextGroup(name, index);
   }

   ms_GlobalConfig->SetPath(oldpath);

   return identities;
}

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

wxConfigBase * Profile::ms_GlobalConfig;

/** This function sets profile parameters and is applied
    to all profiles directly after creation.
*/
inline static
void EnforcePolicy(Profile *p)
{
   p->SetExpandEnvVars(true);
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
   Profile *p =  Identity::CreateIdentity(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateFilterProfile(const String & idName)
{
   ASSERT(idName.Length() == 0 ||  // only relative paths allowed
          (idName[0u] != '.' && idName[0u] != '/'));
   Profile *p =  FilterProfile::CreateFilterProfile(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateModuleProfile(const String & classname, Profile const *parent)
{
   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != '/'));
   String newName = "Modules/" + classname;
   Profile *p =  ProfileImpl::CreateProfile(newName, parent);

   EnforcePolicy(p);
   return p;
}


Profile *
Profile::CreateEmptyProfile(Profile const *parent)
{
   Profile *p =  EmptyProfile::CreateEmptyProfile(parent);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateGlobalConfig(const String & filename)
{
   ASSERT( ! strutil_isempty(filename) );
#  ifdef OS_WIN
   // don't give explicit name, but rather use the default logic (it's
   // perfectly ok, for the registry case our keys are under
   // vendor\appname)

   ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  "", "",
                                  wxCONFIG_USE_LOCAL_FILE |
                                  wxCONFIG_USE_GLOBAL_FILE);
#  else  // Unix
   // look for the global config file in the following places in order:
   //    1. compile-time specified installation dir
   //    2. run-time specified installation dir
   //    3. default installation dir
   String globalFileName, globalFile;
   globalFileName << '/' << M_APPLICATIONNAME << ".conf";
   globalFile = String(M_PREFIX) + globalFileName;
   if ( !wxFileExists(globalFile) )
   {
      const char *dir = getenv("MAHOGANY_DIR");
      if ( dir )
      {
         globalFile = String(dir) + globalFileName;
      }
   }
   if ( !wxFileExists(globalFile) )
   {
      // wxConfig will look for it in the default location(s)
      globalFile = globalFileName;
   }

   // we don't need the config file manager for this profile
   ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  filename, globalFile,
                                  wxCONFIG_USE_LOCAL_FILE|
                                  wxCONFIG_USE_GLOBAL_FILE);

   // don't give the others even read access to our config file as it stores,
   // among other things, the passwords
   ((wxFileConfig *)ms_GlobalConfig)->SetUmask(0077);
#  endif // Unix/Windows
   Profile *p = ProfileImpl::CreateProfile("",NULL);
   EnforcePolicy(p);
   return p;
}

void
Profile::DeleteGlobalConfig()
{
   if ( ms_GlobalConfig )
      delete ms_GlobalConfig;
}

String
Profile::readEntry(const String & key,
                       const char *defaultvalue,
                       bool * found) const
{
   PCHECK();
   String str;
   str = readEntry(key, String(defaultvalue), found);
   return str;
}

// ----------------------------------------------------------------------------
// ProfileImpl
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
   m_ProfileName = ( Parent && Parent->GetName().Length())
                     ? ( Parent->GetName() + '/' )
                     : String(GetRootPath());
   if(iName.Length())
      m_ProfileName << '/' << iName;
   m_Suspended = 0;
   m_Identity = NULL;

   String id = readEntry(MP_PROFILE_IDENTITY, MP_PROFILE_IDENTITY_D);
   if ( !id && mApplication->GetProfile() )
      id = READ_APPCONFIG(MP_CURRENT_IDENTITY);
   if ( !!id )
      SetIdentity(id);
}


Profile *
ProfileImpl::CreateProfile(const String & iClassName,
                       Profile const *parent)
{
   return new ProfileImpl(iClassName, parent);
}

ProfileImpl::~ProfileImpl()
{
   PCHECK();
   ASSERT(this != mApplication->GetProfile());

   if ( m_Suspended )
   {
      FAIL_MSG( "deleting a suspended profile" );

      Discard(); // but we tidy up, no big deal
   }

   if(m_Identity) m_Identity->DecRef();
}

Profile *
ProfileImpl::GetParent(void) const
{
   return CreateProfile(GetName().BeforeLast('/'), NULL);
}

bool
ProfileImpl::HasGroup(const String & name) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasGroup(name);
}

bool
ProfileImpl::HasEntry(const String & key) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasEntry(key) || ms_GlobalConfig->HasGroup(key);
}

bool
ProfileImpl::Rename(const String& oldName, const String& newName)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->RenameGroup(oldName, newName);
}

bool
ProfileImpl::DeleteEntry(const String& key)
{
   PCHECK();

   String root = GetName();
   if ( !m_ProfilePath.IsEmpty() )
       root  << '/' << m_ProfilePath;
   ms_GlobalConfig->SetPath(root);
   return ms_GlobalConfig->DeleteEntry(key);
}

bool
ProfileImpl::DeleteGroup(const String & path)
{
   PCHECK();

   String root = GetName();
   if ( !m_ProfilePath.IsEmpty() )
       root  << '/' << m_ProfilePath;
   ms_GlobalConfig->SetPath(root);
   return ms_GlobalConfig->DeleteGroup(path);
}

String
ProfileImpl::readEntry(const String & key, const String & def,
                       bool * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found) *found = ld.GetFound();

   // normally wxConfig does this for us, but if we didn't find the entry at
   // all, it won't happen, so do it here
   return ms_GlobalConfig->ExpandEnvVars(ld.GetString());
}

long
ProfileImpl::readEntry(const String & key, long def, bool * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found) *found = ld.GetFound();
   return ld.GetLong();
}

// Notice that we always look first under SUSPEND_PATH: if the profile is not
// in suspend mode, the entry there simply doesn't exist, so it doesn't harm,
// and if it is it should override the "normal" entry. This is *necessary* for
// all parent profiles because they may be suspended without us knowing about
// it.

void
ProfileImpl::readEntry(LookupData &ld) const
{
   PCHECK();

   String pathProfile = GetName();

   if( m_ProfilePath.length() )
      pathProfile << '/' << m_ProfilePath;

   ms_GlobalConfig->SetPath(pathProfile);

   String keySuspended;
   keySuspended << SUSPEND_PATH << '/' << ld.GetKey();

   // the value read from profile
   String strResult;
   long   longResult;

   bool foundHere;
   if( ld.GetType() == LookupData::LD_STRING )
      foundHere = ms_GlobalConfig->Read(keySuspended, &strResult, ld.GetString());
   else
      foundHere = ms_GlobalConfig->Read(keySuspended, &longResult, ld.GetLong());

   if ( !foundHere )
   {
      if( ld.GetType() == LookupData::LD_STRING )
         foundHere = ms_GlobalConfig->Read(ld.GetKey(), &strResult,
                                           ld.GetString());
      else
         foundHere = ms_GlobalConfig->Read(ld.GetKey(), &longResult,
                                           ld.GetLong());
   }

   // if we don't have our own setting, check for identity override before
   // quering the parent
   if ( !foundHere && m_Identity )
   {
      // try suspended path first:
      bool idFound = FALSE;
      if( ld.GetType() == LookupData::LD_STRING )
         strResult = m_Identity->readEntry(keySuspended, ld.GetString(),
                                           &idFound);
      else
         strResult = m_Identity->readEntry(keySuspended, ld.GetLong(),
                                           &idFound);
      if(! idFound)
      {
         if( ld.GetType() == LookupData::LD_STRING )
            strResult = m_Identity->readEntry(ld.GetKey(), ld.GetString(), &idFound);
         else
            longResult = m_Identity->readEntry(ld.GetKey(), ld.GetLong(), &idFound);
      }
      if(idFound)
      {
         if(ld.GetType() ==  LookupData::LD_STRING)
            ld.SetResult(strResult);
         else
            ld.SetResult(longResult);
         ld.SetFound(FALSE);
         return;
      }

      // restore the global path possibly changed by readEntry() calls above
      ms_GlobalConfig->SetPath(pathProfile);
   }

   bool foundAnywhere = foundHere;
   while ( !foundAnywhere &&
           (ms_GlobalConfig->GetPath() != GetRootPath()) )
   {
      ms_GlobalConfig->SetPath("..");
      // try suspended global profile first:
      if( ld.GetType() == LookupData::LD_STRING )
      {
         foundAnywhere = ms_GlobalConfig->Read(keySuspended,
                                               &strResult,
                                               ld.GetString());
         if ( !foundAnywhere )
            foundAnywhere = ms_GlobalConfig->Read(ld.GetKey(), &strResult,
                                                  ld.GetString());
      }
      else
      {
         foundAnywhere = ms_GlobalConfig->Read(keySuspended,
                                               &longResult,
                                               ld.GetLong());
         if ( !foundAnywhere )
            foundAnywhere = ms_GlobalConfig->Read(ld.GetKey(), &longResult,
                                                  ld.GetLong());
      }
   }
   if(foundAnywhere)
   {
      if( ld.GetType() == LookupData::LD_STRING )
         ld.SetResult(strResult);
      else
         ld.SetResult(longResult);
   }
   ld.SetFound(foundHere);
}

bool
ProfileImpl::writeEntry(const String & key, const String & value)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_Suspended)
      keypath << SUSPEND_PATH << '/';
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, value);
}

bool
ProfileImpl::writeEntry(const String & key, long value)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_Suspended)
      keypath << SUSPEND_PATH << '/';
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, (long) value);
}


static void CommitGroup(wxConfigBase *cfg, String truePath, String suspPath)
{
   String strValue;
   long   numValue;

   long index;
   String name;

   cfg->SetPath(suspPath);

   bool cont = cfg->GetFirstEntry(name, index);
   while(cont)
   {
      switch ( cfg->GetEntryType(name) )
      {
         case wxConfig::Type_String:
            if ( !cfg->Read(name, &strValue) ||
                 !cfg->Write(truePath + name, strValue) )
            {
               FAIL_MSG("failed to copy config entry");
            }
            break;

         case wxConfig::Type_Integer:
            if ( !cfg->Read(name, &numValue) ||
                 !cfg->Write(truePath + name, numValue) )
            {
               FAIL_MSG("failed to copy config entry");
            }
            break;

         default:
            FAIL_MSG("unsupported config entry type");
      }

      cont = cfg->GetNextEntry(name, index);
   }


   cont = cfg->GetFirstGroup(name, index);
   while(cont)
   {
      CommitGroup(cfg, truePath + name + "/", suspPath + name + "/");
      cfg->SetPath(suspPath);
      cont = cfg->GetNextGroup(name, index);
   }
}


void
ProfileImpl::Commit(void)
{
   PCHECK();

   if ( !m_Suspended )
   {
      // as we don't provide IsSuspended() function, just silently ignore this
      // call instead of giving an error
      return;
   }
   
   String truePath = GetName();
   truePath << '/';

   if( m_ProfilePath.length() > 0 )
      truePath << m_ProfilePath << '/';

   String suspPath = truePath;
   suspPath << SUSPEND_PATH << '/';
   ms_GlobalConfig->SetPath(suspPath);

   CommitGroup(ms_GlobalConfig, truePath, suspPath);

   Discard(); // now we just forget the suspended sub-group
}

void
ProfileImpl::Discard(void)
{
   PCHECK();

   if ( !m_Suspended )
   {
      // as we don't provide IsSuspended() function, just silently ignore this
      // call instead of giving an error
      return;
   }

   if ( !--m_Suspended )
   {
      String path;
      ms_GlobalConfig->SetPath(GetName());
      if( m_ProfilePath.length() > 0 )
         path << m_ProfilePath << '/';
      path << SUSPEND_PATH;
#ifdef DEBUG
      bool success =
#endif
         ms_GlobalConfig->DeleteGroup(path);
      ASSERT(success);
   }
}

#ifdef DEBUG
String
ProfileImpl::DebugDump() const
{
   PCHECK();

   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("name '%s'", m_ProfileName.c_str());

   return s1 + s2;
}

#endif





// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(Profile * conf, const wxArrayString& astr, const String & key)
{
   // save all array entries
   conf->DeleteGroup(key);    // remove all old entries

   String path;
   path << key << '/';

   size_t nCount = astr.Count();
   String strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf("%d", n);
      conf->writeEntry(path+strkey, astr[n]);
   }
}

// restores array saved by SaveArray
void RestoreArray(Profile * conf, wxArrayString& astr, const String & key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   String path;
   path << key << '/';

   String strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf("%d", n);
      if ( !conf->HasEntry(path+strkey) )
         break;
      strVal = conf->readEntry(path+strkey, "");
      astr.Add(strVal);
   }
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// some characters are invalid in the profile name, replace them
String
Profile::FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const char *aValidChars = "_-.";   // NOT '/' and '\\'!

   String filteredName;
   size_t len = profileName.Len();
   filteredName.Alloc(len);
   for ( size_t n = 0; n < len; n++ )
   {
      char ch = profileName[n];
      if ( isalnum(ch) || strchr(aValidChars, ch) )
      {
         filteredName << ch;
      }
      else
      {
         // replace it -- hopefully the name will stay unique (FIXME)
         filteredName << '_';
      }
   }

   return filteredName;
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

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

