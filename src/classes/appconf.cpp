/*****************************************************************************\
 * Project:   CppLib: C++ library for Windows/UNIX platfroms                 *
 * File:      config.cpp - implementation of Config class                    *
 *---------------------------------------------------------------------------*
 * Language:  C++                                                            *
 * Platfrom:  any (tested under Windows NT)                                  *
 *---------------------------------------------------------------------------*
 * (c) Karsten Ballüder & Vadim Zeitlin                                      *
 *     Ballueder@usa.net  Vadim.zeitlin@dptmaths.ens-cachan.fr               *
 *---------------------------------------------------------------------------*
 * Classes:                                                                  *
 *  Config  - manages configuration files or registry database               *
 *---------------------------------------------------------------------------*
 * History:                                                                  *
 *  25.10.97  adapted from wxConfig by Karsten Ballüder                      *
 *  09.11.97  corrected bug in RegistryConfig::enumSubgroups                 *
 *     --- for further changes see appconf.h or the CVS log ---              *    
\*****************************************************************************/

/**********************************************************************\
 *                                                                    *
 * This library is free software; you can redistribute it and/or      *
 * modify it under the terms of the GNU Library General Public        *
 * License as published by the Free Software Foundation; either       *
 * version 2 of the License, or (at your option) any later version.   *
 *                                                                    *
 * This library is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Library General Public License for more details.                   *
 *                                                                    *
 * You should have received a copy of the GNU Library General Public  *
 * License along with this library; if not, write to the Free         *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. *
 *                                                                    *
\**********************************************************************/

//static const char
//*cvs_id = "$Id$";

// ============================================================================
// headers, constants, private declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// standard headers
#ifdef    __WIN32__
#	ifdef  _MSC_VER
    // nonstandard extension used : nameless struct/union
#		pragma warning(disable: 4201)  
#	endif  // VC++

#	include  <windows.h>
#endif    // WIN32

#if       defined(__unix__)
#	include <sys/param.h>
#	include	<sys/stat.h>
#	include <unistd.h>
#	define MAX_PATH	      MAXPATHLEN
#elif     defined(__WINDOWS__)
# define MAX_PATH	      _MAX_PATH
#endif

#include  <fcntl.h>
#include  <sys/types.h>

#ifdef    USE_IOSTREAMH
  #include  <iostream>
  #include  <fstream>

  using     namespace std;

  #define   STREAM_READ_MODE  ios::in
#else     // old style headers
  #include  <iostream.h>
  #include  <fstream.h>

  #define   STREAM_READ_MODE  ios::in | ios::nocreate
#endif    // iostream style

#include  <string.h>
#include  <ctype.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <assert.h>

// our headers
#include  "appconf.h"

// ----------------------------------------------------------------------------
// some debug/error reporting functions
// ----------------------------------------------------------------------------


#if	APPCONF_USE_GETTEXT
#	include	<libintl.h>
#	define	_(x)	dgettext(APPCONF_DOMAIN,x)
#else
#	define	_(x)	(x)
#endif


#ifndef   __WXWIN__

// in general, these messages could be treated all differently
// define a standard log function, to be called like printf()
#ifndef LogInfo
#	define   LogInfo     LogError
#endif

// define a standard error log function, to be called like printf()
#ifndef LogWarning
#	define   LogWarning  LogError
#endif

// logs an error message (with a name like that it's really strange)
// message length is limited to 1Kb
void LogError(const char *pszFormat, ...)
{
  char szBuf[1025];

  va_list argptr;
  va_start(argptr, pszFormat);
  vsprintf(szBuf, pszFormat, argptr);
  va_end(argptr);

  strcat(szBuf, "\n");
  fputs(szBuf, stderr);
}

#endif

#ifdef  __WIN32__

const char *SysError()
{
  static char s_szBuf[1024];

  // get error message from system
  LPVOID lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, GetLastError(), 
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf,
                0, NULL);

  // copy it to our buffer and free memory
  strncpy(s_szBuf, (const char *)lpMsgBuf, sizeof(s_szBuf)/sizeof(char));
  LocalFree(lpMsgBuf);

  // returned string is capitalized and ended with '\n' - no good
  s_szBuf[0] = (char)tolower(s_szBuf[0]);
  size_t len = strlen(s_szBuf);
  if ( (len > 0) && (s_szBuf[len - 1] == '\n') )
    s_szBuf[len - 1] = '\0';

  return s_szBuf;
}

#endif

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------
inline size_t Strlen(const char *pc) { return pc == NULL ? 0 : strlen(pc); }
//WHY? does not exist on unix! extern size_t Strlen(const char *pc);
inline Bool   IsValid(char c) { return isalnum(c) || strchr("_/-!.*%", c); }
inline Bool   IsCSym (char c) { return isalnum(c) || ( c == '_');          }
inline size_t Min(size_t n1, size_t n2) { return n1 < n2 ? n1 : n2; }

#define   SIZE(array)       (sizeof(array)/sizeof(array[0]))

#if APPCONF_CASE_SENSITIVE
# define StrCmp(s1,s2)  strcmp((s1),(s2))
#else
# ifdef   __unix__
    extern "C" int strcasecmp(const char *s1, const char *s2); // it's not ansi
#   define  StrCmp(s1,s2)  strcasecmp((s1),(s2))
# else
#   ifdef _MSC_VER
#     define  StrCmp(s1,s2)  _stricmp((s1),(s2))
#   else
#     error "Please define 'stricmp' function for your compiler."
#   endif // compilers
# endif   // strcasecmp/strcimp
#endif

// perform environment variable substitution
char *ExpandEnvVars(const char *psz)
{
  // don't change the values the enum elements: they must be equal
  // to the matching [closing] delimiter.
  enum Bracket
  { 
    Bracket_None, 
    Bracket_Normal  = ')', 
    Bracket_Curly   = '}' 
  };
          
  // max size for an environment variable name is fixed
  char szVarName[256];

  if(psz == NULL)
     return NULL;
  
  // first calculate the length of the resulting string
  size_t nNewLen = 0;
  const char *pcIn;
  for ( pcIn = psz; *pcIn != '\0'; pcIn++ ) {
    switch ( *pcIn ) {
      case '$':
        {
          Bracket bracket;
          switch ( *++pcIn ) {
            case '(': 
              bracket = Bracket_Normal; 
              pcIn++;                   // skip the bracket
              break;

            case '{':
              bracket = Bracket_Curly;
              pcIn++;                   // skip the bracket
              break;

            default:
              bracket = Bracket_None;
          }
          const char *pcStart = pcIn;

          while ( IsCSym(*pcIn) ) pcIn++;

          size_t nCopy = Min(pcIn - pcStart, SIZE(szVarName));
          strncpy(szVarName, pcStart, nCopy);
          szVarName[nCopy] = '\0';

          if ( bracket != Bracket_None ) {
            if ( *pcIn != (char)bracket ) {
              // # what to do? we decide to give warning and ignore 
              //   the opening bracket
              LogWarning(_("'%c' expected in '%s' after '${%s'"), 
                         (char)bracket, psz, szVarName);
              pcIn--;
            }
          }
          else {
            // everything is ok but we took one extra character
            pcIn--;
          }

          // Strlen() acceps NULL as well
          nNewLen += Strlen(getenv(szVarName));
        }
        break;

      case '\\':
        pcIn++;
        // fall through

      default:
        nNewLen++;
    }
  }

  // # we always realloc buffer (could reuse the old one if nNewLen < nOldLen)
  char *szNewValue = new char[nNewLen + 1];
  char *pcOut = szNewValue;

  // now copy it to the new location replacing the variables with their values
  for ( pcIn = psz; *pcIn != '\0'; pcIn++ ) {
    switch ( *pcIn ) {
      case '$':
        {
          Bracket bracket;
          switch ( *++pcIn ) {
            case '(': 
              bracket = Bracket_Normal; 
              pcIn++;                   // skip the bracket
              break;

            case '{':
              bracket = Bracket_Curly;
              pcIn++;                   // skip the bracket
              break;

            default:
              bracket = Bracket_None;
          }
          const char *pcStart = pcIn;

          while ( IsCSym(*pcIn) ) pcIn++;

          size_t nCopy = Min(pcIn - pcStart, SIZE(szVarName));
          strncpy(szVarName, pcStart, nCopy);
          szVarName[nCopy] = '\0';

          if ( bracket != Bracket_None ) {
            if ( *pcIn != (char)bracket ) {
              // warning message already given, just ignore opening bracket
              pcIn--;
            }
          }
          else {
            // everything is ok but we took one extra character
            pcIn--;
          }

          const char *pszValue = getenv(szVarName);
          if ( pszValue != NULL ) {
            strcpy(pcOut, pszValue);
            pcOut += strlen(pszValue);
          }
        }
        break;

      case '\\':
        pcIn++;
        // fall through

      default:
        *pcOut++ = *pcIn;
    }
  }

  *pcOut = '\0';

  return szNewValue;
}

