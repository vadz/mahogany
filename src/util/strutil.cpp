/*-*- c++ -*-********************************************************
 * strutil.cc : utility functions for string handling               *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"
#include "Mcommon.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef   USE_PCH
#   include "strutil.h"
#   include "kbList.h"
#endif

#ifdef OS_UNIX
#   include <pwd.h>
#   include <sys/types.h>
#endif

#include <wx/textfile.h>  // just for strutil_enforceNativeCRLF()

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
   "https:",
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
   const char * cptr;


   for(i = 0; urlnames[i]; i++)
      if(strcmp(string,urlnames[i]) >= 0)
         {
            cptr = string + strlen(urlnames[i]);
            if(isspace(*cptr) || *cptr == '\0')
               break;  // "file: " doesn't count!
            cptr = string;
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
   String tmp;

   url = "";
   while(*cptr)
   {
      for(i = 0; urlnames[i]; i++)
      {
         if(strncmp(cptr,urlnames[i],strlen(urlnames[i])) == 0
            && strutil_isurlchar(cptr[strlen(urlnames[i])])
            && ((cptr == str.c_str() || !strutil_isurlchar(*(cptr-1))))
           )
         {
            while(strutil_isurlchar(*cptr))
               url += *cptr++;
            tmp = cptr; // cannot assign directly as cptr points into
            str = tmp;  // str, so using a temporary string in between
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
   // TODO doesn't recognize all possible formats nor backslashes (!)
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

         // keep the compiler happy by initializing the var in any case
         char ch = '\0';
         switch ( *format ) {
            // these are all types I know about
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
         }

         if ( ch != '\0' ) {
            specs += ch;
         }
      }

      format++;
   }

   return specs;
}

// return the basename of a file (i.e. the part after final path separator)
String
strutil_getfilename(const String& path)
{
   const char *pLast1 = strrchr(path, '/');
   size_t nPos1 = pLast1 ? pLast1 - path.c_str() : 0;

   // under Windows we understand both '/' and '\\' as path separators, but
   // '\\' doesn't count as path separator under Unix
#ifdef OS_WIN
   const char *pLast2 = strrchr(path, '\\');
   size_t nPos2 = pLast2 ? pLast2 - path.c_str() : 0;
   if ( nPos2 > nPos1 )
      nPos1 = nPos2;
#endif // Win

   if ( nPos1 == 0 )
      return path;
   else
      return path.c_str() + nPos1 + 1;
}

bool
strutil_isabsolutepath(const String &path)
{
#ifdef OS_UNIX
   return !strutil_isempty(path) && (path[(size_t)0] == DIR_SEPARATOR ||
                                     path[(size_t)0] == '~');
#elif defined ( OS_WIN )
   // TODO copy the code from wxIsAbsolutePath() here if Karsten insists on it
   return wxIsAbsolutePath(path);
#endif
}

static void
strutil_squeeze_slashes(String& path)
{
   String result;
   for ( const char *p = path.c_str(); *p != '\0'; p++ )
   {
      if ( *p == '/' )
      {
         while ( *p++ == '/' )
            ;

         // we are one position too far - return to the last '/'
         p--;
      }

      result += *p;
   }

   path = result;
}

// TODO it doesn't work properly: should take into account the symbolic names
//      under Unix and drive letters under Windows. Also, relative paths are not
//      supported at all
bool
strutil_compare_filenames(const String& path1, const String& path2)
{
   String file1(path1), file2(path2);

   // replace all '\\' with '/' under Windows - they're the same
#ifdef OS_WIN
   file1.Replace("\\", "/");
   file2.Replace("\\", "/");
#endif // Windows

   file1 = strutil_expandpath(file1);
   file2 = strutil_expandpath(file2);

   // replace all repeating '/' with only one
   strutil_squeeze_slashes(file1);
   strutil_squeeze_slashes(file2);

   return file1.IsSameAs(file2, FALSE /* no case */);
}

String
strutil_expandpath(const String &ipath)
{
#ifdef OS_UNIX
   String path;

   if(strutil_isempty(ipath))
      return "";

   if(ipath[0u]=='~')
   {
      if(ipath[1u] == DIR_SEPARATOR)
      {
         path = getenv("HOME");
         path << (ipath.c_str() + 1);
         return path;
      }
      else
      {
         String user =
            strutil_before(String(ipath.c_str()+1),DIR_SEPARATOR);
         struct passwd *entry;
         do
         {
            entry = getpwent();
            if(entry && entry->pw_name == user)
               break;
         } while(entry);
         if(entry)
            path << entry->pw_dir;
         else
            path << DIR_SEPARATOR << "home" << DIR_SEPARATOR << user; // improvise!
         path << DIR_SEPARATOR
              << strutil_after(String(ipath.c_str()+1),DIR_SEPARATOR);
         return path;
      }
   }
   else
#endif // Unix
      return ipath;
}

