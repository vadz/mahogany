/*-*- c++ -*-********************************************************
 * strutil.cc : utility functions for string handling               *
 *                                                                  *
 * (C) 1998,1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

// ============================================================================
// interface
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "Profile.h"
#  include "kbList.h"
#  include "MApplication.h"
#  include "Mcclient.h"
#  include "Mdefaults.h"
#endif // USE_PCH

#include "gui/wxMDialogs.h"
#include "Mpers.h"
#include "MailFolder.h"

#ifdef OS_UNIX
#   include <pwd.h>
#endif

#include <wx/textfile.h>  // just for strutil_enforceNativeCRLF()
#include <wx/regex.h>

extern "C"
{
   #include "utf8.h"  // for utf8_text_utf7()
}

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CRYPTALGO;
extern const MOption MP_CRYPT_TESTDATA;
extern const MOption MP_CRYPT_TWOFISH_OK;
extern const MOption MP_MBOXDIR;
extern const MOption MP_REPLY_SIG_SEPARATOR;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_EXPLAIN_GLOBALPASSWD;

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------

static String strutil_encrypt_tf(const String& cleartext);
static String strutil_decrypt_tf(const String& cleartext);

// ============================================================================
// implementation
// ============================================================================

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

wxChar *
strutil_strdup(const wxChar *in)
{
   wxChar *cptr = new wxChar[strlen(in)+1];
   wxStrcpy(cptr,in);
   return cptr;
}

wxChar *
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

String
strutil_extract_formatspec(const char *format)
{
   // TODO doesn't recognize all possible formats nor backslashes (!)
   String specs;
   while ( *format != '\0' ) {
      if ( *format == '%' ) {
         // skip flags, width and precision which may optionally follow '-'
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
                     FAIL_MSG(_T("unknown size field"));
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
   const char *cptr = strrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return "";

   return path.Left(cptr - path.c_str());
}

/** Cut off last name from path and return string that (filename).

    @param pathname the path
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_filename(String const &path, char separator)
{
   const char *cptr = strrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return "";

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


/** Enforces LF '\n' newline convention.

    @param in string to copy
    @return the UNIXified string
*/
String
strutil_enforceLF(String const &in)
{
   String out;
   size_t cursor = 0;
   
   while ( cursor < in.size() )
   {
      wxChar one = in[cursor];
      if ( one != _T('\r') )
      {
         out += one;
         cursor++;
      }
      else
      {
         out += '\n';
         if ( cursor + 1 < in.size() )
         {
            if ( in[cursor + 1] == '\n' )
               cursor += 2;
            else
               cursor++;
         }
         else
            cursor++;
      }
   }
   
   return out;
}


String
strutil_enforceNativeCRLF(String const &in)
{
   return wxTextFile::Translate(in);
}


/******* TwoFish encryption/decryption of strings *************/

extern "C" {
#include "twofish/aes.h"
};

struct CryptData
{
   size_t len;
   BYTE  *data;

   CryptData()
      {  len = 0; data = NULL; }
   CryptData(const char *str)
      {
         len = strlen(str)+1;
         data = (BYTE *)malloc(len);
         strcpy((char*)data, str);
      }

   ~CryptData()
      { if(data) free(data); }

   String ToHex(void)
      {
         String to = "";
         String tmp;
         for(size_t i = 0; i < len; i++)
         {
            tmp.Printf("%02x", (int) data[i]);
            to << tmp;
         }
         return to;
      }
   void FromHex(const String &hexdata)
      {
         if(data) free(data);
         len = hexdata.Length() / 2;
         data = (BYTE *)malloc( len );
         const char *cptr = hexdata.c_str();
         String tmp;
         int val, idx = 0;
         while(*cptr)
         {
            tmp = "";
            tmp << *cptr << *(cptr+1);
            cptr += 2;
            sscanf(tmp.c_str(),"%02x", &val);
            data[idx++] = (BYTE)val;
         }
      }
};

