/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "Profile.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"

#   include "Profile.h"
#   include "strutil.h"
#   include "PathFinder.h"
#   include "MApplication.h"
#   ifdef  OS_WIN
#      include <wx/msw/regconf.h>
#   else
#      include <wx/confbase.h>
#      include <wx/file.h>
#      include <wx/textfile.h>
#      include <wx/fileconf.h>
#   endif

#   include <wx/config.h>
#   include <wx/dir.h>
#endif

#include "Mdefaults.h"

#include <wx/fileconf.h>

#include <ctype.h>
#include <errno.h>

#ifdef OS_UNIX
   #include <sys/types.h>
   #include <unistd.h>
   #include <sys/stat.h>
   #include <fcntl.h>
#endif //Unix


// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CURRENT_IDENTITY;
extern const MOption MP_PROFILEPATH;
extern const MOption MP_PROFILE_IDENTITY;
extern const MOption MP_PROFILE_TYPE;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// a name for the empty profile, like this it is invalid for wxConfig, so it
///  will never conflict with a real profile name
#define   PROFILE_EMPTY_NAME "EMPTYPROFILE?(*[]{}"

/** Name for the subgroup level used for suspended profiles. Must
    never appear as part of a profile path name. */
#define SUSPEND_PATH "__suspended__"

#ifdef DEBUG
   // there are too many of profile trace messages - filter them. They won't
   // appear by default, if you do want them change the wxLog::ms_ulTraceMask
   // to include wxTraceProfileClass bit
   static const int wxTraceProfileClass = 0x200;
#endif

/// flags for readEntry
enum
{
   /// use identity, if any
   Lookup_Identity = 1,
   /// look under parent profile if not found here
   Lookup_Parent   = 2,
   /// honour suspended status
   Lookup_Suspended = 4,
   /// default: look everywhere
   Lookup_All = 7
};

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

/// a small struct holding key and value
class LookupData
{
public:
   LookupData(const String &key, const String &def)
      {
         m_Type = LD_STRING;
         m_Str = def;
         m_Key = key;
         m_Found = Profile::Read_Default;
      }
   LookupData(const String &key, long def)
      {
         m_Type = LD_LONG;
         m_Key = key;
         m_Long = def;
         m_Found = Profile::Read_Default;
      }

   enum Type { LD_LONG, LD_STRING };

   Type GetType(void) const { return m_Type; }
   const String & GetKey(void) const { return m_Key; }
   const String & GetString(void) const
      { ASSERT(m_Type == LD_STRING); return m_Str; }
   long GetLong(void) const
      { ASSERT(m_Type == LD_LONG); return m_Long; }
   Profile::ReadResult GetFound(void) const
      {
         return m_Found;
      }
   void SetResult(const String &str)
      {
         ASSERT(m_Type == LD_STRING);
         m_Str = str;
      }
   void SetResult(long l)
      {
         ASSERT(m_Type == LD_LONG);
         m_Long = l;
      }
   void SetFound(Profile::ReadResult found)
      {
         m_Found = found;
      }
private:
   Type m_Type;
   String m_Key;
   String m_Str;
   long   m_Long;
   Profile::ReadResult m_Found;
};

#ifdef DEBUG
#   define   PCHECK() MOcheck(); ASSERT(ms_GlobalConfig)
#else
#   define   PCHECK()
#endif


/// A dummy Profile just inheriting from the top level
class EmptyProfile : public Profile
{
public:
   /// Create a dummy Profile just inheriting from the top level
   static Profile * CreateEmptyProfile(const Profile *parent = NULL)
      { return new EmptyProfile(parent); }
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
   /// Read a character entry.
   virtual String readEntry(const String & key,
                            const String & defaultvalue = (const char*)NULL,
                            ReadResult *found = NULL) const
      {
         if ( m_Parent )
            return m_Parent->readEntry(key, defaultvalue, found);

         if ( found )
            *found = Read_Default;

         return defaultvalue;
      }
   /// Read an integer value.
   virtual long readEntry(const String & key,
                          long defaultvalue,
                          ReadResult *found = NULL) const
      {
         if ( m_Parent )
            return m_Parent->readEntry(key, defaultvalue, found);

         if ( found )
            *found = Read_Default;

         return defaultvalue;
      }

   virtual int readEntryFromHere(const String& key, int defvalue) const
   {
      return defvalue;
   }

   /// Write back the character value.
   virtual bool writeEntry(const String & key, const String & Value)
      { return false ; }
   /// Write back the int value.
   virtual bool writeEntry(const String & key, long Value)
      { return false; }

   virtual bool writeEntryIfNeeded(const String& key,
                                   long value,
                                   long defvalue)
      { return false; }
   //@}