/** Cut off last directory from path and return string before that.

    @param pathname the path in which to go up
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_parent(String const &path, char separator)
{
   char *cptr = strrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return "";
   else
      return path.Left((int)(cptr-path.c_str()));
}

/** Cut off last name from path and return string that (filename).

    @param pathname the path
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_filename(String const &path, char separator)
{
   char *cptr = strrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return "";
   else
      return String(cptr+1);
}



/** Enforces CR/LF newline convention.

    @param in string to copy
    @return the DOSified string
*/
String
strutil_enforceCRLF(String const &in)
{
   String out;
   const char *cptr = in.c_str();
   bool has_cr =  false;

   if(! cptr)
      return "";
   while(*cptr)
   {
      switch(*cptr)
      {
      case '\r':
         has_cr = true;
         out += '\r';
         break;
      case '\n':
         if(! has_cr)
            out += '\r';
         out += '\n';
         has_cr = false;
         break;
      default:
         out += *cptr;
         has_cr = false;
         break;
      }
      cptr++;
   }
   return out;
}


String
strutil_enforceNativeCRLF(String const &in)
{
   return wxTextFile::Translate(in);
}

/* This is not strictly a string utility function, but somehow it is,
   so I don't think we need a separate source file for it just yet.
   This is not a safe encryption system!
*/

#define STRUTIL_ENCRYPT_MIX   1000
#define STRUTIL_ENCRYPT_DELTA 289  // 17*17

static unsigned char strutil_encrypt_table[256];
static bool strutil_encrypt_initialised = false;

static void
strutil_encrypt_initialise(void)
{
   for(int c = 0; c < 256; c++)
      strutil_encrypt_table[c] = (unsigned char )c;

   unsigned char
      tmp;
   int
      a = 0,
      b = STRUTIL_ENCRYPT_DELTA % 256;
   for(int i = 0 ; i < STRUTIL_ENCRYPT_MIX ; i++)
   {
      tmp = strutil_encrypt_table[a];
      strutil_encrypt_table[a] = strutil_encrypt_table[b];
      strutil_encrypt_table[b] = tmp;
      a += STRUTIL_ENCRYPT_DELTA;
      b += STRUTIL_ENCRYPT_DELTA;
      a %= 256;
      b %= 256;
   }
   strutil_encrypt_initialised = true;
}

static void
strutil_encrypt_pair(unsigned char pair[2])
{
   int
      a,b;
   for(a = 0; strutil_encrypt_table[a] != pair[0]; a++)
      ;
   for(b = 0; strutil_encrypt_table[b] != pair[1]; b++)
      ;
   int r1, r2, c1, c2;
   r1 = a / 16; r2 = b / 16;
   c1 = a % 16; c2 = b % 16;
   pair[0] = strutil_encrypt_table[(r2<<4) + c1];
   pair[1] = strutil_encrypt_table[(r1<<4) + c2];
}

String
strutil_encrypt(const String &original)
{
   if(original.Length() == 0)
      return "";

   if(! strutil_encrypt_initialised)
      strutil_encrypt_initialise();

   String
      tmpstr,
      newstr;
   const char
      *cptr = original.c_str();

   unsigned char pair[2];
   while(*cptr)
   {
      pair[0] = (unsigned char) *cptr;
      pair[1] = (unsigned char) *(cptr+1);
      strutil_encrypt_pair(pair);
      // now we have the encrypted pair, which could be binary data,
      // so we write hex values instead:
      tmpstr.Printf("%02x%02x", pair[0], pair[1]);
      newstr << tmpstr;
      cptr ++;
      if(*cptr) cptr++;
   }
   return newstr;
}

String
strutil_decrypt(const String &original)
{
   if(original.Length() == 0)
      return "";
   CHECK(original.Length() % 4 == 0, "", "Decrypt function called with illegal string.");

   if(! strutil_encrypt_initialised)
      strutil_encrypt_initialise();

   String
      tmpstr,
      newstr;
   const char
      *cptr = original.c_str();

   unsigned char pair[2];
   unsigned int i;
   while(*cptr)
   {
      tmpstr = "";
      tmpstr << *cptr << *(cptr+1);
      cptr += 2;
      sscanf(tmpstr.c_str(), "%02x", &i);
      pair[0] = (unsigned char) i;
      tmpstr = "";
      tmpstr << *cptr << *(cptr+1);
      cptr += 2;
      sscanf(tmpstr.c_str(), "%02x", &i);
      pair[1] = (unsigned char) i;
      strutil_encrypt_pair(pair);
      newstr << (char) pair[0] << (char) pair[1];
   }
   return newstr;
}


