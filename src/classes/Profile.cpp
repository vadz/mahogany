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
// a tiny utility class which is used to temporary change the config path, for
// example:
//    {
//       ProfilePathChanger ppc(profile->GetConfig(), "/M/Frames");
//       profile->WriteEntry("x", 400);
//       ...
//       // path automatically restored here
//    }
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// wxConfigProfile
// ----------------------------------------------------------------------------

wxConfigProfile::wxConfigProfile(const char *fileName)
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

String const &
wxConfigProfile::GetPath(void) const
{
   return m_Config->GetPath();
}

wxConfigProfile::~wxConfigProfile(const char *appName)
{
   delete m_Config;
}

String
wxConfigProfile::readEntry(const char *key, const char *def) const
{
   return m_Config->Read(key,def);
}

int
wxConfigProfile::readEntry(const char *key, int def) const
{
   return m_Config->Read(key,def);
}

bool
wxConfigProfile::readEntry(const char *key, bool def) const
{
   return m_Config->Read(key,def);
}

bool
wxConfigProfile::writeEntry(const char *key, const char *value)
{
   return m_Config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(const char *key, int value)
{
   return m_Config->Write(key,value);
}

bool
wxConfigProfile::writeEntry(const char *key, bool value)
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

      String fileName = profileName + READ_APPCONFIG(MC_PROFILE_EXTENSION);
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
Profile::readEntry(const char *szKey, const char *szDefault) const
{
   // config object must be created
   CHECK( fileConfig != NULL, "", "no fileConfig in Profile" );

   wxString rc;

   fileConfig->Read(&rc, szKey, (const char *)NULL);

   if( strutil_isempty(rc) && parentProfile != NULL)
   {
      rc = parentProfile->readEntry(szKey, (const char *)NULL);
   }

   if( strutil_isempty(rc) )
   {
      ProfilePathChanger ppc(mApplication->GetProfile(), M_PROFILE_CONFIG_SECTION);

      mApplication->GetProfile()->SetPath(profileName.c_str());
      rc = mApplication->GetProfile()->readEntry(szKey, (const char *)NULL);
      if( strutil_isempty(rc))
      {
         mApplication->GetProfile()->SetPath(M_PROFILE_CONFIG_SECTION);
         rc = mApplication->GetProfile()->readEntry(szKey, (const char *)NULL);
      }
   }

   if( strutil_isempty(rc) )
   {
      if(READ_APPCONFIG(MC_RECORDDEFAULTS) )
         fileConfig->Write(szKey,szDefault); // so default value can be recorded
      rc = szDefault;
   }

#  ifdef DEBUG
   String dbgtmp = "Profile(" + profileName + String(")::readEntry(") +
      String(szKey) + ") returned: " + 
      (strutil_isempty(rc) ? (String(szDefault == NULL
                                     ? "null" : szDefault)
                              + " (default)")
       : rc);
   DBGLOG(Str(dbgtmp));
#  endif

   return rc;
}

int
Profile::readEntry(const char *szKey, int Default) const
{
   int rc;

   if ( !fileConfig->Read((long *)&rc, szKey, Default) )
   {
      if ( !parentProfile ||
           (rc = parentProfile->readEntry(szKey, Default)) == Default
         )
      {
         if ( mApplication->GetProfile() )
         {
            rc = mApplication->GetProfile()->readEntry(szKey,Default);
            {
               if ( READ_APPCONFIG(MC_RECORDDEFAULTS) )
                  fileConfig->Write(szKey, Default);
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

String const &
Profile::GetPath(void) const
{
   return fileConfig->GetPath();
}

bool
Profile::readEntry(const char *szKey, bool Default) const
{
   return readEntry(szKey, (int) Default) != 0;
}

bool
Profile::writeEntry(const char *szKey, int Value)
{
   CHECK( fileConfig != NULL, false, "no fileConfig in Profile" );

   return fileConfig->Write(szKey, (long int) Value) != 0;
}

bool
Profile::writeEntry(const char *szKey, const char *szValue)
{
   CHECK( fileConfig != NULL, false, "no fileConfig in Profile" );

   return fileConfig->Write(szKey, szValue) != 0;
}

bool
Profile::writeEntry(const char *szKey, bool Value)
{
   return writeEntry(szKey, (int) Value);
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

   if ( isApp )
      newEntry->fileConfig = GLOBAL_NEW wxConfig(newEntry->fileName);
   else
      newEntry->fileConfig = GLOBAL_NEW wxConfig(newEntry->fileName,
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

// all settings are saved as entries 0, 1, 2, ... of group szKey
void SaveArray(ProfileBase& conf, const wxArrayString& astr, const char *szKey)
{
   // save all array entries
   conf.DeleteGroup(szKey);    // remove all old entries
   conf.SetPath(szKey);

   size_t nCount = astr.Count();
   wxString strKey;
   for ( size_t n = 0; n < nCount; n++ ) {
      strKey.Printf("%d", n);
      conf.Write(strKey, astr[n]);
   }

   conf.SetPath("..");
}

// restores array saved by SaveArray
void RestoreArray(ProfileBase& conf, wxArrayString& astr, const char *szKey)
{
   wxASSERT( astr.IsEmpty() ); // should be called in the very beginning

   conf.SetPath(szKey);

   wxString strKey, strVal;
   for ( size_t n = 0; ; n++ ) {
      strKey.Printf("%d", n);
      if ( !conf.HasEntry(strKey) )
         break;
      conf.readEntry(&strVal, strKey);
      astr.Add(strVal);
   }

   conf.SetPath("..");
}

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
