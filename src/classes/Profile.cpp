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
#include  "Mcommon.h"

#ifndef USE_PCH
#   include "strutil.h"
#   include "PathFinder.h"
#   include "MimeList.h"
#   include "MimeTypes.h"
#   include "kbList.h"
#   include "MFrame.h"
#   include "gui/wxMFrame.h"
#   include "Profile.h"

#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   ifdef  OS_WIN
#      include   <wx/msw/regconf.h>
#   else
#      include   <wx/confbase.h>
#      include   <wx/file.h>
#      include   <wx/textfile.h>
#      include   <wx/fileconf.h>
#   endif
#   include   <wx/config.h>
#endif

#include "Mdefaults.h"

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

   // env vars stuff
   virtual bool IsExpandingEnvVars() const
      { return m_config->IsExpandingEnvVars(); }
   virtual void SetExpandEnvVars(bool bDoIt)
      { m_config->SetExpandEnvVars(bDoIt); }

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MC_PROFILEPATH).

   */
   Profile(STRINGARG iClassName, ProfileBase const *Parent);
   /// The wxConfig object.
   wxConfigBase  *m_config;
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
   wxConfigProfile(STRINGARG fileName);

   void SetPath(STRINGARG path);
   String GetPath(void) const;
   virtual bool HasEntry(STRINGARG key) const;
   virtual void DeleteGroup(STRINGARG path);
   /// return the name of the profile
   virtual String GetProfileName(void) { return String("/"); }

   // env vars stuff
   virtual bool IsExpandingEnvVars() const
      { return m_Config->IsExpandingEnvVars(); }
   virtual void SetExpandEnvVars(bool bDoIt)
      { m_Config->SetExpandEnvVars(bDoIt); }

private:
   wxConfigBase *m_Config;
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
// private functions
// ----------------------------------------------------------------------------

// some characters are invalid in the profile name, replace them
static String FilterProfileName(const String& profileName);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

ProfileBase *
ProfileBase::CreateProfile(STRINGARG classname, ProfileBase const *parent)
{
   return  Profile::CreateProfile(classname, parent);
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
   // the code for Unix is specific to wxFileConfig - unacceptable
#  ifdef OS_WIN
      // don't give explicit name, but rather use the default logic (it's
      // perfectly ok, for the registry case our keys are under vendor\appname)
      m_Config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                              "", "",
                              wxCONFIG_USE_LOCAL_FILE |
                              wxCONFIG_USE_GLOBAL_FILE);
#  else  // Unix
      // we don't need the config file manager for this profile
      PathFinder pf(M_ETC_PATH);
      String globalconfig = pf.FindFile(M_GLOBAL_CONFIG_NAME);
      m_Config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                              fileName, globalconfig,
                              wxCONFIG_USE_LOCAL_FILE|
                              wxCONFIG_USE_GLOBAL_FILE);

      // set the default path for configuration entries
      m_Config->SetPath(M_APPLICATIONNAME);
#  endif // Unix/Windows
}

void wxConfigProfile::SetPath(STRINGARG path)
{
   MOcheck();
   m_Config->SetPath(path);
}

String
wxConfigProfile::GetPath(void) const
{
   MOcheck();
   return m_Config->GetPath();
}

bool
wxConfigProfile::HasEntry(STRINGARG key) const
{
   MOcheck();
   return m_Config->HasEntry(key);
}

void
wxConfigProfile::DeleteGroup(STRINGARG path)
{
   MOcheck();
   m_Config->DeleteGroup(path);
}

wxConfigProfile::~wxConfigProfile()
{
   MOcheck();
}

String
wxConfigProfile::readEntry(STRINGARG key, STRINGARG def) const
{
   MOcheck();
   String str;
   str = m_Config->Read(key.c_str(),def.c_str());
   return str;
}

long
wxConfigProfile::readEntry(STRINGARG key, long def) const
{
   MOcheck();
   return m_Config->Read(key.c_str(),def);
}

bool
wxConfigProfile::writeEntry(STRINGARG key, STRINGARG value)
{
   MOcheck();
   return m_Config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(STRINGARG key, long value)
{
   MOcheck();
   return m_Config->Write(key,value);
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
   String fullFileName;
   if ( !IsAbsPath(profileName) )
   {
      String tmp = READ_APPCONFIG(MC_PROFILE_PATH);
      PathFinder pf(tmp);

      String fileName = profileName + READ_APPCONFIG(MC_PROFILE_EXTENSION);
      fullFileName = pf.FindFile(fileName, &isOk);
      if( !isOk )
         fullFileName = mApplication->GetLocalDir() + DIR_SEPARATOR + fileName;
   }
   else
   {
      // easy...
      fullFileName << profileName << READ_APPCONFIG(MC_PROFILE_EXTENSION);
   }

   // under Windows I only create entries in the registry and I think it should
   // be also done with wxFileconfig under Unix as well
#  ifdef OS_WIN
      m_config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME);
      String path;
      path << M_PROFILE_CONFIG_SECTION << '/' << fullFileName;
      m_config->SetPath(path);
#  else // Unix
      m_config = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                              fullFileName,wxString(""),
                              wxCONFIG_USE_LOCAL_FILE);
#  endif // Unix/Win

   ms_cfManager.Register(iClassName, this);
}

ProfileBase *
Profile::CreateProfile(STRINGARG iClassName, ProfileBase const *parent)
{
   ProfileBase *profile = ms_cfManager.Find(iClassName);
   if(! profile)
      profile = new Profile(iClassName, parent);
   return profile;
}