   /// return true if the entry is defined
   virtual bool HasEntry(const String & key) const
      { return false; }
   /// return the type of entry
   virtual wxConfigBase::EntryType GetEntryType(const String & key) const
      { return wxConfigBase::Type_Unknown; }
   /// return true if the group exists
   virtual bool HasGroup(const String & name) const
      { return false; }
   /// delete the entry specified by path
   virtual bool DeleteEntry(const String& key)
      { return false; }
   /// delete the entry group specified by path
   virtual bool DeleteGroup(const String & path)
      { return false; }
   /// rename a group
   virtual bool Rename(const String& oldName, const String& newName)
      { return false; }
   /// return the name of the profile
   virtual const String GetName(void) const
      { return String(""); }

   /** @name Enumerating groups/entries
       again, this is just directly forwarded to wxConfig
   */
   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const{ return false; }
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const{ return false; }
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const{ return false; }
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const{ return false; }

   /// Returns a unique, not yet existing sub-group name: //MT!!
   virtual String GetUniqueGroupName(void) const
      { return "NOSUCHGROUP"; }

   virtual wxConfigBase *GetConfig() const { return NULL; }
   virtual String GetConfigPath() const
   {
      // we don't even have an associated wxConfig!
      FAIL_MSG( _T("not supposed to be called") );

      return "";
   }

   /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const
      { return m_Parent; }
   virtual void SetPath(const String & /*path*/ ) {}
   virtual void ResetPath(void) {} ;

   /// Set temporary/suspended operation.
   virtual void Suspend(void) { };
   /// Commit changes from suspended mode.
   virtual void Commit(void) { };
   /// Discard changes from suspended mode.
   virtual void Discard(void) { };
   /// Is the profile currently suspended?
   virtual bool IsSuspended(void) const { return false; }

   /// Set the identity to be used for this profile
   virtual void SetIdentity(const String & /*idName*/) { };
   /// Unset the identity set by SetIdentity
   virtual void ClearIdentity(void) { };
   // Get the currently used identity
   virtual String GetIdentity(void) const { return ""; };

   /// is this profile a (grand) parent of the given one?
   virtual bool IsAncestor(Profile *profile) const
      {
         // if our  parent is one, then so are we
         return m_Parent ? m_Parent->IsAncestor(profile) : false;
      };

   virtual String GetFolderName() const { return ""; }

private:
   Profile *m_Parent;

   EmptyProfile(const Profile *parent)
      {
         m_Parent = (Profile *) parent;
         if(m_Parent) m_Parent->IncRef();
      }
   ~EmptyProfile()
      {
         if(m_Parent) m_Parent->DecRef();
      }

   GCC_DTOR_WARN_OFF
};


/**
   ProfileImpl class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
   If no corresponding profile file is found, the profile will not
   have a wxConfig object associated with it but only refer to its
   parent or the global config.
   @see Profile
   @see wxConfig
*/

class ProfileImpl : public Profile
{
public:
   /// creates a normal ProfileImpl
   static Profile * CreateProfile(const String & ipathname,
                                      Profile const *Parent);
   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
      /// Read a character entry.
   String readEntry(const String & key,
                    const String & defaultvalue = (const char *)NULL,
                    ReadResult * found = NULL) const;
   /// Read an integer value.
   long readEntry(const String & key,
                  long defaultvalue,
                  ReadResult * found = NULL) const;
   /// read entry without recursing upwards
   virtual int readEntryFromHere(const String& key, int defvalue) const;
   /// Read string or integer
   virtual void readEntry(LookupData &ld,
                          int flags = Lookup_All) const;

   /// Write back the character value.
   bool writeEntry(const String & key,
                   const String & Value);
   /// Write back the int value.
   bool writeEntry(const String & key,
                   long Value);
   virtual bool writeEntryIfNeeded(const String& key,
                                   long value,
                                   long defvalue);

   //@}

   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const;
   /// Returns a unique, not yet existing sub-group name:
   virtual String GetUniqueGroupName(void) const;

      /// Returns a pointer to the parent profile.
   virtual Profile *GetParent(void) const;

   virtual bool HasEntry(const String & key) const;
   virtual wxConfigBase::EntryType GetEntryType(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual bool DeleteEntry(const String& key);
   virtual bool DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);

   virtual const String GetName(void) const { return m_ProfileName;}

   virtual void SetPath(const String &path)
      {
         PCHECK();

         ASSERT_MSG( path.Length() == 0 || path[0u] != '/',
                     _T("only relative paths allowed") );

         m_ProfilePath = path;
      }
   virtual void ResetPath(void)
      {
         PCHECK();
         m_ProfilePath = "";
      }

   virtual wxConfigBase *GetConfig() const { return ms_GlobalConfig; }

   virtual String GetConfigPath() const
   {
      String path = GetName();
      if ( !m_ProfilePath.empty() )
         path << '/' << m_ProfilePath;
      return path;
   }

   /// Set temporary/suspended operation.
   virtual void Suspend(void)
      {
         PCHECK();

         m_Suspended++;

         ms_suspendCount++;

         m_wroteSuspended = false;
      }

