/*-*- c++ -*-********************************************************
 * PathFinder - a class for finding files or directories            *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$             *
 ********************************************************************/

#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifdef __GNUG__
#   pragma interface "PathFinder.h"
#endif

#ifndef STRINGARG
#   define   STRINGARG String const &
//#   define   STRINGARG String
#endif

/**@name PathFinder class for finding files */
//@{

#ifdef   OS_UNIX
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
   /// the list of absolute paths
   class kbStringList *pathList;
public:
   /**
      Constructor.
      @param initial pathlist, separated by either colons (unix) or semicolons (dos)
      @param recursive if true, add all subdirectories
   */
   PathFinder(STRINGARG ipathlist = "", bool recursive = false);

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
   void AddPaths(STRINGARG pathlist, bool recursive = false,
                 bool prepend = true);
   
   /** 
       Returns the path to the file or directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String Find(STRINGARG filename,
               bool *found = NULL,
               int mode = R_OK) const;

   /** 
       Returns the path to the file or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindFile(STRINGARG filename,
                   bool *found = NULL,
                   int mode = R_OK) const;

   /** 
       Returns the path to the directory or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindDir(STRINGARG filename,
                  bool *found = NULL,
                  int mode = R_OK) const;

   /** 
       Returns the path to the directory where the file resides or an empty string.
       @param filename without path
       @param found if non-NULL, gets set to success flag
       @param mode   the mode to test for access()
       @return full path or an empty string
   */
   String FindDirFile(STRINGARG filename,
                      bool *found = NULL,
                      int mode = R_OK) const;

   /**
      check whether pathname is a directory
      @return true if directory
   */
   static bool IsDir(STRINGARG pathname);

   /**
      check whether pathname is a file
      @return true if file
   */
   static bool IsFile(STRINGARG pathname);
};

//@}

#endif
