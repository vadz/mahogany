/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "PathFinder.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#ifndef   USE_PCH
#   include   <guidef.h>
#   include   <string.h>
#   include   "kbList.h"
#endif

#if defined(OS_UNIX)
#   include <sys/stat.h>
#   define   ANYFILE   "/*"
#elif defined(OS_WIN)
#   include <sys/stat.h>
#   define  S_ISDIR(mode)     ((mode) & _S_IFDIR)
#   define  S_ISREG(mode)     ((mode) & _S_IFREG)
#   define   ANYFILE   "/*.*"
#endif

#include   "PathFinder.h"

PathFinder::PathFinder(STRINGARG ipathlist, bool recursive)
{
   pathList = new kbStringList(FALSE);
   AddPaths(ipathlist,recursive);
}

void
PathFinder::AddPaths(STRINGARG ipathlist, bool recursive)
{
   char *work = new char[ipathlist.length()+1];
   char   *found;
   String   tmp;
   String   subdirList = "";
   char   *nextfile;
   
   strcpy(work,ipathlist.c_str());
   found = strtok(work, PATHFINDER_DELIMITER);
   
   while(found)
   {
      pathList->push_back(new String(found));
      if(recursive && IsDir(found))   // look for subdirectories
      {
         tmp = String(found) + ANYFILE;
         nextfile = wxFindFirstFile(tmp.c_str());
         while(nextfile)
         {
            if(IsDir(nextfile))
            {
               if(subdirList.length() > 0)
                  subdirList += ":";
               subdirList = subdirList + String(nextfile);
            }
            nextfile = wxFindNextFile();
         }
      }
      found = strtok(NULL, PATHFINDER_DELIMITER);
   }
   delete[] work;
   if(subdirList.length() > 0)
      AddPaths(subdirList);
}

String
PathFinder::Find(STRINGARG filename, bool *found,
                 int mode) const
{
   kbStringList::iterator i;
   String   work;
   int   result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0)
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return "";
}

String
PathFinder::FindFile(STRINGARG filename, bool *found,
                     int mode) const
{
   kbStringList::iterator i;
   String   work;
   int   result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsFile(work))
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return "";
}

String
PathFinder::FindDir(STRINGARG filename, bool *found,
                    int mode) const
{
   kbStringList::iterator i;
   String   work;
   int   result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsDir(work))
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return "";
}

String
PathFinder::FindDirFile(STRINGARG filename, bool *found,
                        int mode) const
{
   kbStringList::iterator i;
   String   work;
   int   result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsFile(work) && IsDir(*(*i)))
      {
         if(found)   *found = true;
         return *(*i);
      }
   }
   if(found)
      *found = false;
   return "";
}

bool
PathFinder::IsDir(STRINGARG pathname)
{
   struct stat buf;
   
   if(stat(pathname.c_str(),&buf) == 0 && S_ISDIR(buf.st_mode))
      return true;
   else
      return false;
}

bool
PathFinder::IsFile(STRINGARG pathname) 
{
   struct stat buf;
   if(stat(pathname.c_str(),&buf) == 0 && S_ISREG(buf.st_mode))
      return true;
   else
      return false;
}


PathFinder::~PathFinder()
{
   kbStringList::iterator i;
   for ( i = pathList->begin(); i != pathList->end(); i++ ) {
      String *data = *i;
      delete data;
   }

   delete pathList;
}

