/*-*- c++ -*-********************************************************
 * Profile - managing configuration options on a per class basis    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:20  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "Profile.h"
#endif

#include	<Profile.h>
#include	<Mdefaults.h>
#include	<MApplication.h>
#include	<strutil.h>

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

   fileConfig = cfManager.GetConfig(fullFileName);	//new FileConfig();
   
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
   const char *rc = NULL;

   DBGLOG("Profile(" << profileName << ")::readEntry(" << szKey << ",(const char *)" << szDefault << ')' << " name: " << profileName);
   if(fileConfig)
   {
      DBGLOG("   looking in fileConfig");
      rc = fileConfig->readEntry(szKey, (const char *)NULL);
   }
   if((! rc ) && parentProfile != NULL)
   {
      DBGLOG("   looking in parentProfile");
      rc = parentProfile->readEntry(szKey, (const char *)NULL);
   }
   if(! rc)
   {
      DBGLOG("   looking in mApplication");
      String tmp = mApplication.getCurrentPath();
      mApplication.setCurrentPath(M_PROFILE_CONFIG_SECTION);
      mApplication.changeCurrentPath(profileName.c_str());
      rc = mApplication.readEntry(szKey, (const char *)NULL);
      mApplication.setCurrentPath(tmp.c_str());
   }
   if(rc == NULL)
   {
      DBGLOG("   writing entry back");
      fileConfig->writeEntry(szKey,szDefault); // so default value can
					       // be recorded
      rc = szDefault;
   }
   DBGLOG("Profile::readEntry() returned: " << rc);
   return rc;
}

int
Profile::readEntry(const char *szKey,  int Default) const
{
   int
      rc = Default;
   char
      *buf = strutil_strdup(strutil_ltoa(Default));
   
   //DBGLOG("Profile::readEntry(" << szKey << ',' << Default << ')' << " name: " << profileName);
   rc = atoi(readEntry(szKey,buf));
   delete [] buf;
   //DBGLOG("Profile::readEntry() returned: " << rc);
   return rc;
}

bool
Profile::readEntry(const char *szKey, bool Default) const
{
   //DBGLOG("Profile::readEntry(" << szKey << ',' << Default << ')' << " name: " << profileName);
   return (bool) readEntry(szKey, (int) Default);
}

bool
Profile::writeEntry(const char *szKey, int Value)
{
   if(! fileConfig)
      return false;
   return fileConfig->writeEntry(szKey, (long int) Value);
}

bool
Profile::writeEntry(const char *szKey, const char *szValue)
{
   if(! fileConfig)
      return false;
   return fileConfig->writeEntry(szKey, szValue);
}

bool
Profile::writeEntry(const char *szKey, bool Value)
{
   return writeEntry(szKey, (int) Value);
}

ConfigFileManager Profile::cfManager;

ConfigFileManager::ConfigFileManager()
{
   fcList = new list<FCData>;

}

ConfigFileManager::~ConfigFileManager()
{
   list<FCData>::iterator i;
   FileConfig *fcp;
   
#ifdef DEBUG
   Debug();
#endif
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      fcp = (*i).fileConfig;
      fcp->flush();
      delete fcp;
   }
   delete fcList;
}

FileConfig *
ConfigFileManager::GetConfig(String const &fileName)
{
   list<FCData>::iterator i;

#ifdef DEBUG
   cerr << "ConfigFileManager.GetConfig(" << fileName << ")" << endl;
#endif
   
   for(i = fcList->begin(); i != fcList->end(); i++)
   {
      if((*i).fileName == fileName)
	 return (*i).fileConfig;
   }
   FCData	newEntry;
   newEntry.fileName = fileName;
   newEntry.fileConfig = new FileConfig;
   newEntry.fileConfig->readFile(newEntry.fileName.c_str());
   newEntry.fileConfig->expandVariables(M_PROFILE_VAREXPAND);
   fcList->push_front(newEntry);

   //  activate recording of configuration entries
   if(mApplication.readEntry(MC_RECORDDEFAULTS,MC_RECORDDEFAULTS_D))
      newEntry.fileConfig->recordDefaults(TRUE);

   
   return newEntry.fileConfig;
}

#ifdef DEBUG
void
ConfigFileManager::Debug(void)
{
   list<FCData>::iterator i;

   cerr << "------ConfigFileManager------" << endl;
   for(i = fcList->begin(); i != fcList->end(); i++)
      cerr << '"' << (*i).fileName << '"' << '\t' << (*i).fileConfig << endl;
   cerr << "-----------------------------" << endl;
}
#endif
