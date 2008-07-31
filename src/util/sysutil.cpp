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

#  ifdef OS_UNIX
#     include <sys/types.h>
#     include <sys/stat.h>               // needed by wxStructStat
#     include <unistd.h>
#  endif // OS_UNIX
#endif // USE_PCH

#ifdef OS_UNIX
#   include <unistd.h>
#   include <errno.h>
#   include <fcntl.h>
#endif

#include <wx/mimetype.h>

bool sysutil_compare_filenames(String const &file1, String const
                              &file2)
{
#ifdef OS_UNIX
   struct stat statbuf1, statbuf2;
   int rc;

   rc = wxStat(file1, &statbuf1);
   if(rc != 0)
      if(rc == ENOENT)
      {
         // using all flags but x, will be modified by umask anyway
         int fd = creat(file1.fn_str(), S_IWUSR|S_IRUSR|S_IWGRP|S_IWUSR|S_IWOTH|S_IROTH);
         if(fd == -1)
            return strutil_compare_filenames(file1, file2);
         close(fd); //    created the empty file
         rc = wxStat(file1, &statbuf1);
         unlink(file1.fn_str());
         if(rc != 0)
            return strutil_compare_filenames(file1, file2);
      }
      else
            return strutil_compare_filenames(file1, file2);

   rc = wxStat(file2, &statbuf2);
   if(rc != 0)
      if(rc == ENOENT)
      {
         // using all flags but x, will be modified by umask anyway
         int fd = creat(file2.fn_str(), S_IWUSR|S_IRUSR|S_IWGRP|S_IWUSR|S_IWOTH|S_IROTH);
         if(fd == -1)
            return strutil_compare_filenames(file1, file2);
         close(fd); //    created the empty file
         rc = wxStat(file2, &statbuf2);
         unlink(file2.fn_str());
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

String ExpandExternalCommand(String command, const String& parameter)
{
   if ( command.find("%s") == String::npos )
      command += " %s";

   return wxFileType::ExpandCommand(command, parameter);
}

