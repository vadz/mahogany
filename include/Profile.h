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
//
// ----------------------------------------------------------------------------

class kbStringList;

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

/**@name Profile management classes. */
//@{

/// read from the global config section
#if 0
//FIXME!!!! Cannot verify correct global profile path settings!
//#ifdef DEBUG
   // assert(0) is called when the app profile path is incorrect (should never
   // be changed by the app or must be restored until the next call of
   // READ_APPCONFIG() if it is)
#   define   READ_APPCONFIG(key) ( \
               mApplication->GetProfile()->GetConfig()->GetPath() == M_PROFILE_CONFIG_SECTION ? \
               (assert(0), mApplication->GetProfile()->readEntry(key, key##_D)) : \
              mApplication->GetProfile()->readEntry(key, key##_D))
//#else
#endif
#   define READ_APPCONFIG(key) \
      (mApplication->GetProfile()->readEntry(key, key##_D))
//#endif

/// read from a normal profile
#define   READ_CONFIG(profile, key) profile->readEntry(key, key##_D)

// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------

class wxConfigBase;


/** ProfileBase, an abstract base class for all Profile classes.
    This class serves as a common base class for the ProfileAppConfig
    and Profile classes. The real implementation is class Profile. In
    order to allow it to inherit values from an AppConfig class
    instance, the wrapper class ProfileAppConfig is available.
*/
class ProfileBase : public MObjectRC
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
   static ProfileBase * CreateGlobalConfig(const String & filename);
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(String const & classname,
                                      ProfileBase const *parent = NULL);
   /// Create a Profile object for a plugin module
   static ProfileBase * CreateModuleProfile(String const & classname,
                                            ProfileBase const *parent = NULL);
   /// Create a dummy Profile just inheriting from the top level
   static ProfileBase * CreateEmptyProfile(ProfileBase const *parent = NULL);

   /// Delete the global config object
   static void DeleteGlobalConfig();

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
   virtual String readEntry(String const & key,
                            String const & defaultvalue = (const char*)NULL,
                            bool *found = NULL) const = 0;
   /// Read a character entry.
   String readEntry(String const &  key,
                    const char *defaultvalue = NULL,
                    bool *found = NULL) const;
   /// Read an integer value.
   virtual long readEntry(String const & key,
                          long defaultvalue,
                          bool *found = NULL) const = 0;
   /// Read an integer value.
   int readEntry(String const & key,
                 int defaultvalue,
                 bool *found = NULL) const
      { return (int)readEntry(key, (long)defaultvalue, found); }
   /// Read a bool value.
   bool readEntry(const String & key,
                  bool defaultvalue,
                  bool *found = NULL) const
      { return readEntry(key, (long)defaultvalue, found) != 0; }

   /// Write back the character value.
   virtual bool writeEntry(String const & key, String const & Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(String const & key, long Value) = 0;
   //@}

   /// return true if the entry is defined
   virtual bool HasEntry(String const & key) const = 0;
   /// return true if the group exists
   virtual bool HasGroup(String const & name) const = 0;
   /// delete the entry specified by path
   virtual void DeleteEntry(const String& key) = 0;
   /// delete the entry group specified by path
   virtual void DeleteGroup(String const & path) = 0;
   /// rename a group
   virtual bool Rename(const String& oldName, const String& newName) = 0;
   /// return the name of the profile
   virtual const String &GetName(void) const = 0;

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
   virtual ProfileBase *GetParent(void) const = 0;
   /** @name Managing environment variables
       just expose wxConfig methods (we do need them to be able to read the
       real config values, i.e. to disable expansion, sometimes)
   */
   /// are we automatically expanding env vars?
   bool IsExpandingEnvVars() const;
   /// set the flag which tells if we should auto expand env vars
   void SetExpandEnvVars(bool bDoIt = TRUE);

   // for internal use by wxWindows related code only: get the pointer to the
   // underlying wxConfig object. ProfileBase readEntry() functions should be
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

   /// is this profile a (grand) parent of the given one?
   virtual bool IsAncestor(ProfileBase *profile) const = 0;

protected:
   /// why does egcs want this?
   ProfileBase() {}

   /// global wxConfig object, shared by all profiles
   static wxConfigBase *ms_GlobalConfig;
private:
   /// forbid copy construction
   ProfileBase(ProfileBase const &);
   /// forbid assignments
   ProfileBase & operator=(const ProfileBase & );
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
      { m_profile = ProfileBase::CreateProfile(name); }
   ~Profile_obj()
      { SafeDecRef(m_profile); }

   // provide access to the real thing via operator->
   ProfileBase *operator->() const { return m_profile; }

   // implicit conversion to the real pointer (dangerous, but necessary for
   // backwards compatibility)
   operator ProfileBase *() const { return m_profile; }

private:
   // no assignment operator/copy ctor
   Profile_obj(const Profile_obj&);
   Profile_obj& operator=(const Profile_obj&);

   ProfileBase *m_profile;
};

// ----------------------------------------------------------------------------
// a small class to temporarily suspend env var expansion
// ----------------------------------------------------------------------------
class ProfileEnvVarSave
{
public:
   ProfileEnvVarSave(ProfileBase const *profile, bool expand = false)
   {
      // we don't really change it, just temporarily change its behaviour
      m_profile = (ProfileBase *)profile;
      m_wasExpanding = profile->IsExpandingEnvVars();
      m_profile->SetExpandEnvVars(expand);
   }

   ~ProfileEnvVarSave()
   {
      m_profile->SetExpandEnvVars(m_wasExpanding);
   }

private:
  ProfileBase *m_profile;
  bool         m_wasExpanding;
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
   ProfilePathChanger(ProfileBase const *config, const String& path)
      {
         // we don't really change it, just temporarily change the path
         m_config = (ProfileBase *)config;
         m_config->SetPath(path);
      }

   ~ProfilePathChanger() { m_config->ResetPath(); }

private:
   ProfileBase *m_config;
};

// ----------------------------------------------------------------------------
// two handy functions for savings/restoring arrays of strings to/from config
// ----------------------------------------------------------------------------
void SaveArray(ProfileBase *conf, const wxArrayString& astr, String const & key);
void RestoreArray(ProfileBase * conf, wxArrayString& astr, String const & key);
//@}
#endif // PROFILE_H
