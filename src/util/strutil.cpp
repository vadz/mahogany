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
#include <wx/fontmap.h>

extern "C"
{
   #include "utf8.h"  // for utf8_text_utf7()

   // arrays used by GuessUnicodeCharset()
   #include "charset/iso_8859.c"
   #include "charset/windows.c"
   #include "charset/koi8_r.c"
}

class MOption;

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_CRYPTALGO;
extern const MOption MP_CRYPT_TESTDATA;
extern const MOption MP_CRYPT_TWOFISH_OK;
extern const MOption MP_MBOXDIR;

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
   str = _T("");
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
   str = _T("");
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
strutil_before(const String &str, const wxChar delim)
{
   String newstr = _T("");
   const wxChar *cptr = str.c_str();
   while(*cptr && *cptr != delim)
      newstr += *cptr++;
   return newstr;
}

String
strutil_after(const String &str, const wxChar delim)
{
   String newstr = _T("");
   const wxChar *cptr = str.c_str();
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
   String newstr = _T("");

   const wxChar *cptr = str.c_str();
   while(wxIsspace(*cptr))
      cptr++;
   while(*cptr)
      newstr += *cptr++;
   str = newstr;
}

void
strutil_toupper(String &str)
{
   String s = _T("");
   const wxChar *cptr = str.c_str();
   while(*cptr)
      s += toupper(*cptr++);
   str = s;
}

void
strutil_tolower(String &str)
{
   String s = _T("");
   const wxChar *cptr = str.c_str();
   while(*cptr)
      s += tolower(*cptr++);
   str = s;
}

bool
strutil_cmp(String const & str1, String const & str2,
      int offs1, int offs2)
{
   return wxStrcmp(str1.c_str()+offs1, str2.c_str()+offs2) == 0;
}

bool
strutil_ncmp(String const &str1, String const &str2, int n, int offs1,
      int offs2)
{
   return wxStrncmp(str1.c_str()+offs1, str2.c_str()+offs2, n) == 0;
}

String
strutil_ltoa(long i)
{
   wxChar buffer[256];   // much longer than any integer
   wxSprintf(buffer, _T("%ld"), i);
   return String(buffer);
}

String
strutil_ultoa(unsigned long i)
{
   wxChar buffer[256];   // much longer than any integer
   wxSprintf(buffer, _T("%lu"), i);
   return String(buffer);
}

#if wxUSE_UNICODE
wxChar *
strutil_strdup(const wxChar *in)
{
   wxChar *cptr = new wxChar[wxStrlen(in)+1];
   wxStrcpy(cptr,in);
   return cptr;
}
#endif

char *
strutil_strdup(const char *in)
{
   char *cptr = new char[strlen(in)+1];
   strcpy(cptr,in);
   return cptr;
}

wxChar *
strutil_strdup(String const &in)
{
   return strutil_strdup(in.c_str());
}

char *
strutil_strsep(char **stringp, const char *delim)
{
#ifdef HAVE_STRSEP
   return strsep(stringp, delim);
#else // !HAVE_STRSEP
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
#endif // HAVE_STRSEP/!HAVE_STRSEP
}

void
strutil_tokenise(char *string, const char *delim, kbStringList &tlist)
{
   char *found;

   for(;;)
   {
      found = strutil_strsep(&string, delim);
      if(! found || ! *found)
         break;
      tlist.push_back(new String(wxConvertMB2WX(found)));
   }
}

String
strutil_extract_formatspec(const wxChar *format)
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
   const wxChar *pLast1 = wxStrrchr(path, '/');
   size_t nPos1 = pLast1 ? pLast1 - path.c_str() : 0;

   // under Windows we understand both '/' and '\\' as path separators, but
   // '\\' doesn't count as path separator under Unix
