/*-*- c++ -*-********************************************************
 * strutil.cc : utility functions for string handling               *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"
#include "Mcommon.h"

#include <ctype.h>
#include <stdio.h>

#ifndef   USE_PCH
#   include "strutil.h"
#   include "kbList.h"
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
   char buffer[256];   // much longer than any integer
   sprintf(buffer,"%ld", i);
   return String(buffer);
}

String
strutil_ultoa(unsigned long i)
{
   char buffer[256];   // much longer than any integer
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

#ifndef   HAVE_STRSEP
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
   return   cptr;
}
#endif // HAVE_STRSEP

void
strutil_tokenise(char *string, const char *delim, kbStringList &tlist)
{
   char *found;

   for(;;)
   {
      found = strsep(&string, delim);
      if(! found || ! *found)
         break;
      tlist.push_back(new String(found));
   }
}

static const char *urlnames[] =
{
   "http:",
   "ftp:",
   "gopher:",
   "wysiwyg:",
   "telnet:",
   "wais:",
   "mailto:",
   "file:",
   NULL
};

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
   const char *cptr = str.c_str();

   url = "";
   while(*cptr)
   {
      for(i = 0; urlnames[i]; i++)
      {
         if(strncmp(cptr,urlnames[i],strlen(urlnames[i])) == 0
               && (i > 0 || ((cptr != str.c_str() && isspace(*(cptr-1)))))
           )
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
strutil_extract_formatspec(const char *format)
{
   // TODO doesn't reckognize all possible formats nor backslashes (!)
   String specs;
   while ( *format != '\0' ) {
      if ( *format == '%' ) {
         // skip flags, width and precision which may optionally follow '%'
         while ( !isalpha(*format) )
            format++;

         // size prefix
         enum SizePrefix
         {
            Size_None,
            Size_Short,
            Size_Long
         } sizePrefix;
         switch ( *format ) {
            case 'h':
               sizePrefix = Size_Short;
               format++;
               break;

            case 'l':
            case 'L':
               sizePrefix = Size_Long;
               format++;
               break;

            default:
               sizePrefix = Size_None;
         }

         // these are all types I know about
         char ch;
         switch ( *format ) {
            case 'c':
            case 'C':
               ch = 'c';
               break;

            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
               switch ( sizePrefix ) {
                  case Size_None:
                     ch = 'd';
                     break;

                  case Size_Short:
                     ch = 'h';
                     break;

                  case Size_Long:
                     ch = 'l';
                     break;

                  default:
                     FAIL_MSG("unknown size field");
               }
               break;

            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
               ch = 'f';
               break;

            case 'n':
            case 'p':
               ch = 'p';
               break;

            case 's':
            case 'S':
               ch = 's';
               break;

            default:
               ch = '\0';
         }

         if ( ch != '\0' ) {
            specs += ch;
         }
      }

      format++;
   }

   return specs;
}

String
strutil_getfilename(const String& path)
{
   const char *pc = path;
   const char *pLast1 = strrchr(pc, '/');
   const char *pLast2 = strrchr(pc, '\\');
   size_t nPos1 = pLast1 ? pLast1 - pc : 0;
   size_t nPos2 = pLast2 ? pLast2 - pc : 0;
   if ( nPos2 > nPos1 )
      nPos1 = nPos2;

   if ( nPos1 == 0 )
      return path;
   else
      return path.Right(path.Len() - nPos1 - 1);
}

