/*-*- c++ -*-********************************************************
 * strutil.h : utility functions for string handling                *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef STRUTIL_H
#define STRUTIL_H

#ifndef  USE_PCH
#  include  "Mconfig.h"
#endif

#include <time.h>

#include "FolderType.h"    // for strutil_expandfoldername

class kbStringList;

/**@name String class helper functions */
//@{


#ifdef USE_WXSTRING // use std::string
   inline bool strutil_isempty(const String &s) { return IsEmpty(s); }
#else
   inline bool strutil_isempty(const String &s) { return *s.c_str() == '\0'; }
#endif

/// return true if string is empty
inline bool strutil_isempty(const char *s) { return s == NULL || *s == '\0'; }

/** Read a NL terminated line into a string.

    @param istr reference to an input stream
    @param str reference to the string to write to
*/
void strutil_getstrline(istream &istr, String &str);

/** Read a folded string into a string variable.

    A folded string is one which can contain newline characters if
    they are followed by whitespace.
    @param istr reference to an input stream
    @param str reference to the string to write to
*/
void strutil_getfoldedline(istream &istr, String &str);

/** Get the part of a string before the delimiter.

    @param str reference to the string to split
    @param delim the delimiter character
*/
String strutil_before(const String &str, const char delim);

/** Get the part of a string after the delimiter.

    @param str reference to the string to split
    @param delim the delimiter character
*/
String strutil_after(const String &str, const char delim);

/** Delete whitespace from the beginning of a string.

    @param str the string to operate on
*/
void strutil_delwhitespace(String &str);

/** Convert string to upper case.

    @param str string to change
*/
void strutil_toupper(String &str);

/** Convert string to lower case.
    @param str string to change
*/
void strutil_tolower(String &str);



/** Compare two strings, starting at an offset each.

    @param str1 the first string
    @param offs1 at which position to start in string 1
    @param str2 the second string
    @param offs2 at which position to start in string 2
    @return true if they are identical
*/
bool strutil_cmp(String const & str1, String const & str2,
                 int offs1 = 0, int offs2 = 0);

/** Compare two strings, starting at an offset each.

    @param str1 the first string
    @param str2 the second string
    @param n the number of characters to compare
    @param offs1 at which position to start in string 1
    @param offs2 at which position to start in string 2
    @return true if they are identical
*/
bool
strutil_ncmp(String const &str1, String const &str2, int n,
             int offs1 = 0,
             int offs2 = 0);

/** Convert a long integer to a string.

    @param i the long integer
    @return the string representing the integer
*/
String strutil_ltoa(long i);

/** Convert an unsigned long integer to a string.

    @param i the unsigned long integer
    @return the string representing the integer
*/
String strutil_ultoa(unsigned long i);

/** Duplicate a character string.

    @param the string to duplicate
    @return the newly allocated string, must be deleted by caller
*/
char * strutil_strdup(const char *in);

/** Duplicate a string.

    @param the string to duplicate
    @return the newly allocated string, must be deleted by caller
*/
char *strutil_strdup(String const &in);

/* * Duplicate a string.

    @param the string to duplicate
    @return the newly allocated string, must be deleted by caller

char *strutil_strdup(String const *in)
{
   return in ? strutil_strdup(in->c_str()) : NULL;
}
*/

/*

void strutil_splitlist(String const &str, std::map<String,String> &table);
  */

/**
   This takes the string and splits it into tokens delimited by the
   given delimiters, by using the strsep() function. The tokens are
   returned as an STL list.
   @param string     pointer to the character array holding the string
   @param delim          character array holding the delimiters
   @param tlist          reference to an STL String list to append the tokens to
  */
void strutil_tokenise(char *string, const char *delim, kbStringList &tlist);

/** Check whether string starts with an URL, if so, return URL.
    @param string the string to check
    @return either the url or an empty string
  */
String strutil_matchurl(const char *string);

/** Checks string for next URL
    @param str the string to examine
    @param url where to store the url
    @return the component of the string before url, sets str to part
    after url
  */
String
strutil_findurl(String &str, String &url);

/// extracts the last path component from a path
String strutil_getfilename(const String& path);

/** Count levels of quoting on the first line of passed string
    (i.e. before the first \n). It understands standard e-mail
    quoting methods: ">" and "XY>"
    @param string the string to check
    @param max_white max number of white characters before quotation mark
    @param max_alpha max number of A-Z characters before quotation mark
    @return number of quoting levels
  */
int strutil_countquotinglevels(const char *string, int max_white, int max_alpha);


/*
   This function is used to verify that a given string may be used to pass it
   to printf(). The requirment is that the number and types of format
   specificators really correspond to the arguments we will pass to printf().
   It's clear, however, that it makes no sense to distinguish between 'o' and
   'u' and 'x' or 'X' format specificators because they all accept the same
   argument. So this function extracts the format specificators from the given
   format string and returns the "canonical form" of them (as a suite of
   characters), which means that only 'c', 'h', 'd', 'l', 'e', 's' and 'p'
   chars will be returned (where 'h' and 'l' stand for short and long int).
 */
String strutil_extract_formatspec(const char *format);