// ============================================================================
// implementation of the class BaseConfig
// ============================================================================

// ----------------------------------------------------------------------------
// ctor and dtor
// ----------------------------------------------------------------------------

BaseConfig::BaseConfig()
{
   m_szCurrentPath = NULL;
   m_bRecordDefaults = FALSE;
}

BaseConfig::~BaseConfig()
{
  if ( m_szCurrentPath != NULL )
    delete [] m_szCurrentPath;
}

void
BaseConfig::recordDefaults(Bool enable)
{
   m_bRecordDefaults = enable;
}

// ----------------------------------------------------------------------------
// handle long int and double values
// ----------------------------------------------------------------------------

Bool
BaseConfig::writeEntry(const char *szKey, int Value)
{
   return writeEntry(szKey, (long int) Value);
}

Bool
BaseConfig::writeEntry(const char *szKey, long int Value)
{
   char buffer[APPCONF_STRBUFLEN]; // ugly
   sprintf(buffer, "%ld", Value);
   return writeEntry(szKey,buffer);
}

Bool
BaseConfig::writeEntry(const char *szKey, double Value)
{
   char buffer[APPCONF_STRBUFLEN]; // ugly
   sprintf(buffer,"%g", Value);
   return writeEntry(szKey,buffer);
}	

int
BaseConfig::readEntry(const char *szKey, int Default) const
{
   return (int) readEntry(szKey, (long int) Default);
}

long int
BaseConfig::readEntry(const char *szKey, long int Default) const
{
   const char *cptr = readEntry(szKey,(const char *)NULL);
   if(cptr)
      return atol(cptr);
   else {
      if(m_bRecordDefaults)
        ((BaseConfig *)this)->writeEntry(szKey,Default);
      return Default;
   }
}

double
BaseConfig::readEntry(const char *szKey, double Default) const
{
   const char *cptr = readEntry(szKey,(const char *)NULL);
   if(cptr)
      return atof(cptr);
   else {
      if(m_bRecordDefaults)
        ((BaseConfig *)this)->writeEntry(szKey,Default);
      return Default;
   }
}

// ----------------------------------------------------------------------------
// set/get current path
// ----------------------------------------------------------------------------

// this function resolves all ".." (but not '/'!) in the path
// returns pointer to dynamically allocated buffer, free with "delete []"
// ## code here is inefficient and difficult to understand, to rewrite
char *BaseConfig::normalizePath(const char *szStartPath, const char *szPath)
{
  char    *szNormPath;

  // array grows in chunks of this size
#define   COMPONENTS_INITIAL    (10)

  char   **aszPathComponents;   // component is something between 2 '/'
  size_t   nComponents = 0,
           nMaxComponents;

  aszPathComponents = new char *[nMaxComponents = COMPONENTS_INITIAL];

  const char *pcStart;   // start of last component
  const char *pcIn;

  // concatenate the two adding APPCONF_PATH_SEPARATOR to the end if not there
  size_t len = Strlen(szStartPath);
  size_t nOldLen = len + Strlen(szPath) + 1;
  szNormPath = new char[nOldLen + 1];
  strcpy(szNormPath, szStartPath);
  szNormPath[len++] = APPCONF_PATH_SEPARATOR;
  szNormPath[len] = '\0';
  strcat(szNormPath, szPath);

  // break combined path in components
  Bool bEnd = FALSE;
  for ( pcStart = pcIn = szNormPath; !bEnd; pcIn++ ) {
    if ( *pcIn == APPCONF_PATH_SEPARATOR || *pcIn == '\0' ) {
      if ( *pcIn == '\0' )
        bEnd = TRUE;

      // another component - is it "." or ".."?
      if ( *pcStart == '.' ) {
        if ( pcIn == pcStart + 1 ) {
          // "./" - ignore
          pcStart = pcIn + 1;
          continue;
        }
        else if ( (pcIn == pcStart + 2) && (*(pcStart + 1) == '.') ) {
          // "../" found - delete last component
          if ( nComponents > 0 ) {
            delete [] aszPathComponents[--nComponents];
          }
          else {
            LogWarning(_("extra '..' in the path '%s'."), szPath);
          }

          pcStart = pcIn + 1;
          continue;
        }
      }
      else if ( pcIn == pcStart ) {
        pcStart = pcIn + 1;
        continue;
      }

      // normal component, add to the list

      // grow array?
      if ( nComponents == nMaxComponents ) {    
        // realloc array
        char **aszOld = aszPathComponents;
        nMaxComponents += COMPONENTS_INITIAL;
        aszPathComponents = new char *[nMaxComponents];

        // move data
        memmove(aszPathComponents, aszOld, 
                sizeof(aszPathComponents[0]) * nComponents);

        // free old
        delete [] aszOld;
      }

      // do add
      aszPathComponents[nComponents] = new char[pcIn - pcStart + 1];
      strncpy(aszPathComponents[nComponents], pcStart, pcIn - pcStart);
      aszPathComponents[nComponents][pcIn - pcStart] = '\0';
      nComponents++;

      pcStart = pcIn + 1;
    }
  }

  if ( nComponents == 0 ) {
    // special case
    szNormPath[0] = '\0';
  }
  else {
    // put all components together
    len = 0;
    for ( size_t n = 0; n < nComponents; n++ ) {
      // add '/' before each new component except the first one
      if ( len != 0 ) {
        szNormPath[len++] = APPCONF_PATH_SEPARATOR;
      }
      szNormPath[len] = '\0';

      // concatenate
      strcat(szNormPath, aszPathComponents[n]);

      // update length
      len += strlen(aszPathComponents[n]);

      // and free memory
      delete [] aszPathComponents[n];
    }
  }

  delete [] aszPathComponents;

  return szNormPath;
}

void BaseConfig::changeCurrentPath(const char *szPath)
{
  // special case (default value)
  if ( Strlen(szPath) == 0 ) {
    if ( m_szCurrentPath != NULL ) {
      delete [] m_szCurrentPath;
      m_szCurrentPath = NULL;
    }
  }    
  else {
    char *szNormPath;

    // if absolute path, start from top, otherwise from current
    if ( *szPath == APPCONF_PATH_SEPARATOR )
      szNormPath = normalizePath("", szPath + 1);
    else
      szNormPath = normalizePath(m_szCurrentPath ? m_szCurrentPath : "", szPath);

    size_t len = Strlen(szNormPath);
    if ( m_szCurrentPath == NULL || len > strlen(m_szCurrentPath) ) {
      // old buffer too small, alloc new one
      if ( m_szCurrentPath != NULL )
        delete [] m_szCurrentPath;
      m_szCurrentPath = new char[len + 1];
    }

    // copy and delete pointer returned by normalizePath
    strcpy(m_szCurrentPath, szNormPath);

    delete [] szNormPath;
  }
}

void BaseConfig::setCurrentPath(const char *szPath)
{
  changeCurrentPath();		      // goto root
  changeCurrentPath(szPath);	  // now use relative change
}

const char *BaseConfig::getCurrentPath() const
{
  return m_szCurrentPath == NULL ? "" : m_szCurrentPath;
}

