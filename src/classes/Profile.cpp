/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (Ballueder@usa.net)            *
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
#   include "kbList.h"
#   include "MApplication.h"
#   ifdef  OS_WIN
#      include <wx/msw/regconf.h>
#   else
#      include <wx/confbase.h>
#      include <wx/file.h>
#      include <wx/textfile.h>
#      include <wx/fileconf.h>
#   endif
#   include   <wx/config.h>
#endif

#include "Mdefaults.h"

#include <wx/fileconf.h>

#include <ctype.h>

#ifdef OS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// a name for the empty profile, like this it is invalid for wxConfig, so it
///  will never conflict with a real profile name
#define   PROFILE_EMPTY_NAME "EMPTYPROFILE?(*[]{}"

#ifdef DEBUG
   // there are t many of profile trace messages - filter them. They won't
   // appear by default, if you do want them change the wxLog::ms_ulTraceMask
   // to include wxTraceProfileClass bit
   static const int wxTraceProfileClass = 0x200;
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#ifdef DEBUG
#   define   PCHECK() MOcheck(); ASSERT(ms_GlobalConfig)
#else
#   define   PCHECK()
#endif

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
   If no corresponding profile file is found, the profile will not
   have a wxConfig object associated with it but only refer to its
   parent or the global config.
   @see ProfileBase
   @see wxConfig
*/

class Profile : public ProfileBase
{
public:
   /// like the constructor, but reuses existing objects
   static ProfileBase * CreateProfile(const String & ipathname,
                                      ProfileBase const *Parent);
   /// like the constructor, but reuses existing objects
   static ProfileBase * CreateEmptyProfile(const ProfileBase *Parent);

   /**@name Reading and writing entries.
      All these functions are just identical to the wxConfig ones.
   */
   //@{
      /// Read a character entry.
   String readEntry(const String & key,
                    const String & defaultvalue = (const char *)NULL,
                    bool * found = NULL) const;
   /// Read an integer value.
   long readEntry(const String & key,
                  long defaultvalue,
                  bool * found = NULL) const;
   /// Write back the character value.
   bool writeEntry(const String & key,
                   const String & Value);
   /// Write back the int value.
   bool writeEntry(const String & key,
                   long Value);
   //@}

   /// see wxConfig docs
   virtual bool GetFirstGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextGroup(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetFirstEntry(String& s, long& l) const;
   /// see wxConfig docs
   virtual bool GetNextEntry(String& s, long& l) const;

   virtual bool HasEntry(const String & key) const;
   virtual bool HasGroup(const String & name) const;
   virtual void DeleteGroup(const String & path);
   virtual bool Rename(const String& oldName, const String& newName);

   virtual const String & GetName(void) const { return m_ProfileName;}

   virtual void SetPath(const String &path)
      {
         PCHECK();
         ASSERT(path.Length() == 0 ||  // only relative paths allowed
                (path[0u] != '.' && path[0u] != '/'));
         m_ProfilePath = path;
      }
   virtual void ResetPath(void)
      {
         PCHECK();
         m_ProfilePath = "";
      }

   MOBJECT_DEBUG(Profile)

   virtual bool IsAncestor(ProfileBase *profile) const;

private:
   /** Constructor.
       @param iClassName the name of this profile
       @param iParent the parent profile
       This will try to load the configuration file given by
       iClassName".profile" and look for it in all the paths specified
       by GetAppConfig()->readEntry(MP_PROFILEPATH).

   */
   Profile(const String & iClassName, ProfileBase const *Parent);
   /// Name of this profile == path in wxConfig
   String   m_ProfileName;
   /// If not empty, temporarily modified path for this profile.
   String   m_ProfilePath;
   /// Destructor, writes back those entries that got changed.
   ~Profile();
   GCC_DTOR_WARN_OFF();
   /// this is an empty dummy profile, readonly
   bool     m_IsEmpty;
};
//@}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

void ProfileBase::FlushAll()
{
   ASSERT(ms_GlobalConfig);

   ms_GlobalConfig->Flush();
}

bool ProfileBase::IsExpandingEnvVars() const
{
   PCHECK();
   return ms_GlobalConfig->IsExpandingEnvVars();
}

void ProfileBase::SetExpandEnvVars(bool bDoIt)
{
   PCHECK();
   ms_GlobalConfig->SetExpandEnvVars(bDoIt);
}

/** List all profiles of a given type or all profiles in total.
    @param type Type of profile to list or PT_Any for all.
    @return a pointer to kbStringList of profile names to be freed by caller.
*/
static void
ListProfilesHelper(wxConfigBase *config,
                   kbList *list,
                   int type,
                   String const &path)
{
   wxString oldpath = config->GetPath();
   config->SetPath(path);

   int ptype;

   // these variables will be filled by GetFirstGroup/GetNextGroup
   long index = 0;
   wxString name;

   bool ok = config->GetFirstGroup(name, index);
   while ( ok )
   {
      config->Read(name + '/' + MP_PROFILE_TYPE, &ptype, MP_PROFILE_TYPE_D);
      wxString pathname = path;
      pathname << '/' << name;
      if(type == ProfileBase::PT_Any || ptype == type)
         list->push_back(new String(pathname));
      ListProfilesHelper(config, list, type, pathname);

      ok = config->GetNextGroup (name, index);
   }

   // restore path
   config->SetPath(oldpath);
}

kbStringList *
ProfileBase::ListProfiles(int type)
{
   ASSERT(ms_GlobalConfig);
   kbStringList *list = new kbStringList;
   if( ms_GlobalConfig )
   {
      // may be give a (debug) warning here?
      ListProfilesHelper(ms_GlobalConfig, list, type, "");
   }

   return list;
}


//FIXME:MT calling wxWindows from possibly non-GUI code
bool Profile::GetFirstGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetFirstGroup(s, l);
}

