/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
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
#   include "kbList.h"
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

#ifdef DEBUG
   // there are t many of profile trace messages - filter them. They won't
   // appear by default, if you do want them change the wxLog::ms_ulTraceMask
   // to include wxTraceProfileClass bit
   static const int wxTraceProfileClass = 0x200;
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/** ConfigFileManager class, this class allocates and deallocates
    wxFileConfig objects for the profile so to ensure that every config
    file gets opened only once.
*/
class ConfigFileManager
{
private:
   class FCDataList *fcList;

public:
   /** Constructor
     */
   ConfigFileManager();
   /** Destructor
       writes back all entries
   */
   ~ConfigFileManager();

   /**
        Flushes all profiles, i.e. writes them to disk.
   */
   void Flush();

   /**  Finds a profile if it already exists
        @param fileName name of configuration file
    */
   ProfileBase *Find(const String & fileName);

   /** registers a profile object for reuse
       @param fileName name of configuration file
       @param prof pointer to existing profile
    */
   void Register(const String & fileName, ProfileBase *prof);

   /** deregisters a profile object for reuse when it no longer exists
       @param prof pointer to profile
    */
   void DeRegister(ProfileBase *prof);

   /// Prints a list of all entries.
   DEBUG_DEF
};

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
   If no corresponding profile file is found, the profile will not
   have a wxConfig object associated with it but only refer to its
   parent or the global config.
   @see ProfileBase
   @see wxConfig
*/

class Profile : public ProfileBase
{
public:
   /// like the constructor, but reuses existing objects
   static ProfileBase * CreateProfile(const String & iClassName,
                                      ProfileBase const *Parent);
   static ProfileBase * CreateFolderProfile(const String & iClassName,
                                            ProfileBase const *Parent);
   /// makes an empty profile, just inheriting from Parent
   static ProfileBase * CreateEmptyProfile(ProfileBase const *Parent);

   /// flushes all profiles to disk
   static void FlushAll() { ms_cfManager.Flush(); }

   /// get the associated config object
   wxConfigBase *GetConfig() const { return m_config; }

   /// get our name
   const String& GetProfileName() const { return profileName; }

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
   /// Write back the character value.
   bool writeEntry(const String & key,
                   const String & Value);
   /// Write back the int value.
   bool writeEntry(const String & key,
                   long Value);
   //@}

   void SetPath(const String & path);
   String GetPath(void) const;
   virtual bool HasEntry(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual void DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);
   /// return the name of the profile
   virtual String GetProfileName(void) { return profileName; }

   MOBJECT_DEBUG(Profile)

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       @param section the section to use if not M_PROFILE_CONFIG_SECTION
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   Profile(const String & iClassName, ProfileBase const *Parent,
           const char *section);
   /// constructor for empty, inheriting profile
   Profile(ProfileBase const *Parent);
   /// The parent profile.
   ProfileBase const *parentProfile;
   /// Name of this profile
   String   profileName;
   /// true if the object was successfully initialized
   bool isOk;
   /// true for empty dummy profile
   bool m_isEmpty;
   /** Destructor, writes back those entries that got changed.
   */
   ~Profile();

   /// config file manager class
   static ConfigFileManager ms_cfManager;

   GCC_DTOR_WARN_OFF();
};
//@}

ConfigFileManager Profile::ms_cfManager;


/** wxConfigProfile, a wrapper around wxConfig.
    This is not really a Profile as it is not inheriting from
    anywhere. It simply wraps around wxConfig to mask it as a
    profile. There is only one instance of this class in existence and
    it serves as the "mother of all profiles", i.e. the top level
    profile from which all others inherit. This object can be accessed
    as mApplication->GetProfile(). Its m_config pointer must never be
    NULL.
 */

class wxConfigProfile : public ProfileBase
{
public:
   // ctor
   wxConfigProfile(const String & fileName);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   String readEntry(const String & key,
                    const String & defaultvalue = (const char *) NULL,
                    bool * found = NULL) const;
   /// Read an integer value.
   long readEntry(const String & key,
                  long defaultvalue,
                  bool * found = NULL) const;
   /// Write back the character value.
   bool writeEntry(const String & key,
                   const String & Value);
   /// Write back the int value.
   bool writeEntry(const String & key,
                   long Value);
   //@}

   void SetPath(const String & path);
   String GetPath(void) const;
   virtual bool HasEntry(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual void DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);

   /// return the name of the profile
   virtual String GetProfileName(void) { return String("/"); }

private:
   /// forbid deletion
   ~wxConfigProfile();