   /// Commit changes from suspended mode.
   virtual void Commit(void);
   /// Discard changes from suspended mode.
   virtual void Discard(void);
   /// Is the profile currently suspended?
   virtual bool IsSuspended(void) const { return m_Suspended != 0; }

   /** This temporarily overloads this profile with another Identity,
       i.e. the name of an Identity profile. */
   virtual void SetIdentity(const String & idName);
   virtual void ClearIdentity(void);
   virtual String GetIdentity(void) const;

   virtual bool IsAncestor(Profile *profile) const;

   String GetRootPath(void) const
   {
      return GetSectionPath(GetProfileSection());
   }

   virtual String GetFolderName() const;

#ifdef OS_WIN
   /// true if we use reg config under Windows, false otherwise
   static bool ms_usingRegConfig;
#endif // OS_WIN

protected:
   ProfileImpl()
      {
         m_Suspended = 0;
         m_Identity = NULL;
      }

   /// Destructor, writes back those entries that got changed.
   ~ProfileImpl();

   virtual const char * GetProfileSection(void) const
      {
         return M_PROFILE_CONFIG_SECTION;
      }

   /// Name of this profile == path in wxConfig
   String   m_ProfileName;

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   ProfileImpl(const String & iClassName, Profile const *Parent);

   /// If not empty, temporarily modified path for this profile.
   String m_ProfilePath;

   /// suspend count: if positive, we're in suspend mode
   int m_Suspended;

   /// did we actually write any settings while suspended (have to undo them?)?
   bool m_wroteSuspended;

   /// Is this profile using a different Identity at present?
   Profile *m_Identity;

   /// the count of all suspended profiles: if 0, nothing is suspended
   static size_t ms_suspendCount;

   MOBJECT_DEBUG(ProfileImpl)

   GCC_DTOR_WARN_OFF
};

size_t ProfileImpl::ms_suspendCount = 0;

#ifdef OS_WIN
   bool ProfileImpl::ms_usingRegConfig = true;
#endif // OS_WIN

//@}


/**
   Identity class which is a Profile representing a single identity
   setting.
   @see Profile
   @see wxConfig
*/

class Identity : public ProfileImpl
{
public:
   static Profile * CreateIdentity(const String &name)
      { return new Identity(name); }

   virtual const char * GetProfileSection(void) const
      {
         return M_IDENTITY_CONFIG_SECTION;
      }
private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   Identity(const String & name)
      {
         m_ProfileName = GetRootPath();
         m_ProfileName << '/' << name;
      }
};
/**
   Filter profile class which is a Profile representing a single
   filter rule.
   @see Profile
   @see wxConfig
*/

class FilterProfile : public ProfileImpl
{
public:
   static FilterProfile * CreateFilterProfile(const String &name)
      { return new FilterProfile(name); }

   virtual const char * GetProfileSection(void) const
      {
         return M_FILTERS_CONFIG_SECTION;
      }
private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   FilterProfile(const String & name)
      {
         m_ProfileName = GetRootPath();
         m_ProfileName << '/' << name;
      }
};




// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

void Profile::FlushAll()
{
   if ( ms_GlobalConfig )
      ms_GlobalConfig->Flush();
}

bool Profile::IsExpandingEnvVars() const
{
   PCHECK();
   return ms_GlobalConfig->IsExpandingEnvVars();
}

void Profile::SetExpandEnvVars(bool bDoIt)
{
   PCHECK();
   ms_GlobalConfig->SetExpandEnvVars(bDoIt);
}

//FIXME:MT calling wxWindows from possibly non-GUI code
bool ProfileImpl::GetFirstGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length())
      ms_GlobalConfig->SetPath(m_ProfilePath);

   bool success = ms_GlobalConfig->GetFirstGroup(s, l);
   if(success && s == SUSPEND_PATH)
      success = GetNextGroup(s,l);

   return success;
}

bool ProfileImpl::GetNextGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length())
      ms_GlobalConfig->SetPath(m_ProfilePath);

   bool success = ms_GlobalConfig->GetNextGroup(s, l);
   while(success && s == SUSPEND_PATH)
      success = ms_GlobalConfig->GetNextGroup(s, l);

   return success;
}

String ProfileImpl::GetUniqueGroupName(void) const
{
   PCHECK();
   String name; // We use hex numbers
   unsigned long number = 0;
   do
   {
      name.Printf("%lX", number++);
   }
   while ( HasGroup(name) );

   return name;
}


bool ProfileImpl::GetFirstEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length())
      ms_GlobalConfig->SetPath(m_ProfilePath);

   return ms_GlobalConfig->GetFirstEntry(s, l);
}

bool ProfileImpl::GetNextEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length())
      ms_GlobalConfig->SetPath(m_ProfilePath);

   return ms_GlobalConfig->GetNextEntry(s, l);
}