// ----------------------------------------------------------------------------
// filters (NB: filterOut(filterIn()) shouldn't change the string)
// ----------------------------------------------------------------------------

// called before writing
char *BaseConfig::filterOut(const char *szValue)
{
  if ( !szValue ) {
    assert( 0 );
    return NULL;
  }

  // quote entire string if it starts with space or with quote
  Bool bDoQuote = isspace(*szValue) || (*szValue == '"');

  // calculate the length of resulting string
  size_t len = Strlen(szValue);

  const char *pcIn = szValue;
  while ( *pcIn ) {
    switch ( *pcIn++ ) {
      case '"':
        if ( !bDoQuote )
          break;
        //else: fall through

      case '\n':
      case '\t':
      case '\\':
        // one for extra '\'
        len++;
        break;
    }
  }

  if ( bDoQuote )
    len += 2;

  char *szBuf = new char[len + 1];
  char *pcOut = szBuf;

  if ( bDoQuote )
    *pcOut++ = '"';

  char c;
  for ( pcIn = szValue; *pcIn != '\0'; pcIn++ ) {
    switch ( *pcIn ) {
      case '\n':
        c = 'n';
        break;

      case '\t':
        c = 't';
        break;

      case '\\':
        c = '\\';
        break;

      case '"':
        if ( bDoQuote )
          c = '"';
        //else: fall through

      default:
        *pcOut++ = *pcIn;
        continue;   // skip special character procession
    }

    // we get here only for special characters
    *pcOut++ = '\\';
    *pcOut++ = c;
  }

  if ( bDoQuote )
    *pcOut++ = '"';

  *pcOut = '\0';

  return szBuf;
}

// called after reading
char *BaseConfig::filterIn(const char *szValue)
{
  if ( !szValue ) {
    assert( 0 );
    return NULL;
  }

  // it will be a bit smaller, but who cares
  char *szBuf = new char[Strlen(szValue) + 1];

  const char *pcIn  = szValue;
  char *pcOut = szBuf;

  Bool bQuoted = *pcIn == '"';
  if ( bQuoted )
    pcIn++;

  while ( *pcIn != '\0' ) {
    switch ( *pcIn ) {
      case '\\':
        switch ( *++pcIn ) {
          case 'n':
            *pcOut++ = '\n';
            break;

          case 't':
            *pcOut++ = '\t';
            break;

          case '\\':
            *pcOut++ = '\\';
            break;

          case '"':
          default:
            // ignore '\'
            *pcOut++ = *pcIn;
        }
        break;

      case '"':
        if ( bQuoted ) {
          if ( *(pcIn + 1) != '\0' ) {
            LogWarning(_("invalid string '%s' in configuration file."), szValue);
          }
          break;
        }
        //else: fall through

      default:
        *pcOut++ = *pcIn;
    }

    pcIn++;
  }

  *pcOut = '\0';

  return szBuf;
}

// ----------------------------------------------------------------------------
// implementation of Enumerator subclass
// ----------------------------------------------------------------------------
BaseConfig::Enumerator::Enumerator(size_t nCount, Bool bOwnsStrings)
{
  m_bOwnsStrings = bOwnsStrings;
  m_aszData      = new char *[nCount];
  m_nCount       = 0;
}

// free memory
BaseConfig::Enumerator::~Enumerator()
{
  if ( m_bOwnsStrings ) {
    for ( size_t n = 0; n < m_nCount; n++ )
      delete [] m_aszData[n];
  }

  delete [] m_aszData;
}

void BaseConfig::Enumerator::AddString(char *sz)
{
  m_aszData[m_nCount++] = sz;
}

void BaseConfig::Enumerator::AddString(const char *sz)
{
  assert( !m_bOwnsStrings );

  // we won't delete it (see assert above), so we can cast in "char *"
  m_aszData[m_nCount++] = (char *)sz;
}

// remove duplicate strings
void BaseConfig::Enumerator::MakeUnique()
{
  char  **aszUnique = new char *[m_nCount];
  size_t  nUnique = 0;
  
  Bool bUnique;
  for ( size_t n = 0; n < m_nCount; n++ ) {
    bUnique = TRUE;
    for ( size_t n2 = n + 1; n2 < m_nCount; n2++ ) {
      if ( StrCmp(m_aszData[n], m_aszData[n2]) == 0 ) {
        bUnique = FALSE;
        break;
      }
    }

    if ( bUnique )
      aszUnique[nUnique++] = m_aszData[n];
    else if ( m_bOwnsStrings )
      delete [] m_aszData[n];
  }

  delete [] m_aszData;
  m_aszData = aszUnique;
  m_nCount = nUnique;
}

// ============================================================================
// implementation of FileConfig class
// ============================================================================

// ----------------------------------------------------------------------------
// FileConfig::ConfigEntry class
// ----------------------------------------------------------------------------

// ctor
FileConfig::ConfigEntry::ConfigEntry(ConfigGroup *pParent, 
                                     ConfigEntry *pNext, 
                                     const char *szName)
{
  m_pParent     = pParent;
  m_pNext       = pNext;
  m_szExpValue  =
  m_szValue     = 
  m_szComment   = NULL;
  m_bDirty      = 
  m_bLocal      = FALSE;

  // check for special prefix
  if ( *szName == APPCONF_IMMUTABLE_PREFIX ) {
    m_bImmutable = TRUE;
    szName++;             // skip prefix
  }
  else
    m_bImmutable = FALSE;

  m_szName  = new char[Strlen(szName) + 1]; 
  strcpy(m_szName, szName);
}

// dtor
FileConfig::ConfigEntry::~ConfigEntry()
{
  if ( m_szName != NULL )
    delete [] m_szName;

  if ( m_szValue != NULL )
    delete [] m_szValue;
    
  if ( m_szComment != NULL )
    delete [] m_szComment;

  if ( m_szExpValue != NULL )
    delete [] m_szExpValue;
}

// set value and dirty flag
void FileConfig::ConfigEntry::SetValue(const char *szValue, 
                                       Bool bLocal, Bool bFromFile)
{
   if ( m_szExpValue != NULL ) {
      delete [] m_szExpValue;
      m_szExpValue = NULL;
   }

   if ( m_szValue != NULL ) {
    // immutable changes can't be overriden (only check it here, because
    // they still can be set the first time)
    if ( m_bImmutable ) {
      LogWarning(_("attempt to change an immutable entry '%s' ignored."),
                 m_szName);
      return;
    }

    delete [] m_szValue;
  }

  // immutable entries are never changed and if an entry was just read
  // from file it shouldn't be dirty neither
  if ( !m_bImmutable && !bFromFile )
    SetDirty();

  m_bLocal = bLocal;
  if ( bLocal ) {
    // to ensure that local entries are saved we make them always dirty
    SetDirty();
  }

  if ( szValue != NULL )
  {
     m_szValue = new char[Strlen(szValue) + 1];
     strcpy(m_szValue, szValue);
  }
  else
  {
     m_szValue = NULL;
     SetDirty(FALSE);
  }
}

// set comment associated with this entry
void FileConfig::ConfigEntry::SetComment(char *szComment)
{
  assert( m_szComment == NULL );  // should be done only once
  
  // ... because we don't copy, just take the pointer
  m_szComment = szComment;
}

// set dirty flag
void FileConfig::ConfigEntry::SetDirty(Bool bDirty)
{
  // ## kludge: local entries are always dirty, otherwise they would be lost
  m_bDirty = m_bLocal ? TRUE : bDirty;
  if ( m_bDirty )
    m_pParent->SetDirty();
}

const char *FileConfig::ConfigEntry::ExpandedValue()
{
  if ( m_szExpValue == NULL ) {
    // we have only to do expansion once
    m_szExpValue = ExpandEnvVars(m_szValue);
  }

  return m_szExpValue;
}

// ----------------------------------------------------------------------------
// FileConfig::ConfigGroup class
// ----------------------------------------------------------------------------

// ctor & dtor
// -----------

