/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "PathFinder.h"
#endif

#include	<guidef.h>

#include	<PathFinder.h>
#include	<strings.h>
#ifdef	OS_UNIX
#	include	<unistd.h>
#	include <sys/stat.h>
#	define	ANYFILE	"/*"
#else
#	define	ANYFILE	"/*.*"
#endif

PathFinder::PathFinder(String const &ipathlist, bool recursive)
{
   pathList = new list<String>;
   AddPaths(ipathlist,recursive);
}

void
PathFinder::AddPaths(String const &ipathlist, bool recursive)
{
   char *work = new char[ipathlist.length()+1];
   char	*found;
   String	tmp;
   String	subdirList = "";
   char	*nextfile;
   
   strcpy(work,ipathlist.c_str());
   found = strtok(work, PATHFINDER_DELIMITER);
   
   while(found)
   {
      pathList->push_back(String(found));
      if(recursive && IsDir(found))	// look for subdirectories
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
   list<String>::const_iterator i;
   String	work;
   int	result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = (*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0)
      {
	 if(found)	*found = true;
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
   list<String>::const_iterator i;
   String	work;
   int	result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = (*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsFile(work))
      {
	 if(found)	*found = true;
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
   list<String>::const_iterator i;
   String	work;
   int	result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = (*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsDir(work))
      {
	 if(found)	*found = true;
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
   list<String>::const_iterator i;
   String	work;
   int	result;
   
   for(i = pathList->begin(); i != pathList->end(); i++)
   {
      work = (*i) + '/' + filename;
      result = access(work.c_str(),mode);
      if(result == 0 && IsFile(work) && IsDir(*i))
      {
	 if(found)	*found = true;
	 return *i;
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
   delete pathList;
}