//************************************************************************
//        Profile and other classes dependent functions:
//************************************************************************

#include   "Profile.h"
#include   "Mdefaults.h"
#include   "MApplication.h"
#include   "MailFolderCC.h"   // for InitializeMH

/// A small helper function to expand mailfolder names:
String
strutil_expandfoldername(const String &name, FolderType folderType)
{
   if( folderType != MF_FILE && folderType != MF_MH)
       return name;

   if( strutil_isabsolutepath(name) )
      return strutil_expandpath(name);

   if ( folderType == MF_FILE )
   {
      String mboxpath;
      mboxpath << READ_APPCONFIG(MP_MBOXDIR) << DIR_SEPARATOR << name;

      return strutil_expandpath(mboxpath);
   }
   else // if ( folderType == MF_MH )
   {
      // the name is a misnomer, it is used here just to get MHPATH value
      String mhpath = MailFolderCC::InitializeMH();
      if ( !mhpath )
      {
         // oops - failed to init MH
         FAIL_MSG("can't construct MH folder full name");
      }
      else
      {
         if ( !wxEndsWithPathSeparator(mhpath) )
            mhpath += DIR_SEPARATOR;

         mhpath += name;
      }

      return mhpath; // no need to expand, MHPATH should be already expanded
   }
}


String
strutil_ftime(time_t time, const String & format, bool gmtflag)
{
   struct tm *tmvalue = gmtflag ? gmtime(&time) : localtime(&time);

   char buffer[256];
   strftime(buffer, 256, format.c_str(), tmvalue);
   return String(buffer);
}


String
strutil_removeReplyPrefix(const String &isubject)
{
   /* Removes Re: Re[n]: Re(n): and the local translation of
      it. */
   String subject = isubject;
   strutil_delwhitespace(subject);

   String trPrefix = _("Re");

   size_t idx = 0;

   if(subject.Length() > strlen("Re")
      && Stricmp(subject.Left(strlen("Re")), "Re") == 0)
      idx = strlen("Re");
   else if(subject.Length() > trPrefix.Length()
           && Stricmp(subject.Left(trPrefix.Length()), trPrefix) == 0)
      idx = trPrefix.Length();

   if(idx == 0)
      return subject;

   char needs = '\0';
   while(idx < subject.Length())
   {
      if(subject[idx] == '(')
      {
         if(needs == '\0')
            needs = ')';
         else
            return subject; // syntax error
      }
      else if(subject[idx] == '[')
      {
         if(needs == '\0')
            needs = ']';
         else
            return subject; // syntax error
      }
      else if(subject[idx] == ':' && needs == '\0')
         break; //done
      else if(! isdigit(subject[idx]))
      {
         if(subject[idx] != needs)
            return subject; // syntax error
         else
            needs = '\0';
      }
      idx++;
   }
   subject = subject.Mid(idx+1);
   strutil_delwhitespace(subject);
   return subject;
}
/* Read and remove the next number from string. */
long
strutil_readNumber(String &string, bool *success)
{
   strutil_delwhitespace(string);
   String newstr;
   const char *cptr;
   for(cptr = string.c_str(); *cptr && isdigit(*cptr);
       cptr++)
      newstr << *cptr;
   string = cptr;
   long num = -123456;
   int count = sscanf(newstr.c_str(),"%ld", &num);
   if(success)
      *success = (count == 1);
   return num;
}

/* Read and remove the next quoted string from string. */
String
strutil_readString(String &string, bool *success)
{
   strutil_delwhitespace(string);
   const char *cptr = string.c_str();
   if(*cptr != '"')
   {
      if(success)
         *success = false;
      return "";
   }
   else
      cptr++;
   
   String newstr;
   bool escaped = false;
   for(; *cptr && (*cptr != '"' || escaped);
       cptr++)
   {
      if(*cptr == '\\' && ! escaped)
      {
         escaped = true;
         continue;
      }
      newstr << *cptr;
   }
   if(*cptr == '"')
   {
      if(success)
         *success = false;
      cptr++;
   }
   else
      if(success)
         *success = false;
   string = cptr;
   return newstr;
}

/* Return an escaped string. */
String
strutil_escapeString(const String &string)
{
   String newstr;
   for(const char *cptr = string.c_str();
       *cptr; cptr++)
   {
      if(*cptr == '\\' || *cptr == '"')
         newstr << '\\';
      newstr << *cptr;
   }
   return newstr;
}
