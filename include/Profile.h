/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef PROFILE_H
#define PROFILE_H

#ifdef __GNUG__
#   pragma interface "Profile.h"
#endif

// ----------------------------------------------------------------------------
// headers
//
// ----------------------------------------------------------------------------

#ifndef  USE_PCH
#   include  "kbList.h"
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

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// the classes in this file can use either the (old) standalone appconf,
// or wxConfig included in wxWindows 2
#  define   READ_ENTRY     Read
#  define   WRITE_ENTRY    Write
#  define   FLUSH          Flush
#  define   SET_PATH       SetPath
#  define   CHANGE_PATH    SetPath
#  define   GET_PATH       GetPath



// this macro enforces the convention that for config entry FOO the default
// value is FOO_D(EFAULT).
#define READ_APPCONFIG(key) (Profile::GetAppConfig()->READ_ENTRY(key, key##_D))
#define READ_CONFIG(profile, key) profile->readEntry(key, key##_D)

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

/**@name Profile management classes. */
//@{
   
/** ProfileBase, an abstract base class for all Profile classes.
    This class serves as a common base class for the ProfileAppConfig
    and Profile classes. The real implementation is class Profile. In
    order to allow it to inherit values from an AppConfig class
    instance, the wrapper class ProfileAppConfig is available.
    @see Profile
    @see ProfileAppConfig
    @see AppConfig
*/
class ProfileBase : public CommonBase
{
public:
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(const char *szKey,
                            const char *szDefault = NULL) const = 0;
   /// Read an integer value.
   virtual int readEntry(const char *szKey, int Default) const = 0;
   /// Read a bool value.
   virtual bool readEntry(const char *szKey, bool Default) const = 0;
   /// Write back the character value.
   virtual bool writeEntry(const char *szKey, const char *szValue) = 0;
   /// Write back the int value.
   virtual bool writeEntry(const char *szKey, int Value) = 0;
   /// Write back the bool value.
   virtual bool writeEntry(const char *szKey, bool Value) = 0;
   //@}
};

/** ConfigFileManager class, this class allocates and deallocates
   wxConfig objects for the profile so to ensure that every config
   file gets opened only once.
*/

/** A structure holding name and wxConfig pointer.
   This is the element of the list.
*/
struct FCData
{
  String      fileName;
  wxConfig *fileConfig;

  IMPLEMENT_DUMMY_COMPARE_OPERATORS(FCData)
};

/** A list of all loaded wxConfigs
   @see FCData
*/
KBLIST_DEFINE(FCDataList, FCData);

class ConfigFileManager : public CommonBase
{
private:
   FCDataList *fcList;
   
public:
   /** Constructor
     */
   ConfigFileManager();

   /** Destructor
       writes back all entries
   */
   ~ConfigFileManager();

   /** Get a wxConfig object.
       @param fileName name of configuration file
       @param isApp if we're creating the app config
       @return the wxConfig object
   */
   wxConfig *GetConfig(String const &fileName, bool isApp = FALSE);

   /// Prints a list of all entries.
   DEBUG_DEF

   CB_DECLARE_CLASS(ConfigFileManager, CommonBase);
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
private:
   /// The wxConfig object.
   wxConfig  *fileConfig;
   /// The parent profile.
   ProfileBase const *parentProfile;
   /// Name of this profile
   String   profileName;
   // true if the object was successfully initialized
   bool isOk;

   /** This static member variable contains the global (application-level)
       config object where the entries are searched if not found everywhere
       else.
   */
   static wxConfig *appConfig;

public:
   /// get the top level config object
   static wxConfig *GetAppConfig() { return appConfig; }

   /** Constructor for the appConfig entry 
       @param appConfigFile - the full file name of app config file
   */
   Profile(const String& appConfigFile);

   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MC_PROFILEPATH).
       
   */
   Profile(String const &iClassName, ProfileBase const *Parent);

   /** Destructor, writes back those entries that got changed.
   */
   ~Profile();

   /** Query if Profile has been initialised successfully.
       @return true if everything is fine
   */
   bool IsInitialised(void) const { return isOk; }

   /// get the associated config object
   wxConfig *GetConfig() const { return fileConfig; }

   /// get our name
   const String& GetProfileName() const { return profileName; }

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
      /// Read a character entry.
   String readEntry(const char *szKey, const char *szDefault = NULL) const;
      /// Read an integer value.
   int readEntry(const char *szKey, int Default) const;
      /// Read a bool value.
   bool readEntry(const char *szKey, bool Default) const;
      /// Write back the character value.
   bool writeEntry(const char *szKey, const char *szValue);
      /// Write back the int value.
   bool writeEntry(const char *szKey, int Value);
      /// Write back the bool value.
   bool writeEntry(const char *szKey, bool Value);
   //@}

   /// return class name
   const char *GetClassName(void) const { return "MailFolder"; }
   
   CB_DECLARE_CLASS(Profile, CommonBase);
};
//@}

// ----------------------------------------------------------------------------
// a tinu utility class which is used to temporary change the config path, for
// example:
//    {
//       ProfilePathChanger ppc(profile->GetConfig(), "/M/Frames");
//       profile->WriteEntry("x", 400);
//       ...
//       // path automatically restored here
//    }
// ----------------------------------------------------------------------------
class ProfilePathChanger
{
public:
   ProfilePathChanger(wxConfig *config, const String& path)
   {
      m_config = config;
      m_strOldPath = m_config->GetPath();
      m_config->SetPath(path);
   }

   ~ProfilePathChanger() { m_config->SetPath(m_strOldPath); }

private:
   wxConfig *m_config;
   String        m_strOldPath;
};

// ----------------------------------------------------------------------------
// two handy functions for savings/restoring arrays of strings to/from config
// ----------------------------------------------------------------------------
void SaveArray(wxConfigBase& conf, const wxArrayString& astr, const char *key);
void RestoreArray(wxConfigBase& conf, wxArrayString& astr, const char *key);

#endif // PROFILE_H
