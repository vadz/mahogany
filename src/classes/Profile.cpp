/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/** ConfigFileManager class, this class allocates and deallocates
   wxConfig objects for the profile so to ensure that every config
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

   /**  Finds a profile if it already exists
        @param fileName name of configuration file
    */
   ProfileBase *Find(STRINGARG fileName);

   /** registers a profile object for reuse
       @param fileName name of configuration file
       @param prof pointer to existing profile
    */
   void Register(STRINGARG fileName, ProfileBase *prof);

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
   @see ProfileBase
   @see wxConfig
*/

class Profile : public ProfileBase
{
public:
   /// like the constructor, but reuses existing objects
   static ProfileBase * CreateProfile(STRINGARG iClassName,
                                      ProfileBase const *Parent);
   /// makes an empty profile, just inheriting from Parent
   static ProfileBase * CreateEmptyProfile(ProfileBase const *Parent);

   /// get the associated config object
   wxConfigBase *GetConfig() const { return m_config; }

   /// get our name
   const String& GetProfileName() const { return profileName; }

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
      /// Read a character entry.
   String readEntry(STRINGARG key,
                    STRINGARG defaultvalue = (const char *)NULL) const;
   /// Read an integer value.
   long readEntry(STRINGARG key, long defaultvalue) const;
   /// Read an integer value.
   virtual int readEntry(STRINGARG key, int defaultvalue) const
      { return (int) readEntry(key, (long)defaultvalue); }
   /// Read a bool value.
   bool readEntry(STRINGARG key, bool defaultvalue) const;
   /// Write back the character value.
   bool writeEntry(STRINGARG key, STRINGARG Value);
   /// Write back the int value.
   bool writeEntry(STRINGARG key, long Value);
   /// Write back the bool value.
   bool writeEntry(STRINGARG key, bool Value);
   //@}

   void SetPath(STRINGARG path);
   String GetPath(void) const;
   virtual bool HasEntry(STRINGARG key) const;
   virtual void DeleteGroup(STRINGARG path);
   /// return the name of the profile
   virtual String GetProfileName(void) { return profileName; }

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MC_PROFILEPATH).

   */
   Profile(STRINGARG iClassName, ProfileBase const *Parent);
   /// constructor for empty, inheriting profile
   Profile(ProfileBase const *Parent);
   /// The parent profile.
   ProfileBase const *parentProfile;
   /// Name of this profile
   String   profileName;
   // true if the object was successfully initialized
   bool isOk;
   /** Destructor, writes back those entries that got changed.
   */
   ~Profile();

   /// config file manager class
   static ConfigFileManager ms_cfManager;
};
//@}

ConfigFileManager Profile::ms_cfManager;


/** wxConfigProfile, a wrapper around wxConfig.
 */

class wxConfigProfile : public ProfileBase
{
public:
   // ctor
   wxConfigProfile(STRINGARG fileName);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   String readEntry(STRINGARG key,
                    STRINGARG defaultvalue = (const char *) NULL) const;
   /// Read an integer value.
   long readEntry(STRINGARG key, long defaultvalue) const;
   /// Read an integer value.
   virtual int readEntry(STRINGARG key, int defaultvalue) const
      { return (int) readEntry(key, (long)defaultvalue); }
   /// Read a bool value.
   bool readEntry(STRINGARG key, bool defaultvalue) const;
   /// Write back the character value.
   bool writeEntry(STRINGARG key, STRINGARG Value);
   /// Write back the int value.
   bool writeEntry(STRINGARG key, long Value);
   /// Write back the bool value.
   bool writeEntry(STRINGARG key, bool Value);
   //@}

   void SetPath(STRINGARG path);
   String GetPath(void) const;
   virtual bool HasEntry(STRINGARG key) const;
   virtual void DeleteGroup(STRINGARG path);

