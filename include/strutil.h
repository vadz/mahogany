/*-*- c++ -*-********************************************************
 * strutil.h : utility functions for string handling                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/
#ifndef STRUTIL_H
#define STRUTIL_H

#include  "Mconfig.h"

#ifndef  USE_PCH
#  include "kbList.h"
#endif

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

/** Duplicate a string.

    @param the string to duplicate
    @return the newly allocated string, must be deleted by caller
*/
/*char *strutil_strdup(String const *in)
{
   return in ? strutil_strdup(in->c_str()) : NULL;
}
*/

/**
   
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

/// checks a character to be a valid part of an URL
#define strutil_isurlchar(c) \
   (isalnum(c)  || c == '.' || c == '/' || c == ':' \
    || c == '_' || c == '&' || c == '#' || c == '-' \
    || c == '%' || c == '~' || c == '!' || c == '?' \
    || c == '*' || c == '+' || c == '$' || c == '@' )


#ifndef     HAVE_STRSEP
      /// implements the strsep() function found on some Unix systems
      char * strsep(char **stringp, const char *delim);
#endif

//@}
#endif