   // remember the umask
#ifdef OS_UNIX
   long m_umask;
#endif // Unix

   GCC_DTOR_WARN_OFF();
};

/** A structure holding name and wxConfig pointer.
   This is the element of the list.
*/
struct FCData
{
  String      className;
  ProfileBase *profile;
};

/** A list of all loaded wxConfigs
   @see FCData
*/
KBLIST_DEFINE(FCDataList, FCData);

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// code which is thrown out under Windows
#ifdef OS_WIN
   #define NOT_WIN(anything)
#else  // !Win
   #define NOT_WIN(anything)  anything
#endif // Win/!Win

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

void ProfileBase::FlushAll()
{
   Profile::FlushAll();
}

size_t ProfileBase::GetGroupCount() const
{
   return m_config ? m_config->GetNumberOfGroups() : 0u;
}

//FIXME:MT calling wxWindows from possibly non-GUI code
bool ProfileBase::GetFirstGroup(String& s, long& l) const
{
   return m_config && m_config->GetFirstGroup(s, l);
}

bool ProfileBase::GetNextGroup(String& s, long& l) const
{
   return m_config && m_config->GetNextGroup(s, l);
}

bool ProfileBase::IsExpandingEnvVars() const
{
   return (m_config && m_config->IsExpandingEnvVars()) || m_expandEnvVars;
}

void ProfileBase::SetExpandEnvVars(bool bDoIt)
{
   if(m_config)
      m_config->SetExpandEnvVars(bDoIt);
   else
      m_expandEnvVars = bDoIt;
}

/** List all profiles of a given type or all profiles in total.
    @param type Type of profile to list or PT_Any for all.
    @return a pointer to kbStringList of profile names to be freed by caller.
*/
static void 
ListProfilesHelper(wxConfigBase *config,
                   kbList *list,
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
      if(type == ProfileBase::PT_Any || ptype == type)
         list->push_back(new String(pathname));
      ListProfilesHelper(config, list, type, pathname);

      ok = config->GetNextGroup (name, index);
   }

   // restore path
   config->SetPath(oldpath);
}

kbStringList *
ProfileBase::ListProfiles(int type)
{
   kbStringList *list = new kbStringList;
   wxConfigBase *global_config = mApplication->GetProfile()->m_config;
   if( global_config )
   {
      // may be give a (debug) warning here?
      ListProfilesHelper(global_config, list, type, "");
   }

   return list;
}

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

/** This function sets profile parameters and is applied
    to all profiles directly after creation.
*/
inline static
void EnforcePolicy(ProfileBase *p)
{
   p->SetExpandEnvVars(true);
}

ProfileBase *
ProfileBase::CreateProfile(const String & classname, ProfileBase const *parent)
{
   ProfileBase *p =  Profile::CreateProfile(FilterProfileName(classname), parent);
   EnforcePolicy(p);
   return p;
}

ProfileBase *
ProfileBase::CreateEmptyProfile(ProfileBase const *parent)
{
   ProfileBase *p = Profile::CreateEmptyProfile(parent);
   EnforcePolicy(p);
   return p;
}

ProfileBase *
ProfileBase::CreateGlobalConfig(const String & filename)
{
   ASSERT( ! strutil_isempty(filename) );
   ProfileBase *p = new wxConfigProfile(filename);
   EnforcePolicy(p);
   return p;
}

ProfileBase *
ProfileBase::CreateFolderProfile(const String & iClassName,
                                 ProfileBase const *parent)
{
   ProfileBase *p = Profile::CreateFolderProfile(iClassName,parent);
   EnforcePolicy(p);
   return p;
}

// some characters are invalid in the profile name, replace them
String
ProfileBase::FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const char *aValidChars = "_-.";   // NOT '/' and '\\'!

   String filteredName;
   size_t len = profileName.Len();
   for ( size_t n = 0; n < len; n++ )
   {
      char ch = profileName[n];
      if ( isalnum(ch) || strchr(aValidChars, ch) )
      {
         filteredName << ch;
      }
      else
      {
         // replace it -- hopefully the name will stay unique (@@)
         filteredName << '_';
      }
   }

   return filteredName;
}

String
ProfileBase::readEntry(const String & key,
                       const char *defaultvalue,
                       bool * found) const
{
   MOcheck();
   String str;
   str = readEntry(key, String(defaultvalue), found);
   return str;
}