/// checks a character to be a valid part of an URL
#define strutil_isurlchar(c) \
   (isalnum(c)  || c == '.' || c == '/' || c == ':' \
    || c == '_' || c == '&' || c == '#' || c == '-' \
    || c == '%' || c == '~' || c == '!' || c == '?' \
    || c == '*' || c == '+' || c == '$' || c == '@' \
    || c == '=' || c == ',')

/// If path is an absolute path, return true.
bool strutil_isabsolutepath(const String &path);

/** Expands tilde in a pathname. A tilde followed by a slash is the
    user's home directory, taken from the environment. Tilde+name will
    be expanded to the user's home directory.
    @param ipath the path to look up
    @return the expanded path
*/
String strutil_expandpath(const String &ipath);


#ifndef     HAVE_STRSEP
      /// implements the strsep() function found on some Unix systems
      char * strsep(char **stringp, const char *delim);
#endif

/** A small helper function to expand mailfolder names or path names.
    This function takes absolute or relative names. Absolute ones are
    expanded and returned, relative names are expanded relative to the
    mail folder directory.
    @param name name of a mail folder
    @param ftype type of the mail folder, MBOX (default) or MH only
    @return the path to the folder file
*/
String
strutil_expandfoldername(const String &name, FolderType ftype = MF_FILE);

#ifdef OS_UNIX
#   define STRUTIL_PATH_SEPARATOR '/'
#else
#   define STRUTIL_PATH_SEPARATOR '\\'
#endif

/** Cut off last directory from path and return string before that.

    @param pathname the path in which to go up
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_parent(String const &path, char separator = STRUTIL_PATH_SEPARATOR);

/** Cut off last name from path and return string that (filename).

    @param pathname the path
    @param separator the path separator
    @return the parent directory to the one specified
*/
String
strutil_path_filename(String const &path, char separator = STRUTIL_PATH_SEPARATOR);

/** Compare 2 filenames and return true if they refer to the same file. Notice
    that the files don't have to exist when this function is called.
*/
bool
strutil_compare_filenames(const String& path1, const String& path2);

/** Enforces DOS/RFC822 CR/LF newline convention.

    @param in string to copy
    @return the DOSified string
*/
String
strutil_enforceCRLF(String const &in);

/** Enforces native CR/LF newline convention for this platform.
    
    @param in string to copy
    @return the new string
*/
String
strutil_enforceNativeCRLF(String const &in);

/**@name Simple encryption/decryption functionality. */
//@{
/**  Encrypts a string into a (printable) string using a simple table
     based encryption mechanism. TOTALLY UNSAFE!
     The table is always identical, so anyone using this function can
     encrypt/decrypt the string.
     @param original the clear text to encrypt
     @return the printable encrypted text
*/
String strutil_encrypt(const String &original);

/**  Decrypts a string into a (printable) string using a simple table
     based encryption mechanism. TOTALLY UNSAFE!
     The table is always identical, so anyone using this function can
     encrypt/decrypt the string.
     This function is the inverse of strutil_encrypt().
     @param original the encrypted text to decrypt
     @return the clear text
*/
String strutil_decrypt(const String &original);

/// set the global password
extern void strutil_setpasswd(const String &newpasswd);
// get the global password
extern String strutil_getpasswd(void);

/** Takes a time_t time value and returns either local time or GMT in
    a string.
    @param time time value
    @param format format string for strftime()
    @param gmtflag if true, return GMT
    @return a string with the time informaton
*/
String strutil_ftime(time_t time, const String & format = "%c",
                     bool gmtflag = false);


/* Removes Re: Re[n]: Re(n): and the local translation of
   it from the beginning of a subject line, used by sorting routines. */
String
strutil_removeReplyPrefix(const String &isubject);

/* Read and remove the next number from string. */
long
strutil_readNumber(String &string, bool *success = NULL);

/* Read and remove the next quoted string from string. */
String
strutil_readString(String &string, bool *success = NULL);
/* Return an escaped string. */
String
strutil_escapeString(const String &string);

/**
   Combine personal name and mailaddress into a mailaddress line.
*/
String
strutil_makeMailAddress(const String &personal,
                        const String &mailaddress);

/**
   Parse a mail address line and return personal/mailbox/hostname
   bits.
   @return true on success
*/
bool
strutil_getMailAddress(const String &inputline,
                       String * personal = NULL,
                       String * mailbox = NULL,
                       String * hostname = NULL);
//@}

/// Check if text is 7bit only:
bool strutil_is7bit(const char *text);

/** @name regular expression matching */
//@{
/** Compile a string into a regular expression
    @param flags combination of 0, RE_ICASE and RE_EXTENDED
    @return true if ok
*/
class strutil_RegEx *
strutil_compileRegEx(const String &pattern, int flags = 0);

/** Check if the regex matches the string.
    @param flags combination of RE_NOTBOL and RE_NOTEOL *only*
    @return true if it matches
*/
bool
strutil_matchRegEx(const class strutil_RegEx *regex,
                   const String &pattern,
                   int flags = 0);
/// free the regex
void
strutil_freeRegEx(class strutil_RegEx *regex);
//@}

// convert a string array to/from 'ch' separated string
extern wxArrayString strutil_restore_array(char ch, const String& str);
extern String strutil_flatten_array(const wxArrayString& array, char ch = ':');

// return an array containing unique strings from sorted array
extern wxArrayString strutil_uniq_array(const wxSortedArrayString& arrSorted);

//@}
#endif
