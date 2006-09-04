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

#ifdef __GNUG__
#   pragma implementation "PathFinder.h"
#endif

#include  "Mpch.h"

#ifndef   USE_PCH
#  include  "Mcommon.h"
#  include   "kbList.h"
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
   pathList = new kbStringList(FALSE);
   AddPaths(ipathlist,recursive);
}

void
PathFinder::AddPaths(const String & ipathlist, bool recursive, bool prepend)
{
   wxChar *work = new wxChar[ipathlist.length()+1];
   wxChar   *found, *save_ptr;
   String   tmp;
   String   subdirList = wxEmptyString;

   MOcheck();
   wxStrcpy(work,ipathlist.c_str());
   found = wxStrtok(work, PATHFINDER_DELIMITER, &save_ptr);

   while(found)
   {
      if(prepend)
         pathList->push_front(new String(found));
      else
         pathList->push_back(new String(found));
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
   kbStringList::iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + DIR_SEPARATOR + filename;
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
   kbStringList::iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + DIR_SEPARATOR + filename;
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
   kbStringList::iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + DIR_SEPARATOR + filename;
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
   kbStringList::iterator i;
   String   work;
   int   result;

   MOcheck();
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = *(*i) + DIR_SEPARATOR + filename;
      result = wxAccess(work.c_str(),mode);
      if(result == 0 && IsFile(work) && IsDir(*(*i)))
      {
         if(found)   *found = true;
         return *(*i);
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


PathFinder::~PathFinder()
{
   kbStringList::iterator i;

   MOcheck();
   for ( i = pathList->begin(); i != pathList->end(); i++ ) {
      String *data = *i;
      delete data;
   }

   delete pathList;
}