kbStringList *
ProfileBase::GetExternalProfiles(void)
{
   kbStringList *profiles = new kbStringList;
   String pattern = READ_APPCONFIG(MP_USERDIR);
   String s;
   pattern << STRUTIL_PATH_SEPARATOR << "*.profile";
   wxString file = wxFindFirstFile(pattern);
   while( file )
   {
      s = file;
      s = strutil_path_filename(s);
      profiles->push_back(strutil_strdup(s));
      file = wxFindNextFile();
   }
   return profiles;
}

// ----------------------------------------------------------------------------
// wxConfigProfile - global wrapper
// ----------------------------------------------------------------------------

wxConfigProfile::wxConfigProfile(const String & fileName)
{
#  ifdef OS_WIN
   // don't give explicit name, but rather use the default logic (it's
   // perfectly ok, for the registry case our keys are under
   // vendor\appname)

   m_config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                            "", "",
                            wxCONFIG_USE_LOCAL_FILE |
                            wxCONFIG_USE_GLOBAL_FILE);
#  else  // Unix
   m_umask = 0;

   static String s_filename;

   // remember the global config file name
   // we don't need the config file manager for this profile
   m_config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                           fileName, mApplication->GetGlobalDir(),
                           wxCONFIG_USE_LOCAL_FILE|
                           wxCONFIG_USE_GLOBAL_FILE);
#  endif // Unix/Windows

   // set the default path for configuration entries
   m_config->SetPath(M_PROFILE_CONFIG_SECTION);
}

void wxConfigProfile::SetPath(const String & path)
{
#ifdef OS_UNIX
   if ( !m_umask )
   {
      // remember the default umask - we won't be able to retrieve it from the
      // dtor because it's too late to call config functions then
      m_umask = READ_APPCONFIG(MP_UMASK);
   }
#endif // Unix

   MOcheck();
   m_config->SetPath(path);
}

String
wxConfigProfile::GetPath(void) const
{
   MOcheck();
   return m_config->GetPath();
}

bool
wxConfigProfile::HasGroup(const String & name) const
{
   MOcheck();
   return m_config->HasGroup(name);
}

bool
wxConfigProfile::HasEntry(const String & key) const
{
   MOcheck();
   return m_config->HasEntry(key) || m_config->HasGroup(key);
}

bool
wxConfigProfile::Rename(const String& oldName, const String& newName)
{
   MOcheck();

   return m_config->RenameGroup(oldName, newName);
}

void
wxConfigProfile::DeleteGroup(const String & path)
{
   MOcheck();
   m_config->DeleteGroup(path);
}

wxConfigProfile::~wxConfigProfile()
{
   MOcheck();

#ifdef OS_UNIX
   umask(066);  // turn off read/write for all other users
#endif // Unix

   m_config->Flush();
   delete m_config;

   // reset it to a sensible default
#ifdef OS_UNIX
   if ( m_umask )
      umask(m_umask);  // turn off read/write for all other users
#endif
}

String
wxConfigProfile::readEntry(const String & key, const String & def, bool * found) const
{
   MOcheck(); ASSERT(m_config);
   String str;
   bool f = m_config->Read(key,&str, def);
   if(found)
     *found = f; 
   return str;
}

long
wxConfigProfile::readEntry(const String & key, long def, bool * found) const
{
   MOcheck(); ASSERT(m_config);
   long val;
   bool f = m_config->Read(key,&val,def);
   if(found)
     *found = f; 
   return val;
}

bool
wxConfigProfile::writeEntry(const String & key, const String & value)
{
   MOcheck();
   CHECK( m_config, false, "No config object in wxConfigProfile" );

   return m_config->Write(key, value);
}

