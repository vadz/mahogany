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

#include <wx/confbase.h>

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "MObject.h"

class kbStringList;

// ----------------------------------------------------------------------------
// profile-related constants
// ----------------------------------------------------------------------------

/** @name The sections of the configuration file. */
///@{

/**
  The section in the global configuration file used for storing
  profiles (without trailing '/')
*/
#ifndef M_PROFILE_CONFIG_SECTION
   #define   M_PROFILE_CONFIG_SECTION   _T("/M/Profiles")
   #define   M_IDENTITY_CONFIG_SECTION  _T("/M/Ids")
   #define   M_FILTERS_CONFIG_SECTION   _T("/M/Filters")
   #define   M_FRAMES_CONFIG_SECTION    _T("/M/Frames")
   #define   M_TEMPLATES_CONFIG_SECTION _T("/M/Templates")
#endif

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

   /// Delete the global config object
   static void DeleteGlobalConfig();

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
   virtual bool writeEntry(const String & key, const String & Value) = 0;
   /// Write back the int value.
   virtual bool writeEntry(const String & key, long Value) = 0;

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
                                   long defvalue) = 0;

   //@}

   /// return true if the entry is defined
   virtual bool HasEntry(const String & key) const = 0;
   /// return the type of entry (Type_String, Type_Integer or Type_Unknown
   virtual wxConfigBase::EntryType GetEntryType(const String & key) const = 0;
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
   virtual wxConfigBase *GetConfig() const = 0;

   // for internal use only: get the path corresponding to this profile in
   // config
   virtual String GetConfigPath() const = 0;

   virtual void SetPath(const String &path) = 0;
   virtual void ResetPath(void) = 0;

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
      { return GetSectionPath(M_PROFILE_CONFIG_SECTION); }

   /// return the path under which the identities are stored
   static String GetIdentityPath() 
      { return GetSectionPath(M_IDENTITY_CONFIG_SECTION); }

   /// return the path under which the filters are stored
   static String GetFiltersPath() 
      { return GetSectionPath(M_FILTERS_CONFIG_SECTION); }

   /// return the path under which the frame options are stored
   static String GetFramesPath() 
      { return GetSectionPath(M_FRAMES_CONFIG_SECTION); }

   /// return the path under which the templates are stored
   static String GetTemplatesPath() 
      { return GetSectionPath(M_TEMPLATES_CONFIG_SECTION); }

   //@}

protected:
   // egcs wants this
   Profile() { }

   /// helper of all GetXXXPath() functions
   static String GetSectionPath(const String& section);

   /// global wxConfig object, shared by all profiles
   static wxConfigBase *ms_GlobalConfig;

private:
   /// helper for GetAllIdentities/GetAllFilters
   static wxArrayString GetAllGroupsUnder(const String& path);

   /// forbid copy construction
   Profile(const Profile &);
   /// forbid assignments
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
