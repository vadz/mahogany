/*-*- c++ -*-********************************************************
 * strutil.h : utility functions for string handling                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/
#ifndef STRUTIL_H
#define STRUTIL_H

#include  "Mpch.h"

/**@name String class helper functions */
//@{

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

/** Compare two strings, starting at an offset each.

    @param str1 the first string
    @param offs1 at which position to start in string 1
    @param str2 the second string
    @param offs2 at which position to start in string 2
    @param ignorecase true to ignore case
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
    @param ignorecase true to ignore case
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

/**
   
void strutil_splitlist(String const &str, std::map<String,String> &table);
  */

/**
   This takes the string and splits it into tokens delimited by the
   given delimiters, by using the strsep() function. The tokens are
   returned as an STL list.
   @param string	pointer to the character array holding the string
   @param delim		character array holding the delimiters
   @param tlist		reference to an STL String list to append the tokens to
  */
void strutil_tokenise(char *string, const char *delim, STL_LIST<String> &tlist);

#ifndef	HAVE_STRSEP
char * strsep(char **stringp, const char *delim);
#endif

//@}
#endif