bool
wxConfigProfile::writeEntry(const String & key, long value)
{
   MOcheck();
   CHECK( m_config, false, "No config object in wxConfigProfile" );

   return m_config->Write(key, value);
}

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/
Profile::Profile(const String & iClassName, ProfileBase const *Parent,
                 const char *section)
{
   m_config = NULL;   // set it before using CHECK()
   parentProfile = Parent;
   profileName = iClassName;
   m_isEmpty = false; // by default we are no dummy

   // find our config file unless we already have an absolute path
   // name
   // This is also done if a section is specified.
   String fullName;
   if ( !IsAbsPath(profileName) )
   {
      String tmp = READ_APPCONFIG(MP_PROFILE_PATH);
      PathFinder pf(tmp);

      String fileName = profileName + READ_APPCONFIG(MP_PROFILE_EXTENSION);
      fullName = pf.FindFile(fileName, &isOk);
      if( !isOk )
         fullName = mApplication->GetLocalDir() + DIR_SEPARATOR + fileName;
   }
   else
   {
      // easy...
      fullName << profileName << READ_APPCONFIG(MP_PROFILE_EXTENSION);
   }

   if ( READ_APPCONFIG(MP_CREATE_PROFILES) || wxFileExists(fullName) )
   {
#ifdef OS_UNIX
     umask(066);  // turn off read/write for all other users
#endif
      m_config = new wxFileConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  fullName, wxString(""),
                                  wxCONFIG_USE_LOCAL_FILE);
#ifdef OS_UNIX
     umask(READ_APPCONFIG(MP_UMASK));  // turn off read/write for all other users
#endif
   }
   else
   {
      m_config = NULL; // we don't have a profile but write to the
      // global profile in the corresponding section
   }

   // now the profileName which is used as the section must be
   // prefixed with the section name:
   if(section)
      profileName = String(section) + profileName;
   ms_cfManager.Register(profileName, this);
}

// constructor for empty profile
Profile::Profile(ProfileBase const *Parent)
       : profileName(PROFILE_EMPTY_NAME)
{
   m_config = NULL;   // set it before using CHECK()
   parentProfile = Parent;
   m_isEmpty = true; // don't de-register with config file manager
}

ProfileBase *
Profile::CreateProfile(const String & iClassName, ProfileBase const *parent)
{
   ProfileBase *profile = ms_cfManager.Find(iClassName);
   if(! profile)
      profile = new Profile(iClassName, parent, M_PROFILE_CONFIG_SECTION);
   return profile;
}

ProfileBase *
Profile::CreateFolderProfile(const String & iClassName, ProfileBase const *parent)
{
   String classname = String(M_FOLDER_CONFIG_SECTION) + iClassName;
   ProfileBase *profile = ms_cfManager.Find(classname);
   if(! profile)
      profile = new Profile(iClassName, parent, M_FOLDER_CONFIG_SECTION);
   return profile;
}

ProfileBase *
Profile::CreateEmptyProfile(ProfileBase const *parent)
{
   return new Profile(parent); // constructor for empty profile
}

Profile::~Profile()
{

#ifdef OS_UNIX
     umask(022);  // turn off read/write for all other users
#endif
   if ( m_config )
      m_config->Flush();
#ifdef OS_UNIX
     umask(READ_APPCONFIG(MP_UMASK));  // turn off read/write for all other users
#endif
   if( ! m_isEmpty)
      ms_cfManager.DeRegister(this);
   delete m_config;
}

// we can't use readEntry(String) to implement readEntry(long) because some
// wxConfig implementations don't allow reading string values from keys storing
// numeric data (especially wxRegConfig - the Win32 registry is typed). To avoid
// code duplication we use this hack (we might use templates too...)

// this strucutre holds either a string or a number
class KeyValue
{
public:
   // ctors
   KeyValue() { /* don't initialize anything */ }
   KeyValue(long l) : m_lValue(l) { m_isString = false; }
   KeyValue(const String& str) : m_strValue(str) { m_isString = true; }

   // assignment
   void SetValue(const String& str) { m_isString = true; m_strValue = str; }
   void SetValue(int l) { m_isString = false; m_lValue = l; }

   // accessors
   bool IsString() const { return m_isString; }

   String GetString() const { ASSERT( m_isString ); return m_strValue; }
   long GetNumber() const { ASSERT( !m_isString ); return m_lValue; }

private:
   // can't use the union to hold this data because of String
   String m_strValue;
   long   m_lValue;

   bool   m_isString; // type selector for the union
};

// read the value from the given profile, return false if not found
static bool
readEntryFromProfile(KeyValue& value,
                     const ProfileBase *profile,
                     const String & key,
                     const KeyValue& defaultValue)
{
   if ( defaultValue.IsString() )
   {
      const char * illegal = "^%%&*^&*^y--illegal-profile-value^&%%$&^";
      //FIXME: need readEntry which returns success value
      String strValue = profile->readEntry(key, (const char *)illegal);
      value.SetValue(strValue);
      return strValue != illegal;
   }
   else
   {
      // FIXME I don't know how to distinguish between the default value and
      //       "real" dummyValue here.
      static const long dummyValue = 17;
      long lValue = profile->readEntry(key, dummyValue);
      value.SetValue(lValue);
      return lValue != dummyValue;
   }
}