Profile::~Profile()
{
   m_config->Flush();
   ms_cfManager.DeRegister(this);
   delete m_config;
}


String
Profile::readEntry(STRINGARG key, STRINGARG defaultvalue) const
{
   MOcheck();
   // config object must be created
   CHECK( m_config != NULL, "", "no m_config in Profile" );

   wxString rc = m_config->Read(key.c_str());

   if( strutil_isempty(rc) && parentProfile != NULL)
   {
      rc = parentProfile->readEntry(key, (const char *)NULL);
   }

   if( strutil_isempty(rc) )
   {
      ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);

      mApplication->GetProfile()->SetPath(profileName.c_str());
      rc = mApplication->GetProfile()->readEntry(key, (const char *)NULL);
      if( strutil_isempty(rc))
      {
         mApplication->GetProfile()->SetPath(M_PROFILE_CONFIG_SECTION);
         rc = mApplication->GetProfile()->readEntry(key, (const char *)NULL);
      }
   }

   if( strutil_isempty(rc) )
   {
      if(READ_APPCONFIG(MC_RECORDDEFAULTS) )
         m_config->Write(key,defaultvalue); // so default value can be recorded
      rc = defaultvalue;
   }

#  ifdef DEBUG
   String dbgtmp = "Profile(" + profileName + String(")::readEntry(") +
      String(key) + ") returned: " + rc;
   DBGLOG(Str(dbgtmp));
#  endif

   return rc;
}

long
Profile::readEntry(STRINGARG key, long defaultvalue) const
{
   long rc;
   MOcheck();

   if ( !m_config->Read(key, &rc, defaultvalue) )
   {
      if ( !parentProfile ||
           (rc = parentProfile->readEntry(key, defaultvalue)) == defaultvalue
         )
      {
         if ( mApplication->GetProfile() )
         {
            rc = mApplication->GetProfile()->readEntry(key,defaultvalue);
            {
               if ( READ_APPCONFIG(MC_RECORDDEFAULTS) )
                  m_config->Write(key, defaultvalue);
            }
         }
      }
   }

   return rc;
}


void Profile::SetPath(STRINGARG path)
{
   MOcheck();
   m_config->SetPath(path);
}

String
Profile::GetPath(void) const
{
   MOcheck();
   return m_config->GetPath();
}

bool
Profile::HasEntry(STRINGARG key) const
{
   MOcheck();
   return m_config->HasEntry(key);
}

void
Profile::DeleteGroup(STRINGARG path)
{
   MOcheck();
   m_config->DeleteGroup(path);
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
   CHECK( m_config != NULL, false, "no m_config in Profile" );

   return m_config->Write(key, (long int) Value) != 0;
}

bool
Profile::writeEntry(STRINGARG key, STRINGARG Value)
{
   MOcheck();
   CHECK( m_config != NULL, false, "no m_config in Profile" );

   return m_config->Write(key, Value) != 0;
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

   DBGLOG("ConfigFileManager.DeRegister()");

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if((*i)->profile == prof)
      {
         fcList->erase(i);
         return;
      }
   }
}

void
ConfigFileManager::Register(STRINGARG className, ProfileBase *profile)
{
   DBGLOG("ConfigFileManager.Register(" << Str(className) << ")");

   FCData   *newEntry = new FCData;
   newEntry->className = className;
   newEntry->profile = profile;
   fcList->push_front(newEntry);
}

#ifdef DEBUG
void
ConfigFileManager::Debug() const
{
   kbListIterator i;

   DBGLOG("------ConfigFileManager------\n");
   for(i = fcList->begin(); i != fcList->end(); i++) {
      // @@ where is operator<<(m_config)?
#     if 0
      DBGLOG('"' << (*i).fileName << '"' << '\t' << (*i).m_config << '\n');
#     endif
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
// a small class to temporarily suspend env var expansion
// ----------------------------------------------------------------------------

ProfileEnvVarSuspend::ProfileEnvVarSuspend(ProfileBase *profile)
{
  m_profile = profile;
  m_wasExpanding = profile->IsExpandingEnvVars();
  profile->SetExpandEnvVars(FALSE);
}

ProfileEnvVarSuspend::~ProfileEnvVarSuspend()
{
  m_profile->SetExpandEnvVars(m_wasExpanding);
}

// ----------------------------------------------------------------------------
// a tiny utility class which is used to temporary change the config path, for
// example:
//    {
//       ProfilePathChanger ppc(profile->GetConfig(), "/M/Frames");
//       profile->WriteEntry("x", 400);
//       ...
//       // path automatically restored here
//    }
// ----------------------------------------------------------------------------

ProfilePathChanger::ProfilePathChanger(ProfileBase *config, STRINGARG path)
{
   m_config = config;
   m_strOldPath = m_config->GetPath();
   m_config->SetPath(path);
}

ProfilePathChanger::~ProfilePathChanger()
{
   m_config->SetPath(m_strOldPath);
}

// some characters are invalid in the profile name, replace them
static String FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const char *aValidChars = "_-.";

   String filteredName = profileName;
   for ( size_t n = 0; n < filteredName.Len(); n++ )
   {
      char ch = filteredName[n];
      if ( !isalnum(ch) && !strchr(aValidChars, ch) )
      {
         // replace it -- hopefully the name will stay unique (@@)
         filteredName[n] = '_';
      }
   }

   return filteredName;
}
