/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$            *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/08/08 22:29:15  VZ
 * memory leaks and crashes and memory corruptio and ... fixes
 *
 * Revision 1.4  1998/05/24 14:47:58  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.3  1998/05/18 17:48:32  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
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

PathFinder::PathFinder(String const &ipathlist, bool recursive)
{
   pathList = new kbStringList(FALSE);
   AddPaths(ipathlist,recursive);
}

void
PathFinder::AddPaths(String const &ipathlist, bool recursive)
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
PathFinder::Find(String const &filename, bool *found,
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
PathFinder::FindFile(String const &filename, bool *found,
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
PathFinder::FindDir(String const &filename, bool *found,
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
PathFinder::FindDirFile(String const &filename, bool *found,
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
PathFinder::IsDir(String const &pathname)
{
   struct stat buf;
   
   if(stat(pathname.c_str(),&buf) == 0 && S_ISDIR(buf.st_mode))
      return true;
   else
      return false;
}

bool
PathFinder::IsFile(String const &pathname) 
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