int TwoFishCrypt(
   int direction, /* 1=encrypt or 0=decrypt */
   int keySize,
   const char *passwd,
   const struct CryptData *data_in,
   struct CryptData *data_out
   )
{
   keyInstance    ki;         /* key information, including tables */
   cipherInstance ci;         /* keeps mode (ECB, CBC) and IV */
   int  i;
   int pwLen, result;
   int blkCount = (data_in->len+1)/(BLOCK_SIZE/8) + 1;
   int byteCnt = (BLOCK_SIZE/8) * blkCount;

   BYTE * input = (BYTE *) calloc(byteCnt,1);
   BYTE * output = (BYTE *) calloc(byteCnt,1);
   memcpy(input, data_in->data, byteCnt);

   if ( !makeKey(&ki,DIR_ENCRYPT,keySize,NULL) )
   {
      free(input);
      free(output);
      return 0;
   }
   if ( !cipherInit(&ci,MODE_ECB,NULL) )
   {
      free(input);
      free(output);
      return 0;
   }

   /* Set key bits from password. */
   pwLen = strlen(passwd);
   for (i=0;i<keySize/32;i++)   /* select key bits */
   {
      ki.key32[i] = (i < pwLen) ? passwd[i] : 0;
      ki.key32[i] ^= passwd[0];
   }
   reKey(&ki);

   /* encrypt the bytes */
   result = direction ? blockEncrypt(&ci, &ki, input, byteCnt*8, output)
                      : blockDecrypt(&ci, &ki, input, byteCnt*8, output);

   if(result == byteCnt*8)
   {
      data_out->data = (BYTE *) malloc(byteCnt);
      memcpy(data_out->data, output, byteCnt);
      data_out->len = byteCnt;
      free(input);
      free(output);
      return 1;
   }
   free(input);
   free(output);
   return 0;
}


static String gs_GlobalPassword;
static bool strutil_has_twofish = false;

// return true if ok, false if no password
static bool
setup_twofish(void)
{
   if ( !gs_GlobalPassword.empty() )
   {
      // already have it
      return true;
   }

   MDialog_Message(
      _("Mahogany uses a global password to protect sensitive\n"
        "information in your configuration files.\n\n"
        "The next dialog will ask you for your global password.\n"
        "If you have not previously chosen one, please do it\n"
        "now, otherwise enter the one you chose previously.\n\n"
        "If you do not want to use a global password, just cancel\n"
        "the next dialog.\n\n"
        "(Tick the box below to never see this message again.)"),
      NULL,
      _("Global Password"),
      GetPersMsgBoxName(M_MSGBOX_EXPLAIN_GLOBALPASSWD));

   bool retry;
   do
   {
      MInputBox(&gs_GlobalPassword,
                _("Global Password:"),
                _("Please enter the global password:"),
                NULL,NULL,NULL,TRUE);

      if ( gs_GlobalPassword.empty() )
      {
         // cancelled, don't insist
         return false;
      }

      // TODO: ask to confirm it?

      wxString testdata = READ_APPCONFIG(MP_CRYPT_TESTDATA);
      if ( testdata.empty() )
      {
         // we hadn't used the global password before
         mApplication->GetProfile()->writeEntry(MP_CRYPT_TESTDATA,
               strutil_encrypt_tf("TESTDATA"));

         return true;
      }

      if ( strutil_decrypt(testdata) == "TESTDATA" )
      {
         // correct password
         return true;
      }

      retry = MDialog_YesNoDialog
              (
               _("The password is wrong.\nDo you want to try again?"),
               NULL,
               MDIALOG_YESNOTITLE,
               M_DLG_YES_DEFAULT
              );

   } while( retry );

   return false;
}

/*
 * All encrypted data is hex numbers. To distinguish between strong
 * and weak data, we prefix the strong encryption with a '-'.
 */
static String
strutil_encrypt_tf(const String &original)
{
   if ( setup_twofish() )
   {
      CryptData input(original);
      CryptData output;
      int rc = TwoFishCrypt(1, 128, gs_GlobalPassword, &input ,&output);
      if(rc)
      {
         String tmp = output.ToHex();
         String tmp2;
         tmp2 << '-' << tmp;
         return tmp2;
      }
   }
   else
   {
      ERRORMESSAGE((_("Impossible to use encryption without password.")));
   }

   return "";
}

