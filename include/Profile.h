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

/// read from the global config section
#define   READ_APPCONFIG(key) (mApplication->GetProfile()->readEntry(key, key##_D))
/// read from a normal profile
#define   READ_CONFIG(profile, key) profile->readEntry(key, key##_D)

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
    @see ProfileAppConfig (VZ: there is no such class??)
    @see AppConfig
*/
class ProfileBase : public MObjectRC
{
public:
   /// Create a normal Profile object
   static ProfileBase * CreateProfile(String const & classname,
                                      ProfileBase const *parent = NULL);
   /// Create a global configuration profile object
   static ProfileBase * CreateGlobalConfig(String const &  filename);
   /// Create a dummy Profile just inheriting from the top level
   static ProfileBase * CreateEmptyProfile(ProfileBase const *parent = NULL);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(String const & key,
                            String const & defaultvalue = (const char *)NULL) const = 0;
   /// Read a character entry.
   String readEntry(String const &  key,
                    const char *defaultvalue = NULL) const;
   /// Read an integer value.
   virtual long readEntry(String const & key, long defaultvalue) const = 0;
   /// Read an integer value.
   virtual int readEntry(String const & key, int defaultvalue) const
      { return (int) readEntry(key, (long)defaultvalue); }
   /// Write back the character value.
   virtual bool writeEntry(String const & key, String const & Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(String const & key, long Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(String const & key, int Value)
      { return writeEntry(key, (long)Value); }
   //@}
   /// set the path within the profile,just like cd
   virtual void   SetPath(String const & path) = 0;
   /// query the current path
   virtual String GetPath(void) const = 0;
   /// return true if the entry is defined
   virtual bool HasEntry(String const & key) const = 0;
   /// delete the entry group specified by path
   virtual void DeleteGroup(String const & path) = 0;
   /// return the name of the profile
   virtual String GetProfileName(void) = 0;

   /** @name Enumerating groups/entries

       again, this is just directly forwarded to wxConfig
   */
   bool GetFirstGroup(String& s, long l);
   bool GetNextGroup(String& s, long l);

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
   /// the config object we use (may be NULL)
   // VZ: when can it be NULL exactly??
   wxConfigBase  *m_config;

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
class ProfileEnvVarSuspend
{
public:
   ProfileEnvVarSuspend(ProfileBase *profile)
   {
      m_profile = profile;
      m_wasExpanding = profile->IsExpandingEnvVars();
      profile->SetExpandEnvVars(FALSE);
   }

   ~ProfileEnvVarSuspend()
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
   ProfilePathChanger(ProfileBase *config, const String& path)
   {
      m_config = config;
      m_strOldPath = m_config->GetPath();
      m_config->SetPath(path);
   }

   ~ProfilePathChanger() { m_config->SetPath(m_strOldPath); }

private:
   ProfileBase *m_config;
   String       m_strOldPath;
};


// ----------------------------------------------------------------------------
// two handy functions for savings/restoring arrays of strings to/from config
// ----------------------------------------------------------------------------
void SaveArray(ProfileBase& conf, const wxArrayString& astr, String const & key);
void RestoreArray(ProfileBase& conf, wxArrayString& astr, String const & key);

#endif // PROFILE_H
