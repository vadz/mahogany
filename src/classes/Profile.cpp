/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
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
#  include "strutil.h"
#  include "MFrame.h"
#  include "MLogFrame.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "kbList.h"
#endif

#include "Mdefaults.h"

#include "MApplication.h"
#include "gui/wxMApp.h"

#ifdef  USE_WXCONFIG
#  include "wx/file.h"
#  include "wx/textfile.h"
#  include "wx/fileconf.h"
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// Profile
// ----------------------------------------------------------------------------

// static member variables
FileConfig *Profile::appConfig = NULL;

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the FileConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/

Profile::Profile(const String& appConfigFile)
{
   // we shouldn't be called twice normally
   ASSERT( appConfig == NULL );

   parentProfile = NULL;

   fileConfig = cfManager.GetConfig(appConfigFile);
   isOk = fileConfig != NULL;
   if ( isOk )
      appConfig = fileConfig;
}

Profile::Profile(String const &iClassName, ProfileBase const *Parent)
       : profileName(iClassName)
{
   fileConfig = NULL;   // set it before using CHECK()

   // the top entry should already exist and we must have a parent
   CHECK_RET( appConfig != NULL, 
              "appConfig should have been constructed before" );

   parentProfile = Parent;

   // find our config file
   String tmp = READ_APPCONFIG(MC_PROFILE_PATH);
   PathFinder pf(tmp);

   String fileName = profileName + READ_APPCONFIG(MC_PROFILE_EXTENSION);
   String fullFileName = pf.FindFile(fileName, &isOk);
   
   if( !isOk )
      fullFileName = mApplication.GetLocalDir() + DIR_SEPARATOR + fileName;

   fileConfig = cfManager.GetConfig(fullFileName);
   
   isOk = fileConfig != NULL;
}


Profile::~Profile()
{
   if ( appConfig == fileConfig )
      appConfig = NULL;

   if(fileConfig)
      delete fileConfig;
}


const char *
Profile::readEntry(const char *szKey, const char *szDefault) const
{
   // config object must be created
   CHECK( fileConfig != NULL, "", "no fileConfig in Profile" );

   const char *rc = NULL;

   rc = fileConfig->READ_ENTRY(szKey, (const char *)NULL);

   if( strutil_isempty(rc) && parentProfile != NULL)
   {
      rc = parentProfile->readEntry(szKey, (const char *)NULL);
   }

   if( strutil_isempty(rc) && appConfig != NULL )
   {
      String tmp = Str(appConfig->GET_PATH());
      appConfig->SET_PATH(M_PROFILE_CONFIG_SECTION);
      appConfig->CHANGE_PATH(profileName.c_str());
      rc = appConfig->READ_ENTRY(szKey, (const char *)NULL);
      appConfig->SET_PATH(tmp.c_str());
   }

   if( strutil_isempty(rc) )
   {
      fileConfig->WRITE_ENTRY(szKey,szDefault); // so default value can be recorded
      rc = szDefault;
   }

   String dbgtmp = "Profile(" + profileName
      + String(")::readEntry(")+String(szKey)
      +") returned: " + (rc == NULL ? ((szDefault == NULL ? String("null"):String(szDefault)) + " (default)")
                                      : String(rc));
   DBGLOG(Str(dbgtmp));
   return rc == NULL ? szDefault : rc;
}

int
Profile::readEntry(const char *szKey, int Default) const
{
   int
      rc = Default;
   char
      *buf = strutil_strdup(strutil_ltoa(Default));
   char const
      * tmp;
   
   tmp = readEntry(szKey,buf);
   rc = tmp ? atoi(tmp) : 0;
   
   delete [] buf;

   return rc;
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

   return fileConfig->WRITE_ENTRY(szKey, (long int) Value) != 0;
}

bool
Profile::writeEntry(const char *szKey, const char *szValue)
{
   CHECK( fileConfig != NULL, false, "no fileConfig in Profile" );

   return fileConfig->WRITE_ENTRY(szKey, szValue) != 0;
}

bool
Profile::writeEntry(const char *szKey, bool Value)
{
   return writeEntry(szKey, (int) Value);
}

// ----------------------------------------------------------------------------
// ConfigFileManager
// ----------------------------------------------------------------------------

ConfigFileManager Profile::cfManager;

ConfigFileManager::ConfigFileManager()
{
   fcList = new FCDataList;
}

ConfigFileManager::~ConfigFileManager()
{
   FCDataList::iterator i;
   FileConfig *fcp;
   
#  ifdef DEBUG
      Debug();
#  endif

   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      fcp = (*i)->fileConfig;
      fcp->FLUSH();
      delete fcp;
   }
   delete fcList;
}

FileConfig *
ConfigFileManager::GetConfig(String const &fileName)
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

#  ifdef  USE_WXCONFIG
      newEntry->fileConfig = GLOBAL_NEW wxFileConfig(Str(newEntry->fileName));
#  else
      newEntry->fileConfig = GLOBAL_NEW FileConfig;
      newEntry->fileConfig->readFile(newEntry->fileName->c_str());
      newEntry->fileConfig->expandVariables(M_PROFILE_VAREXPAND);

      //  activate recording of configuration entries
      if ( READ_APPCONFIG(MC_RECORDDEFAULTS) )
         newEntry.fileConfig->recordDefaults(TRUE);
#  endif

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