void
ProfileImpl::SetIdentity(const String & idName)
{
   PCHECK();

   if ( m_Identity )
      ClearIdentity();

   if ( !!idName )
      m_Identity = Profile::CreateIdentity(idName);
}

void
ProfileImpl::ClearIdentity(void)
{
   PCHECK();
   if(m_Identity) m_Identity->DecRef();
   m_Identity = NULL;
}

String
ProfileImpl::GetIdentity(void) const
{
   PCHECK();
   return m_Identity ? m_Identity->GetName() : String("");
}

wxArrayString Profile::GetAllFilters()
{
   return GetAllGroupsUnder(GetFiltersPath());
}

wxArrayString Profile::GetAllIdentities()
{
   return GetAllGroupsUnder(GetIdentityPath());
}

wxArrayString Profile::GetAllGroupsUnder(const String& path)
{
   wxArrayString identities;

   wxString oldpath = ms_GlobalConfig->GetPath();
   ms_GlobalConfig->SetPath(path);

   long index;
   wxString name;
   bool cont = ms_GlobalConfig->GetFirstGroup(name, index);
   while ( cont )
   {
      if ( name != SUSPEND_PATH )
      {
         identities.Add(name);
      }

      cont = ms_GlobalConfig->GetNextGroup(name, index);
   }

   ms_GlobalConfig->SetPath(oldpath);

   return identities;
}

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

wxConfigBase * Profile::ms_GlobalConfig;

/** This function sets profile parameters and is applied
    to all profiles directly after creation.
*/
static inline
void EnforcePolicy(Profile *p)
{
   // currently we don't do anything here
}

