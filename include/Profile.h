///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Profile.h: Mahogany Profile class
// Purpose:     Profiles are used for storing all program options and much more
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef PROFILE_H
#define PROFILE_H

#ifdef __GNUG__
#   pragma interface "Profile.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "MObject.h"

class ConfigSource;

// ----------------------------------------------------------------------------
// profile-related constants
// ----------------------------------------------------------------------------

/** @name The sections of the configuration file. */
///@{

/**
  The section in the global configuration file used for storing
  profiles (without trailing '/')
*/
#define   M_PROFILE_CONFIG_SECTION   _T("/Profiles")
#define   M_IDENTITY_CONFIG_SECTION  _T("/Ids")
#define   M_FILTERS_CONFIG_SECTION   _T("/Filters")
#define   M_FRAMES_CONFIG_SECTION    _T("/Frames")
#define   M_TEMPLATES_CONFIG_SECTION _T("/Templates")

/**
  The section in the global configuration file used for storing
  folder profiles (without trailing '/')
*/
#ifndef M_FOLDER_CONFIG_SECTION
#  define   M_FOLDER_CONFIG_SECTION   M_PROFILE_CONFIG_SECTION
#endif

/**
  The section in the global configuration file used for storing
  control settings.
*/
#define M_SETTINGS_CONFIG_SECTION       _T("Settings")

/// the root path for all ADB editor config entries
#define ADB_CONFIG_PATH _T("AdbEditor")

/** @name Keys where the template for messages of given type is stored */
//@{
#define MP_TEMPLATE_NEWMESSAGE   _T("NewMessage")
#define MP_TEMPLATE_NEWARTICLE   _T("NewArticle")
#define MP_TEMPLATE_REPLY        _T("Reply")
#define MP_TEMPLATE_FOLLOWUP     _T("Followup")
#define MP_TEMPLATE_FORWARD      _T("Forward")
//@}

