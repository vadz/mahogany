/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
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
#ifdef DEBUG
   // assert(0) is called when the app profile path is incorrect (should never
   // be changed by the app or must be restored until the next call of
   // READ_APPCONFIG() if it is)
   #define   READ_APPCONFIG(key) ( \
      mApplication->GetProfile()->GetPath() == M_PROFILE_CONFIG_SECTION ? \
      (assert(0), mApplication->GetProfile()->readEntry(key, key##_D)) : \
      mApplication->GetProfile()->readEntry(key, key##_D))
#else
   #define READ_APPCONFIG(key) \
      (mApplication->GetProfile()->readEntry(key, key##_D))
#endif

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
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(String const & classname,
                                      ProfileBase const *parent = NULL);
   /// Create a Profile object for a mail folder.
   static ProfileBase * CreateFolderProfile(const String & iClassName,
                                            ProfileBase const *Parent);
   /// Create a global configuration profile object
   static ProfileBase * CreateGlobalConfig(String const &  filename);
   /// Create a dummy Profile just inheriting from the top level
   static ProfileBase * CreateEmptyProfile(ProfileBase const *parent = NULL);

   /// Flush all (disk-based) profiles now
   static void FlushAll();

   /** An enum explaining the possible types of profiles. In fact,
       just a number stored as a normal profile entry which must be
       maintained manually.
   */
   enum Type
   {
      /// No profile type specified.
      PT_unknown,
      /// This profile belongs to a folder.
      PT_FolderProfile
   };

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

   /// set the path within the profile,just like cd
   virtual void   SetPath(String const & path) = 0;
   /// query the current path
   virtual String GetPath(void) const = 0;
   /// return true if the entry is defined
   virtual bool HasEntry(String const & key) const = 0;
   /// return true if the group exists
   virtual bool HasGroup(String const & name) const = 0;
   /// delete the entry group specified by path
   virtual void DeleteGroup(String const & path) = 0;
   /// rename a group
   virtual bool Rename(const String& oldName, const String& newName) = 0;
   /// return the name of the profile
   virtual String GetProfileName(void) = 0;

   /** @name Enumerating groups/entries

       again, this is just directly forwarded to wxConfig
   */
      /// return the number of subgroups in the current group
   size_t GetGroupCount() const;
      /// see wxConfig docs
   bool GetFirstGroup(String& s, long& l) const;
      /// see wxConfig docs
   bool GetNextGroup(String& s, long& l) const;

   /** @name Listing all external profiles
       @return a kbStringList of profile names, must be freed by
       caller, never NULL
    */
   static kbStringList *GetExternalProfiles(void);

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
   wxConfigBase *GetConfig() const { return m_config; }

protected:
   /** The config object we use (may be NULL).
       A normal profile either has a wxFileConfig associated with it
       or not. If not, m_config is NULL and all read/write operations
       will be passed on to the parent profile if it exists or applied
       to the corresponding section in the global profile. Of course,
       the global profile must always have a non-NULL pointer here.
   */
   wxConfigBase  *m_config;

   /// Do we need to expand environment variables (only if m_config == NULL)
   bool m_expandEnvVars;

   /// why does egcs want this?
   ProfileBase() {}

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
         m_strOldPath = m_config->GetPath();
         m_config->SetPath(path);
      }

   ~ProfilePathChanger() { m_config->SetPath(m_strOldPath); }

private:
   ProfileBase *m_config;
   String      m_strOldPath;
};


// ----------------------------------------------------------------------------
// two handy functions for savings/restoring arrays of strings to/from config
// ----------------------------------------------------------------------------
void SaveArray(ProfileBase& conf, const wxArrayString& astr, String const & key);
void RestoreArray(ProfileBase& conf, wxArrayString& astr, String const & key);
//@}
#endif // PROFILE_H
