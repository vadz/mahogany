/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifdef __GNUG__
#pragma interface "PathFinder.h"
#endif

#include	<Mcommon.h>
#include	<CommonBase.h>

#include	<list>

/**@name PathFinder class for finding files */
//@{

#ifdef OS_UNIX
/// define a delimiter for separating paths
#	define	PATHFINDER_DELIMITER	":"
#	include	<unistd.h>
#else
#	define	PATHFINDER_DELIMITER	";"
#endif

/**
   PathFinder class which finds a file,  given its name and the list
   of possible paths.
*/
class PathFinder : public CommonBase
{
   /// the list of absolute paths
   list<String> *pathList;
public:
   /**
      Constructor.
      @param initial pathlist, separated by either colons (unix) or semicolons (dos)
      @param recursive if true, add all subdirectories
   */
   PathFinder(String const &ipathlist = "", bool recursive = false);

   /**
      Destructor.
   */
   ~PathFinder();

   /**
      Adds the pathlist to the list of paths.
      @param pathlist like for constructor
      @param recursive if true, add all subdirectories
   */
   void AddPaths(String const &pathlist, bool recursive = false);
   
   /** 
       Returns the path to the file or directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode	the mode to test for access()
       @return full path or an empty string
   */
   String Find(String const &filename,  bool *found = NULL, int mode = R_OK) const;

   /** 
       Returns the path to the file or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode	the mode to test for access()
       @return full path or an empty string
   */
   String FindFile(String const &filename,  bool *found = NULL, int
		   mode = R_OK) const;

   /** 
       Returns the path to the directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode	the mode to test for access()
       @return full path or an empty string
   */
   String FindDir(String const &filename,  bool *found = NULL, int mode = R_OK) const;

   /** 
       Returns the path to the directory where the file resides or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode	the mode to test for access()
       @return full path or an empty string
   */
   String FindDirFile(String const &filename,  bool *found = NULL, int mode = R_OK) const;

   /**
      check whether pathname is a directory
      @return true if directory
   */
   static bool	IsDir(String const &pathname);

   /**
      check whether pathname is a file
      @return true if file
   */
   static bool	IsFile(String const &pathname);
   
   /// initialised == there is a list of paths
   bool	IsInitialised(void) const { return ! pathList->empty(); }

   CB_DECLARE_CLASS(PathFinder,CommonBase);
};

//@}

#endif