bool Profile::GetNextGroup(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetNextGroup(s, l);
}
bool Profile::GetFirstEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetFirstEntry(s, l);
}

bool Profile::GetNextEntry(String& s, long& l) const
{
   PCHECK();
   ms_GlobalConfig->SetPath(GetName());
   if(m_ProfilePath.Length()) ms_GlobalConfig->SetPath(m_ProfilePath);
   return ms_GlobalConfig->GetNextEntry(s, l);
}

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

wxConfigBase * ProfileBase::ms_GlobalConfig;

/** This function sets profile parameters and is applied
    to all profiles directly after creation.
*/
inline static
void EnforcePolicy(ProfileBase *p)
{
   p->SetExpandEnvVars(true);
}

ProfileBase *
ProfileBase::CreateProfile(const String & classname, ProfileBase const *parent)
{
   ASSERT(classname.Length() == 0 ||  // only relative paths allowed
          (classname[0u] != '.' && classname[0u] != '/'));
   ProfileBase *p =  Profile::CreateProfile(classname, parent);
   EnforcePolicy(p);
   return p;
}

ProfileBase *
ProfileBase::CreateEmptyProfile(ProfileBase const *parent)
{
   ProfileBase *p =  Profile::CreateEmptyProfile(parent);
   EnforcePolicy(p);
   return p;
}

ProfileBase *
ProfileBase::CreateGlobalConfig(const String & filename)
{
   ASSERT( ! strutil_isempty(filename) );
#  ifdef OS_WIN
   // don't give explicit name, but rather use the default logic (it's
   // perfectly ok, for the registry case our keys are under
   // vendor\appname)

   ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  "", "",
                                  wxCONFIG_USE_LOCAL_FILE |
                                  wxCONFIG_USE_GLOBAL_FILE);
#  else  // Unix
   mode_t um = umask(0077);
   String globalFile;
   globalFile << M_BASEDIR << '/'
              << M_APPLICATIONNAME << ".conf";
   if(! wxFileExists(globalFile))
      globalFile = String(M_APPLICATIONNAME) + ".conf";
   // we don't need the config file manager for this profile
   ms_GlobalConfig = new wxConfig(M_APPLICATIONNAME, M_VENDORNAME,
                                  filename, globalFile,
                                  wxCONFIG_USE_LOCAL_FILE|
                                  wxCONFIG_USE_GLOBAL_FILE);
   umask(um);
#  endif // Unix/Windows
   ProfileBase *p = Profile::CreateProfile("",NULL);
   EnforcePolicy(p);
   return p;
}

void
ProfileBase::DeleteGlobalConfig()
{
   if ( ms_GlobalConfig )
      delete ms_GlobalConfig;
}

String
ProfileBase::readEntry(const String & key,
                       const char *defaultvalue,
                       bool * found) const
{
   PCHECK();
   String str;
   str = readEntry(key, String(defaultvalue), found);
   return str;
}

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the wxConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/
Profile::Profile(const String & iName, ProfileBase const *Parent)
{
   m_ProfileName = ( Parent && Parent->GetName().Length()) ?
      ( Parent->GetName() + '/' ) :
      String(M_PROFILE_CONFIG_SECTION);
   if(iName.Length())
      m_ProfileName << '/' << iName;
   m_IsEmpty = false;
}


