/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
 ********************************************************************
 * $Log$
 * Revision 1.6  1998/05/18 17:48:33  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.5  1998/05/14 11:00:16  KB
 * Code cleanups and conflicts resolved.
 *
 * Revision 1.4  1998/05/14 09:48:51  KB
 * added IsEmpty() to strutil, minor changes
 *
 * Revision 1.3  1998/05/11 20:57:28  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:20  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "Profile.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#ifndef   USE_PCH
#   include   "strutil.h"
#   include   "kbList.h"
#   include "MApplication.h"
#   include "Mdefaults.h"
#   include "Profile.h"
#endif

#include "PathFinder.h"

#ifdef  USE_WXCONFIG
#   include "wx/file.h"
#   include "wx/textfile.h"
#   include "wx/fileconf.h"
#endif

/**
   Profile class, managing configuration options on a per class basis.
   This class does essentially the same as the FileConfig class, but
   when initialised gets passed the name of the class it is related to
   and a parent profile. It then tries to load the class configuration
   file. If an entry is not found, it tries to get it from its parent
   profile. Thus, an inheriting profile structure is created.
*/


Profile::Profile(String const &iClassName, ProfileBase const *Parent)
{
   isOk = false;
   fileConfig = NULL;
   parentProfile = NULL;

   profileName = iClassName;

   String
      fileName, fullFileName,
      tmp = mApplication.readEntry(MC_PROFILE_PATH,MC_PROFILE_PATH_D);
   PathFinder
      pf(tmp);

   fileName = iClassName +
      String(mApplication.readEntry(MC_PROFILE_EXTENSION,MC_PROFILE_EXTENSION_D));
   
   fullFileName = pf.FindFile(fileName, &isOk);
   
   if(! isOk)
      fullFileName = mApplication.GetLocalDir() + DIR_SEPARATOR + fileName;

   fileConfig = cfManager.GetConfig(fullFileName);   //new FileConfig();
   
   if(Parent)
   {
      parentProfile = Parent;
      isOk = true;
   }
}


Profile::~Profile()
{
   if(fileConfig)
      delete fileConfig;
}


const char *
Profile::readEntry(const char *szKey, const char *szDefault) const
{
#ifdef   USE_WXCONFIG
   static char s_szBuffer[1024];  // @@ static buffer
   String rc;
#else
   const char *rc = NULL;
#endif

   DBGLOG("Profile(" << profileName << ")::readEntry(" << szKey 
          << ",(const char *)" << szDefault << ')'
          << " name: " << profileName);
   if(fileConfig)
   {
      DBGLOG("   looking in fileConfig");
#ifdef   USE_WXCONFIG
      fileConfig->Read(&rc, szKey, NULL);
#else
      rc = fileConfig->readEntry(szKey, (const char *)NULL);
#endif
   }
   if( strutil_isempty(rc) && parentProfile != NULL)
   {
      DBGLOG("   looking in parentProfile");
      rc = parentProfile->readEntry(szKey, (const char *)NULL);
   }
   if( strutil_isempty(rc) )
   {
      DBGLOG("   looking in mApplication");
      String tmp = mApplication.getCurrentPath();
      mApplication.setCurrentPath(M_PROFILE_CONFIG_SECTION);
      mApplication.changeCurrentPath(profileName.c_str());
      rc = mApplication.readEntry(szKey, (const char *)NULL);
      mApplication.setCurrentPath(tmp.c_str());
   }
   if( strutil_isempty(rc) )
   {
      DBGLOG("   writing entry back");
      fileConfig->WRITE_ENTRY(szKey,szDefault); // so default value can be recorded
      rc = szDefault;
   }

   DBGLOG("Profile::readEntry() returned: " << rc);

#ifdef   USE_WXCONFIG
   strncpy(s_szBuffer, rc, SIZEOF(s_szBuffer));
   return s_szBuffer;
#else
   return rc;
#endif
}

int
Profile::readEntry(const char *szKey,  int Default) const
{
   int
      rc = Default;
   char
      *buf = strutil_strdup(strutil_ltoa(Default));
   char const
      * tmp;
   
   //DBGLOG("Profile::readEntry(" << szKey << ',' << Default << ')' << " name: " << profileName);
   tmp = readEntry(szKey,buf);
   rc = tmp ? atoi(tmp) : 0;
   
   delete [] buf;
   //DBGLOG("Profile::readEntry() returned: " << rc);
   return rc;
}

bool
Profile::readEntry(const char *szKey, bool Default) const
{
   //DBGLOG("Profile::readEntry(" << szKey << ',' << Default << ')' << " name: " << profileName);
   return readEntry(szKey, (int) Default) != 0;
}

bool
Profile::writeEntry(const char *szKey, int Value)
{
   if(! fileConfig)
      return false;
   return fileConfig->WRITE_ENTRY(szKey, (long int) Value) != 0;
}

bool
Profile::writeEntry(const char *szKey, const char *szValue)
{
   if(! fileConfig)
      return false;
   return fileConfig->WRITE_ENTRY(szKey, szValue) != 0;
}

bool
Profile::writeEntry(const char *szKey, bool Value)
{
   return writeEntry(szKey, (int) Value);
}

ConfigFileManager Profile::cfManager;

ConfigFileManager::ConfigFileManager()
{
   fcList = new FCDataList;
}

ConfigFileManager::~ConfigFileManager()
{
   kbListIterator i;
   FileConfig *fcp;
   
#ifdef DEBUG
   Debug();
#endif
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      fcp = kbListICast(FCData,i)->fileConfig;
      fcp->FLUSH();
      delete fcp;
   }
   delete fcList;
}

FileConfig *
ConfigFileManager::GetConfig(String const &fileName)
{
   kbListIterator i;

#ifdef DEBUG
   cerr << "ConfigFileManager.GetConfig(" << fileName << ")" << endl;
#endif
   
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if(kbListICast(FCData,i)->fileName == fileName)
         return kbListICast(FCData,i)->fileConfig;
   }
   FCData   *newEntry = new FCData;
   newEntry->fileName = fileName;

#ifdef  USE_WXCONFIG
   newEntry->fileConfig = new wxFileConfig(newEntry.fileName);
#else
   newEntry->fileConfig = new FileConfig;
   newEntry->fileConfig->readFile(newEntry->fileName.c_str());
   newEntry->fileConfig->expandVariables(M_PROFILE_VAREXPAND);

   //  activate recording of configuration entries
   if(mApplication.readEntry(MC_RECORDDEFAULTS,MC_RECORDDEFAULTS_D))
      newEntry->fileConfig->recordDefaults(TRUE);
#endif

   fcList->push_front(newEntry);

   return newEntry->fileConfig;
}

#ifdef DEBUG
void
ConfigFileManager::Debug(void)
{
   kbListIterator i;

   cerr << "------ConfigFileManager------" << endl;
   for(i = fcList->begin(); i != fcList->end(); i++)
      cerr << '"' << kbListICast(FCData,i)->fileName << '"' << '\t'
           << kbListICast(FCData,i)->fileConfig << endl;
   cerr << "-----------------------------" << endl;
}
#endif