   /// return the name of the profile
   virtual String GetProfileName(void) { return String("/"); }

private:
   /// forbid deletion
   ~wxConfigProfile();
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

// create the global wxConfig object
static wxConfigBase *CreateGlobalConfigBase(NOT_WIN(const String& filename));

// some characters are invalid in the profile name, replace them
static String FilterProfileName(const String& profileName);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------
bool ProfileBase::GetFirstGroup(String& s, long& l)
{
   return m_config && m_config->GetFirstGroup(s, l);
}

bool ProfileBase::GetNextGroup(String& s, long& l)
{
   return m_config && m_config->GetNextGroup(s, l);
}

bool ProfileBase::IsExpandingEnvVars() const
{
   return m_config && m_config->IsExpandingEnvVars();
}

void ProfileBase::SetExpandEnvVars(bool bDoIt)
{
   if(m_config) m_config->SetExpandEnvVars(bDoIt);
}

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

ProfileBase *
ProfileBase::CreateProfile(STRINGARG classname, ProfileBase const *parent)
{
   return  Profile::CreateProfile(FilterProfileName(classname), parent);
}

ProfileBase *
ProfileBase::CreateEmptyProfile(ProfileBase const *parent)
{
   return Profile::CreateEmptyProfile(parent);
}

ProfileBase *
ProfileBase::CreateGlobalConfig(STRINGARG filename)
{
   return new wxConfigProfile(filename);
}

String
ProfileBase::readEntry(STRINGARG key,
                       const char *defaultvalue) const
{
   MOcheck();
   String str;
   str = readEntry(key, String(defaultvalue));
   return str;
}

// ----------------------------------------------------------------------------
// wxConfigProfile
// ----------------------------------------------------------------------------

wxConfigProfile::wxConfigProfile(STRINGARG fileName)
{
   m_config = CreateGlobalConfigBase(NOT_WIN(fileName));
}

void wxConfigProfile::SetPath(STRINGARG path)
{
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
wxConfigProfile::HasEntry(STRINGARG key) const
{
   MOcheck();
   return m_config->HasEntry(key);
}

void
wxConfigProfile::DeleteGroup(STRINGARG path)
{
   MOcheck();
   m_config->DeleteGroup(path);
}

wxConfigProfile::~wxConfigProfile()
{
   MOcheck();
   m_config->Flush();
   delete m_config;
}

String
wxConfigProfile::readEntry(STRINGARG key, STRINGARG def) const
{
   MOcheck();
   String str;
   str = m_config->Read(key.c_str(),def.c_str());
   return str;
}

long
wxConfigProfile::readEntry(STRINGARG key, long def) const
{
   MOcheck();
   return m_config->Read(key.c_str(),def);
}

bool
wxConfigProfile::writeEntry(STRINGARG key, STRINGARG value)
{
   MOcheck();
   return m_config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(STRINGARG key, long value)
{
   MOcheck();
   return m_config->Write(key,value);
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
Profile::Profile(STRINGARG iClassName, ProfileBase const *Parent)
       : profileName(iClassName)
{
   m_config = NULL;   // set it before using CHECK()
   parentProfile = Parent;

   // find our config file unless we already have an absolute path name
   String fullName;
   if ( !IsAbsPath(profileName) )
   {
      String tmp = READ_APPCONFIG(MC_PROFILE_PATH);
      PathFinder pf(tmp);

      String fileName = profileName + READ_APPCONFIG(MC_PROFILE_EXTENSION);
      fullName = pf.FindFile(fileName, &isOk);
      if( !isOk )
         fullName = mApplication->GetLocalDir() + DIR_SEPARATOR + fileName;
   }
   else
   {
      // easy...
      fullName << profileName << READ_APPCONFIG(MC_PROFILE_EXTENSION);
   }

   if ( READ_APPCONFIG(MC_CREATE_PROFILES) || wxFileExists(fullName) )
   {
      m_config = new wxFileConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  fullName, wxString(""),
                                  wxCONFIG_USE_LOCAL_FILE);
   }
   else
   {
      // ok, so we don't have our _own_ file, but we still must have read our
      // data from/write to somewhere!
      fullName = M_PROFILE_CONFIG_SECTION;
      fullName << '/' << iClassName;
      m_config = CreateGlobalConfigBase(NOT_WIN(""));
      m_config->SetPath(fullName);
   }

   ms_cfManager.Register(iClassName, this);
}

// constructor for empty profile
Profile::Profile(ProfileBase const *Parent)
       : profileName("anonymous")
{
   m_config = NULL;   // set it before using CHECK()
   parentProfile = Parent;
}

ProfileBase *
Profile::CreateProfile(STRINGARG iClassName, ProfileBase const *parent)
{
   ProfileBase *profile = ms_cfManager.Find(iClassName);
   if(! profile)
      profile = new Profile(iClassName, parent);
   return profile;
}

ProfileBase *
Profile::CreateEmptyProfile(ProfileBase const *parent)
{
   return new Profile(parent); // constructor for empty profile
}

Profile::~Profile()
{
   if ( m_config )
      m_config->Flush();
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
   void SetValue(long l) { m_isString = false; m_lValue = l; }

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
                     STRINGARG key,
                     const KeyValue& defaultValue)
{
   if ( defaultValue.IsString() )
   {
      String strValue = profile->readEntry(key, (const char *)NULL);
      value.SetValue(strValue);
      return !strutil_isempty(strValue);
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

// read the value from the specified key and return it in KeyValue
static KeyValue
readEntryHelper(wxConfigBase *config,
                const ProfileBase *parentProfile,
                const String& profileName,
                STRINGARG key,
                const KeyValue& defaultValue)
{
   KeyValue value;
   String strValue;
   long   lValue;

   bool bRead = false;

   // first, try our own config
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

   // second, try the parent profile
   if ( !bRead && parentProfile != NULL )
   {
      bRead = readEntryFromProfile(value, parentProfile, key, defaultValue);
   }

   // third, try our name in the global config file:
   if ( !bRead )
   {
      ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);
      mApplication->GetProfile()->SetPath(profileName);

      bRead = readEntryFromProfile(value, mApplication->GetProfile(),
                                   key, defaultValue);
   }

   // record the default value back into config if asked for
   if( !bRead )
   {
      if ( config && READ_APPCONFIG(MC_RECORDDEFAULTS) )
      {
         if ( defaultValue.IsString() )
            config->Write(key, defaultValue.GetString());
         else
            config->Write(key, defaultValue.GetNumber());
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

      DBGLOG(Str(dbgtmp));
#  endif

   return value;
}

String
Profile::readEntry(STRINGARG key, STRINGARG defaultvalue) const
{
   MOcheck();

   return readEntryHelper(m_config, parentProfile, profileName,
                          key, KeyValue(defaultvalue)).GetString();
}

long
Profile::readEntry(STRINGARG key, long defaultvalue) const
{
   MOcheck();

   return readEntryHelper(m_config, parentProfile, profileName,
                          key, KeyValue(defaultvalue)).GetNumber();
}


void Profile::SetPath(STRINGARG path)
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
Profile::HasEntry(STRINGARG key) const
{
   MOcheck();
   return m_config && m_config->HasEntry(key);
}

void
Profile::DeleteGroup(STRINGARG path)
{
   MOcheck();
   if(m_config) m_config->DeleteGroup(path);
}

bool
Profile::readEntry(STRINGARG key, bool defaultvalue) const
{
   MOcheck();
   return readEntry(key, (long) defaultvalue) != 0;
}

bool
Profile::writeEntry(STRINGARG key, long Value)
{
   MOcheck();
   return m_config && m_config->Write(key, (long int) Value) != 0;
}

bool
Profile::writeEntry(STRINGARG key, STRINGARG Value)
{
   MOcheck();
   return m_config && m_config->Write(key, Value) != 0;
}

bool
Profile::writeEntry(STRINGARG key, bool Value)
{
   return writeEntry(key, (long) Value);
}

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

ProfileBase *
ConfigFileManager::Find(STRINGARG className)
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
         TRACEMESSAGE(("ConfigFileManager::DeRegister(%s)",
                       (*i)->className.c_str()));
         delete fcList->remove(i);
         return;
      }
   }

   FAIL_MSG("ConfigFileManager::DeRegister(): bogus profile");
}

void
ConfigFileManager::Register(STRINGARG className, ProfileBase *profile)
{
   TRACEMESSAGE(("ConfigFileManager.Register(%s)", className.c_str()));

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
void SaveArray(ProfileBase& conf, const wxArrayString& astr, STRINGARG key)
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
void RestoreArray(ProfileBase& conf, wxArrayString& astr, STRINGARG key)
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

static wxConfigBase *
CreateGlobalConfigBase(NOT_WIN(const String& filename))
{
   wxConfigBase *config;
#  ifdef OS_WIN
      // don't give explicit name, but rather use the default logic (it's
      // perfectly ok, for the registry case our keys are under vendor\appname)
      config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                            "", "",
                            wxCONFIG_USE_LOCAL_FILE |
                            wxCONFIG_USE_GLOBAL_FILE);
#  else  // Unix
      static String s_filename;

      // remember the global config file name
      if ( !filename.IsEmpty() )
      {
         s_filename = filename;
      }
      //else: if no filename is given, reuse the last one

      // we don't need the config file manager for this profile
      PathFinder pf(M_ETC_PATH);
      String globalconfig = pf.FindFile(M_GLOBAL_CONFIG_NAME);
      config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                            s_filename, globalconfig,
                            wxCONFIG_USE_LOCAL_FILE|
                            wxCONFIG_USE_GLOBAL_FILE);

      // set the default path for configuration entries
      config->SetPath(M_APPLICATIONNAME);
#  endif // Unix/Windows

   return config;
}

// some characters are invalid in the profile name, replace them
static String FilterProfileName(const String& profileName)
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