//@}

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
   /// flags returned by readEntry()
   enum ReadResult
   {
      /// entry hasn't been found anywhere, default value is returned
      Read_Default = 0,

      /// entry has been found in this entry
      Read_FromHere = 1,

      /// entry has been found in one of the ancestors of this entry
      Read_FromParent = 2
   };


   /// Creates the one global config object.
   static Profile * CreateGlobalConfig(const String & filename);

   /// Create a normal Profile object
   static Profile * CreateProfile(const String & classname,
                                  const Profile *parent = NULL);

   /// Create a Profile object for a plugin module
   static Profile * CreateModuleProfile(const String & classname,
                                        const Profile *parent = NULL);

   /// Create a profile object for a folder with the given (full) name
   static Profile * CreateFolderProfile(const String& foldername);

   /// Create a dummy Profile just inheriting from the top level
   static Profile * CreateEmptyProfile(const Profile *parent = NULL);

   /// creates/gets an Identity entry in the configuration
   static Profile * CreateIdentity(const String &name);

   /// creates/gets a FilterProfile entry in the configuration
   static Profile * CreateFilterProfile(const String &name);

   /**
      Create a temporary profile.

      Such temporary profiles are used for messages opened separately from the
      folder view associated with their folder and for temporary folders (i.e.
      those not in the folder tree at all). They inherit all settings from
      their parent profile but any changes to them -- intentionally --
      disappear when they are destroyed.

      @param parent the parent profile to inherit settings from, global profile
                    is used if this parameter is NULL (default)
      @return a temporary profile object which can be used as any other and
              must be DecRef()'d by caller or NULL if creation failed
    */
   static Profile *CreateTemp(Profile *parent = NULL);


   /// Delete the global config object
   static void DeleteGlobalConfig();

   /// Flush all (disk-based) profiles now, return true if ok, false on error
   static bool FlushAll();

   /// some characters are invalid in the profile name, replace them
   static String FilterProfileName(const String& profileName);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(const String & key,
                            const String & defaultvalue = (const wxChar*)NULL,
                            ReadResult *found = NULL) const = 0;
   /// Read a character entry.
   String readEntry(const String &  key,
                    const wxChar *defaultvalue = NULL,
                    ReadResult *found = NULL) const;
   /// Read an integer value.
   virtual long readEntry(const String & key,
                          long defaultvalue,
                          ReadResult *found = NULL) const = 0;
   /// Read an integer value.
   int readEntry(const String & key,
                 int defaultvalue,
                 ReadResult *found = NULL) const
      { return (int)readEntry(key, (long)defaultvalue, found); }
   /// Read a bool value.
   bool readEntry(const String & key,
                  bool defaultvalue,
                  ReadResult *found = NULL) const
      { return readEntry(key, (long)defaultvalue, found) != 0; }

   /// Read an integer entry from this profile only, don't look upwards
   virtual int readEntryFromHere(const String& key, int defvalue) const = 0;

   /// Write back the character value.
   virtual bool writeEntry(const String& key,
                           const String& value,
                           ConfigSource *config = NULL) = 0;

   /// Write back the int value.
   virtual bool writeEntry(const String& key,
                           long value,
                           ConfigSource *config = NULL) = 0;

   /**
      Write the value in profile only if it is different from the current one.

      This function will only modify the real storage (i.e. config file or the
      registry) if the value specified is different from the one which would be
      returned currently, i.e. different from the value inherited from parent.

      @param key the key to modify
      @param value the new value
      @param defvalue the default value for this key
      @return true if ok, false if an error occured
    */
   virtual bool writeEntryIfNeeded(const String& key,
                                   long value,
                                   long defvalue,
                                   ConfigSource *config = NULL) = 0;

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
   virtual const String& GetName(void) const = 0;

   /**
     Returns a pointer to the parent profile.

     @return pointer to Profile to be DecRef()'d by the caller or NULL
    */
   virtual Profile *GetParent(void) const = 0;


   /**
      @name Enumerating groups/entries

      The enumeration is done over all config sources, i.e. the groups/entries
      of all of them are returned but no name is returned twice even if it
      appears in more than one config source.

      All methods below return true if a group/entry is returned in s and
      false if there are no more of them. For both groups and entries, the
      cookie passed to GetNextXXX() must be the same as passed to
      GetFirstXXX().
    */
   //@{

   /// Class used with the enumeration methods
   class EnumData
   {
   public:
      EnumData();
      ~EnumData();

   private:
      class ProfileEnumDataImpl *m_impl;

      EnumData(const EnumData&);
      EnumData& operator=(const EnumData&);

      friend class Profile;
   };


   /// Get the first subgroup under this profile
   virtual bool GetFirstGroup(String& s, EnumData& cookie) const = 0;

   /// Get the next subgroup of this profile
   virtual bool GetNextGroup(String& s, EnumData& cookie) const = 0;

   /// Get the first entry in this profile
   virtual bool GetFirstEntry(String& s, EnumData& cookie) const = 0;

   /// Get the next entry
   virtual bool GetNextEntry(String& s, EnumData& cookie) const = 0;

   //@}


   /**
      @name Expanding environment variables

      We can (and do, by default) expand the env variables in the values we
      read from config. Sometimes, however, we need to disable this because,
      for example, we want to allow the user to edit the real value form
      profile and not its expansion.

      @sa ProfileEnvVarSave
    */
   //@{

   /// are we automatically expanding env vars?
   bool IsExpandingEnvVars() const { return m_expandEnvVars; }

   /// set the flag which tells if we should auto expand env vars
   void SetExpandEnvVars(bool expand = true) { m_expandEnvVars = expand; }

   //@}


   // for internal use by wxWindows related code only: get the pointer to the
   // underlying wxConfig object.
   //
   // this is DEPRECATED now as it doesn't make sense with multiple config
   // sources, do not use it!
   virtual wxConfigBase *GetConfig() const = 0;

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

   /// get the name of the folder his profile is for or an empty string
   virtual String GetFolderName() const = 0;

   /**
     @name Return config paths used

     Returns the paths we use in wxConfig object for various entities stored in
     the profiles.
    */
   //@{

   /// return the path used for the normal config info (folders options &c)
   static String GetProfilePath()
      { return M_PROFILE_CONFIG_SECTION; }

   /// return the path under which the identities are stored
   static String GetIdentityPath() 
      { return M_IDENTITY_CONFIG_SECTION; }

   /// return the path under which the filters are stored
   static String GetFiltersPath() 
      { return M_FILTERS_CONFIG_SECTION; }

   /// return the path under which the frame options are stored
   static String GetFramesPath() 
      { return M_FRAMES_CONFIG_SECTION; }

   /// return the path under which the templates are stored
   static String GetTemplatesPath() 
      { return M_TEMPLATES_CONFIG_SECTION; }

   //@}

protected:
   // egcs wants this
   Profile() { }

   /// provide access to ProfileEnumDataImpl for the derived classes
   ProfileEnumDataImpl& GetEnumData(EnumData& cookie) const
      { return *cookie.m_impl; }

   /// expand env vars if necessary
   String ExpandEnvVarsIfNeeded(const String& val) const;

private:
   /// helper for GetAllIdentities/GetAllFilters
   static wxArrayString GetAllGroupsUnder(const String& path);

   /// should we expand the environment variables in string values?
   bool m_expandEnvVars;

   /// forbid copying
   Profile(const Profile &);
   Profile& operator=(const Profile & );
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
   // ctors & dtor
   Profile_obj(Profile *profile)
      { m_profile = profile; }
   Profile_obj(const String& name)
      { m_profile = Profile::CreateFolderProfile(name); }
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
// helper functions
// ----------------------------------------------------------------------------

/// functions for savings/restoring arrays of strings to/from config
//@{

extern void
SaveArray(wxConfigBase *conf, const wxArrayString& astr, const String& key);
extern void
RestoreArray(wxConfigBase *conf, wxArrayString& astr, const String& key);

extern void
SaveArray(Profile *conf, const wxArrayString& astr, const String& key);
extern void
RestoreArray(Profile *conf, wxArrayString& astr, const String& key);

//@}

#endif // PROFILE_H