Profile *
Profile::CreateProfile(const String & classname,
                       Profile const *parent)
{
   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != '/'));
   Profile *p =  ProfileImpl::CreateProfile(classname, parent);

   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateIdentity(const String & idName)
{
   ASSERT(idName.Length() == 0 ||  // only relative paths allowed
          (idName[0u] != '.' && idName[0u] != '/'));
   Profile *p =  Identity::CreateIdentity(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateFilterProfile(const String & idName)
{
   ASSERT(idName.Length() == 0 ||  // only relative paths allowed
          (idName[0u] != '.' && idName[0u] != '/'));
   Profile *p =  FilterProfile::CreateFilterProfile(idName);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateModuleProfile(const String & classname, Profile const *parent)
{
   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != '/'));
   String newName = "Modules/" + classname;
   Profile *p =  ProfileImpl::CreateProfile(newName, parent);

   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateFolderProfile(const String& foldername)
{
   return CreateProfile(foldername);
}

Profile *
Profile::CreateEmptyProfile(Profile const *parent)
{
   Profile *p =  EmptyProfile::CreateEmptyProfile(parent);
   EnforcePolicy(p);
   return p;
}

Profile *
Profile::CreateGlobalConfig(const String& filename)
{
   // if the filename is empty, use the default one
   String strConfFile = filename;

#ifdef OS_UNIX
   if ( strConfFile.empty() )
   {
      strConfFile = wxGetHomeDir();
      if ( strConfFile.empty() )
      {
         // don't create our files in the root directory, try the current one
         // instead
         strConfFile = wxGetCwd();
      }

      if ( !strConfFile.empty() )
      {
         strConfFile << '/';
      }
      //else: it will be in the current one, what else can we do?

      strConfFile << '.' << M_APPLICATIONNAME;

      if ( !wxDir::Exists(strConfFile) )
      {
         if ( !wxMkdir(strConfFile, 0700) )
         {
            wxLogError(_("Cannot create the directory for configuration "
                         "files '%s'."), strConfFile.c_str());

            return NULL;
         }

         wxLogInfo(_("Created directory '%s' for configuration files."),
                   strConfFile.c_str());

         // also create an empty config file with the right permissions:
         String filename = strConfFile + "/config";

         // pass false to Create() to avoid overwriting the existing file
         wxFile file;
         if ( !file.Create(filename, false, wxS_IRUSR | wxS_IWUSR) &&
               !wxFile::Exists(filename) )
         {
            wxLogError(_("Could not create initial configuration file."));
         }
      }

      // Check whether other users can write our config dir.
      //
      // This must not be allowed as they could change the behaviour of the
      // program unknowingly to the user!
      struct stat st;
      if ( stat(strConfFile, &st) == 0 )
      {
         if ( st.st_mode & (S_IWGRP | S_IWOTH) )
         {
            // No other user must have write access to the config dir.
            String msg;
            msg.Printf(_("Configuration directory '%s' was writable for other users.\n"
                         "The programs settings might have been changed without "
                         "your knowledge and the passwords stored in your config\n"
                         "file (if any) could have been compromised!\n\n"),
                       strConfFile.c_str());

            if ( chmod(strConfFile, st.st_mode & ~(S_IWGRP | S_IWOTH)) == 0 )
            {
               msg += _("This has been fixed now, the directory is no longer writable for others.");
            }
            else
            {
               msg << String::Format(_("Failed to correct the directory access "
                                       "rights (%s), please do it manually and "
                                       "restart the program!"),
                                     strerror(errno));
            }

            wxLogError(msg);
         }
      }
      else
      {
         wxLogSysError(_("Failed to access the directory '%s' containing "
                         "the configuration files."),
                       strConfFile.c_str());
      }

      strConfFile += "/config";

      // Check whether other users can read our config file.
      //
      // This must not be allowed as we store passwords in it!
      if ( stat(strConfFile, &st) == 0 )
      {
         if ( st.st_mode & (S_IWGRP | S_IWOTH | S_IRGRP | S_IROTH) )
         {
            // No other user must have access to the config file.
            String msg;
            if ( st.st_mode & (S_IWGRP | S_IWOTH) )
            {
               msg.Printf(_("Configuration file %s was writable by other users.\n"
                            "The program settings could have been changed without\n"
                            "your knowledge, please consider reinstalling the "
                            "program!"),
                          strConfFile.c_str());
            }

            if ( st.st_mode & (S_IRGRP | S_IROTH) )
            {
               if ( !msg.empty() )
                  msg += "\n\n";

               msg += String::Format
                      (
                        _("Configuration file '%s' was readable for other users.\n"
                          "Passwords may have been compromised, please "
                          "consider changing them!"),
                        strConfFile.c_str()
                      );
            }

            msg += "\n\n";

            if ( chmod(strConfFile, S_IRUSR | S_IWUSR) == 0 )
            {
               msg += _("This has been fixed now, the file is no longer readable for others.");
            }
            else
            {
               msg << String::Format(_("The attempt to make the file unreadable "
                                       "for others has failed: %s"),
                                     strerror(errno));
            }

            wxLogError(msg);
         }
      }
      //else: not an error, file may be simply not there yet
   }
#endif // Unix

#ifdef OS_WIN
   // no such concerns here, we just use the registry if the filename is not
   // specified
   if ( strConfFile.empty() )
   {
      // don't give explicit name, but rather use the default logic (it's
      // perfectly ok, for the registry case our keys are under
      // vendor\appname)
      ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                     "", "",
                                     wxCONFIG_USE_LOCAL_FILE |
                                     wxCONFIG_USE_GLOBAL_FILE);
   }
   else // we do have the config file name
   {
      // use the config file: this allows to use the same file for Windows and
      // Unix Mahogany installations
      ms_GlobalConfig = new wxFileConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                         strConfFile, "",
                                         wxCONFIG_USE_LOCAL_FILE|
                                         wxCONFIG_USE_GLOBAL_FILE);

      // we need to modify the path handling under Windows in this case
      ProfileImpl::ms_usingRegConfig = false;
   }
#else  // Unix
   // look for the global config file in the following places in order:
   //    1. compile-time specified installation dir
   //    2. run-time specified installation dir
   //    3. default installation dir
   String globalFileName;
   globalFileName << M_APPLICATIONNAME << ".conf";
   String globalFile;
   globalFile << M_PREFIX << "/etc/" << globalFileName;
   if ( !wxFileExists(globalFile) )
   {
      const char *dir = getenv("MAHOGANY_DIR");
      if ( dir )
      {
         globalFile.clear();
         globalFile << dir << "/etc/" << globalFileName;
      }
   }
   if ( !wxFileExists(globalFile) )
   {
      // wxConfig will look for it in the default location(s)
      globalFile = globalFileName;
   }

   // we don't need the config file manager for this profile
   ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  strConfFile, globalFile,
                                  wxCONFIG_USE_LOCAL_FILE|
                                  wxCONFIG_USE_GLOBAL_FILE);

   // don't give the others even read access to our config file as it stores,
   // among other things, the passwords
   ((wxFileConfig *)ms_GlobalConfig)->SetUmask(0077);
#endif // Unix/Windows

   Profile *p = ProfileImpl::CreateProfile("",NULL);
   EnforcePolicy(p);
   return p;
}

void
Profile::DeleteGlobalConfig()
{
   if ( ms_GlobalConfig )
   {
      delete ms_GlobalConfig;
      ms_GlobalConfig = NULL;

      // we just deleted it, prevent wxWin from deleting it again!
      wxConfig::Set(NULL);
   }
}

String
Profile::readEntry(const String & key,
                       const char *defaultvalue,
                       ReadResult * found) const
{
   PCHECK();
   String str;
   str = readEntry(key, String(defaultvalue), found);
   return str;
}

// ----------------------------------------------------------------------------
// ProfileImpl
// ----------------------------------------------------------------------------