FileConfig::ConfigGroup::ConfigGroup(ConfigGroup *pParent, 
                                     ConfigGroup *pNext, 
                                     const char *szName)
{ 
  m_pParent     = pParent; 
  m_pNext       = pNext; 
  m_pEntries    =
  m_pLastEntry  = NULL;
  m_pSubgroups  =
  m_pLastGroup  = NULL;
  m_bDirty      = FALSE;    // no entries yet

  m_szComment   = NULL;
  m_szName      = new char[Strlen(szName) + 1]; 
  strcpy(m_szName, szName); }

FileConfig::ConfigGroup::~ConfigGroup()
{
  // delete all entries
  ConfigEntry *pEntry, *pNextEntry;
  for ( pEntry = m_pEntries; pEntry != NULL; pEntry = pNextEntry ) {
    pNextEntry = pEntry->Next();
    delete pEntry;
  }

  // delete all subgroups
  ConfigGroup *pGroup, *pNextGroup;
  for ( pGroup = m_pSubgroups; pGroup != NULL; pGroup = pNextGroup ) {
    pNextGroup = pGroup->Next();
    delete pGroup;
  }

  if ( m_szName != NULL )
    delete [] m_szName;
}

// find
// ----

FileConfig::ConfigGroup *
            FileConfig::ConfigGroup::FindSubgroup(const char *szName) const
{
  ConfigGroup *pGroup;
  for ( pGroup = m_pSubgroups; pGroup != NULL; pGroup = pGroup->Next() ) {
    if ( !StrCmp(pGroup->Name(), szName) )
      return pGroup;
  }

  return NULL;
}

FileConfig::ConfigEntry *
            FileConfig::ConfigGroup::FindEntry(const char *szName) const
{
  ConfigEntry *pEntry;
  for ( pEntry = m_pEntries; pEntry != NULL; pEntry = pEntry->Next() ) {
    if ( !StrCmp(pEntry->Name(), szName) )
      return pEntry;
  }

  return NULL;
}

// add
// ---

// add an item at the bottom of the stack (i.e. it's a LILO really)
FileConfig::ConfigGroup *FileConfig::ConfigGroup::AddSubgroup(const char *szName)
{
  ConfigGroup *pGroup = new ConfigGroup(this, NULL, szName);

  if ( m_pSubgroups == NULL ) {
    m_pSubgroups = 
    m_pLastGroup = pGroup;
  }
  else {
    m_pLastGroup = m_pLastGroup->m_pNext = pGroup;
  }

  return pGroup;
}

// add an item at the bottom
FileConfig::ConfigEntry *FileConfig::ConfigGroup::AddEntry(const char *szName)
{
  ConfigEntry *pEntry = new ConfigEntry(this, NULL, szName);
  
  if ( m_pEntries == NULL ) {
    m_pEntries   = 
    m_pLastEntry = pEntry;
  }
  else {
    m_pLastEntry->SetNext(pEntry);
    m_pLastEntry = pEntry;
  }

  return pEntry;
}

// delete
// ------

Bool FileConfig::ConfigGroup::DeleteSubgroup(const char *szName)
{
  ConfigGroup *pGroup, *pPrevGroup = NULL;
  for ( pGroup = Subgroup(); pGroup != NULL; pGroup = pGroup->Next() ) {
    if ( StrCmp(pGroup->Name(), szName) == 0 ) {
      break;
    }

    pPrevGroup = pGroup;
  }

  if ( pGroup == NULL )
    return FALSE;

  // remove the next element in the linked list
  if ( pPrevGroup == NULL ) {
    m_pSubgroups = pGroup->Next();
  }
  else {
    pPrevGroup->m_pNext = pGroup->Next();
  }

  // adjust pointer to the last element
  if ( pGroup->Next() == NULL ) {
    m_pLastGroup = pPrevGroup == NULL ? m_pSubgroups : pPrevGroup;
  }
  
  // shouldn't have any entries/subgroups or they would be never deleted
  // resulting in memory leaks (or we then should delete them here)
  assert( pGroup->Entries() == NULL && pGroup->Subgroup() == NULL );
  delete pGroup;

  return TRUE;
}

Bool FileConfig::ConfigGroup::DeleteEntry(const char *szName)
{
  ConfigEntry *pEntry, *pPrevEntry = NULL;
  for ( pEntry = Entries(); pEntry != NULL; pEntry = pEntry->Next() ) {
    if ( StrCmp(pEntry->Name(), szName) == 0 ) {
      break;
    }

    pPrevEntry = pEntry;
  }

  if ( pEntry == NULL )
    return FALSE;

  // remove the element from the linked list
  if ( pPrevEntry == NULL ) {
    m_pEntries = pEntry->Next();
  }
  else {
    pPrevEntry->SetNext(pEntry->Next());
  }

  // adjust the pointer to the last element
  if ( pEntry->Next() == NULL ) {
    m_pLastEntry = pPrevEntry == NULL ? m_pEntries : pPrevEntry;
  }

  // ... and free memory
  delete pEntry;

  m_pParent->SetDirty();

  return TRUE;
}

// deletes this group if it has no more entries/subgroups
Bool FileConfig::DeleteIfEmpty()
{
  // check if there any other subgroups/entries left
  if ( m_pCurGroup->Entries() != NULL || m_pCurGroup->Subgroup() != NULL )
    return FALSE;

  if ( m_pCurGroup->Parent() == NULL ) {
    // top group, can't delete it but mark it as not dirty,
    // so that local config file will not even be created
    m_pCurGroup->SetDirty(FALSE);
  }
  else {
    // delete current group
    const char *szName = m_pCurGroup->Name();
    m_pCurGroup = m_pCurGroup->Parent();
    m_pCurGroup->DeleteSubgroup(szName);
  }

  // recursively call ourselves
  DeleteIfEmpty();

  return TRUE;
}

// other
// -----

// get absolute name
char *FileConfig::ConfigGroup::FullName() const
{
  char *szFullName;
  if ( m_pParent == NULL ) {
    // root group has no name
    return NULL;
  }
  else {
    char *pParentFullName = m_pParent->FullName();
    if ( pParentFullName == NULL ) {
      szFullName = new char[Strlen(m_szName) + 1];
      strcpy(szFullName, m_szName);
    }
    else {
      size_t len = Strlen(pParentFullName);
      szFullName = new char[len + Strlen(m_szName) + 2];
      strcpy(szFullName, pParentFullName);
      szFullName[len] = APPCONF_PATH_SEPARATOR;   // unfortunately strcat doesn't
      szFullName[len + 1] = '\0';                 // take 'char' argument
      strcat(szFullName, m_szName);
      delete [] pParentFullName;
    }
  }

  return szFullName;
}

// set dirty flag and propagate it as needed
// NB: when we set dirty, propagate to parent; when clear - to children
void FileConfig::ConfigGroup::SetDirty(Bool bDirty)
{
  m_bDirty = bDirty;
  if ( bDirty ) {
    if ( m_pParent != NULL )
      m_pParent->SetDirty();
  }
  else {
    ConfigEntry *pEntry;
    for ( pEntry = Entries(); pEntry != NULL; pEntry = pEntry->Next() )
      pEntry->SetDirty(FALSE);

    ConfigGroup *pGroup;
    for ( pGroup = Subgroup(); pGroup != NULL; pGroup = pGroup->Next() )
      pGroup->SetDirty(FALSE);
  }
}

