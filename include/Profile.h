/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
// ----------------------------------------------------------------------------

#include "MObject.h"

class kbStringList;

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/**@name Profile management classes. */
//@{

/// read from the global config section
#   define READ_APPCONFIG(key) \
      (mApplication->GetProfile()->readEntry(key, key##_D))
//#endif

/// read from a normal profile
#define   READ_CONFIG(profile, key) profile->readEntry(key, key##_D)

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

class wxConfigBase;


/** Profile, an abstract base class for all Profile classes.
    This class serves as a common base class for the ProfileAppConfig
    and Profile classes. The real implementation is class Profile. In
    order to allow it to inherit values from an AppConfig class
    instance, the wrapper class ProfileAppConfig is available.
*/
class Profile : public MObjectRC
{
public:
   /** An enum explaining the possible types of profiles. In fact,
       just a number stored as a normal profile entry which must be
       maintained manually.
   */
   enum Type
   {
      /// No profile type specified.
      PT_Unknown = -1,
      /// Any profile, default argument for ListProfiles().
      PT_Any = -1,  // PT_Unknown
      /// This profile belongs to a folder.
      PT_FolderProfile = 1
   };

   /// Creates the one global config object.
   static Profile * CreateGlobalConfig(const String & filename);
   /// Create a normal Profile object
   static Profile * CreateProfile(const String & classname,
                                      const Profile *parent = NULL);
   /// Create a Profile object for a plugin module
   static Profile * CreateModuleProfile(const String & classname,
                                            const Profile *parent = NULL);
   /// Create a dummy Profile just inheriting from the top level
   static Profile * CreateEmptyProfile(const Profile *parent = NULL);

   /// creates/gets an Identity entry in the configuration
   static Profile * CreateIdentity(const String &name);

   /// creates/gets a FilterProfile entry in the configuration
   static Profile * CreateFilterProfile(const String &name);

   /// Delete the global config object
   static void DeleteGlobalConfig();

   /// does a profile/config group with this name exist?
   static bool ProfileExists(const String &name);
   /** List all profiles of a given type or all profiles in total.
       @param type Type of profile to list or -1 for all.
       @return a pointer to kbStringList of profile names to be freed by caller.
   */
   static kbStringList * ListProfiles(int type = PT_Any);

   /// Flush all (disk-based) profiles now
   static void FlushAll();

   /// some characters are invalid in the profile name, replace them
   static String FilterProfileName(const String& profileName);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(const String & key,
                            const String & defaultvalue = (const char*)NULL,
                            bool *found = NULL) const = 0;
   /// Read a character entry.
   String readEntry(const String &  key,
                    const char *defaultvalue = NULL,
                    bool *found = NULL) const;
   /// Read an integer value.
   virtual long readEntry(const String & key,
                          long defaultvalue,
                          bool *found = NULL) const = 0;
   /// Read an integer value.
   int readEntry(const String & key,
                 int defaultvalue,
                 bool *found = NULL) const
      { return (int)readEntry(key, (long)defaultvalue, found); }
   /// Read a bool value.
   bool readEntry(const String & key,
                  bool defaultvalue,
                  bool *found = NULL) const
      { return readEntry(key, (long)defaultvalue, found) != 0; }

   /// Read an integer entry from this profile only, don't look upwards
   virtual int readEntryFromHere(const String& key, int defvalue) const = 0;

   /// Write back the character value.
   virtual bool writeEntry(const String & key, const String & Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(const String & key, long Value) = 0;
   //@}

   /// return true if the entry is defined
   virtual bool HasEntry(const String & key) const = 0;
   /// return true if the group exists
   virtual bool HasGroup(const String & name) const = 0;
   /// delete the entry specified by path
   virtual bool DeleteEntry(const String& key) = 0;
   /// delete the entry group specified by path
   virtual bool DeleteGroup(const String & path) = 0;
   /// rename a group
   virtual bool Rename(const String& oldName, const String& newName) = 0;
   /// return the name of the profile
   virtual const String GetName(void) const = 0;

   /** @name Enumerating groups/entries
       again, this is just directly forwarded to wxConfig
   */
   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const = 0;
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const = 0;
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const = 0;
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const = 0;

   /// Returns a unique, not yet existing sub-group name: //MT!!
   virtual String GetUniqueGroupName(void) const = 0;
   
   /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const = 0;
   /** @name Managing environment variables
       just expose wxConfig methods (we do need them to be able to read the
       real config values, i.e. to disable expansion, sometimes)
   */
   /// are we automatically expanding env vars?
   bool IsExpandingEnvVars() const;
   /// set the flag which tells if we should auto expand env vars
   void SetExpandEnvVars(bool bDoIt = TRUE);

   // for internal use by wxWindows related code only: get the pointer to the
   // underlying wxConfig object. Profile readEntry() functions should be
   // used for reading/writing the entries!
   wxConfigBase *GetConfig() const { return ms_GlobalConfig; }

   virtual void SetPath(const String &path) = 0;
   virtual void ResetPath(void) = 0;
/*     virtual const String GetPath() const = 0;
*/

   /// Set temporary/suspended operation.
   virtual void Suspend(void) = 0;
   /// Commit changes from suspended mode.
   virtual void Commit(void) = 0;
   /// Discard changes from suspended mode.
   virtual void Discard(void) = 0;
   /// Is the profile currently suspended?
   virtual bool IsSuspended(void) const = 0;

   /// return the array containing the names of all existing identities
   static wxArrayString GetAllIdentities();

   /// return the array containing the names of all existing filters
   static wxArrayString GetAllFilters();

   /// Set the identity to be used for this profile
   virtual void SetIdentity(const String & idName) = 0;
   /// Unset the identity set by SetIdentity
   virtual void ClearIdentity(void) = 0;
   // Get the currently used identity
   virtual String GetIdentity(void) const = 0;

   /// is this profile a (grand) parent of the given one?
   virtual bool IsAncestor(Profile *profile) const = 0;

protected:
   /// why does egcs want this?
   Profile() {}

   /// global wxConfig object, shared by all profiles
   static wxConfigBase *ms_GlobalConfig;

private:
   /// helper for GetAllIdentities/GetAllFilters
   static wxArrayString GetAllGroupsUnder(const String& path);

   /// forbid copy construction
   Profile(const Profile &);
   /// forbid assignments
   Profile & operator=(const Profile & );
};


// ============================================================================
// Helper classes
// ============================================================================

// ----------------------------------------------------------------------------
// A smart reference to Profile - an easy way to avoid memory leaks
// ----------------------------------------------------------------------------

class Profile_obj
{
public:
   // ctor & dtor
   Profile_obj(const String& name)
      { m_profile = Profile::CreateProfile(name); }
   ~Profile_obj()
      { SafeDecRef(m_profile); }

   // provide access to the real thing via operator->
   Profile *operator->() const { return m_profile; }

   // implicit conversion to the real pointer (dangerous, but necessary for
   // backwards compatibility)
   operator Profile *() const { return m_profile; }

private:
   // no assignment operator/copy ctor
   Profile_obj(const Profile_obj&);
   Profile_obj& operator=(const Profile_obj&);

   Profile *m_profile;
};

// ----------------------------------------------------------------------------
// a small class to temporarily suspend env var expansion
// ----------------------------------------------------------------------------

class ProfileEnvVarSave
{
public:
   ProfileEnvVarSave(const Profile *profile, bool expand = false)
   {
      // we don't really change it, just temporarily change its behaviour
      m_profile = (Profile *)profile;
      m_wasExpanding = profile->IsExpandingEnvVars();
      m_profile->SetExpandEnvVars(expand);
   }

   ~ProfileEnvVarSave()
   {
      m_profile->SetExpandEnvVars(m_wasExpanding);
   }

private:
  Profile *m_profile;
  bool     m_wasExpanding;
};


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

class ProfilePathChanger
{
public:
   ProfilePathChanger(const Profile *config, const String& path)
      {
         // we don't really change it, just temporarily change the path
         m_config = (Profile *)config;
         m_config->SetPath(path);
      }

   ~ProfilePathChanger() { m_config->ResetPath(); }

private:
   Profile *m_config;
};

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// functions for savings/restoring arrays of strings to/from config
void SaveArray(Profile *conf, const wxArrayString& astr, const String & key);
void RestoreArray(Profile * conf, wxArrayString& astr, const String & key);

//@}

#endif // PROFILE_H