ProfileBase *
Profile::CreateProfile(const String & iClassName, ProfileBase const *parent)
{
   return new Profile(iClassName, parent);
}

ProfileBase *
Profile::CreateEmptyProfile(ProfileBase const *parent)
{
   Profile * p = new Profile("", parent);
   p->m_IsEmpty = true;
   return p;
}

Profile::~Profile()
{
   ASSERT(this != mApplication->GetProfile());
}

bool
Profile::HasGroup(const String & name) const
{
   PCHECK();
   if(m_IsEmpty) return false;
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasGroup(name);
}

bool
Profile::HasEntry(const String & key) const
{
   PCHECK();
   if(m_IsEmpty) return false;
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->HasEntry(key) || ms_GlobalConfig->HasGroup(key);
}

bool
Profile::Rename(const String& oldName, const String& newName)
{
   PCHECK();
   if(m_IsEmpty) return false;
   ms_GlobalConfig->SetPath(GetName());
   return ms_GlobalConfig->RenameGroup(oldName, newName);
}

void
Profile::DeleteGroup(const String & path)
{
   PCHECK();
   if(m_IsEmpty) return;
   ms_GlobalConfig->SetPath(GetName());
   ms_GlobalConfig->DeleteGroup(path);
}


String
Profile::readEntry(const String & key, const String & def, bool * found) const
{
   PCHECK();

   ms_GlobalConfig->SetPath(GetName());

   String keypath;
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   String str;

   bool foundHere = ms_GlobalConfig->Read(keypath,&str, def);
   bool foundAnywhere = foundHere;
   while ( !foundAnywhere &&
           (ms_GlobalConfig->GetPath() != M_PROFILE_CONFIG_SECTION) )
   {
      ms_GlobalConfig->SetPath("..");
      foundAnywhere = ms_GlobalConfig->Read(keypath,&str, def);
   }

   if ( found )
      *found = foundHere;

   return str;
}

long
Profile::readEntry(const String & key, long def, bool * found) const
{
   PCHECK();

   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   long val;
   bool foundHere = ms_GlobalConfig->Read(keypath,&val,def);
   bool foundAnywhere = foundHere;
   while ( !foundAnywhere &&
           (ms_GlobalConfig->GetPath() != M_PROFILE_CONFIG_SECTION) )
   {
      ms_GlobalConfig->SetPath("..");
      foundAnywhere = ms_GlobalConfig->Read(keypath,&val,def);
   }

   if ( found )
       *found = foundHere;

   return val;
}

bool
Profile::writeEntry(const String & key, const String & value)
{
   PCHECK();
   if(m_IsEmpty) return false;
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, value);
}

bool
Profile::writeEntry(const String & key, long value)
{
   PCHECK();
   if(m_IsEmpty) return false;
   ms_GlobalConfig->SetPath(GetName());
   String keypath;
   if(m_ProfilePath.Length())
      keypath << m_ProfilePath << '/';
   keypath << key;
   return ms_GlobalConfig->Write(keypath, (long) value);
}


#ifdef DEBUG
String
Profile::DebugDump() const
{
   PCHECK();
   return m_ProfileName;
}

#endif

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(ProfileBase * conf, const wxArrayString& astr, const String & key)
{
   // save all array entries
   conf->DeleteGroup(key);    // remove all old entries

   String path;
   path << key << '/';

   size_t nCount = astr.Count();
   String strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf("%d", n);
      conf->writeEntry(path+strkey, astr[n]);
   }
}

// restores array saved by SaveArray
void RestoreArray(ProfileBase * conf, wxArrayString& astr, const String & key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   String path;
   path << key << '/';

   String strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf("%d", n);
      if ( !conf->HasEntry(path+strkey) )
         break;
      strVal = conf->readEntry(path+strkey, "");
      astr.Add(strVal);
   }
}

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// some characters are invalid in the profile name, replace them
String
ProfileBase::FilterProfileName(const String& profileName)
{
   // the list of characters which are allowed in the profile names (all other
   // non alphanumeric chars are not)
   static const char *aValidChars = "_-.";   // NOT '/' and '\\'!

   String filteredName;
   size_t len = profileName.Len();
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

bool Profile::IsAncestor(ProfileBase *profile) const
{
   if ( !profile )
      return false;

   // can't compare as strings because '/' are sometimes duplicated...
   wxArrayString aMyComponents, aOtherComponents;
   wxSplitPath(aMyComponents, m_ProfileName);
   wxSplitPath(aOtherComponents, ((Profile *)profile)->m_ProfileName);

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
