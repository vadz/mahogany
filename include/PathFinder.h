/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$             *
 ********************************************************************/



#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifdef __GNUG__
#   pragma interface "PathFinder.h"
#endif

#include "MObject.h"

/**@name PathFinder class for finding files */
//@{

#if defined(OS_UNIX) || defined(__CYGWIN__)
/// define a delimiter for separating paths
#  define   PATHFINDER_DELIMITER ":"
#  include  <unistd.h>  // for R_OK
#elif   defined(OS_WIN)
#  define   PATHFINDER_DELIMITER ";"
#  define  R_OK                  4       // access() mode
#endif

/**
   PathFinder class which finds a file,  given its name and the list
   of possible paths.
*/
class PathFinder : public MObject
{
public:
   /**
      Constructor.
      @param initial pathlist, separated by either colons (unix) or semicolons (dos)
      @param recursive if true, add all subdirectories
   */
   PathFinder(const String & ipathlist = M_EMPTYSTRING, bool recursive = false);

   /**
      Destructor.
   */
   ~PathFinder();

   /**
      Adds the pathlist to the list of paths.
      @param pathlist like for constructor
      @param recursive if true, add all subdirectories
      @param prepend if true, add paths to head of list
   */
   void AddPaths(const String & pathlist, bool recursive = false,
                 bool prepend = true);
   
   /** 
       Returns the path to the file or directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String Find(const String & filename,
               bool *found = NULL,
               int mode = R_OK) const;

   /** 
       Returns the path to the file or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindFile(const String & filename,
                   bool *found = NULL,
                   int mode = R_OK) const;

   /** 
       Returns the path to the directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindDir(const String & filename,
                  bool *found = NULL,
                  int mode = R_OK) const;

   /** 
       Returns the path to the directory where the file resides or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindDirFile(const String & filename,
                      bool *found = NULL,
                      int mode = R_OK) const;

   /**
      check whether pathname is a directory
      @return true if directory
   */
   static bool IsDir(const String & pathname);

   /**
      check whether pathname is a file
      @return true if file
   */
   static bool IsFile(const String & pathname);
private:
   /// the list of absolute paths
   class kbStringList *pathList;
   MOBJECT_NAME(PathFinder)
};

//@}

#endif
