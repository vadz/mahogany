/*-*- c++ -*-********************************************************
 * MimeList.cc - a Mime handler class                               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MimeList.h"
#endif

#include  "Mpch.h"

#ifndef   USE_PCH
#  include  "Mcommon.h"
#  include  "Mdefaults.h"
#  include  "strutil.h"
#  include  "Profile.h"
#  include  "MApplication.h"
#endif

#include   "PathFinder.h"
#include   "MimeList.h"

MimeEntry::MimeEntry(void)
{
   type = "";
   command = MIME_UNKNOWN_COMMAND;
   flags = MIME_DEFAULT_FLAGS;
}

MimeEntry::MimeEntry(String const & itype,
                     String const & icommand,
                     String const & iflags)
{
   type = itype;
   command = icommand;
   flags = iflags;
}

bool
MimeEntry::Parse(String const & str)
{
   MOcheck();
   if(*str.c_str() == '#')
      return false;

   char *cptr = new char[str.length()+1];
   char *token;

   strcpy(cptr, str.c_str());

   token = strtok(cptr, ";");
   if(! token) goto bailout;
   type = token;

   token = strtok(NULL, ";");
   if(! token) goto bailout;
   command = token;
   strutil_delwhitespace(command);

   token = strtok(NULL, ";");
   if(! token)
      flags = "";
   else
      flags = token;
   strutil_delwhitespace(flags);

   strutil_toupper(type);
   return true;
 bailout:
   delete [] cptr;
   return false;
}


MimeList::MimeList(void)
{
   MOcheck();
#  ifdef OS_UNIX
      bool   found;
      MimeEntry   *newEntry;
      String   tmp;

      PathFinder pf(READ_APPCONFIG(MP_ETCPATH));

      String file = pf.FindFile(READ_APPCONFIG(MP_MAILCAP), &found);
      if(! found)
         return;
      ifstream str(file.c_str());
      while(str.good())
      {
         strutil_getstrline(str,tmp);
         strutil_delwhitespace(tmp);
         if(! str.eof())
         {
            newEntry = new MimeEntry;
            if(newEntry->Parse(tmp))
               push_back(newEntry);
            else
               delete newEntry;
         }
      }
#  else  // Windows
      // TODO: read the Mime types from the registry
#  endif // Unix
}

bool
MimeList::GetCommand(String const & type,
                     String &command, String &flags)
{
   MOcheck();

   iterator i;
   // look for exact match first:
   for(i = begin(); i != end(); i++)
      if(strutil_cmp((*i)->type, type))
      {
         command = (*i)->command;
         flags  = (*i)->flags;
         return true;
      }

   // now look for first type match only:
   String   a,b;
   a = strutil_before(type,'/');

   for(i = begin(); i != end(); i++)
   {
      b = strutil_before((*i)->type,'/');
      if(strutil_cmp(a,b) && *strutil_after((*i)->type,'/').c_str() == '*')
      {
         command = (*i)->command;
         flags  = (*i)->flags;
         return true;
      }
   }
   return false;
}

String MimeList::ExpandCommand(String const &commandline,
                               String const &filename,
                               String const &mimetype)
{
   MOcheck();

   String line = "";
   const char *cptr = commandline.c_str();

   while(*cptr)
   {
      while(*cptr && *cptr != '%')
         line += *cptr++;
      if(*cptr == '\0')
         break;
      cptr++;
      if(*cptr == 's') // insert file name
      {
         line += '"';
         line += filename;
         line += '"';
         cptr++;
      }
      else if(*cptr == 't')
      {
         line += '"';
         line += mimetype;
         line += '"';
         cptr++;
      }
   }
   return line;
}


MimeList::~MimeList()
{
}