/**
   ProfileImpl class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/
ProfileImpl::ProfileImpl(const String & iName, Profile const *Parent)
{
   m_ProfileName = ( Parent && Parent->GetName().Length())
                     ? ( Parent->GetName() + '/' )
                     : String(GetRootPath());
   if(iName.Length())
      m_ProfileName << '/' << iName;
   m_Suspended = 0;
   m_Identity = NULL;

   String id = readEntry(GetOptionName(MP_PROFILE_IDENTITY),
                         GetStringDefault(MP_PROFILE_IDENTITY));
   if ( !id && mApplication->GetProfile() )
      id = READ_APPCONFIG_TEXT(MP_CURRENT_IDENTITY);
   if ( !!id )
      SetIdentity(id);
}


Profile *
ProfileImpl::CreateProfile(const String & iClassName,
                       Profile const *parent)
{
   return new ProfileImpl(iClassName, parent);
}

ProfileImpl::~ProfileImpl()
{
   PCHECK();

   if ( m_Suspended )
   {
      FAIL_MSG( _T("deleting a suspended profile") );

      Discard(); // but we tidy up, no big deal
   }

   if(m_Identity)
      m_Identity->DecRef();
}

Profile *
ProfileImpl::GetParent(void) const
{
   return CreateProfile(GetName().BeforeLast('/'), NULL);
}

bool
ProfileImpl::HasGroup(const String & name) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasGroup(name);
}

bool
ProfileImpl::HasEntry(const String & key) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasEntry(key) || ms_GlobalConfig->HasGroup(key);
}

wxConfigBase::EntryType
ProfileImpl::GetEntryType(const String & key) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->GetEntryType(key);
}

bool
ProfileImpl::Rename(const String& oldName, const String& newName)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->RenameGroup(oldName, newName);
}

bool
ProfileImpl::DeleteEntry(const String& key)
{
   PCHECK();

   String root = GetName();
   if ( !m_ProfilePath.empty() )
       root  << '/' << m_ProfilePath;
   ms_GlobalConfig->SetPath(root);
   return ms_GlobalConfig->DeleteEntry(key);
}

bool
ProfileImpl::DeleteGroup(const String & path)
{
   PCHECK();

   CHECK( !path.empty(), false, _T("must have a valid group name to delete it") );

   if ( path[0u] != '/' )
   {
      String root = GetName();
      if ( !m_ProfilePath.empty() )
          root  << '/' << m_ProfilePath;
      ms_GlobalConfig->SetPath(root);
   }

   return ms_GlobalConfig->DeleteGroup(path);
}

String
ProfileImpl::readEntry(const String & key, const String & def,
                       ReadResult * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found)
      *found = ld.GetFound();

   // normally wxConfig does this for us, but if we didn't find the entry at
   // all, it won't happen, so do it here
   return ms_GlobalConfig->ExpandEnvVars(ld.GetString());
}

long
ProfileImpl::readEntry(const String & key, long def, ReadResult * found) const
{
   LookupData ld(key, def);
   readEntry(ld);
   if(found)
      *found = ld.GetFound();

   return ld.GetLong();
}

int ProfileImpl::readEntryFromHere(const String& key, int defvalue) const
{
   LookupData ld(key, defvalue);
   readEntry(ld, Lookup_All & ~Lookup_Parent);

   return (int)ld.GetLong();
}

// Notice that we always look first under SUSPEND_PATH: if the profile is not
// in suspend mode, the entry there simply doesn't exist, so it doesn't harm,
// and if it is it should override the "normal" entry. This is *necessary* for
// all parent profiles because they may be suspended without us knowing about
// it.
//
// Correction: now we look under SUSPEND_PATH only if at least some profile is
// suspended, this allows us to gain a lot of time during 99% of normal
// operation when no profiles are suspended which is quite nice according to
// the profiling results
void
ProfileImpl::readEntry(LookupData &ld, int flags) const
{
   PCHECK();

   String pathProfile = GetName();

   if( m_ProfilePath.length() )
      pathProfile << '/' << m_ProfilePath;

   ms_GlobalConfig->SetPath(pathProfile);

   // the values read from profile
   String strResult;
   long   longResult = 0; // unneeded but suppresses the compiler warning

   // did we find the requested value?
   bool foundHere;

   String keySuspended;
   if ( (flags & Lookup_Suspended) && ms_suspendCount )
   {
      keySuspended << SUSPEND_PATH << '/' << ld.GetKey();

      if( ld.GetType() == LookupData::LD_STRING )
         foundHere = ms_GlobalConfig->Read(keySuspended, &strResult, ld.GetString());
      else
         foundHere = ms_GlobalConfig->Read(keySuspended, &longResult, ld.GetLong());
   }
   else
   {
      foundHere = false;
   }

   if ( !foundHere )
   {
      if( ld.GetType() == LookupData::LD_STRING )
         foundHere = ms_GlobalConfig->Read(ld.GetKey(), &strResult,
                                           ld.GetString());
      else
         foundHere = ms_GlobalConfig->Read(ld.GetKey(), &longResult,
                                           ld.GetLong());
   }

   // if we don't have our own setting, check for identity override before
   // quering the parent
   if ( !foundHere && (flags & Lookup_Identity) && m_Identity )
   {
      // try suspended path first:
      ReadResult idFound;
      if( ld.GetType() == LookupData::LD_STRING )
         strResult = m_Identity->readEntry(keySuspended, ld.GetString(),
                                           &idFound);
      else
         longResult = m_Identity->readEntry(keySuspended, ld.GetLong(),
                                            &idFound);
      if(! idFound)
      {
         if( ld.GetType() == LookupData::LD_STRING )
            strResult = m_Identity->readEntry(ld.GetKey(), ld.GetString(), &idFound);
         else
            longResult = m_Identity->readEntry(ld.GetKey(), ld.GetLong(), &idFound);
      }
      if(idFound)
      {
         if(ld.GetType() ==  LookupData::LD_STRING)
            ld.SetResult(strResult);
         else
            ld.SetResult(longResult);
         ld.SetFound(Profile::Read_Default);
         return;
      }

      // restore the global path possibly changed by readEntry() calls above
      ms_GlobalConfig->SetPath(pathProfile);
   }

   bool foundAnywhere = foundHere;
   while ( !foundAnywhere &&
           (flags & Lookup_Parent) &&
           (ms_GlobalConfig->GetPath() != GetRootPath()) )
   {
      ms_GlobalConfig->SetPath("..");

      // try suspended global profile first:
      if ( ms_suspendCount )
      {
         if( ld.GetType() == LookupData::LD_STRING )
         {
            foundAnywhere = ms_GlobalConfig->Read(keySuspended,
                                                  &strResult,
                                                  ld.GetString());
         }
         else
         {
            foundAnywhere = ms_GlobalConfig->Read(keySuspended,
                                                  &longResult,
                                                  ld.GetLong());
         }
      }

      if ( !foundAnywhere )
      {
         if( ld.GetType() == LookupData::LD_STRING )
         {
            foundAnywhere = ms_GlobalConfig->Read(ld.GetKey(), &strResult,
                                                  ld.GetString());
         }
         else
         {
            foundAnywhere = ms_GlobalConfig->Read(ld.GetKey(), &longResult,
                                                  ld.GetLong());
         }
      }
   }

   // did we find it at all?
   if ( foundAnywhere )
   {
      if( ld.GetType() == LookupData::LD_STRING )
         ld.SetResult(strResult);
      else
         ld.SetResult(longResult);

      ld.SetFound(foundHere ? Profile::Read_FromHere : Profile::Read_FromParent);
   }
}

bool
ProfileImpl::writeEntry(const String & key, const String & value)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_Suspended)
   {
      // set the flag telling us that we did write some suspended entries
      m_wroteSuspended = true;

      keypath << SUSPEND_PATH << '/';
   }

   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, value);
}

bool
ProfileImpl::writeEntry(const String & key, long value)
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_Suspended)
   {
      // set the flag telling us that we did write some suspended entries
      m_wroteSuspended = true;

      keypath << SUSPEND_PATH << '/';
   }

   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, (long) value);
}

bool
ProfileImpl::writeEntryIfNeeded(const String& key,
                                long value,
                                long defvalue)
{
   if ( readEntry(key, defvalue) == value )
   {
      // nothing to do
      return true;
   }

   return writeEntry(key, value);
}


static void CommitGroup(wxConfigBase *cfg, String truePath, String suspPath)
{
   String strValue;
   long   numValue;

   long index;
   String name;

   cfg->SetPath(suspPath);

   bool cont = cfg->GetFirstEntry(name, index);
   while(cont)
   {
      switch ( cfg->GetEntryType(name) )
      {
         case wxConfig::Type_String:
            if ( !cfg->Read(name, &strValue) ||
                 !cfg->Write(truePath + name, strValue) )
            {
               FAIL_MSG(_T("failed to copy config entry"));
            }
            break;

         case wxConfig::Type_Integer:
            if ( !cfg->Read(name, &numValue) ||
                 !cfg->Write(truePath + name, numValue) )
            {
               FAIL_MSG(_T("failed to copy config entry"));
            }
            break;

         default:
            FAIL_MSG(_T("unsupported config entry type"));
      }

      cont = cfg->GetNextEntry(name, index);
   }


   cont = cfg->GetFirstGroup(name, index);
   while(cont)
   {
      CommitGroup(cfg, truePath + name + "/", suspPath + name + "/");
      cfg->SetPath(suspPath);
      cont = cfg->GetNextGroup(name, index);
   }
}


void
ProfileImpl::Commit(void)
{
   PCHECK();

   CHECK_RET( m_Suspended, _T("calling Commit() without matching Suspend()") );

   String truePath = GetName();
   truePath << '/';

   if( m_ProfilePath.length() > 0 )
      truePath << m_ProfilePath << '/';

   String suspPath = truePath;
   suspPath << SUSPEND_PATH << '/';
   ms_GlobalConfig->SetPath(suspPath);

   CommitGroup(ms_GlobalConfig, truePath, suspPath);

   Discard(); // now we just forget the suspended sub-group
}

void
ProfileImpl::Discard(void)
{
   PCHECK();

   CHECK_RET( m_Suspended, _T("calling Discard() without matching Suspend()") );

   if ( !--m_Suspended )
   {
      String path;
      ms_GlobalConfig->SetPath(GetName());
      if( m_ProfilePath.length() > 0 )
         path << m_ProfilePath << '/';

      path << SUSPEND_PATH;

      if ( m_wroteSuspended )
      {
         // avoid warning about unused variable in release builds
#ifdef DEBUG
         bool success =
#endif
            ms_GlobalConfig->DeleteGroup(path);

         ASSERT_MSG( success, _T("failed to delete suspended settings") );
      }
   }

   ASSERT_MSG( ms_suspendCount > 0, _T("suspend count broken") );

   ms_suspendCount--;
}

#ifdef DEBUG
String
ProfileImpl::DebugDump() const
{
   PCHECK();

   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("name '%s'", m_ProfileName.c_str());

   return s1 + s2;
}

#endif





// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(wxConfigBase *conf,
               const wxArrayString& astr,
               const String& key)
{
   // save all array entries
   conf->DeleteGroup(key);    // remove all old entries

   String path;
   path << key << '/';

   size_t nCount = astr.Count();
   String strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf("%lu", (unsigned long)n);
      conf->Write(path + strkey, astr[n]);
   }
}

// restores array saved by SaveArray
void RestoreArray(wxConfigBase *conf, wxArrayString& astr, const String& key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   String path;
   path << key << '/';

   String strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf("%lu", (unsigned long)n);
      if ( !conf->HasEntry(path+strkey) )
         break;

      strVal = conf->Read(path+strkey, "");
      astr.Add(strVal);
   }
}

void SaveArray(Profile *profile, const wxArrayString& astr, const String& key)
{
   wxConfigBase *conf = profile->GetConfig();
   CHECK_RET( conf, _T("can't be used with non config based profile!") );

   conf->SetPath(profile->GetConfigPath());

   SaveArray(conf, astr, key);
}

void RestoreArray(Profile *profile, wxArrayString& astr, const String& key)
{
   wxConfigBase *conf = profile->GetConfig();
   CHECK_RET( conf, _T("can't be used with non config based profile!") );

   conf->SetPath(profile->GetConfigPath());

   RestoreArray(conf, astr, key);
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// some characters are invalid in the profile name, replace them
String
Profile::FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const char *aValidChars = "_-.";   // NOT '/' and '\\'!

   String filteredName;
   size_t len = profileName.Len();
   filteredName.Alloc(len);
   for ( size_t n = 0; n < len; n++ )
   {
      char ch = profileName[n];
      if ( isalnum(ch) || strchr(aValidChars, ch) )
      {
         filteredName << ch;
      }
      else
      {
         // replace it -- hopefully the name will stay unique (FIXME)
         filteredName << '_';
      }
   }

   return filteredName;
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

bool ProfileImpl::IsAncestor(Profile *profile) const
{
   if ( !profile )
      return false;

   // can't compare as strings because '/' are sometimes duplicated...
   wxArrayString aMyComponents, aOtherComponents;
   wxSplitPath(aMyComponents, m_ProfileName);
   wxSplitPath(aOtherComponents, profile->GetName());

   if ( aOtherComponents.GetCount() < aMyComponents.GetCount() )
      return false;

   size_t count = aMyComponents.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( aOtherComponents[n] != aMyComponents[n] )
         return false;
   }

   return true;
}

String ProfileImpl::GetFolderName() const
{
   String folderName;
   if ( GetName().StartsWith(GetProfilePath(), &folderName) )
   {
      const char *p = folderName.c_str();

      if ( *p )
      {
         ASSERT_MSG( *p == _T('/'), _T("profile path must start with slash") );

         // erase the slash
         folderName.erase(0, 1);
      }
      //else: leave empty
   }

   return folderName;
}

/* static */
String Profile::GetSectionPath(const String& section)
{
   String path = section;

   // we don't use "/M" prefix when working with wxRegConfig as it would be
   // superfluous: our config key is already Mahogany-Team/M
   //
   // but for the file based config formats (both local and remote) we do use
   // it for mostly historical reasons - even though if it probably would be
   // better to never use it, it's simply too much trouble to write the upgrade
   // code to deal with the existing config files
#ifdef OS_WIN
   if ( ProfileImpl::ms_usingRegConfig )
   {
      // remove "/M" prefix
      ASSERT_MSG( path[0u] == _T('/') && path[1u] == _T('M'),
                  _T("all profile sections must start with \"/M\"") );

      path.erase(0, 2);
   }
#endif // Windows

   return path;
}