#ifdef OS_WIN
   const wxChar *pLast2 = wxStrrchr(path, '\\');
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
   for ( const wxChar *p = path.c_str(); *p != '\0'; p++ )
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
      return _T("");

   if(ipath[0u]=='~')
   {
      if(ipath[1u] == DIR_SEPARATOR)
      {
         path = wxGetenv(_T("HOME"));
         path << (ipath.c_str() + 1);
         return path;
      }
      else
      {
         String user =
            strutil_before(String(ipath.c_str()+1), DIR_SEPARATOR);
         struct passwd *entry;
         do
         {
            entry = getpwent();
            if(entry && entry->pw_name == wxConvertWX2MB(user))
               break;
         } while(entry);
         if(entry)
            path << wxConvertMB2WX(entry->pw_dir);
         else
            path << DIR_SEPARATOR << _T("home") << DIR_SEPARATOR << user; // improvise!
         path << DIR_SEPARATOR
              << strutil_after(String(ipath.c_str()+1), DIR_SEPARATOR);
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
strutil_path_parent(String const &path, wxChar separator)
{
   const wxChar *cptr = wxStrrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return _T("");

   return path.Left(cptr - path.c_str());
}

/** Cut off last name from path and return string that (filename).

    @param pathname the path
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_filename(String const &path, wxChar separator)
{
   const wxChar *cptr = wxStrrchr(path.c_str(),separator);
   if(cptr == NULL) // not found
      return _T("");

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
   const wxChar *cptr = in.c_str();
   bool has_cr =  false;

   if(! cptr)
      return _T("");
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
         String to = _T("");
         String tmp;
         for(size_t i = 0; i < len; i++)
         {
            tmp.Printf(_T("%02x"), (int) data[i]);
            to << tmp;
         }
         return to;
      }
   void FromHex(const String &hexdata)
      {
         if(data) free(data);
         len = hexdata.Length() / 2;
         data = (BYTE *)malloc( len );
         const wxChar *cptr = hexdata.c_str();
         String tmp;
         int val, idx = 0;
         while(*cptr)
         {
            tmp = _T("");
            tmp << *cptr << *(cptr+1);
            cptr += 2;
            wxSscanf(tmp.c_str(), _T("%02x"), &val);
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
               strutil_encrypt_tf(_T("TESTDATA")));

         return true;
      }

      if ( strutil_decrypt(testdata) == _T("TESTDATA") )
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
      CryptData input(wxConvertWX2MB(original));
      CryptData output;
      int rc = TwoFishCrypt(1, 128, wxConvertWX2MB(gs_GlobalPassword), &input ,&output);
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

   return _T("");
}

static String
strutil_decrypt_tf(const String &original)
{
   if(! strutil_has_twofish)
   {
      ERRORMESSAGE((_("Strong encryption algorithm not available.")));
      return _T("");
   }
   if ( !setup_twofish() )
   {
      ERRORMESSAGE((_("Impossible to use encryption without password.")));
   }

   CryptData input;
   input.FromHex(original.c_str()+1); // skip initial '-'
   CryptData output;
   int rc = TwoFishCrypt(0, 128, wxConvertWX2MB(gs_GlobalPassword), &input,&output);
   if(rc)
   {
      return output.data;
   }
   return _T("");
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
      gs_GlobalPassword = _T("testPassword");
      String test = _T("This is a test, in cleartext.");
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
            _T("EncryptionAlgoBroken"));
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
      return _T("");

   if(! strutil_encrypt_initialised)
      strutil_encrypt_initialise();

   if ( READ_APPCONFIG(MP_CRYPTALGO) )
      return strutil_encrypt_tf(original);


   String
      tmpstr,
      newstr;
   const wxChar
      *cptr = original.c_str();

   unsigned char pair[2];
   while(*cptr)
   {
      pair[0] = (unsigned char) *cptr;
      pair[1] = (unsigned char) *(cptr+1);
      strutil_encrypt_pair(pair);
      // now we have the encrypted pair, which could be binary data,
      // so we write hex values instead:
      tmpstr.Printf(_T("%02x%02x"), (int)pair[0], (int)pair[1]);
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
                                          strutil_encrypt(_T("TESTDATA")));
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
   bool ok = strutil_decrypt(testdata) == _T("TESTDATA");
   gs_GlobalPassword = oldPassword;

   return ok;
}

String
strutil_decrypt(const String &original)
{
   if(original.Length() == 0)
      return _T("");

   if(! strutil_encrypt_initialised)
      strutil_encrypt_initialise();

   if(original[0] == '-')
      return strutil_decrypt_tf(original);

   // the encrypted string always has a length divisible by 4
   if ( original.length() % 4 )
   {
      wxLogWarning(_("Decrypt function called with illegal string."));

      return _T("");
   }

   String
      tmpstr,
      newstr;
   const wxChar
      *cptr = original.c_str();

   unsigned char pair[2];
   unsigned int i;
   while(*cptr)
   {
      tmpstr = _T("");
      tmpstr << *cptr << *(cptr+1);
      cptr += 2;
      wxSscanf(tmpstr.c_str(), _T("%02x"), &i);
      pair[0] = (unsigned char) i;
      tmpstr = _T("");
      tmpstr << *cptr << *(cptr+1);
      cptr += 2;
      wxSscanf(tmpstr.c_str(), _T("%02x"), &i);
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
      wxChar buffer[256];
      wxStrftime(buffer, 256, format.c_str(), tmvalue);
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
   const wxChar *cptr;
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

   const wxChar *cptr = string.c_str();
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

   for ( const wxChar *cptr = string.c_str(); *cptr; cptr++ )
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

// note that these functions don't work with strings containing NULs: this
// shouldn't be a problem at all in practice however

wxArrayString strutil_restore_array(const String& str, wxChar chSep)
{
   wxArrayString array;
   if ( !str.empty() )
   {
      String s;
      for ( const wxChar *p = str.c_str(); ; p++ )
      {
         if ( *p == _T('\\') )
         {
            // skip the backslash and treat the next character literally,
            // whatever it is -- but take care to not overrun the string end
            const char ch = *++p;
            if ( !ch )
               break;

            s += ch;
         }
         else if ( *p == chSep || *p == _T('\0') )
         {
            array.Add(s);

            if ( *p == _T('\0') )
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

String strutil_flatten_array(const wxArrayString& array, wxChar chSep)
{
   String s;
   s.reserve(1024);

   const size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      if ( n > 0 )
         s += chSep;

      const wxChar *p = array[n].c_str();
      while ( *p )
      {
         const char ch = *p++;

         // escape the separator characters
         if ( ch == chSep || ch == '\\' )
            s += '\\';

         s += ch;
      }
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

// guess the charset of the given Unicode text
static wxFontEncoding GuessUnicodeCharset(const wchar_t *pwz)
{
   typedef const unsigned short *codepage;
   struct CodePageInfo
   {
      codepage cp;
      wxFontEncoding enc;
   };
   static const CodePageInfo s_codepages[] =
   {
      { iso8859_2tab,         wxFONTENCODING_ISO8859_2 },
      { iso8859_3tab,         wxFONTENCODING_ISO8859_3 },
      { iso8859_4tab,         wxFONTENCODING_ISO8859_4 },
      { iso8859_5tab,         wxFONTENCODING_ISO8859_5 },
      { iso8859_6tab,         wxFONTENCODING_ISO8859_6 },
      { iso8859_7tab,         wxFONTENCODING_ISO8859_7 },
      { iso8859_8tab,         wxFONTENCODING_ISO8859_8 },
      { iso8859_9tab,         wxFONTENCODING_ISO8859_9 },
      { iso8859_10tab,        wxFONTENCODING_ISO8859_10 },
      { iso8859_13tab,        wxFONTENCODING_ISO8859_13 },
      { iso8859_14tab,        wxFONTENCODING_ISO8859_14 },
      { iso8859_15tab,        wxFONTENCODING_ISO8859_15 },
      { windows_1250tab,      wxFONTENCODING_CP1250 },
      { windows_1251tab,      wxFONTENCODING_CP1251 },
      { windows_1252tab,      wxFONTENCODING_CP1252 },
      { windows_1253tab,      wxFONTENCODING_CP1253 },
      { windows_1254tab,      wxFONTENCODING_CP1254 },
      { windows_1255tab,      wxFONTENCODING_CP1255 },
      { windows_1256tab,      wxFONTENCODING_CP1256 },
      { windows_1257tab,      wxFONTENCODING_CP1257 },
      { koi8rtab,             wxFONTENCODING_KOI8 },
   };

   // default value: use system default font
   wxFontEncoding enc = wxFONTENCODING_SYSTEM;

   if ( !pwz )
      return enc;

   // first find a non ASCII character as ASCII ones are present in all (well,
   // many) code pages
   while ( *pwz && *pwz < 0x80 )
      pwz++;

   const wchar_t wch = *pwz;

   if ( !wch )
      return enc;

   // build the array of encodings in which the character appears
   wxFontEncoding encodings[WXSIZEOF(s_codepages)];
   size_t numEncodings = 0;

   // special test for iso8859-1 which is identical to first 256 Unicode
   // characters
   if ( wch < 0xff )
   {
      encodings[numEncodings++] = wxFONTENCODING_ISO8859_1;
   }

   for ( size_t nPage = 0; nPage < WXSIZEOF(s_codepages); nPage++ )
   {
      codepage cp = s_codepages[nPage].cp;
      for ( size_t i = 0; i < 0x80; i++ )
      {
         if ( wch == cp[i] )
         {
            ASSERT_MSG( numEncodings < WXSIZEOF(encodings),
                           _T("encodings array index out of bounds") );

            encodings[numEncodings++] = s_codepages[nPage].enc;
            break;
         }
      }
   }

   // now find an encoding which is available on this system
   for ( size_t nEnc = 0; nEnc < numEncodings; nEnc++ )
   {
      if ( wxFontMapper::Get()->IsEncodingAvailable(encodings[nEnc]) )
      {
         enc = encodings[nEnc];
         break;
      }
   }

   return enc;
}

// convert a string in UTF-8 or 7 into the string in some multibyte encoding:
// of course, this doesn't work in general as Unicode is not representable as
// an 8 bit charset but it works in some common cases and is better than no
// UTF-8 support at all
//
// FIXME this won't be needed when full Unicode support is available
wxFontEncoding
ConvertUTFToMB(wxString *strUtf, wxFontEncoding enc)
{
   CHECK( strUtf, wxFONTENCODING_SYSTEM, _T("NULL string in ConvertUTFToMB") );

   if ( !strUtf->empty() )
   {
      // first convert to UTF-8
      if ( enc == wxFONTENCODING_UTF7 )
      {
         // wxWindows does not support UTF-7 yet, so we first convert
         // UTF-7 to UTF-8 using c-client function and then convert
         // UTF-8 to current environment's encoding.
         SIZEDTEXT text7, text8;
         text7.data = (unsigned char *) strUtf->c_str();
         text7.size = strUtf->length();

         utf8_text_utf7 (&text7, &text8);

         strUtf->clear();
         strUtf->reserve(text8.size);
         for ( unsigned long k = 0; k < text8.size; k++ )
         {
            *strUtf << wxChar(text8.data[k]);
         }
      }
      else
      {
         ASSERT_MSG( enc == wxFONTENCODING_UTF8, _T("unknown Unicode encoding") );
      }

      // try to determine which multibyte encoding is best suited for this
      // Unicode string
      wxWCharBuffer wbuf(strUtf->wc_str(wxConvUTF8));
      enc = GuessUnicodeCharset(wbuf);

      // finally convert to multibyte
      wxString str;
      if ( enc == wxFONTENCODING_SYSTEM )
      {
         str = wxString(wbuf);
      }
      else
      {
         wxCSConv conv(enc);
         str = wxString(wbuf, conv);
      }
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
   else // doesn't really matter what we return from here
   {
      enc = wxFONTENCODING_SYSTEM;
   }

   return enc;
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

