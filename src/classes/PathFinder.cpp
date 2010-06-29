/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

/*
   TODO: rewrite all this using wxWin classes/functions, don't
         duplicate the code needlessly here!
*/

#include  "Mpch.h"

#ifndef   USE_PCH
#  include  "Mcommon.h"
#endif // USE_PCH

#if defined(OS_WIN) && !defined(__WINE__) // cygwin and mingw
#  include  <io.h> // for access()
#endif

#include "PathFinder.h"

#include <wx/file.h>

#if defined(OS_UNIX)
#   define   ANYFILE   _T("/*")
#elif defined(OS_WIN)
#   define   ANYFILE   _T("/*.*")
#endif

PathFinder::PathFinder(const String & ipathlist, bool recursive)
{
   AddPaths(ipathlist,recursive);
}

void
PathFinder::AddPaths(const String & ipathlist, bool recursive, bool prepend)
{
   char *work = new char[ipathlist.length()+1];
   char   *found, *save_ptr;
   String   tmp;
   String   subdirList = wxEmptyString;

   MOcheck();
   wxStrcpy(work,ipathlist.c_str());
   found = wxStrtok(work, PATHFINDER_DELIMITER, &save_ptr);

   while(found)
   {
      if(prepend)
         pathList.push_front(found);
      else
         pathList.push_back(found);
      if(recursive && IsDir(found))   // look for subdirectories
      {
         tmp = String(found) + ANYFILE;
         wxString nextfile = wxFindFirstFile(tmp.c_str(), wxDIR);
         while ( !nextfile.empty() )
         {
            if(IsDir(nextfile))
            {
               if(subdirList.length() > 0)
                  subdirList += _T(":");
               subdirList = subdirList + String(nextfile);
            }
            nextfile = wxFindNextFile();
         }
      }
      found = wxStrtok(NULL, PATHFINDER_DELIMITER, &save_ptr);
   }
   delete[] work;
   if(subdirList.length() > 0)
      AddPaths(subdirList, recursive);
}

String
PathFinder::Find(const String & filename, bool *found,
                 int mode) const
{
   StringList::const_iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList.begin(); i != pathList.end(); i++)
   {
      work = *i + DIR_SEPARATOR + filename;
      result = wxAccess(work.c_str(),mode);
      if(result == 0)
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return wxEmptyString;
}

String
PathFinder::FindFile(const String & filename, bool *found,
                     int mode) const
{
   StringList::const_iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList.begin(); i != pathList.end(); i++)
   {
      work = *i + DIR_SEPARATOR + filename;
      result = wxAccess(work.c_str(),mode);
      if(result == 0 && IsFile(work))
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return wxEmptyString;
}

String
PathFinder::FindDir(const String & filename, bool *found,
                    int mode) const
{
   StringList::const_iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList.begin(); i != pathList.end(); i++)
   {
      work = *i + DIR_SEPARATOR + filename;
      result = wxAccess(work.c_str(),mode);
      if(result == 0 && IsDir(work))
      {
         if(found)   *found = true;
         return work;
      }
   }
   if(found)
      *found = false;
   return wxEmptyString;
}

String
PathFinder::FindDirFile(const String & filename, bool *found,
                        int mode) const
{
   StringList::const_iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList.begin(); i != pathList.end(); i++)
   {
      work = *i + DIR_SEPARATOR + filename;
      result = wxAccess(work.c_str(),mode);
      if(result == 0 && IsFile(work) && IsDir(*i))
      {
         if(found)   *found = true;
         return *i;
      }
   }
   if(found)
      *found = false;
   return wxEmptyString;
}

// static
bool
PathFinder::IsDir(const String & pathname)
{
   return wxDirExists(pathname);
}

//static
bool
PathFinder::IsFile(const String & pathname)
{
   return wxFile::Exists(pathname);
}

