/*-*- c++ -*-********************************************************
 * MimeTypes.cc - a Mime types mapper                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MimeTypes.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#ifndef USE_PCH
#  include <strutil.h>
#  include <string.h>

   extern "C"
   {
#     include   <mail.h>
   }

#  include "Mdefaults.h"
#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"
#  include "MFrame.h"
#  include "MApplication.h"
#endif  // USE_PCH

MimeTEntry::MimeTEntry(void)
{
   type = "";
}

MimeTEntry::MimeTEntry(String const & str)
{
   MOcheck();
   Parse(str);
}

bool
MimeTEntry::Parse(String const & str)
{
   MOcheck();

   if(*str.c_str() == '#')
      return false;
   const char *cptr = str.c_str();
   String   entry;

   type = "";
   
   while(*cptr == ' ' || *cptr == '\t' && *cptr)
      cptr++;

   while(*cptr &&
         *cptr != ' '  && *cptr != '\t' &&
         *cptr != '\n' && *cptr != '\r')
      type += *cptr++;
   strutil_toupper(type);
   
   while(*cptr == ' ' || *cptr == '\t' && *cptr)
      cptr++;
   
   while(*cptr)
   {
      entry = "";
      while(*cptr != ' ' && *cptr != '\n' && *cptr && *cptr != '\n' &&
            *cptr != '\r')
         entry += *cptr++;
      strutil_toupper(entry);
      extensions.push_back(new String(entry));
      while(*cptr == ' ' || *cptr == '\t' && *cptr)
         cptr++;
   }
   return true;
}

bool
MimeTEntry::Match(String const & extension, String &mimeType)
{
   MOcheck();
   
   kbStringList::iterator i;
   for(i = extensions.begin(); i != extensions.end(); i++)
      if( *(*i) == extension)
      {
         mimeType = type;
         return true;
      }
   return false;
}

MimeTypes::MimeTypes(void) // why? should be default : STL_LIST<MimeTEntry>()
{
   MOcheck();
   
#  ifdef OS_UNIX
      bool   found;
      String   tmp;
   
      PathFinder pf(READ_APPCONFIG(MP_ETCPATH));

      String file = pf.FindFile(READ_APPCONFIG(MP_MIMETYPES), &found);
      if(! found)
         return;
      ifstream str(file.c_str());
      while(str.good())
      {
         strutil_getstrline(str,tmp);
         strutil_delwhitespace(tmp);
         if(! str.eof())
         {
            push_back(new MimeTEntry(tmp));  // this will add an empty
                                             // entry as the last one
         }
      }
#  else  // Windows
      // TODO: read the Mime types from the registry
#  endif // Unix
}

bool
MimeTypes::Lookup(String const & filename, String &mimeType, int *numericType)
{
   MOcheck();

   const char *ext = strrchr(filename.c_str(),'.');
   String extension;
   iterator i;
   
   if(ext)
      extension = ext+1;

   strutil_toupper(extension);
   for(i = begin(); i != end() ; i++)
      if((*i)->Match(extension, mimeType))
      {
         if(numericType)
         {
            if(strncmp("VIDEO", mimeType.c_str(), 5) == 0)
               *numericType = TYPEVIDEO;
            else if(strncmp("AUDIO", mimeType.c_str(), 5) == 0)
               *numericType = TYPEAUDIO;
            else if(strncmp("IMAGE", mimeType.c_str(), 5) == 0)
               *numericType = TYPEIMAGE;
            else if(strncmp("TEXT", mimeType.c_str(), 4) == 0)
               *numericType = TYPETEXT;
            else if(strncmp("MESSAGE", mimeType.c_str(), 7) == 0)
               *numericType = TYPEMESSAGE;
            else if(strncmp("APPLICATION", mimeType.c_str(), 11) == 0)
               *numericType = TYPEAPPLICATION;
            else if(strncmp("MODEL", mimeType.c_str(), 5) == 0)
               *numericType = TYPEMODEL;
            else
               *numericType = TYPEOTHER;
         }
         return true;
      }
   return false;
}

MimeTypes::~MimeTypes()
{
}


