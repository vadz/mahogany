/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/

#ifndef PROFILE_H
#define PROFILE_H

#ifdef __GNUG__
#   pragma interface "Profile.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifndef  USE_PCH
#  include  "kbList.h"
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// the classes in this file can use either the (old) standalone appconf,
// or wxConfig included in wxWindows 2
#ifdef   USE_WXCONFIG
#  define   READ_ENTRY     Read
#  define   WRITE_ENTRY    Write
#  define   FLUSH          Flush
#  define   SET_PATH       SetPath
#  define   CHANGE_PATH    SetPath
#  define   GET_PATH       GetPath

#  ifndef   USE_PCH
#     include <wx/config.h>
#  endif

   // both types are mapped just on wxConfig
   typedef  wxConfigBase       FileConfig;
   typedef  wxConfigBase       AppConfig;
#else
#  include  "appconf.h"

#  define   READ_ENTRY     readEntry
#  define   WRITE_ENTRY    writeEntry
#  define   FLUSH          flush
#  define   SET_PATH       setCurrentPath
#  define   CHANGE_PATH    changeCurrentPath
#  define   GET_PATH       getCurrentPath
#endif

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
      All these functions are just identical to the FileConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual const char *readEntry(const char *szKey,
                                 const char *szDefault = NULL) const = 0;
   /// Read an integer value.
   virtual int readEntry(const char *szKey, int Default = 0) const = 0;
   /// Read a bool value.
   virtual bool readEntry(const char *szKey, bool Default = false) const = 0;
   /// Write back the character value.
   virtual bool writeEntry(const char *szKey, const char *szValue) = 0;
   /// Write back the int value.
   virtual bool writeEntry(const char *szKey, int Value) = 0;
   /// Write back the bool value.
   virtual bool writeEntry(const char *szKey, bool Value) = 0;
   //@}
};

/** ConfigFileManager class, this class allocates and deallocates
   FileConfig objects for the profile so to ensure that every config
   file gets opened only once.
*/

/** A structure holding name and FileConfig pointer.
   This is the element of the list.
*/
struct FCData
{
  String      fileName;
  FileConfig *fileConfig;

  IMPLEMENT_DUMMY_COMPARE_OPERATORS(FCData)
};

/** A list of all loaded FileConfigs
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

   /** Get a FileConfig object.
       @param fileName name of configuration file
       @return the FileConfig object
   */
   FileConfig *GetConfig(String const &fileName);

   /// Prints a list of all entries.
   DEBUG_DEF

   CB_DECLARE_CLASS(ConfigFileManager, CommonBase);
};

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the FileConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
   @see ProfileBase
   @see FileConfig
*/

class Profile : public ProfileBase
{
private:
   /// The FileConfig object.
   FileConfig  *fileConfig;
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
   static FileConfig *appConfig;

public:
   /// get the top level config object
   static FileConfig *GetAppConfig() { return appConfig; }

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
   FileConfig *GetConfig() const { return fileConfig; }

   /**@name Reading and writing entries.
      All these functions are just identical to the FileConfig ones.
   */
   //@{
      /// Read a character entry.
   const char *readEntry(const char *szKey, const char *szDefault = NULL) const;
      /// Read an integer value.
   int readEntry(const char *szKey, int Default = 0) const;
      /// Read a bool value.
   bool readEntry(const char *szKey, bool Default = false) const;
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

#endif // PROFILE_H