// write changed data
Bool FileConfig::ConfigGroup::flush(ostream *ostr)
{
  // write all entries that were changed in this group
  Bool bFirstDirty = TRUE;
  ConfigEntry *pEntry;
  for ( pEntry = Entries(); pEntry != NULL; pEntry = pEntry->Next() ) {
    if ( pEntry->IsDirty() && pEntry->Value()) {
      if ( bFirstDirty ) {
        // output preceding comment for this group if any
        if ( Comment() != NULL ) {
          *ostr << Comment();
        }
        
        // output group header
        char *pName = FullName();
        if ( pName != NULL ) {
          *ostr << '[' << pName << ']';
          if ( pEntry->Comment() == NULL )
            *ostr << endl;
          delete [] pName;
        }
        //else: it's the top (default) group

        bFirstDirty = FALSE;  // only first time
      }

      // output preceding comment for this entry if any
      if ( pEntry->Comment() != NULL ) {
        *ostr << pEntry->Comment();
      }
      
      char *szFilteredValue = filterOut(pEntry->Value());
      *ostr << pEntry->Name() << " = " << szFilteredValue << endl;
      delete [] szFilteredValue;

      pEntry->SetDirty(FALSE);
    }
  }

  // flush all subgroups
  ConfigGroup *pGroup;
  Bool bOk = TRUE;
  for ( pGroup = Subgroup(); pGroup != NULL; pGroup = pGroup->Next() ) {
    if ( pGroup->IsDirty() && !pGroup->flush(ostr) ) {
      bOk = FALSE;
    }
  }

  return bOk;
}


// associate a comment with this group (comments are all we ignore,
// including real comments and whitespace)
void FileConfig::ConfigGroup::SetComment(char *szComment)
{
  assert( m_szComment == NULL );  // should be done only once
  
  m_szComment = szComment;
}

// ----------------------------------------------------------------------------
// ctor and dtor
// ----------------------------------------------------------------------------

// common part of both constructors
void FileConfig::Init()
{
  // top group always exists and must be created before reading from files
  m_pRootGroup = new ConfigGroup(NULL, NULL, "");

  m_szComment = NULL;
  m_bOk = FALSE;                // not (yet) initialized ok

  m_bExpandVariables = FALSE;   // don't do it automatically by
				// default
  m_szFileName = NULL;
  m_szFileName = NULL;
  m_szCFileName = NULL;
  m_bUseSubDir = FALSE;
  m_bFullPathGiven = FALSE;
}

// ctor reads data from files
FileConfig::FileConfig(const char *szFileName, Bool bLocalOnly,
		       Bool bFullPathGiven,
		       Bool bUseSubDir, const char *szConfigFileName
		       )
{
  Init();
  
  m_bUseSubDir = bUseSubDir;
  m_bFullPathGiven = bFullPathGiven;
  if(m_bFullPathGiven)
     bLocalOnly = TRUE;
  m_szFileName = new char[Strlen(szFileName) + 1];
  strcpy(m_szFileName, szFileName);
  m_szCFileName = new char[Strlen(szConfigFileName) + 1];
  strcpy(m_szCFileName, szConfigFileName);
  
  // read global (system wide) file
  // ------------------------------
  ifstream inpStream;
  if ( !bLocalOnly ) {
    m_szFullFileName = GlobalConfigFile();
    inpStream.open(m_szFullFileName, STREAM_READ_MODE);
    if ( inpStream ) {
      m_bParsingLocal = FALSE;
      m_bOk = readStream(&inpStream);
    }

    inpStream.close();
    inpStream.clear();
  }

  // read local (user's) file
  // ------------------------
  m_szFullFileName = LocalConfigFile();
  if ( m_szFullFileName != NULL ) {
    inpStream.open(m_szFullFileName, STREAM_READ_MODE);
    if ( inpStream ) {
      m_bParsingLocal = TRUE;
      if ( readStream(&inpStream) ) {
        m_bOk = TRUE;
      }
      //else: depends on global file - if it was read ok, it's ok
    }
  }

  m_pCurGroup = m_pRootGroup;
  BaseConfig::setCurrentPath("");
}

// ctor reads data from files
FileConfig::FileConfig(istream *iStream)
{
  Init();
  
  m_szFileName = NULL;

  if ( iStream == NULL )
     return;

  m_bParsingLocal = TRUE;
  m_bOk = readStream(iStream);
  
  m_pCurGroup = m_pRootGroup;
  BaseConfig::setCurrentPath("");
}

FileConfig::FileConfig(void)
{
   Init();
}

void
FileConfig::readFile(const char *szFileName)
{
  ifstream inpStream;

  Init();

   m_szFileName = new char[Strlen(szFileName) + 1];
   strcpy(m_szFileName, szFileName);
   m_bFullPathGiven = TRUE;
   m_szFullFileName = m_szFileName;
   inpStream.open(m_szFullFileName, STREAM_READ_MODE);
   if ( inpStream ) {
      m_bParsingLocal = TRUE;
      if ( readStream(&inpStream) ) {
	 m_bOk = TRUE;
      }
   }

   m_pCurGroup = m_pRootGroup;
   BaseConfig::setCurrentPath("");
}

// dtor writes changes
FileConfig::~FileConfig()
{
  if( m_szFileName != NULL )	// if empty, was intialised from stream
    flush();
  
  if ( m_szComment != NULL )
    delete [] m_szComment;
    
  delete m_pRootGroup;
  
  if ( m_szFileName != NULL )
     delete m_szFileName;

}

// ----------------------------------------------------------------------------
// config files standard locations
// ----------------------------------------------------------------------------

// ### buffer overflows in sight...
const char *FileConfig::GlobalConfigFile() const
{
  static char s_szBuf[MAX_PATH];

  // check if file has extension
  Bool bNoExt = strchr(m_szFileName, '.') == NULL;

#ifdef  __unix__
    strcpy(s_szBuf, "/etc/");
    strcat(s_szBuf, m_szFileName);
    if ( bNoExt )
      strcat(s_szBuf, ".conf");
#else   // Windows
    char szWinDir[MAX_PATH];
    ::GetWindowsDirectory(szWinDir, MAX_PATH);
    strcpy(s_szBuf, szWinDir);
    strcat(s_szBuf, "\\");
    strcat(s_szBuf, m_szFileName);
    if ( bNoExt )
      strcat(s_szBuf, ".INI");
#endif  // UNIX/Win

  return s_szBuf;
}

// ### buffer overflows in sight...
const char *FileConfig::LocalConfigFile() const
{
  static char s_szBuf[MAX_PATH];

  if(m_bFullPathGiven)
     return m_szFileName;
    
#ifdef  __unix__
    const char *szHome = getenv("HOME");
    if ( szHome == NULL ) {
      // we're homeless...
      LogInfo(_("can't find user's HOME, looking for config file in current directory."));
      szHome = ".";
    }
    strcpy(s_szBuf, szHome);
    strcat(s_szBuf, "/.");
    strcat(s_szBuf, m_szFileName);
    if(m_bUseSubDir) // look for ~/.appname/config instead of ~/.appname
    {
       mkdir(s_szBuf, 0755);	// try to make it, just in case it
				// didn't exist yet
       strcat(s_szBuf, "/");
       strcat(s_szBuf, m_szCFileName);
    }
#else   // Windows
#ifdef  __WIN32__
      const char *szHome = getenv("HOMEDRIVE");
      if ( szHome == NULL )
        szHome = "";
      strcpy(s_szBuf, szHome);
      szHome = getenv("HOMEPATH");
      if ( szHome == NULL )
        strcpy(s_szBuf, ".");
      else
        strcat(s_szBuf, szHome);
      strcat(s_szBuf, m_szFileName);
      if ( strchr(m_szFileName, '.') == NULL )
        strcat(s_szBuf, ".INI");
#else   // Win16
      // Win 3.1 has no idea about HOME, so we use only the system wide file
      if ( !bLocalOnly ) {
        // we've already read it
        s_szBuf = NULL;
      }
      else {
        s_szBuf = GlobalConfigFile();
      }
    }
#endif  // WIN16/32
#endif  // UNIX/Win

  return s_szBuf;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

// parse the stream and create all groups and entries found in it
Bool FileConfig::readStream(istream *istr, ConfigGroup *pRootGroup)
{
  char szBuf[APPCONF_STRBUFLEN];

  m_pCurGroup = pRootGroup == NULL ? m_pRootGroup : pRootGroup;

  m_uiLine = 1;

  for (;;) {
    istr->getline(szBuf, APPCONF_STRBUFLEN, '\n');
    if ( istr->eof() ) {
    // last line may contain text also
    // (if it's not terminated with '\n' EOF is returned)
      return parseLine(szBuf);
    }

    if ( !istr->good() || !parseLine(szBuf) )
      return FALSE;      

    m_uiLine++;
  }
}

