/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
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
#include  "Mcommon.h"

#ifndef USE_PCH
#   include "strutil.h"
#   include "PathFinder.h"

#   include "MimeList.h"
#   include "MimeTypes.h"

#   include "kbList.h"

#   include "MFrame.h"
#   include "gui/wxMFrame.h"

#   include "Profile.h"

#   include "MApplication.h"
#   include "gui/wxMApp.h"
#   ifdef  OS_WIN
#      include   <wx/msw/regconf.h>
#   else
#      include   <wx/confbase.h>
#      include   <wx/file.h>
#      include   <wx/textfile.h>
#      include   <wx/fileconf.h>
#   endif
#   include   <wx/config.h>
#endif

#include "Mdefaults.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ProfileBase
// ----------------------------------------------------------------------------

ProfileBase *
ProfileBase::CreateProfile(String const &classname, ProfileBase const *parent)
{
   return new Profile(classname, parent);
}

ProfileBase *
ProfileBase::CreateGlobalConfig(String const &filename)
{
   return new wxConfigProfile(filename);
}

// ----------------------------------------------------------------------------
// wxConfigProfile
// ----------------------------------------------------------------------------

wxConfigProfile::wxConfigProfile(String const &fileName)
{
   // we shouldn't be called twice normally
   ASSERT( m_Config == NULL );

   m_Config = mApplication->GetConfigManager().GetConfig(fileName, TRUE);
   // set the default path for configuration entries
   m_Config->SetPath(M_APPLICATIONNAME);
}

void wxConfigProfile::SetPath(String const &path)
{
   m_Config->SetPath(path);
}

String 
wxConfigProfile::GetPath(void) const
{
   return m_Config->GetPath();
}

bool
wxConfigProfile::HasEntry(String const &key) const
{
   return m_Config->HasEntry(key);
}

void
wxConfigProfile::DeleteGroup(String const &path)
{
   m_Config->DeleteGroup(path);
}

wxConfigProfile::~wxConfigProfile(String const &appName)
{
   delete m_Config;
}

String 
wxConfigProfile::readEntry(String const &key, String const &def) const
{

   return m_Config->Read(key.c_str(),def.c_str());
}

long
wxConfigProfile::readEntry(String const &key, long def) const
{
   return m_Config->Read(key.c_str(),def);
}

bool
wxConfigProfile::readEntry(String const &key, bool def) const
{
   return m_Config->Read(key.c_str(),def);
}

