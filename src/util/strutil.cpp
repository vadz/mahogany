/*-*- c++ -*-********************************************************
 * strutil.cc : utility functions for string handling               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"
#include "Mcommon.h"

#include <ctype.h>
#include <stdio.h>

#ifndef   USE_PCH
#   include "strutil.h"
#endif

void
strutil_getstrline(istream &istr, String &str)
{
   char ch;
   str = "";
   for(;;)
   {
      istr.get(ch);
      if(istr.fail() || ch == '\n' )
	 break;
      str += ch;
   }
}

void
strutil_getfoldedline(istream &istr, String &str)
{
   char ch;
   str = ""; 
   for(;;)
   {
      istr.get(ch);
      if(istr.fail())
	 break;
      if( ch == '\n' )
      {
	 ch = istr.peek();
	 if( ch != ' ' && ch != '\t' ) /* not folded */
	    break;
	 else
	 {
	    istr.get(ch);
	    str += '\n';
	 }
      }
      str += ch;
   }
}

String
strutil_before(const String &str, const char delim)
{
   String newstr = "";
   const char *cptr = str.c_str();
   while(*cptr && *cptr != delim)
      newstr += *cptr++;
   return newstr;
}

String
strutil_after(const String &str, const char delim)
{
   String newstr = "";
   const char *cptr = str.c_str();
   while(*cptr && *cptr != delim)
      cptr++;
   if(*cptr)
   {
      while(*++cptr)
	 newstr += *cptr;
   }
   return newstr;
}

void
strutil_delwhitespace(String &str)
{
   String newstr = "";

   const char *cptr = str.c_str();
   while(isspace(*cptr))
      cptr++;
   while(*cptr)
      newstr += *cptr++;
   str = newstr;
}

void
strutil_toupper(String &str)
{
   String s = "";
   const char *cptr = str.c_str();
   while(*cptr)
      s += toupper(*cptr++);
   str = s;
}

void
strutil_tolower(String &str)
{
   String s = "";
   const char *cptr = str.c_str();
   while(*cptr)
      s += tolower(*cptr++);
   str = s;
}

bool
strutil_cmp(String const & str1, String const & str2,
	    int offs1, int offs2)
{   
   return strcmp(str1.c_str()+offs1, str2.c_str()+offs2) == 0;
}

bool
strutil_ncmp(String const &str1, String const &str2, int n, int offs1,
	     int offs2)
{
   return strncmp(str1.c_str()+offs1, str2.c_str()+offs2, n) == 0;
}

String
strutil_ltoa(long i)
{
   char buffer[256];	// much longer than any integer
   sprintf(buffer,"%ld", i);
   return String(buffer);
}

String
strutil_ultoa(unsigned long i)
{
   char buffer[256];	// much longer than any integer
   sprintf(buffer,"%lu", i);
   return String(buffer);
}

char *
strutil_strdup(const char *in)
{
   char *cptr = new char[strlen(in)+1];
   strcpy(cptr,in);
   return cptr;
}

char *
strutil_strdup(String const &in)
{
    return strutil_strdup(in.c_str());
}

// split string into name=value;name=value pairs
/*
void
strutil_splitlist(String const &str, std::map<String,String> &table)
{
   char *tmp = strutil_strdup(str);
   char *name, *value, *ptr, *trail;
   char	quoted;
   bool	escaped = false;

   ptr = tmp;

   do
   {
      while(*ptr && (*ptr == ' ' || *ptr == '\t'))
	 ptr++;
      name = ptr;
      while(*ptr && *ptr != '=')
	 ptr++;
      if(*ptr == '\0')
	 goto end;
      *ptr = '\0';
      trail = ptr-1;
      while(*trail == ' ' || *trail == '\t')
	 *trail-- = '\0';	// delete trailing whitespace in name
      
      ptr++;
      value = ptr;
      while(*value && *value == ' ' || *value == '\t')
	 value++;
      if(*value == '\'' || *value == '"')
      {
	 quoted = *value;
	 value++;
      }
      else
	 quoted = '\0';
      ptr = value;
      while(*ptr)
      {
	 if(!escaped && *ptr == '\\')
	 {
	    escaped = true;
	    ptr++;
	 }
	 if(*ptr == '\0')
	    break;
	 if(!escaped && quoted && *ptr == quoted)
	 {
	    *ptr = '\0'; // terminated by quote
	    ptr++;
	    // eat up to and including next semicolon
	    while(*ptr && *ptr != ';')
	       ptr++;
	    ptr++;
	    break;
	 }
	 if(! quoted && !escaped && *ptr == ';')
	 {
	    trail = ptr;
	    *ptr = '\0';
	    ptr++;
	    trail--;
	    while(*trail == ' ' || *trail == '\t')
	       *trail-- ='\0';	// remove trailing whitespace
	    break;
	 }
	 escaped = false;
	 ptr++;
      }
      if(*name && *value)
      {
	 table[name] = value;
      }
   }while(*ptr);
   
   end:
   delete [] tmp;
}
*/

#ifndef	HAVE_STRSEP
char *
strsep(char **stringp, const char *delim)
{
   char
      * cptr = *stringp,
      * nextdelim = strpbrk(*stringp, delim);

   if(**stringp == '\0')
      return NULL;
 
   if(nextdelim == NULL)
   {
      *stringp = *stringp + strlen(*stringp);// put in on the \0
      return cptr;
   }
   if(*nextdelim != '\0')
   {
      *nextdelim = '\0';
      *stringp = nextdelim + 1;
   }
   else
      *stringp = nextdelim;
   return	cptr;
}
#endif

void
strutil_tokenise(char *string, const char *delim, kbStringList &tlist)
{
   char *found;
   
   for(;;)
   {
      found = strsep(&string, delim);
      if(! found)
	 break;
      tlist.push_back(new String(found));
   }
}

static const char *urlnames[] =
{ "http:", "ftp:", "gopher:", "wysiwyg:", "telnet:", "wais:",
  "mailto:", "file:",NULL };

String
strutil_matchurl(const char *string)
{
   int i;
   String url = "";
   const char * cptr = string;


   for(i = 0; urlnames[i]; i++)
      if(strcmp(string,urlnames[i]) >= 0) // found
      {
         while(*cptr && ! isspace(*cptr))
         {
            url += *cptr;
            cptr++;
         }
         return url;
      }
   return String("");
}

String
strutil_findurl(String &str, String &url)
{
   int i;
   String before = "";
   const char *cptr = str.c_str();;

   url = "";
   while(*cptr)
   {
      for(i = 0; urlnames[i]; i++)
      {
         if(strncmp(cptr,urlnames[i],strlen(urlnames[i])) == 0)
         {
            while(strutil_isurlchar(*cptr))
               url += *cptr++;
            str = cptr;
            return before;
         }
      }
      before += *cptr++;
   }
   str = cptr;
   return before;
}

String
strutil_getfilename(const String& path)
{
  const char *pc = path;
  const char *pLast1 = strrchr(pc, '/');
  const char *pLast2 = strrchr(pc, '\\');
  uint nPos1 = pLast1 ? pLast1 - pc : 0;
  uint nPos2 = pLast2 ? pLast2 - pc : 0;
  if ( nPos2 > nPos1 )
    nPos1 = nPos2;

  return path.Right(path.Len() - nPos1 - 1);
}