/** Read the value from the specified key and return it in KeyValue.
    This is only used in Profile.

    If a value has been read, read is set to true, it is false if the
    default value got returned.
*/
static KeyValue
readEntryHelper(wxConfigBase *config,
                const ProfileBase *parentProfile,
                const String& profileName,
                const String & key,
                const KeyValue& defaultValue,
                bool expandEnvVars, bool *read = NULL)
{
   KeyValue value;
   String strValue;
   long   lValue;

   bool bRead = false;

   // first, try our own config file if it exists
   // Don't need to set expansion state as this is done anyway
   // if we own the config.
   if ( config )
   {
      if ( defaultValue.IsString() )
      {
         bRead = config->Read(key, &strValue);
         if ( bRead )
            value.SetValue(strValue);
      }
      else
      {
         bRead = config->Read(key, &lValue);
         if ( bRead )
            value.SetValue(lValue);
      }
   }

   // second, try our name in the global config file:
   if ( !bRead )
   {
      //ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);
      //mApplication->GetProfile()->SetPath(profileName);
      ProfilePathChanger ppc(mApplication->GetProfile(), profileName);
      ProfileEnvVarSave env(mApplication->GetProfile(), expandEnvVars);

      bRead = readEntryFromProfile(value, mApplication->GetProfile(),
                                   key, defaultValue);
   }

   // third, try the parent profile
   if ( !bRead && parentProfile != NULL )
   {
      ProfileEnvVarSave env(parentProfile, expandEnvVars);
      bRead = readEntryFromProfile(value, parentProfile, key, defaultValue);
   }

   // forth: try all parent sections:
   String profile_path = profileName;
   while ( !bRead && ! profile_path.IsEmpty())
   {
      profile_path = strutil_path_parent(profile_path);
      ProfilePathChanger ppc(mApplication->GetProfile(), profile_path);
      ProfileEnvVarSave env(mApplication->GetProfile(), expandEnvVars);

      bRead = readEntryFromProfile(value, mApplication->GetProfile(),
                                   key, defaultValue);
   }

   // record the default value back into config if asked for
   if( !bRead )
   {
      if ( READ_APPCONFIG(MP_RECORDDEFAULTS) )
      {
         if(config)
         {
            if( defaultValue.IsString() )
               config->Write(key, defaultValue.GetString());
            else
               config->Write(key, (long)defaultValue.GetNumber());
         }else
         {
            //ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);
            //mApplication->GetProfile()->SetPath(profileName);
            ProfilePathChanger ppc(mApplication->GetProfile(), profileName);
            ProfileEnvVarSave env(mApplication->GetProfile(), expandEnvVars);
            if( defaultValue.IsString() )
               mApplication->GetProfile()->writeEntry(key, defaultValue.GetString());
            else
               mApplication->GetProfile()->writeEntry(key, defaultValue.GetNumber());
         }
      }
      value = defaultValue;
   }

#  ifdef DEBUG
   String dbgtmp = "Profile(" + profileName + String(")::readEntry(") +
      String(key) + ") returned: ";
   if ( value.IsString() )
      dbgtmp += value.GetString();
   else
      dbgtmp += strutil_ltoa(value.GetNumber());

   wxLogTrace(wxTraceProfileClass, Str(dbgtmp));
#  endif

   if(read)
      *read = bRead;
   return value;
}

String
Profile::readEntry(const String & key, const String & defaultvalue,
                   bool *found) const
{
   MOcheck();
   bool read;
   String value = readEntryHelper(m_config, parentProfile, profileName,
                                  key, KeyValue(defaultvalue),
                                  m_expandEnvVars, &read).GetString();
   if(m_expandEnvVars && ! read)
      value = wxExpandEnvVars(value);
   if(found)
      *found = read;
   return value;
}

long
Profile::readEntry(const String & key, long defaultvalue, bool *found) const
{
   MOcheck();

   return readEntryHelper(m_config, parentProfile, profileName,
                          key, KeyValue(defaultvalue),
                          m_expandEnvVars, found).GetNumber();
}


void Profile::SetPath(const String & path)
{
   MOcheck();
   if(m_config) m_config->SetPath(path);
}

String
Profile::GetPath(void) const
{
   MOcheck();
   if(m_config)
      return m_config->GetPath();
   else
      return (const char *)NULL;
}

bool
Profile::HasGroup(const String & name) const
{
   MOcheck();
   return m_config && m_config->HasGroup(name);
}

bool
Profile::Rename(const String& oldName, const String& newName)
{
   MOcheck();

   return m_config && m_config->RenameGroup(oldName, newName);
}

