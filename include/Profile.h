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
#pragma interface "Profile.h"
#endif

#ifndef	USE_PCH
#	include	"appconf.h"
#endif

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
class ProfileBase
{
public:
   /**@name Reading and writing entries.
      All these functions are just identical to the FileConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual const char *readEntry(const char *szKey, const char *szDefault =
			 NULL) const = 0;
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
struct	FCData
{
  String		fileName;
  FileConfig	*fileConfig;

  IMPLEMENT_DUMMY_COMPARE_OPERATORS(FCData)
};

/** A list of all known icons.
   @see FCData
*/
typedef std::list<FCData>	FCDataList;

class ConfigFileManager : public CommonBase
{
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
   FileConfig * GetConfig(String const &fileName);

#ifdef DEBUG
   /// Prints a list of all entries.
   void Debug(void);
#endif
   /** This class is always initialised.
       @return true 
   */
   bool IsInitialised(void) const
      { return true; }

   CB_DECLARE_CLASS(ConfigFileManager, CommonBase);
};

/** A Profile class which wraps an existing AppConfig object.
    This class provides the same functionality as a profile class but
    is linked to an AppConfig object passed to its constructor on
    creation. Therefor it allows an AppConfig object to serve as the
    root of a Profile inheritance tree.
    @see AppConfig
    @see ProfileBase
  */
class ProfileAppConfig : public ProfileBase, public CommonBase
{
   /// The AppConfig object.
   AppConfig	*appConfig;
   /// Is Profile ready to use?
   bool	isOk;
protected:
public:
   /** Constructor.
       @param ac The existing AppConfig object to wrap.
   */
   ProfileAppConfig(AppConfig *ac)
      {	appConfig = ac; }

   /** Query if Profile has been initialised successfully.
       @return true = This object is considered to be always initialised.
   */
   bool IsInitialised(void) const
      { return true; }
   
   /**@name Reading and writing entries.
      All these functions are just identical to the FileConfig ones.
   */
   //@{
   /// Read a character entry.
   inline
   const char *readEntry(const char *szKey, const char *szDefault =
			 NULL) const
      {
	 DBGLOG("ProfileAppConfig::readEntry(" << szKey << ',' << szDefault << ')');
	 return appConfig->readEntry(szKey, szDefault);
      }
   /// Read an integer value.
   inline
   int readEntry(const char *szKey, int Default = 0) const
      {
	 DBGLOG("ProfileAppConfig::readEntry(" << szKey << ',' << Default << ')');
	 return appConfig->readEntry(szKey, (long) Default);
      }
   /// Read a bool value.
   inline
   bool readEntry(const char *szKey, bool Default = false) const
      {
	 DBGLOG("ProfileAppConfig::readEntry(" << szKey << ',' << Default << ')');
	 return appConfig->readEntry(szKey, (long int)Default) != 0;
      }
   /// Write back the character value.
   inline
   bool writeEntry(const char *szKey, const char *szValue)
      {
	 return appConfig->writeEntry(szKey, szValue) != 0;
      }
   /// Write back the int value.
   inline
   bool writeEntry(const char *szKey, int Value)
      {
	 return appConfig->writeEntry(szKey, (long) Value) != 0;
      }
   /// Write back the bool value.
   inline
   bool writeEntry(const char *szKey, bool Value)
      {
	 return appConfig->writeEntry(szKey, (long int) Value) != 0;
      }
   //@}
   CB_DECLARE_CLASS(ProfileAppConfig, CommonBase);
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

class Profile : public ProfileBase, public CommonBase
{
   /// The FileConfig object.
   FileConfig	*fileConfig;
   /// The parent profile.
   ProfileBase const *parentProfile;
   /// Name of this profile
   String	profileName;
   /// Is Profile ready to use?
   bool	isOk;
   /** Common to all Profile objects: a manager class taking care of
       allocating the FileConfig objects.
   */
   static ConfigFileManager	cfManager;
public:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by mApplication.readEntry(MC_PROFILEPATH).
       
   */
   Profile(String const &iClassName, ProfileBase const *Parent = NULL);

   /** Destructor, writes back those entries that got changed.
   */
   ~Profile();

   /** Query if Profile has been initialised successfully.
       @return true if everything is fine
   */
   bool IsInitialised(void) const
      { return isOk; }
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
   CB_DECLARE_CLASS(Profile, CommonBase);
};
//@}


#endif
