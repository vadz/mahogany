/*-*- c++ -*-********************************************************
 * strutil.h : utility functions for string handling                *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *                                                                  *
 * $Log$
 * Revision 1.12  1998/07/05 12:20:06  KB
 * wxMessageView works and handles mime (segfault on deletion)
 * wsIconManager loads files
 * install target
 *
 * Revision 1.11  1998/06/05 16:56:51  VZ
 *
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
 * Revision 1.10  1998/05/24 14:47:18  KB
 * lots of progress on Python, but cannot call functions yet
 * kbList fixes again?
 *
 * Revision 1.9  1998/05/19 17:02:41  KB
 * several small bugfixes
 *
 * Revision 1.8  1998/05/18 17:48:23  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.7  1998/05/14 11:00:14  KB
 * Code cleanups and conflicts resolved.
 *
 * Revision 1.6  1998/05/14 09:48:31  KB
 * added IsEmpty() to strutil, minor changes
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

#ifndef     HAVE_STRSEP
char * strsep(char **stringp, const char *delim);
#endif

//@}
#endif