bool
Profile::HasEntry(const String & key) const
{
   MOcheck();
   return m_config && m_config->HasEntry(key);
}

void
Profile::DeleteGroup(const String & path)
{
   MOcheck();
   if(m_config) m_config->DeleteGroup(path);
}

bool
Profile::writeEntry(const String & key, long Value)
{
   MOcheck();
   if(m_config)
      return m_config->Write(key, Value) != 0;
   else // we don't have a profile, write to global
   {
      //ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);
      //mApplication->GetProfile()->SetPath(profileName);
      ProfilePathChanger ppc(mApplication->GetProfile(),
                             profileName);
      return mApplication->GetProfile()->writeEntry(key, Value);
   }
}

bool
Profile::writeEntry(const String & key, const String & value)
{
   MOcheck();

   if(m_config)
      return m_config->Write(key, value) != 0;
   else // we don't have a profile, write to global
   {
      //ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);
      //mApplication->GetProfile()->SetPath(profileName);
      ProfilePathChanger ppc(mApplication->GetProfile(),
                             profileName);
      return mApplication->GetProfile()->writeEntry(key, value);
   }
   return false; // keep compiler happy
}

#ifdef DEBUG

wxString
Profile::DebugDump() const
{
   String str = MObjectRC::DebugDump();
   str << "name '" << profileName << '\'';

   return str;
}

#endif // Debug

// ----------------------------------------------------------------------------
// ConfigFileManager
// ----------------------------------------------------------------------------

ConfigFileManager::ConfigFileManager()
{
   fcList = new FCDataList(FALSE);
}

ConfigFileManager::~ConfigFileManager()
{
   FCDataList::iterator i;

#  ifdef DEBUG
      Debug();
#  endif

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      FCData *data = *i;
      ProfileBase *p = data->profile;
      // we cannot do a decref here because MObjets list might no
      // longer exist - order of cleanup is unknown
      // p->DecRef();
      // but we can complain!
      DBGLOG("Profile left over at exit:" << p->GetProfileName());
      delete data;
   }

   delete fcList;
}

void
ConfigFileManager::Flush()
{
   FCDataList::iterator i;
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      ProfileBase *profile = (*i)->profile;
      wxConfigBase *config = profile->GetConfig();
      if ( config )
         config->Flush();
   }
}

ProfileBase *
ConfigFileManager::Find(const String & className)
{
   FCDataList::iterator i;

   DBGLOG("ConfigFileManager.Find(" << Str(className) << ")");

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if((*i)->className == className)
      {
         (*i)->profile->IncRef();
         return (*i)->profile;
      }
   }
   return NULL;
}

void
ConfigFileManager::DeRegister(ProfileBase *prof)
{
   FCDataList::iterator i;

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if((*i)->profile == prof)
      {
         TRACEMESSAGE((wxTraceProfileClass,
                       "ConfigFileManager::DeRegister(%s)",
                       (*i)->className.c_str()));
         delete fcList->remove(i);
         return;
      }
   }

   FAIL_MSG("ConfigFileManager::DeRegister(): bogus profile");
}

void
ConfigFileManager::Register(const String & className, ProfileBase *profile)
{
   TRACEMESSAGE((wxTraceProfileClass,
                 "ConfigFileManager.Register(%s)",
                 className.c_str()));

   FCData   *newEntry = new FCData;
   newEntry->className = className;
   newEntry->profile = profile;
   fcList->push_front(newEntry);
}

#ifdef DEBUG
void
ConfigFileManager::Debug() const
{
   DBGLOG("------ConfigFileManager------\n");

   FCDataList::iterator i;
   for(i = fcList->begin(); i != fcList->end(); i++) {
      DBGMESSAGE(((*i)->className));
   }

   DBGLOG("-----------------------------\n");
}
#endif

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(ProfileBase& conf, const wxArrayString& astr, const String & key)
{
   // save all array entries
   conf.DeleteGroup(key);    // remove all old entries
   conf.SetPath(key);

   size_t nCount = astr.Count();
   wxString strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf("%d", n);
      conf.writeEntry(strkey, astr[n]);
   }

   conf.SetPath("..");
}

// restores array saved by SaveArray
void RestoreArray(ProfileBase& conf, wxArrayString& astr, const String & key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   conf.SetPath(key);

   wxString strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf("%d", n);
      if ( !conf.HasEntry(strkey) )
         break;
      strVal = conf.readEntry(strkey, "");
      astr.Add(strVal);
   }

   conf.SetPath("..");
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

