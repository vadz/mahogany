/*-*- c++ -*-********************************************************
 * sysutil.cpp : utility functions for various OS functionality     *
 *                                                                  *
 * (C) 1999 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"

#ifndef   USE_PCH
#   include "Mcommon.h"
#   include "sysutil.h"
#   include "strutil.h"
#endif // USE_PCH

#ifdef OS_UNIX
#   include <unistd.h>
#   include <fcntl.h>
#endif

      
bool sysutil_compare_filenames(String const &file1, String const
                              &file2)
{
#ifdef OS_UNIX
   struct stat statbuf1, statbuf2;
   int rc;

   rc = stat(file1, &statbuf1);
   if(rc != 0)
      if(rc == ENOENT)
      {
         // using all flags but x, will be modified by umask anyway
         int fd = creat(file1,S_IWUSR|S_IRUSR|S_IWGRP|S_IWUSR|S_IWOTH|S_IROTH);
         if(fd == -1)
            return strutil_compare_filenames(file1, file2);
         close(fd); //    created the empty file
         rc = stat(file1, &statbuf1);
         unlink(file1);
         if(rc != 0)
            return strutil_compare_filenames(file1, file2);
      }
      else
            return strutil_compare_filenames(file1, file2);
   
   rc = stat(file2, &statbuf2);
   if(rc != 0)
      if(rc == ENOENT)
      {
         // using all flags but x, will be modified by umask anyway
         int fd = creat(file2,S_IWUSR|S_IRUSR|S_IWGRP|S_IWUSR|S_IWOTH|S_IROTH);
         if(fd == -1)
            return strutil_compare_filenames(file1, file2);
         close(fd); //    created the empty file
         rc = stat(file2, &statbuf2);
         unlink(file2);
         if(rc != 0)
            return strutil_compare_filenames(file1, file2);
      }
      else
            return strutil_compare_filenames(file1, file2);

   return
      statbuf1.st_dev == statbuf2.st_dev // same device
      && statbuf1.st_ino == statbuf2.st_ino; // same inode
#else
// WINOZE is dumb :-)
   return strutil_compare_filenames(file1, file2);
#endif
}
