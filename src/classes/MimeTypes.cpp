/*-*- c++ -*-********************************************************
 * MimeTypes.cc - a Mime types mapper                               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
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

#if       !USE_PCH
  #include <strutil.h>

  #include <string.h>

  extern "C" {
    #include	<mail.h>
	}
#endif

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

MimeTEntry::MimeTEntry(void)
{
   type = "";
}

bool
MimeTEntry::Parse(String const & str)
{
   if(*str.c_str() == '#')
      return false;
   const char *cptr = str.c_str();
   String	entry;

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
      extensions.push_back(entry);
      while(*cptr == ' ' || *cptr == '\t' && *cptr)
	 cptr++;
   }
   return true;
}

bool
MimeTEntry::Match(String const & extension, String &mimeType)
{
   std::list<String>::iterator i;
   for(i = extensions.begin(); i != extensions.end(); i++)
      if((*i) == extension)
      {
	 mimeType = type;
	 return true;
      }
   return false;
}

MimeTypes::MimeTypes(void) // why? should be default : STL_LIST<MimeTEntry>()
{
   bool	found;
   MimeTEntry	newEntry;
   String	tmp;
   
   PathFinder
      pf(mApplication.readEntry(MC_ETCPATH,MC_ETCPATH_D));

   String file =
      pf.FindFile(mApplication.readEntry(MC_MIMETYPES,MC_MIMETYPES_D), &found);
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
MimeTypes::Lookup(String const & filename, String &mimeType, int *numericType)
{
   const char *ext = strrchr(filename.c_str(),'.');
   String extension;
   iterator i;

   if(ext)
      extension = ext+1;

   strutil_toupper(extension);
   for(i = begin(); i != end() ; i++)
      if((*i).Match(extension, mimeType))
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