bool
wxConfigProfile::writeEntry(String const &key, String const &value)
{
   return m_Config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(String const &key, long value)
{
   return m_Config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(String const &key, bool value)
{
   return m_Config->Write(key,value);
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

Profile::Profile(String const &iClassName, ProfileBase const *Parent)
   : profileName(iClassName)
{
   fileConfig = NULL;   // set it before using CHECK()

   // the top entry should already exist and we must have a parent
   CHECK_RET( mApplication->GetProfile(),
              "MApplication profile should have been constructed before" );

   parentProfile = Parent;

   // find our config file unless we already have an absolute path name
   String fullFileName;
   if ( !IsAbsPath(profileName) )
   {
      String tmp = READ_APPCONFIG(MC_PROFILE_PATH);
      PathFinder pf(tmp);

      String fileName = profileName +
         mApplication->GetProfile()->readEntry((const char *)MC_PROFILE_EXTENSION,
                                               String(MC_PROFILE_EXTENSION_D));
      //READ_APPCONFIG(MC_PROFILE_EXTENSION);
      fullFileName = pf.FindFile(fileName, &isOk);
   
      if( !isOk )
         fullFileName = mApplication->GetLocalDir() + DIR_SEPARATOR + fileName;
   }
   else
   {
      // easy...
      fullFileName << profileName << READ_APPCONFIG(MC_PROFILE_EXTENSION);
   }

   fileConfig = mApplication->GetConfigManager().GetConfig(fullFileName);
   
   isOk = fileConfig != NULL;
}


Profile::~Profile()
{
}


String 
Profile::readEntry(String const &key, String const &defaultvalue) const
{
   // config object must be created
   CHECK( fileConfig != NULL, "", "no fileConfig in Profile" );

   wxString rc = fileConfig->Read(key.c_str());

   if( strutil_isempty(rc) && parentProfile != NULL)
   {
      rc = parentProfile->readEntry(key, (const char *)NULL);
   }

   if( strutil_isempty(rc) )
   {
      ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);

      mApplication->GetProfile()->SetPath(profileName.c_str());
      rc = mApplication->GetProfile()->readEntry(key, (const char *)NULL);
      if( strutil_isempty(rc))
      {
         mApplication->GetProfile()->SetPath(M_PROFILE_CONFIG_SECTION);
         rc = mApplication->GetProfile()->readEntry(key, (const char *)NULL);
      }
   }

   if( strutil_isempty(rc) )
   {
      if(READ_APPCONFIG(MC_RECORDDEFAULTS) )
         fileConfig->Write(key,defaultvalue); // so default value can be recorded
      rc = defaultvalue;
   }

#  ifdef DEBUG
   String dbgtmp = "Profile(" + profileName + String(")::readEntry(") +
      String(key) + ") returned: " + rc;
   DBGLOG(Str(dbgtmp));
#  endif

   return rc;
}

long
Profile::readEntry(String const &key, long defaultvalue) const
{
   long rc;

   if ( !fileConfig->Read(key, &rc, defaultvalue) )
   {
      if ( !parentProfile ||
           (rc = parentProfile->readEntry(key, defaultvalue)) == defaultvalue
         )
      {
         if ( mApplication->GetProfile() )
         {
            rc = mApplication->GetProfile()->readEntry(key,defaultvalue);
            {
               if ( READ_APPCONFIG(MC_RECORDDEFAULTS) )
                  fileConfig->Write(key, defaultvalue);
            }
         }
      }
   }
  
   return rc;
}


void Profile::SetPath(String const &path)
{
   fileConfig->SetPath(path);
}

String 
Profile::GetPath(void) const
{
   return fileConfig->GetPath();
}

bool
Profile::HasEntry(String const &key) const
{
   return fileConfig->HasEntry(key);
}

void
Profile::DeleteGroup(String const &path)
{
   fileConfig->DeleteGroup(path);
}

bool
Profile::readEntry(String const &key, bool defaultvalue) const
{
   return readEntry(key, (long) defaultvalue) != 0;
}

bool
Profile::writeEntry(String const &key, long Value)
{
   CHECK( fileConfig != NULL, false, "no fileConfig in Profile" );

   return fileConfig->Write(key, (long int) Value) != 0;
}

bool
Profile::writeEntry(String const &key, String const &Value)
{
   CHECK( fileConfig != NULL, false, "no fileConfig in Profile" );

   return fileConfig->Write(key, Value) != 0;
}

bool
Profile::writeEntry(String const &key, bool Value)
{
   return writeEntry(key, (long) Value);
}

// ----------------------------------------------------------------------------
// ConfigFileManager
// ----------------------------------------------------------------------------

ConfigFileManager::ConfigFileManager()
{
   fcList = new FCDataList(FALSE);
}

ConfigFileManager::~ConfigFileManager()
{
   FCDataList::iterator i;
   
#  ifdef DEBUG
   Debug();
#  endif

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      FCData *data = *i;
      wxConfigBase *fcp = data->fileConfig;
      fcp->Flush();
      //FIXME: this must be a DecRef()
      delete fcp;
      delete data;
   }

   delete fcList;
}

wxConfigBase *
ConfigFileManager::GetConfig(String const &fileName, bool isApp)
{
   FCDataList::iterator i;

   DBGLOG("ConfigFileManager.GetConfig(" << Str(fileName) << ")");
   
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if((*i)->fileName == fileName)
         return (*i)->fileConfig;
   }
   FCData   *newEntry = new FCData;
   newEntry->fileName = fileName;

   newEntry->fileConfig = new wxConfig(newEntry->fileName,
                                       wxString(""));
   fcList->push_front(newEntry);
   
   return newEntry->fileConfig;
}

#ifdef DEBUG
void
ConfigFileManager::Debug() const
{
   kbListIterator i;

   DBGLOG("------ConfigFileManager------\n");
   for(i = fcList->begin(); i != fcList->end(); i++) {
      // @@ where is operator<<(fileConfig)?
#     if 0
      DBGLOG('"' << (*i).fileName << '"' << '\t' << (*i).fileConfig << '\n');
#     endif
   }
   DBGLOG("-----------------------------\n");
}
#endif

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// all settings are saved as entries 0, 1, 2, ... of group key
void SaveArray(ProfileBase& conf, const wxArrayString& astr, String const &key)
{
   // save all array entries
   conf.DeleteGroup(key);    // remove all old entries
   conf.SetPath(key);

   size_t nCount = astr.Count();
   wxString strkey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strkey.Printf("%d", n);
      conf.writeEntry(strkey, astr[n]);
   }

   conf.SetPath("..");
}

// restores array saved by SaveArray
void RestoreArray(ProfileBase& conf, wxArrayString& astr, String const &key)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   conf.SetPath(key);

   wxString strkey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strkey.Printf("%d", n);
      if ( !conf.HasEntry(strkey) )
         break;
      strVal = conf.readEntry(strkey, "");
      astr.Add(strVal);
   }

   conf.SetPath("..");
}

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

ProfilePathChanger::ProfilePathChanger(ProfileBase *config, const String& path)
{
   m_config = config;
   m_strOldPath = m_config->GetPath();
   m_config->SetPath(path);
}

ProfilePathChanger::~ProfilePathChanger()
{
   m_config->SetPath(m_strOldPath);
}
