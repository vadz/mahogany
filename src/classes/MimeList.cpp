/*-*- c++ -*-********************************************************
 * MimeList.cc - a Mime handler class                               *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$              *
 ********************************************************************
 * $Log$
 * Revision 1.7  1998/06/05 16:56:11  VZ
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.6  1998/05/24 14:47:56  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.5  1998/05/18 17:48:30  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.4  1998/05/11 20:57:27  VZ
 * compiles again under Windows + new compile option USE_WXCONFIG
 *
 * Revision 1.3  1998/04/22 19:55:49  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.2  1998/03/26 23:05:39  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#   pragma implementation "MimeList.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#ifndef   USE_PCH
#   include   <string.h>
#   include   "strutil.h"
#   include   "Mdefaults.h"
#   include   "Profile.h"
#   include   "MApplication.h"
#endif

#include   "PathFinder.h"
#include   "MimeList.h"


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
   : MimeEntryList(true) // own entries
{
   bool   found;
   MimeEntry   *newEntry;
   String   tmp;
   
   PathFinder pf(READ_APPCONFIG(MC_ETCPATH));

   String file = pf.FindFile(READ_APPCONFIG(MC_MAILCAP), &found);
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
}

bool
MimeList::GetCommand(String const & type,
                     String &command, String &flags)
{
   iterator
      i;
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


