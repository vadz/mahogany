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

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define READ_APPCONFIG(key) (mApplication->GetProfile()->readEntry(key, key##_D))
#define READ_CONFIG(profile, key) profile->readEntry(key, key##_D)

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

class wxConfigBase;

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
class ProfileBase : public MObject
{
public:
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(String const &classname, ProfileBase const *parent);
   /// Create a global configuration profile object
   static ProfileBase * CreateGlobalConfig(String const & filename);
   
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(String const &key, String const
                            &defaultvalue = (const char *)NULL) const = 0;
   /// Read a character entry.
   String readEntry(String const &key,
                    const char *defaultvalue = NULL) const
      {
         String str;
         str = readEntry(key, String(defaultvalue));
         return str;
      }
   /// Read an integer value.
   virtual long readEntry(String const &key, long defaultvalue) const = 0;
   /// Read an integer value.
   virtual int readEntry(String const &key, int defaultvalue) const
      { return (int) readEntry(key, (long)defaultvalue); }
   /// Read a bool value.
   virtual bool readEntry(String const &key, bool defaultvalue) const = 0;
   /// Write back the character value.
   virtual bool writeEntry(String const &key, String const &Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(String const &key, long Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(String const &key, int Value)
      { return writeEntry(key, (long)Value); }
   /// Write back the bool value.
   virtual bool writeEntry(String const &key, bool Value) = 0;
   //@}
   /// set the path within the profile,just like cd
   virtual void   SetPath(String const &path) = 0;
   /// query the current path
   virtual String GetPath(void) const = 0;
   /// return true if the entry is defined
   virtual bool HasEntry(String const &key) const = 0;
   /// delete the entry group specified by path
   virtual void DeleteGroup(String const &path) = 0;
   /// return the name of the profile
   virtual String GetProfileName(void) = 0;
protected:
   /// why does egcs want this?
   ProfileBase() {}  
private:
   /// forbid copy construction
   ProfileBase(ProfileBase const &);
   /// forbid assignments
   ProfileBase & operator=(const ProfileBase & );

};


/** ConfigFileManager class, this class allocates and deallocates
   wxConfig objects for the profile so to ensure that every config
   file gets opened only once.
*/
class ConfigFileManager : public CommonBase
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

   /** Get a wxConfig object.
       @param fileName name of configuration file
       @param isApp if we're creating the app config
       @return the wxConfig object
   */
   wxConfigBase *GetConfig(String const &fileName, bool isApp = FALSE);

   /// Prints a list of all entries.
   DEBUG_DEF

   CB_DECLARE_CLASS(ConfigFileManager, CommonBase);
};


class ProfilePathChanger
{
public:
   ProfilePathChanger(ProfileBase *config, const String& path);
   ~ProfilePathChanger();
private:
   ProfileBase *m_config;
   String      m_strOldPath;
};


// ----------------------------------------------------------------------------
// two handy functions for savings/restoring arrays of strings to/from config
// ----------------------------------------------------------------------------
void SaveArray(ProfileBase& conf, const wxArrayString& astr, String const &key);
void RestoreArray(ProfileBase& conf, wxArrayString& astr, String const &key);


#endif // PROFILE_H