// parses one line of config file, returns FALSE on error
Bool FileConfig::parseLine(const char *psz)
{
  size_t len; // temp var

  const char *pStart = psz;
  
  // eat whitespace
  while ( isspace(*psz) ) psz++;

  // ignore empty lines or comments
  if ( *psz == '#' || *psz == ';' || *psz == '\0' ) {
    if ( *pStart != '\0' )
      AppendCommentLine(pStart);
    //else
    //  AppendCommentLine();
    return TRUE;
  }

  if ( *psz == '[' ) {          // a new group
    const char *pEnd = ++psz;

    while ( *pEnd != ']' ) {
      if ( IsValid(*pEnd) ) {
        // continue reading the group name
        pEnd++;
      }
      else {
        if ( *pEnd != APPCONF_PATH_SEPARATOR ) {
          LogError(_("file '%s': unexpected character at line %d (missing ']'?)"),
		           m_szFullFileName, m_uiLine);
          return FALSE;
        }
      }
    }

    len = pEnd - psz;
    char *szGroup = new char[len + 2];
    szGroup[0] = APPCONF_PATH_SEPARATOR;   // always considered as abs path
    szGroup[1] = '\0';
    strncat(szGroup, psz, len);
    
    // will create it if doesn't yet exist
    setCurrentPath(szGroup);

    // was there a comment before it?
    if ( m_szComment != NULL ) {
      m_pCurGroup->SetComment(m_szComment);
      m_szComment = NULL;
    }
    
    delete [] szGroup;
    
    // are there any comments after it?
    Bool bComment = FALSE;
    for ( pStart = ++pEnd ; *pEnd != '\0'; pEnd++ ) {
      switch ( *pEnd ) {
        case '#':
        case ';':
          bComment = TRUE;
          break;
        
        case ' ':
        case '\t':
          // ignore whitespace ('\n' impossible here)
          break;
          
        default:
          if ( !bComment ) {
            LogWarning(_("file '%s', line %d: '%s' ignored after group header."), 
                       m_szFullFileName, m_uiLine, pEnd);
            return TRUE;
          }
      }
    }
      
    if ( bComment ) {
      AppendCommentLine(pStart);
    }
  }
  else {                        // a key
    const char *pEnd = psz;
    while ( IsValid(*pEnd) )
      pEnd++;

    len = pEnd - psz;
    char *szKey = new char[len + 1];
    strncpy(szKey, psz, len + 1);
    szKey[len] = '\0';

    while ( isspace(*pEnd) ) pEnd++;  // eat whitespace

    if ( *pEnd++ != '=' ) {
      LogError(_("file '%s': expected '=' at line %d."), 
               m_szFullFileName, m_uiLine);
      return FALSE;
    }

    while ( isspace(*pEnd) ) pEnd++;  // eat whitespace

    ConfigEntry *pEntry = m_pCurGroup->FindEntry(szKey);

    if ( pEntry == NULL ) {
      pEntry = m_pCurGroup->AddEntry(szKey);
    }
    /* gives erroneous warnings when the same key appears in
       both global and local config files. Would be nice to
       have something happen when the same key appears twice
       (or more) in a section though.
  
      else {
        LogWarning(_("key '%s' has more than one value."), szKey);
      }

    */

    if ( m_szComment != NULL ) {
      pEntry->SetComment(m_szComment);
      m_szComment = NULL;
    }
    
    char *szUnfilteredValue = filterIn(pEnd);
    pEntry->SetValue(szUnfilteredValue, m_bParsingLocal, TRUE);

    delete [] szUnfilteredValue;
    delete [] szKey;
  }

  return TRUE;
}

// ----------------------------------------------------------------------------
// read data (which are fully loaded in memory)
// ----------------------------------------------------------------------------

const char *FileConfig::readEntry(const char *szKey, 
                                  const char *szDefault) const
{
  ConfigEntry *pEntry = m_pCurGroup->FindEntry(szKey);

  if ( pEntry != NULL ) {
    if ( m_bExpandVariables )
      return pEntry->ExpandedValue();
    else
      return pEntry->Value();
  }

  if(m_bRecordDefaults)
     ((FileConfig *)this)->writeEntry(szKey,szDefault);

  // entry wasn't found
  return szDefault;
}

// ----------------------------------------------------------------------------
// functions which update data in memory
// ----------------------------------------------------------------------------

// create new group if this one doesn't exist yet
void FileConfig::changeCurrentPath(const char *szPath)
{
  // normalize path
  BaseConfig::changeCurrentPath(szPath);
  szPath = getCurrentPath();

  m_pCurGroup = m_pRootGroup;

  // special case of empty path
  if ( *szPath == '\0' )
    return;
    
  char   *szGroupName = NULL;
  size_t  len = 0;

  const char *pBegin = szPath;
  const char *pEnd   = pBegin + 1;

  do{//while ( *pEnd != '\0' ) {
    while ( *pEnd != '\0' && *pEnd != APPCONF_PATH_SEPARATOR )
      pEnd++;

    if ( (unsigned)(pEnd - pBegin) + 1 > len ) {
      // not enough space, realloc buffer
      len = pEnd - pBegin + 1;

      // also delete old one
      if ( szGroupName != NULL )
        delete [] szGroupName;

      szGroupName = new char[len];
    }

    strncpy(szGroupName, pBegin, len);
    szGroupName[len - 1] = '\0';

    ConfigGroup *pGroup = m_pCurGroup->FindSubgroup(szGroupName);
    
    if ( pGroup == NULL ) {
      // not found: insert new group in the list
      m_pCurGroup = m_pCurGroup->AddSubgroup(szGroupName);
    }
    else {
      m_pCurGroup = pGroup;
    }

    if ( *pEnd == APPCONF_PATH_SEPARATOR ) {
      pBegin = ++pEnd;
    }
  }while ( *pEnd != '\0' );

  if ( szGroupName != NULL )
    delete [] szGroupName;
}

// set value of a key, creating it if doesn't yet exist
Bool FileConfig::writeEntry(const char *szKey, const char *szValue)
{
  ConfigEntry *pEntry = m_pCurGroup->FindEntry(szKey);
  if ( pEntry == NULL ) {
    pEntry = m_pCurGroup->AddEntry(szKey);
  }

  pEntry->SetValue(szValue);

  return TRUE;
}

// delete entry and the parent group if it's the last entry in this group
Bool FileConfig::deleteEntry(const char *szKey)
{
  Bool bDeleted = m_pCurGroup->DeleteEntry(szKey);

  DeleteIfEmpty();

  return bDeleted;
}

// ----------------------------------------------------------------------------
// functions which update data on disk (or wherever...)
// ----------------------------------------------------------------------------

Bool FileConfig::flush(Bool bCurrentOnly)
{
  ConfigGroup *pRootGroup = bCurrentOnly ? m_pCurGroup : m_pRootGroup;

  if ( m_pRootGroup->IsDirty() ) {
    const char *tmp = LocalConfigFile();
    fstream outStream(tmp, ios::out|ios::trunc);
    
    Bool bOk = pRootGroup->flush(&outStream);
    
    // special case: [comment and blank] lines following the last entry
    // were not yet written to the file, do it now.
    if ( m_szComment != NULL ) {
      outStream << m_szComment;
    }
    
    outStream.sync();
    
    return bOk;
  }
  else
    return TRUE;
}

Bool FileConfig::flush(ostream *oStream, Bool bCurrentOnly)
{
  ConfigGroup *pRootGroup = bCurrentOnly ? m_pCurGroup : m_pRootGroup;

  if ( m_pRootGroup->IsDirty() ) {
    
    Bool bOk = pRootGroup->flush(oStream);
    
    // special case: [comment and blank] lines following the last entry
    // were not yet written to the file, do it now.
    if ( m_szComment != NULL ) {
      *oStream << m_szComment;
    }
    
    return bOk;
  }
  else
    return TRUE;
}