static String
strutil_decrypt_tf(const String &original)
{
   if(! strutil_has_twofish)
   {
      ERRORMESSAGE((_("Strong encryption algorithm not available.")));
      return "";
   }
   if ( !setup_twofish() )
   {
      ERRORMESSAGE((_("Impossible to use encryption without password.")));
   }

   CryptData input;
   input.FromHex(original.c_str()+1); // skip initial '-'
   CryptData output;
   int rc = TwoFishCrypt(0,128,gs_GlobalPassword, &input,&output);
   if(rc)
   {
      return output.data;
   }
   return "";
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
   /* initialise built-in weak encryption table: */
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

   // Now test if twofish works alright on this system
   int status = READ_APPCONFIG(MP_CRYPT_TWOFISH_OK);
   if ( status == -1 )
   {
      String oldPassword = gs_GlobalPassword;
      gs_GlobalPassword = "testPassword";
      String test = "This is a test, in cleartext.";
      String cipher = strutil_encrypt_tf(test);
      strutil_has_twofish = TRUE; // assume or it will fail
      String clearagain = strutil_decrypt_tf(cipher);
      if(clearagain != test)
      {
         MDialog_Message(
            _("The secure encryption algorithm included in Mahogany\n"
              "does not work on your system and will be replaced with\n"
              "insecure weak encryption.\n"
              "Please report this as a bug to the Mahogany developers,\n"
              "so that we can fix it."),
            NULL,
            _("Missing feature"),
            "EncryptionAlgoBroken");
         strutil_has_twofish = FALSE;
      }
      else
      {
         strutil_has_twofish = TRUE;
      }

      gs_GlobalPassword = oldPassword;

      mApplication->GetProfile()->
         writeEntry(MP_CRYPT_TWOFISH_OK, (long)strutil_has_twofish);
   }
   else
   {
      strutil_has_twofish = status != 0;
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

   if ( READ_APPCONFIG(MP_CRYPTALGO) )
      return strutil_encrypt_tf(original);


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

void
strutil_setpasswd(const String &newpasswd)
{
   gs_GlobalPassword = newpasswd;
   mApplication->GetProfile()->writeEntry(MP_CRYPT_TESTDATA,
                                          strutil_encrypt("TESTDATA"));
}

String
strutil_getpasswd(void)
{
   return gs_GlobalPassword;
}

bool
strutil_haspasswd(void)
{
   return !READ_APPCONFIG_TEXT(MP_CRYPT_TESTDATA).empty();
}

bool
strutil_checkpasswd(const String& passwd)
{
   wxString testdata = READ_APPCONFIG(MP_CRYPT_TESTDATA);

   CHECK( !testdata.empty(), true, _T("shouldn't be called if no old password") );

   String oldPassword = gs_GlobalPassword;
   gs_GlobalPassword = passwd;
   bool ok = strutil_decrypt(testdata) == "TESTDATA";
   gs_GlobalPassword = oldPassword;

   return ok;
}

String
strutil_decrypt(const String &original)
{
   if(original.Length() == 0)
      return "";

   if(! strutil_encrypt_initialised)
      strutil_encrypt_initialise();

   if(original[0] == '-')
      return strutil_decrypt_tf(original);

   // the encrypted string always has a length divisible by 4
   if ( original.length() % 4 )
   {
      wxLogWarning(_("Decrypt function called with illegal string."));

      return "";
   }

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

/// A small helper function to expand mailfolder names:
String
strutil_expandfoldername(const String &name, MFolderType folderType)
{
   if( folderType != MF_FILE && folderType != MF_MH)
       return name;

   if ( strutil_isabsolutepath(name) )
      return strutil_expandpath(name);

   if ( folderType == MF_FILE )
   {
      String mboxpath = READ_APPCONFIG(MP_MBOXDIR);
      if ( mboxpath.empty() )
         mboxpath = mApplication->GetLocalDir();

      if ( !mboxpath.empty() )
         mboxpath += DIR_SEPARATOR;

      mboxpath += name;

      return strutil_expandpath(mboxpath);
   }
   else // if ( folderType == MF_MH )
   {
      // the name is a misnomer, it is used here just to get MHPATH value
      String mhpath = MailFolder::InitializeMH();
      if ( !mhpath )
      {
         // oops - failed to init MH
         FAIL_MSG(_T("can't construct MH folder full name"));
      }
      else
      {
         mhpath += name;
      }

      return mhpath; // no need to expand, MHPATH should be already expanded
   }
}


String
strutil_ftime(time_t time, const String & format, bool gmtflag)
{
   struct tm *tmvalue = gmtflag ? gmtime(&time) : localtime(&time);

   String strTime;
   if ( tmvalue )
   {
      char buffer[256];
      strftime(buffer, 256, format.c_str(), tmvalue);
      strTime = buffer;
   }
   else // this can happen if the message has no valid date header, don't crash!
   {
      strTime = _("invalid");
   }

   return strTime;
}

/* Read and remove the next number from string. */
long
strutil_readNumber(String& string, bool *success)
{
   strutil_delwhitespace(string);

   String newstr;
   const char *cptr;
   for ( cptr = string.c_str();
         *cptr && (isdigit(*cptr) || *cptr == '+' || *cptr == '-');
         cptr++ )
   {
      newstr << *cptr;
   }

   string.erase(0, cptr - string.c_str());

   long num = -123456;
   bool ok = newstr.ToLong(&num);
   if ( success )
      *success = ok;

   return num;
}

/* Read and remove the next quoted string from string. */
String
strutil_readString(String &string, bool *success)
{
   strutil_delwhitespace(string);

   String newstr;
   newstr.reserve(string.length());

   bool ok;

   const char *cptr = string.c_str();
   if ( *cptr != '"' )
   {
      ok = false;
   }
   else // starts with quote, ok
   {
      for ( cptr++; *cptr && *cptr != '"'; cptr++ )
      {
         if ( cptr[0] == '\\' && cptr[1] == '"' )
         {
            // escaped quote, ignore it
            cptr++;
         }

         newstr << *cptr;
      }

      // string must be terminated with a quote
      ok = *cptr == '"';

      if ( ok )
      {
         string.erase(0, cptr + 1 - string.c_str());
      }
   }

   if ( success )
      *success = ok;

   return newstr;
}

/* Return an escaped string. */
String
strutil_escapeString(const String& string)
{
   String newstr;
   newstr.reserve(string.length());

   for ( const char *cptr = string.c_str(); *cptr; cptr++ )
   {
      if ( *cptr == '"' )
         newstr << '\\';

      newstr << *cptr;
   }

   return newstr;
}

/* **********************************************************
 *  Regular expression matching, using wxRegEx class
 * *********************************************************/

#include <wx/regex.h>

#if wxUSE_REGEX

class strutil_RegEx : public wxRegEx
{
public:
   strutil_RegEx(const String &pattern, int flags)
      : wxRegEx(pattern, flags) { }

   DECLARE_NO_COPY_CLASS(strutil_RegEx)
};


strutil_RegEx *
strutil_compileRegEx(const String &pattern, int flags)
{
   strutil_RegEx * re = new strutil_RegEx(pattern, flags);
   if( !re->IsValid() )
   {
      delete re;
      re = NULL;
   }

   return re;
}

bool
strutil_matchRegEx(const class strutil_RegEx *regex,
                   const String &pattern,
                   int flags)
{
   CHECK( regex, FALSE, _T("NULL regex") );

   return regex->Matches(pattern, flags);
}

void
strutil_freeRegEx(class strutil_RegEx *regex)
{
   delete regex;
}

#else // !wxUSE_REGEX

class strutil_RegEx *
strutil_compileRegEx(const String &pattern, int flags)
{
   ERRORMESSAGE((_("Regular expression matching not implemented.")));
   return NULL;
}

bool
strutil_matchRegEx(const class strutil_RegEx *regex, const String
                   &pattern, int flags)
{
   return FALSE;
}

void
strutil_freeRegEx(class strutil_RegEx *regex)
{
   // nothing
}

#endif // wxUSE_REGEX/!wxUSE_REGEX

// ----------------------------------------------------------------------------
// array <-> string
// ----------------------------------------------------------------------------

wxArrayString strutil_restore_array(const String& str, wxChar ch)
{
   wxArrayString array;
   if ( !str.empty() )
   {
      String s;
      for ( const wxChar *p = str.c_str(); ; p++ )
      {
         if ( *p == ch || *p == '\0' )
         {
            array.Add(s);

            if ( *p == '\0' )
               break;

            s.clear();
         }
         else
         {
            s += *p;
         }
      }
   }

   return array;
}

String strutil_flatten_array(const wxArrayString& array, wxChar ch)
{
   String s;
   size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( n > 0 )
         s += ch;
      s += array[n];
   }

   return s;
}

// this works for any strings, not only addresses: the variable names are just
// left overs
wxArrayString strutil_uniq_array(const wxSortedArrayString& addrSorted)
{
   wxArrayString addresses;
   wxString addr;
   size_t count = addrSorted.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( addrSorted[n] != addr )
      {
         // add the address we had before (if we did) to the list
         if ( !addr.empty() )
         {
            addresses.Add(addr);
         }

         // and remeber it to avoid adding it more than once
         addr = addrSorted[n];
      }
      //else: another copy, just skip
   }

   if ( !addr.empty() )
   {
      // don't forget to add the last one which we don't have yet
      addresses.Add(addr);
   }

   return addresses;
}

/// Check if text is 7bit only:
bool strutil_is7bit(const unsigned char *text)
{
   if ( !text )
      return TRUE;

   while ( *text )
   {
      if ( !isascii(*text++) )
         return FALSE;
   }

   return TRUE;
}

// convert a string in UTF-8 or 7 into the string in the current encoding: of
// course, this doesn't work in general as Unicode is not representable as an 8
// bit charset but it works in some common cases and is better than no UTF-8
// support at all
//
// FIXME this won't be needed when full Unicode support is available
wxFontEncoding
ConvertUnicodeToSystem(wxString *strUtf, wxFontEncoding enc)
{
   CHECK( strUtf, wxFONTENCODING_SYSTEM,
          _T("NULL string in ConvertUnicodeToSystem") );

   if ( !strUtf->empty() )
   {
      if ( enc == wxFONTENCODING_UTF7 )
      {
         // wxWindows does not support UTF-7 yet, so we first convert
         // UTF-7 to UTF-8 using c-client function and then convert
         // UTF-8 to current environment's encoding.
         SIZEDTEXT text7, text8;
         text7.data = (unsigned char *) strUtf->c_str();
         text7.size = strUtf->Length();

         utf8_text_utf7 (&text7, &text8);

         strUtf->clear();
         for ( unsigned long k = 0; k < text8.size; k++ )
         {
            *strUtf << wxChar(text8.data[k]);
         }
      }
      else
      {
         ASSERT_MSG( enc == wxFONTENCODING_UTF8, _T("unknown Unicode encoding") );
      }

      wxString str(strUtf->wc_str(wxConvUTF8), wxConvLocal);
      if ( str.empty() )
      {
         // conversion failed - use original text (and display incorrectly,
         // unfortunately)
         wxLogDebug(_T("conversion from UTF-8 to default encoding failed"));
      }
      else
      {
         *strUtf = str;
      }
   }

#if wxUSE_INTL
   return wxLocale::GetSystemEncoding();
#else // !wxUSE_INTL
   return wxFONTENCODING_ISO8859_1;
#endif // wxUSE_INTL/!wxUSE_INTL
}

// return the length of the line terminator if we're at the end of line or 0
// otherwise
size_t IsEndOfLine(const wxChar *p)
{
   // although the text of the mail message itself has "\r\n" at the end of
   // each line, when we quote the selection only (which we got from the text
   // control) it has just "\n"s, so we should really understand both of them
   if ( p[0] == '\n' )
      return 1;
   else if ( p[0] == '\r' && p[1] == '\n' )
      return 2;

   return 0;
}

DetectSignature::DetectSignature()
{
#if wxUSE_REGEX
   // don't use RE at all by default because the manual code is faster
   m_useRE = false;
#endif
}

// Don't inline it with wxRegEx destructor call
DetectSignature::~DetectSignature() {}

bool DetectSignature::Initialize(Profile *profile)
{
#if wxUSE_REGEX
   String sig = READ_CONFIG(profile, MP_REPLY_SIG_SEPARATOR);

   if ( sig != GetStringDefault(MP_REPLY_SIG_SEPARATOR) )
   {
      // we have no choice but to use the user-supplied RE
      m_reSig.Initialize(new wxRegEx);

      // we implicitly anchor the RE at start/end of line
      //
      // VZ: couldn't we just use wxRE_NEWLINE in Compile() instead of "\r\n"?
      String sigRE;
      sigRE << '^' << sig << _T("\r\n");

      if ( !m_reSig->Compile(sigRE, wxRE_NOSUB) )
      {
         wxLogError(_("Regular expression '%s' used for detecting the "
                      "signature start is invalid, please modify it."),
                    sigRE.c_str());

         return false;
      }
      
      m_useRE = true;
   }
#endif // wxUSE_REGEX
   return true;
}

bool DetectSignature::StartsHere(const wxChar *cptr)
{
   bool isSig = false;
#if wxUSE_REGEX
   if ( m_useRE )
   {
      isSig = m_reSig->Matches(cptr);
   }
   else
#endif // wxUSE_REGEX
   {
      // hard coded detection for standard signature separator "--"
      // and the mailing list trailer "____...___"
      if ( cptr[0] == '-' && cptr[1] == '-' )
      {
         // there may be an optional space after "--" (in fact the
         // space should be there but some people don't put it)
         const wxChar *p = cptr + 2;
         if ( IsEndOfLine(p) || (*p == ' ' && IsEndOfLine(p + 1)) )
         {
            // looks like the start of the sig
            isSig = true;
         }
      }
      else if ( cptr[0] == '_' )
      {
         const wxChar *p = cptr + 1;
         while ( *p == '_' )
            p++;

         // consider that there should be at least 5 underscores...
         if ( IsEndOfLine(p) && p - cptr >= 5 )
         {
            // looks like the mailing list trailer
            isSig = true;
         }
      }
   }
   return isSig;
}
