/*-*- c++ -*-********************************************************
 * MimeList.cc - a Mime handler class                               *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MimeList.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#if       !USE_PCH
  #include <strutil.h>

  #include <string.h>
#endif

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

//IMPLEMENT_CLASS2(MimeList, CommonBase, list<MimeEntry>)
   
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
        : std::list<MimeEntry>()
{
   bool	found;
   MimeEntry	newEntry;
   String	tmp;
   
   PathFinder
      pf(mApplication.readEntry(MC_ETCPATH,MC_ETCPATH_D));

   String file =
      pf.FindFile(mApplication.readEntry(MC_MAILCAP,MC_MAILCAP_D), &found);
   if(! found)
      return;
   ifstream str(file.c_str());
   while(str.good())
   {
      strutil_getstrline(str,tmp);
      strutil_delwhitespace(tmp);
      if(! str.eof())
      {
	 if(newEntry.Parse(tmp))
	    push_back(newEntry);
      }
   }
}

bool
MimeList::GetCommand(String const & type,
		     String &command, String &flags)
{
   iterator	i;
   // look for exact match first:
   for(i = begin(); i != end(); i++)
      if(strutil_cmp((*i).type, type))
      {
	 command = (*i).command;
	 flags  = (*i).flags;
	 return true;
      }
   
   // now look for first type match only:
   String	a,b;
   a = strutil_before(type,'/');

   for(i = begin(); i != end(); i++)
   {
      b = strutil_before((*i).type,'/');
      if(strutil_cmp(a,b) && *strutil_after((*i).type,'/').c_str() == '*')
      {
	 command = (*i).command;
	 flags  = (*i).flags;
	 return true;
      }
   }
   return false;
}

String MimeList::ExpandCommand(String const &commandline,
		     String const &filename,
		     String const &mimetype)
{
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