// ----------------------------------------------------------------------------
// Comments management
// ## another kludge: this function is always called, but does nothing when
//    parsing global file (we can't write these comments back anyhow)
// ----------------------------------------------------------------------------
void FileConfig::AppendCommentLine(const char *szComment)
{
  if ( !m_bParsingLocal )
    return;

  size_t len = Strlen(m_szComment) + strlen(szComment) + 1;
    
  char *szNewComment = new char[len + 1];
  if ( m_szComment == NULL ) {
    szNewComment[0] = '\0';
  }
  else {
    strcpy(szNewComment, m_szComment);
    delete [] m_szComment;
  }  
  strcat(szNewComment, szComment);
  strcat(szNewComment, "\n");
  
  m_szComment = szNewComment;
}

// ----------------------------------------------------------------------------
// Enumeration of entries/subgroups
// ----------------------------------------------------------------------------

BaseConfig::Enumerator *FileConfig::enumSubgroups() const
{
  size_t nGroups = 0;
  ConfigGroup *pGroup = m_pCurGroup->Subgroup();
  while ( pGroup != NULL ) {
    nGroups++;
    pGroup = pGroup->Next();
  }

  BaseConfig::Enumerator *pEnum = new Enumerator(nGroups, FALSE);

  pGroup = m_pCurGroup->Subgroup();
  for ( size_t n = 0; n < nGroups; n++ ) {
    pEnum->AddString(pGroup->Name());
    pGroup = pGroup->Next();
  }

  return pEnum;
}

BaseConfig::Enumerator *FileConfig::enumEntries() const
{
  size_t nEntries = 0;
  ConfigEntry *pEntry = m_pCurGroup->Entries();
  while ( pEntry != NULL ) {
    nEntries++;
    pEntry = pEntry->Next();
  }

  BaseConfig::Enumerator *pEnum = new Enumerator(nEntries, FALSE);

  pEntry = m_pCurGroup->Entries();
  for ( size_t n = 0; n < nEntries; n++ ) {
    pEnum->AddString(pEntry->Name());
    pEntry = pEntry->Next();
  }

  return pEnum;
}

// ============================================================================
// implementation of the class RegistryConfig
// ============================================================================
#ifdef  __WIN32__

// ----------------------------------------------------------------------------
// ctor & dtor
// ----------------------------------------------------------------------------

// ctor opens our root key under HKLM
RegistryConfig::RegistryConfig(const char *szRootKey)
{
  static const char szPrefix[] = "Software";

  char *szKey = new char[strlen(szPrefix) + Strlen(szRootKey) + 2];
  strcpy(szKey, szPrefix);
  strcat(szKey, "\\");
  strcat(szKey, szRootKey);

  // don't create if doesn't exist
  LONG lRc = RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &m_hGlobalRootKey);
  m_bOk = lRc == ERROR_SUCCESS;
  if ( !m_bOk )
    m_hGlobalRootKey = NULL;

  m_hGlobalCurKey = m_hGlobalRootKey;

  // create if doesn't exist
  lRc = RegCreateKey(HKEY_CURRENT_USER, szKey, &m_hLocalRootKey);
  m_hLocalCurKey = m_hLocalRootKey;
  if ( !m_bOk )
    m_bOk = lRc;

  m_pLastRead = NULL;

  delete [] szKey;
}

// free resources
RegistryConfig::~RegistryConfig()
{
  if ( m_pLastRead != NULL )
    delete [] m_pLastRead;

  if ( m_hGlobalRootKey != NULL )
    RegCloseKey(m_hGlobalRootKey);
  if ( m_hLocalRootKey != NULL )
    RegCloseKey(m_hLocalRootKey);

  // don't close same key twice
  if ( m_hGlobalCurKey != NULL && m_hGlobalCurKey != m_hGlobalRootKey )
    RegCloseKey(m_hGlobalCurKey);
  if ( m_hLocalCurKey != NULL && m_hLocalCurKey != m_hLocalRootKey )
    RegCloseKey(m_hLocalCurKey);
}

// ----------------------------------------------------------------------------
// read/write entry - no buffering, data goes directly to the registry
// ----------------------------------------------------------------------------

// helper for readEntry which needs to read both the global and local ones
// ### MT unsafe because of m_pLastRead
const char *RegistryConfig::ReadValue(void *hKey, const char *szValue) const
{
  // get size of data first
  DWORD dwSize;
  LONG lRc = RegQueryValueEx(hKey, szValue, 0, NULL, NULL, &dwSize);

  if ( lRc != ERROR_SUCCESS )
    return NULL;

  char *szBuffer = new char[dwSize];
  DWORD dwType;
  lRc = RegQueryValueEx(hKey, szValue, 0, &dwType, 
                        (unsigned char *)szBuffer, &dwSize);

  if ( lRc != ERROR_SUCCESS ) {
    // it's not normal for it to fail now (i.e. after the first call was ok)
    LogWarning(_("can't query the value '%s' (%s)."), szValue, SysError());

    return NULL;
  }

  // always automatically expand strings of this type
  if ( dwType == REG_EXPAND_SZ ) {
    // first get length
    DWORD dwLen = ExpandEnvironmentStrings(szBuffer, NULL, 0);    

    // alloc memory and expand
    char *szBufferExp = new char[dwLen + 1];
    ExpandEnvironmentStrings(szBuffer, szBufferExp, dwLen + 1);

    // copy to the original buffer
    delete [] szBuffer;
    szBuffer = szBufferExp;
  }

  // free old pointer (last thing we returned from here)
  if ( m_pLastRead != NULL )
    delete [] m_pLastRead;

  // const_cast
  ((RegistryConfig *)this)->m_pLastRead = filterIn((const char *)szBuffer);
  delete [] szBuffer;

  return m_pLastRead;
}

// read value from the current key
// first try the user's setting, than the system.
const char *RegistryConfig::readEntry(const char *szKey, 
                                      const char *szDefault) const
{
  const char *pRetValue = ReadValue(m_hLocalCurKey, szKey);

  // if no local value exist and current path exists in global part, try it
  if ( pRetValue == NULL && m_hGlobalCurKey != NULL )
    pRetValue = ReadValue(m_hGlobalCurKey, szKey);

  if(pRetValue != NULL)
     return pRetValue;
  else {
    if ( m_bRecordDefaults && szDefault != NULL )
      const_cast<RegistryConfig *>(this)->writeEntry(szKey, szDefault);
    return szDefault;
  }
}

// write (or create) value under current key
// NB: only to user's part
Bool RegistryConfig::writeEntry(const char *szKey, const char *szValue)
{
  char *szFiltered = filterOut(szValue);

  long lRc = RegSetValueEx(m_hLocalCurKey, szKey, 0, REG_SZ, 
                           (unsigned char *)szFiltered, 
                           Strlen(szFiltered) + 1);

  delete [] szFiltered;

  return lRc == ERROR_SUCCESS;
}

// delete the key and the key which contains it if this is the last subkey
Bool RegistryConfig::deleteEntry(const char *szKey)
{
  Bool bDeleted = RegDeleteValue(m_hLocalCurKey, szKey) == ERROR_SUCCESS;
  if ( !bDeleted ) {
    // # don't output error message if the key is not found?
    LogWarning(_("can't delete entry '%s': %s"), szKey, SysError());
  }

  // even if there was an error before, still check if the key is not empty:
  // otherwise an attempt to delete a non existing entry would _create_ a
  // new subkey!
  while ( KeyIsEmpty(m_hLocalCurKey) ) {
    // delete this key
    const char *szPath = getCurrentPath();
    const char *szKeyName = strrchr(szPath, APPCONF_PATH_SEPARATOR);
    if ( szKeyName == NULL )
      szKeyName = szPath;
    else
      szKeyName++;  // to skip APPCONF_PATH_SEPARATOR

    // must copy it because the call to changeCurrentPath will change it!
    char *aszSubkey = new char[strlen(szKeyName) + 1];
    strcpy(aszSubkey, szKeyName);

    // don't delete root key
    if ( *szKeyName != '\0' ) {
      changeCurrentPath("..");
      if ( RegDeleteKey(m_hLocalCurKey, aszSubkey) != ERROR_SUCCESS )
        LogWarning(_("can't delete key '%s': %s"), aszSubkey, SysError());
    }

    delete [] aszSubkey;
  }

  return bDeleted;
}

