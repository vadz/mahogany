/*-*- c++ -*-********************************************************
 * MimeTypes.cc - a Mime types mapper                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.7  1998/06/05 16:56:12  VZ
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
 * Revision 1.6  1998/05/24 14:47:57  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.5  1998/05/18 17:48:31  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.4  1998/05/13 19:02:10  KB
 * added kbList, adapted MimeTypes for it, more python, new icons
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
#pragma implementation "MimeTypes.h"
#endif

#include  "Mpch.h"
#include  "Mcommon.h"

#ifndef USE_PCH
#   include <strutil.h>
#   include <string.h>

extern "C" {
#   include   <mail.h>
           }

#endif

#include   "MFrame.h"
#include   "MLogFrame.h"

#include   "Mdefaults.h"

#include   "PathFinder.h"
#include   "MimeList.h"
#include   "MimeTypes.h"
#include   "Profile.h"

#include  "MApplication.h"

MimeTEntry::MimeTEntry(void)
{
   type = "";
}

MimeTEntry::MimeTEntry(String const & str)
{
   Parse(str);
}

bool
MimeTEntry::Parse(String const & str)
{
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
   bool   found;
   String   tmp;
   
   PathFinder pf(READ_APPCONFIG(MC_ETCPATH));

   String file = pf.FindFile(READ_APPCONFIG(MC_MIMETYPES), &found);
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
}

bool
MimeTypes::Lookup(String const & filename, String &mimeType, int *numericType)
{
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