// ----------------------------------------------------------------------------
// Enumeration of entries/subgroups (both system and user)
// ### this code is messy :-( The problem is that we try to enumerate both the
//     global and local keys at once, but we don't want keys common to both
//     branches appear twice, hence we check for each global key that it 
//     doesn't appear locally at the end (very inefficient too).
// ----------------------------------------------------------------------------

BaseConfig::Enumerator *RegistryConfig::enumSubgroups() const
{
  DWORD dwKeysLocal,   dwKeysGlobal,    // number of subkeys
        dwKeyLenLocal, dwKeyLenGlobal;  // length of the longest subkey

  long lRc = RegQueryInfoKey(m_hLocalCurKey, NULL, NULL, NULL, 
                             &dwKeysLocal, &dwKeyLenLocal, 
                             NULL, NULL, NULL, NULL, NULL, NULL);

  assert( lRc == ERROR_SUCCESS );  // it should never fail here

  if ( m_hGlobalCurKey == NULL ) {
    // key doesn't exist
    dwKeysGlobal = dwKeyLenGlobal = 0;
  }
  else {
    lRc = RegQueryInfoKey(m_hGlobalCurKey, NULL, NULL, NULL, &dwKeysGlobal, 
                          &dwKeyLenGlobal, NULL, NULL, NULL, NULL, NULL, NULL);

    assert( lRc == ERROR_SUCCESS );  // it should never fail here
  }

  DWORD dwKeys = dwKeysLocal + dwKeysGlobal;
  DWORD dwKeyLen = max(dwKeyLenLocal, dwKeyLenGlobal);

  BaseConfig::Enumerator *pEnum = new Enumerator((size_t)dwKeys, TRUE);

  char *szKey;
  HKEY hKey = m_hGlobalCurKey;
  for ( DWORD dwKey = 0; dwKeys > 0; dwKey++, dwKeys-- ) {
    if ( dwKeys == dwKeysLocal ) {
      // finished with global keys, now for local ones
      hKey = m_hLocalCurKey;
      dwKey = 0;
    }

    szKey = new char[dwKeyLen + 1];
    if ( RegEnumKey(hKey, dwKey, szKey, dwKeyLen + 1) != ERROR_SUCCESS ) {
      delete [] szKey;
      break;
    }

    pEnum->AddString(szKey);  // enumerator will delete it
  }

  pEnum->MakeUnique();

  return pEnum;
}

BaseConfig::Enumerator *RegistryConfig::enumEntries() const
{
  DWORD dwEntriesLocal, dwEntriesGlobal,    // number of values
        dwEntryLenLocal, dwEntryLenGlobal;  // length of the longest value name

  long lRc = RegQueryInfoKey(m_hLocalCurKey, NULL, NULL, NULL, 
                             NULL, NULL, NULL, 
                             &dwEntriesLocal, &dwEntryLenLocal, 
                             NULL, NULL, NULL);

  assert( lRc == ERROR_SUCCESS );  // it should never fail here

  if ( m_hGlobalCurKey == NULL ) {
    // key doesn't exist
    dwEntriesGlobal = dwEntryLenGlobal = 0;
  }
  else {
    lRc = RegQueryInfoKey(m_hGlobalCurKey, 
                          NULL, NULL, NULL, 
                          NULL, NULL, NULL, 
                          &dwEntriesGlobal, 
                          &dwEntryLenGlobal, 
                          NULL, NULL, NULL);

    assert( lRc == ERROR_SUCCESS );  // it should never fail here
  }

  DWORD dwEntries = dwEntriesLocal + dwEntriesGlobal;
  DWORD dwEntryLen = max(dwEntryLenLocal, dwEntryLenGlobal);

  BaseConfig::Enumerator *pEnum = new Enumerator((size_t)dwEntries, TRUE);

  char *szVal;
  DWORD dwLen;
  HKEY  hKey = m_hGlobalCurKey;
  for ( DWORD dwEntry = 0; dwEntries > 0; dwEntry++, dwEntries-- ) {
    if ( dwEntries == dwEntriesLocal ) {
      // finished with global keys, now for local ones
      hKey = m_hLocalCurKey;
      dwEntry = 0;
    }

    dwLen = dwEntryLen + 1;
    szVal = new char [dwLen];
    lRc = RegEnumValue(hKey, dwEntry, szVal, &dwLen, 0, NULL, NULL, NULL);
    if ( lRc != ERROR_SUCCESS ) {
      delete [] szVal;
      break;
    }

    pEnum->AddString(szVal);    // enumerator will delete it
  }

  pEnum->MakeUnique();

  return pEnum;
}

// ----------------------------------------------------------------------------
// change current key
// ----------------------------------------------------------------------------

// only in user's registry hive (not in global one)
void RegistryConfig::changeCurrentPath(const char *szPath)
{
  // normalize path
  BaseConfig::changeCurrentPath(szPath);
  szPath = getCurrentPath();

  // special case because RegCreateKey(hKey, "") fails!
  if ( *szPath == '\0' ) {
    if ( m_hLocalCurKey != m_hLocalRootKey ) {
      RegCloseKey(m_hLocalCurKey);
      m_hLocalCurKey = m_hLocalRootKey;
    }

    if ( m_hGlobalCurKey != m_hGlobalRootKey ) {
      RegCloseKey(m_hGlobalCurKey);
      m_hGlobalCurKey = m_hGlobalRootKey;
    }

    return;
  }

  char *szRegPath = new char[Strlen(szPath) + 1];
  strcpy(szRegPath, szPath);

  // replace all APPCONF_PATH_SEPARATORs with '\'
  char *pcSeparator = szRegPath;
  while ( pcSeparator != NULL ) {
    pcSeparator = strchr(pcSeparator, APPCONF_PATH_SEPARATOR);
    if ( pcSeparator != NULL )
      *pcSeparator = '\\';
  }

  HKEY hOldKey = m_hLocalCurKey;

  if ( RegCreateKey(m_hLocalRootKey, szRegPath, 
                    &m_hLocalCurKey) == ERROR_SUCCESS ) {
    // at any time, we keep open two keys
    // here we check that we don't close them
    if ( (m_hLocalCurKey != hOldKey) && (hOldKey != m_hLocalRootKey) )
      RegCloseKey(hOldKey);
  }
  else {
    LogWarning(_("can not open key '%s' (%s)."), szPath, SysError());
  }

  if ( m_hGlobalCurKey != 0 ) {
    hOldKey = m_hGlobalCurKey;
    if ( RegOpenKey(m_hGlobalRootKey, szRegPath, 
                    &m_hGlobalCurKey) == ERROR_SUCCESS ) {
      if ( (m_hGlobalCurKey != hOldKey) && (hOldKey != m_hGlobalRootKey) )
        RegCloseKey(hOldKey);
    }
    else {
      // no such key
      if ( m_hGlobalCurKey != m_hGlobalRootKey )
        RegCloseKey(m_hGlobalCurKey);
      m_hGlobalCurKey = NULL;
    }
  }

  delete [] szRegPath;
}

// ----------------------------------------------------------------------------
// helper function: return TRUE if the key has no subkeys/values
// ----------------------------------------------------------------------------
Bool RegistryConfig::KeyIsEmpty(void *hKey)
{
  DWORD dwKeys, dwValues;
  long lRc = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwKeys, NULL, NULL, 
                             &dwValues, NULL, NULL, NULL, NULL);

  return (lRc == ERROR_SUCCESS) && (dwValues == 0) && (dwKeys == 0);
}

#endif  // WIN32
