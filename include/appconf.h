/*-*- c++ -*-*****************************************************************\
 * Project:   CppLib: C++ library for Windows/UNIX platfroms                 *
 * File:      config.h - Config class, .INI/.rc file manager                 *
 *---------------------------------------------------------------------------*
 * Language:  C++                                                            *
 * Platfrom:  any (tested under Windows NT)                                  *
 *---------------------------------------------------------------------------*
 * (c) Karsten Ballüder & Vadim Zeitlin                                      *
 *     Ballueder@usa.net  Vadim.zeitlin@dptmaths.ens-cachan.fr               *
 *---------------------------------------------------------------------------*
 * $Id$                    *
 *---------------------------------------------------------------------------*
 * Classes:                                                                  *
 *  BaseConfig      - ABC for class that manage tree organized config info   *
 *  FileConfig      - implementation of BaseConfig based on disk files       *
 *  RegistryConfig  - implementation of BaseConfig based Win32 registry      *
 *---------------------------------------------------------------------------*
 * To do:                                                                    *
 *  - derive wxConfig                                                        *
 *  - EXTENSIVE ERROR CHECKING (none done actually)                          *
 *  - derive IniConfig for standard dumb Windows INI files using Win APIs    *
 *---------------------------------------------------------------------------*
 * History:                                                                  *
 *  25.10.97  adapted from wxConfig by Karsten Ballüder                      *
 *  29.10.97  FileConfig now saves comments back (VZ)                        *
 *  30.10.97  Immutable entries support added (KB)                           *
 *  01.11.97  Fixed NULL pointer access in BaseConfig::setCurrentPath (KB)   *
 *            Replaced filterIn() with original function from wxConfig,      *
 *            doing environment variable expansion, too (KB)                 *
 *            Made parsing case insensitive (Compile time option) (KB)       *
 *  02.11.97  Added deleteEntry function (VZ)                                *
 *  08.11.97  Environment variable expansion on demand (VZ)                  *
 *            RegistryConfig::changeCurrentPath added (VZ)                   *
 *            AppConf defined to be used instead of File/RegistryConfig      *
 *  18.01.98  Several fixes and changes to FileConfig (KB)                   *
 *            Added recordDefaults() method (KB)                             *
\*****************************************************************************/

#ifndef   _APPCONF_H
#define   _APPCONF_H


/**@name AppConf - Configuration entry management 
 * 
 *
 * <b>Copyright</b><br>
 * This library is free software; you can redistribute it and/or      
 * modify it under the terms of the GNU Library General Public        
 * License as published by the Free Software Foundation; either       
 * version 2 of the License, or (at your option) any later version.   
 * <p>                                                                   
 * This library is distributed in the hope that it will be useful,    
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.                   
 * <p>                                                                   
 * You should have received a copy of the GNU Library General Public  
 * License along with this library; if not, write to the Free         
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
 * <p>                                                                   
 *
 *These classes support easy to use management of configuration settings.
 *On Unix systems these settings get written to configuration files, of the
 *type used by Windows (.INI) and KDE (.lnk). On Win32 systems (Windows NT
 *or 95), the settings get written to the registry database.<br>
 *Therefor AppConfig provides a consistent API for configuration management
 *across both platforms. The API is defined by the BaseConfig virtual base
 *class and two implemenations, FileConfig and RegistryConfig are derived
 *from this. Nevertheless, you can also use FileConfig under Windows if
 *you need it for some special purpose.
 *<p>
 *The class AppConfig is defined which is the 'native' implementation on
 *each platform. 
 *<p>
 *Also, if the gettext() library function is available (use -lintl on
 *Unix systems), AppConfig can be compiled with -DAPPCONF_USE_GETTEXT=1
 *to support multiple languages in its error messages.
 *Precompiled message catalogues for German and French are included,
 *copy them to the appropriate directories under Unix 
 *(e.g. /usr/share/locale/fr/LC_MESSAGES/appconf.mo or
 * /usr/share/locale/de/LC_MESSAGES/appconf.mo).
 *<p>
 *Two simple programs demonstrating the use of AppConfig are included.
 *<p>
 *Full documentation of the classes is included in HTML format in the
 *doc directory. Please use your web browser to view it.<br>
 *The LaTeX documentation appconf.tex in the same directory contains a
 *few errors (because it was auto-generated with doc++), but is usable.
 *Just keep pressing Enter when LaTeX complains. An A4 PostScript version
 *is also included.
 *<p>
 *Feel free to contact us with any questions or suggestions. If you use
 *it in your programs, we would be pleased to hear about it.
 * @memo	A configuration management system for Unix and Windows.
 * @author	Karsten Ball&uuml;der and Vadim Zeitlin
 */
//@{

#if APPCONF_USE_CONFIG_H
#	include <config.h>
#	ifdef	HAVE_LIBINTL
#		undef 	APPCONF_USE_GETTEXT
#		define	APPCONF_USE_GETTEXT	1
#	endif
#endif

#ifndef USE_IOSTREAMH
  #define USE_IOSTREAMH 1
#endif

#if     USE_IOSTREAMH
  #include  <iostream.h>
#else   // old C style headers
  #include  <iostream>
#endif  // new/old style headers

#include  <stdlib.h>

// make sure we have a Bool type
#ifndef Bool
#	define	Bool	int
#endif

//typedef   int Bool;
#ifndef    TRUE
# define    TRUE  1
# define    FALSE 0
#endif

// make sure we have NULL defined properly
#undef  NULL
#define NULL 0

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------


/**@name configuration options */
//@{

/// can we use gettext() for multi language support?

#ifndef	APPCONF_USE_GETTEXT
#		define	APPCONF_USE_GETTEXT	0
#else
#		define	APPCONF_DOMAIN	"appconf"
#endif

/// shall we be case sensitive in parsing variable names?
#ifndef APPCONF_CASE_SENSITIVE
#	define	APPCONF_CASE_SENSITIVE	0
#endif

/// separates group and entry names
#ifndef APPCONF_PATH_SEPARATOR
#	define   APPCONF_PATH_SEPARATOR     '/'
#endif

/// introduces immutable entries
#ifndef APPCONF_IMMUTABLE_PREFIX
#	define   APPCONF_IMMUTABLE_PREFIX   '!'
#endif

/// length for internal character array for expansion (e.g. for gcvt())
#ifndef APPCONF_STRBUFLEN
#	define   APPCONF_STRBUFLEN          1024
#endif

/// should we use registry instead of configuration files under Win32?
#ifndef	  APPCONF_WIN32_NATIVE
#	define APPCONF_WIN32_NATIVE 		1 	// default: TRUE
#endif

//@}

/**@name helper functions */
//@{
// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------
/** Replace environment variables ($SOMETHING) with their values.  The
   format is $VARNAME or ${VARNAME} where VARNAME contains
   alphanumeric characters and '_' only. '$' must be escaped ('\$') in
   order to be taken literally.
  @param  pointer to the string possibly contianing $VAR and/or ${VAR}
  @return the resulting string, caller must 'delete []' it
  @memo Performs environment variable substitution.  */
char *ExpandEnvVars(const char *psz);
//@}

/**@name Configuration Management Classes */
//@{

// ----------------------------------------------------------------------------
///	abstract base class config
// ----------------------------------------------------------------------------
class BaseConfig
{
public:
  /** @name Constructors and destructor */
  //@{
    /// default ctor
  BaseConfig();
    /// dtor
  virtual ~BaseConfig();
  //@}

  /** @name Set and retrieve current path */
  //@{
  /**
    Specify the new current path by its absolute name.
    @param szPath name of the group to search by default (created if !exists)
  */
  virtual void setCurrentPath(const char *szPath = "");

     /**
	Change the current path.
	Works like 'cd' and supports "..".
    @param szPath name of the group to search by default (created if !exists)
  */
  virtual void changeCurrentPath(const char *szPath = "");

  /**
    Query the current path.
    @return current '/' separated path
  */
  const char *getCurrentPath() const;
  /** 
    Resolve ".." and "/" in the path.
    @param  Start path (i.e. current directory)
    @param  Relative path (".." allowed) from start path
    @return Pointer to normalized path (caller should "delete []" it)
  */
  static char *normalizePath(const char *szStartPath, const char *szPath);
  //@}

   /** activates recording of default values
       @param enable TRUE to activate, FALSE to deactivate
   */
   void recordDefaults(Bool enable = TRUE);

  /**@name Enumeration of subgroups/entries */
  //@{
   /**
      This class allows access to an array of strings, which are entries or group names.
      @memo Class that supports enumeration.
      @see  enumSubgroups, enumEntries
    */
  class Enumerator
  {
  public:
    // if bOwnsStrings, it will delete them in dtor
    Enumerator(size_t nCount, Bool bOwnsStrings);
    ~Enumerator();

    // add a string
      // this one may be used only if bOwnsString was TRUE in ctor
    void AddString(const char *sz);
      // 
    void AddString(char *sz);
    
    // kludge: eliminate duplicate strings
    void MakeUnique();

    // accessors
      /// return number of elements
     size_t      Count()                   const
	{ return m_nCount;          } 
      /// return the element #nIndex
     const char *operator[](size_t nIndex) const
	{ return m_aszData[nIndex]; }	

  private:
    char    **m_aszData;
    size_t    m_nCount;
    Bool      m_bOwnsStrings;
  };
    /**
      Enumerate subgroups of the current group.
      Caller must delete the returned pointer.
      Example of usage:
      <br><pre>
      char **aszGroups;
      config.setCurrentPath("mygroup");
      BaseConfig::Enumerator *pEnum = config.enumSubgroups();
      size_t nGroups = pEnum->Count();
      for ( size_t n = 0; n < nGroups; n++ )
        cout << "Name of subgorup #" << n << " is " << (*pEnum)[n] << endl;
      delete pEnum;
      </pre>
      @param  Pointer to an array of strings which is allocated by the function
      @return Number of subgroups
      @see    Enumerator, setCurrentPath, enumEntries 
    */
  virtual Enumerator *enumSubgroups() const = 0;
    /** 
      Enumerate entries of the current group.
      @return Number of entries
      @see    Enumerator, enumSubgroups
    */
  virtual Enumerator *enumEntries() const = 0;
  //@}

  /** @name Key access */
  //@{
  /**
    Get the value of an entry, or the default value.
    @param szKey      The key to search for.
    @param szDefault  The default value to return if not found.
    @return The value of the key, or defval if not found
  */
  virtual const char *readEntry(const char *szKey, 
                                const char *szDefault = NULL) const = 0;

   /**
     Get the value of an entry, or the default value, interpreted as a
     long integer.
     @param szKey	The key to search for.
     @param Default	The default value to return if not found.
     @return The value of the key converted to long int or the default value.
   */
   int readEntry(const char *szKey, int Default) const;

   /**
     Get the value of an entry, or the default value, interpreted as a
     long integer.
     @param szKey	The key to search for.
     @param Default	The default value to return if not found.
     @return The value of the key converted to long int or the default value.
   */
   long int readEntry(const char *szKey, long int Default) const;

   /**
     Get the value of an entry, or the default value, interpreted as a
     double value.
     @param szKey	The key to search for.
     @param Default	The default value to return if not found.
     @return The value of the key converted to double or the default value.
   */
   double readEntry(const char *szKey, double Default) const;

   /**
    Set the value of an entry.
    @param szKey    The key whose value to change.
    @param szValue	The new value.
    @return TRUE on success, FALSE on failure
  */
  virtual Bool writeEntry(const char *szKey, const char *szValue) = 0;

  /**
    Set the value of an entry to a long int value.
    @param szKey    The key whose value to change.
    @param Value	The new value.
    @return TRUE on success, FALSE on failure
  */
   Bool writeEntry(const char *szKey, int Value);

   /**
    Set the value of an entry to a long int value.
    @param szKey    The key whose value to change.
    @param Value	The new value.
    @return TRUE on success, FALSE on failure
  */
   Bool writeEntry(const char *szKey, long int Value);

   /**
    Set the value of an entry to a double value.
    @param szKey    The key whose value to change.
    @param Value	The new value.
    @return TRUE on success, FALSE on failure
   */
   Bool writeEntry(const char *szKey, double Value);

   /**
    Delets the entry. Notice that there is intentionally no such function 
    as deleteGroup: the group is automatically deleted when it's last entry 
    is deleted.

    @memo   Deletes the entry.
    @param  szKey    The key to delete
    @return TRUE on success, FALSE on failure
  */
  virtual Bool deleteEntry(const char *szKey) = 0;
  //@}

  /** @name Other functions */
  //@{
    /// permanently writes changes, returns TRUE on success
   virtual Bool flush(Bool /* bCurrentOnly */ = FALSE)
      { return TRUE; } 
    /// returns TRUE if object was correctly initialized
   Bool isInitialized() const
      { return m_bOk; }	
  //@}

  /** 
    @name Filter functions.
    All key values should pass by these functions, derived classes should
    call them in their read/writeEntry functions.
    Currently, these functions only escape meta-characters (mainly spaces
    which would otherwise confuse the parser), but they could also be used 
    to encode/decode key values.
  */
  //@{
    /// should be called from writeEntry, returns pointer to dynamic buffer
  static char *filterOut(const char *szValue);
    /// should be called from readEntry, returns pointer to dynamic buffer
  static char *filterIn(const char *szValue);
   /// should environment variables be automatically expanded?
   void expandVariables(Bool bExpand = TRUE)
      { m_bExpandVariables = bExpand; } 
   /// do environment variables get automatically expanded?
   Bool doesExpandVariables(void) const
      { return m_bExpandVariables; } 
   //@}

protected:
  /// TRUE if ctor successfully initialized the object
  Bool  m_bOk;
  /// TRUE if environment variables are to be auto-expanded
  Bool  m_bExpandVariables;
  /// TRUE if default values are to be recorded
  Bool	m_bRecordDefaults; 
private:
  char *m_szCurrentPath;
};

// ----------------------------------------------------------------------------
/**
  FileConfig derives from BaseConfig and implements file based config class, 
  i.e. it uses ASCII disk files to store the information. These files are
  alternatively called INI, .conf or .rc in the documentation. They are 
  organized in groups or sections, which can nest (i.e. a group contains
  subgroups, which contain their own subgroups &c). Each group has some
  number of entries, which are "key = value" pairs. More precisely, the format 
  is: 
  <pre>
  # comments are allowed after either ';' or '#' (Win/UNIX standard)
  # blank lines (as above) are ignored
  # global entries are members of special (no name) top group
  written_for = wxWindows
  platform    = Linux
  # the start of the group 'Foo'
  [Foo]                           # may put comments like this also
  # following 3 lines are entries
  key = value
  another_key = "  strings with spaces in the beginning should be quoted, \
                   otherwise the spaces are lost"
  last_key = but you don't have to put " normally (nor quote them, like here)
  # subgroup of the group 'Foo'
  # (order is not important, only the name is: separator is '/', as in paths)
  [Foo/Bar]
  # entries prefixed with "!" are immutable, i.e. can't be changed if they are
  # set in the system wide .conf file
  !special_key = value
  bar_entry = whatever
  [Foo/Bar/Fubar]   # depth is (theoretically :-) unlimited
  # may have the same name as key in another section
  bar_entry = whatever not
  </pre>
  You {have read/write/delete}Entry functions (guess what they do) and also
  setCurrentPath to select current group. enum{Subgroups/Entries} allow you
  to get all entries in the config file (in the current group). Finally,
  flush() writes immediately all changed entries to disk (otherwise it would
  be done automatically in dtor)
  FileConfig manages not less than 2 config files for each program: global
  and local (or system and user if you prefer). Entries are read from both of
  them and the local entries override the global ones unless the latter is
  immutable (prefixed with '!') in which case a warning message is generated
  and local value is ignored. Of course, the changes are always written to local
  file only.
*/
// ----------------------------------------------------------------------------

class FileConfig : public BaseConfig
{
public:
  /** @name Constructors and destructor */
  //@{
    /**
      FileConfig will
      <ll>
      <li>1) read global config file (/etc/appname.conf under UNIX or 
             windir\<file>.ini under Windows) unless bLocalOnly is TRUE
      <li>2) read user's config file ($HOME/.appname or $HOME/.appname/config or %USERPROFILE%/file.ini
             under NT, same as global otherwise)
      <li>3) write changed entries to user's file, never to the system one.
      </ll>
      @memo   Ctor for FileConfig takes file name argument.
      @param  szName  Config file (or directory), default extension appended if not given
      @param  bLocalOnly  TRUE => don't look for system-wide config file
      @param  bFullPathGiven TRUE => take the filename as an absolute complete path (automatically sets bLocalOnly to TRUE)
      @param  bUseSubDir  TRUE => local configuration goes to $HOME/.<szName>/<szConfigFileName>
      @param  szConfigFileName if bUseSubDir is TRUE, use this as the name of the configuration file and szName as the directory name
    */
   FileConfig(const char *szName, Bool bLocalOnly = FALSE,
	      Bool bFullPathGiven = FALSE,
	      Bool bUseSubDir = FALSE, const char *szConfigFileName = "config");

   /**
      @memo   Ctor for FileConfig for reading from a stream
      @param  input stream to read from
    */
   FileConfig(istream *iStream);

   /**
      Another constructor, creating an empty object. For use with the
      readFile() method.
   */
   FileConfig(void);

  /**
      Notice that if you're interested in error code, you should use
      flush() function.
      @memo Dtor will save unsaved data.
    */
 ~FileConfig();
  //@}

   /**
      Like reading from a local configuration file, but takes a whole
      path as argument. No default configuration files used. Use with
      FileConfig() constructor.
   */
   void readFile(const char *szFileName);
   
  /** @name Implementation of inherited pure virtual functions */
  //@{
    ///
  const char *readEntry(const char *szKey, const char *szDefault = NULL) const;
    ///
  int readEntry(const char *szKey, int Default) const { return BaseConfig::readEntry(szKey, Default); } 
    ///
  long int readEntry(const char *szKey, long int Default) const      { return BaseConfig::readEntry(szKey, Default); } 
    ///
  double readEntry(const char *szKey, double Default) const      { return BaseConfig::readEntry(szKey, Default); } 
   ///
  Bool writeEntry(const char *szKey, const char *szValue);
   ///
  Bool writeEntry(const char *szKey, int Value)      { return BaseConfig::writeEntry(szKey, Value);} 
   ///
  Bool writeEntry(const char *szKey, long int Value)     { return BaseConfig::writeEntry(szKey, Value);} 
   ///
  Bool writeEntry(const char *szKey, double Value)     { return BaseConfig::writeEntry(szKey, Value); } 
   ///
  Bool deleteEntry(const char *szKey);
    ///
  Bool flush(Bool bCurrentOnly = FALSE);
   /// writes changes to ostream, returns TRUE on success
  Bool flush(ostream *oStream, Bool /* bCurrentOnly */ = FALSE);

   /// parses one line of config file
  Bool parseLine(const char *psz);

   
    ///
  void changeCurrentPath(const char *szPath = "");
  //@}

  /** 
    @name Enumeration 
    @see  BaseConfig::enumSubgroups, BaseConfig::enumEntries
  */
  //@{
    /// Enumerate subgroups of the current group
  Enumerator *enumSubgroups() const;
    /// Enumerate entries of the current group
  Enumerator *enumEntries() const;
  //@}

//protected: --- if FileConfig::ConfigEntry is not public, functions in
//               ConfigGroup such as Find/AddEntry can't return ConfigEntry*!
  class ConfigGroup;
  class ConfigEntry
  {
  private:
    ConfigGroup *m_pParent;     // group that contains us
    ConfigEntry *m_pNext;       // next entry (in linked list)
    char        *m_szName,      // entry name
                *m_szValue,     //       value
                *m_szExpValue,  // value with expanded env variables
                *m_szComment;   // optional comment preceding the entry
    Bool         m_bDirty,      // changed since last read?
                 m_bLocal,      // read from user's file or global one?
                 m_bImmutable;  // can be overriden locally?

  public:
    ConfigEntry(ConfigGroup *pParent, ConfigEntry *pNext, const char *szName);
   ~ConfigEntry();

    // simple accessors
     const char  *Name()         const	{ return m_szName;     } 
     const char  *Value()        const	{ return m_szValue;    } 
     const char  *Comment()      const	{ return m_szComment;  } 
     ConfigEntry *Next()         const	{ return m_pNext;      } 
     Bool         IsDirty()      const	{ return m_bDirty;     } 

    // expand environment variables and return the resulting string
    const char *ExpandedValue();

    // modify entry attributes
    void SetValue(const char *szValue, 
                  Bool bLocal = TRUE, 
                  Bool bFromFile = FALSE);
    void SetComment(char *szComment);
    void SetDirty(Bool bDirty = TRUE);
    void SetNext(ConfigEntry *pNext) { m_pNext = pNext; }
  };

protected:
  class ConfigGroup
  {
  private:
    ConfigEntry *m_pEntries,    // list of entries in this group
                *m_pLastEntry;  // last entry
    ConfigGroup *m_pSubgroups,  // list of subgroups
                *m_pLastGroup,  // last subgroup
                *m_pNext,       // next group at the same level as this one
                *m_pParent;     // parent group
    char        *m_szName,      // group's name
                *m_szComment;   // optional comment preceding this group
    Bool         m_bDirty;      // if FALSE => all subgroups are not dirty

  public:
    // ctors
    ConfigGroup(ConfigGroup *pParent, ConfigGroup *pNext, const char *szName);

    // dtor deletes all entries and subgroups also
    ~ConfigGroup();

    // simple accessors
     const char  *Name()     const	{ return m_szName;      } 
     const char  *Comment()  const	{ return m_szComment;   } 
     ConfigGroup *Next()     const	{ return m_pNext;       } 
     ConfigGroup *Parent()   const	{ return m_pParent;     } 
     ConfigGroup *Subgroup() const	{ return m_pSubgroups;  } 
     ConfigEntry *Entries()  const	{ return m_pEntries;    } 
     Bool         IsDirty()  const	{ return m_bDirty;      } 

    // full name ('/' separated path from top)
    // caller must free buffer
    char *FullName() const;

    // find entry/subgroup (NULL if not found)
    ConfigGroup *FindSubgroup(const char *szName) const;
    ConfigEntry *FindEntry   (const char *szName) const;

    // delete entry/subgroup, return FALSE if doesn't exist
    Bool DeleteSubgroup(const char *szName);
    Bool DeleteEntry   (const char *szName);

    // create new entry/subgroup returning pointer to newly created element
    ConfigGroup *AddSubgroup(const char *szName);
    ConfigEntry *AddEntry   (const char *szName);

    // will also recursively call parent's dirty flag
    void SetDirty(Bool bDirty = TRUE);
    
    // comment shouldn't be empty if this function is used
    void SetComment(char *szComment);
    
    // write dirty data
    Bool flush(ostream *ostr);
  };

  // delete current group if has no more entries/subgroups
  // NB: changes m_pCurGroup is returns TRUE (i.e. the group was deleted)
  Bool DeleteIfEmpty();

  ConfigGroup *m_pRootGroup,  // there is one and only one root group
              *m_pCurGroup;   // and also unique current (default) one

  // create all groups found in the strem under group pRootGroup or
  // m_pRootGroup if parameter is NULL
  // returns TRUE on success
  Bool readStream(istream *istr, ConfigGroup *pRootGroup = NULL);

  // construct global and local filenames based on the base file name
  // (which nevertheless may contain "/")
    // get the name of system-wide config file
  const char *GlobalConfigFile() const;
    // get the name of user's config file
  const char *LocalConfigFile() const;

  /// trivial initialization of member variables (common to all ctors)
  void Init();

private:
  char     *m_szFileName;        // config file name
   char     *m_szCFileName;	// if working with local subdir,
				// config file name
  const char *m_szFullFileName;  // full file name for error messages
  unsigned  m_uiLine;            // line number for error messages
  Bool      m_bParsingLocal;     // TRUE while parsing user's config file
  Bool      m_bUseSubDir;	// shall we use a subdirectory for config file?
   Bool	    m_bFullPathGiven;   // is m_szFileName the full path? 
  // we store in a variable all comments + whitespace preceding the
  // current entry or group in order to be able to store it later
  void      AppendCommentLine(const char *psz = NULL);
  char     *m_szComment;
};

// ----------------------------------------------------------------------------
/// RegistryConfig uses Win32 registry API to store configuration info
// ----------------------------------------------------------------------------
#ifdef  __WIN32__

class RegistryConfig : public BaseConfig
{
public:
  /** @name Constructors and destructor */
  //@{
    /** 
      szRootKey is the name of the top registry key (relative to HKLM\Software
      for system-wide settings and to HKCU\Software for user settings)
      @memo Ctor takes a string - root for our keys in registry.
     */
  RegistryConfig(const char *szRootKey);
    /// dtor frees the resources
 ~RegistryConfig();
  //@}

  /** @name Implementation of inherited pure virtual functions */
  //@{
    ///
   const char *readEntry(const char *szKey, const char *szDefault = NULL) const;
   ///
   int readEntry(const char *szKey, int Default) const      { return BaseConfig::readEntry(szKey, Default); } 
   ///
   long int readEntry(const char *szKey, long int Default) const      { return BaseConfig::readEntry(szKey, Default); } 
    ///
   double readEntry(const char *szKey, double Default) const      { return BaseConfig::readEntry(szKey, Default); } 

    ///
   Bool writeEntry(const char *szKey, const char *szValue);
    ///
   Bool writeEntry(const char *szKey, int Value)     { return BaseConfig::writeEntry(szKey, Value);} 
    ///
   Bool writeEntry(const char *szKey, long int Value)     { return BaseConfig::writeEntry(szKey, Value);} 
   ///
   Bool writeEntry(const char *szKey, double Value)     { return BaseConfig::writeEntry(szKey, Value); } 

    ///
  Bool deleteEntry(const char *szKey);
  //@}

  /** 
    @name Enumeration 
    @see  Enumerator, BaseConfig::enumSubgroups, BaseConfig::enumEntries
  */
  //@{
    /// Enumerate subgroups of the current group
  Enumerator *enumSubgroups() const;
    /// Enumerate entries of the current group
  Enumerator *enumEntries() const;
  //@}

  // intercept this function and change m_hCurrentKey
  virtual void changeCurrentPath(const char *szPath = "");

private:
  const char *ReadValue(void *hKey, const char *szValue) const;
  static Bool KeyIsEmpty(void *hKey);

  void *m_hGlobalRootKey,       // top of system settings
       *m_hLocalRootKey,        // top of user settings
       *m_hGlobalCurKey,        // current registry key (child of global root)
       *m_hLocalCurKey;         //                                local

  char *m_pLastRead;      // pointer to last read value (###)
};

#endif  // WIN32

/// AppConfig is mapped on the class most appropriate for the target platform
#if	  APPCONF_WIN32_NATIVE && defined(__WIN32__)
  typedef class RegistryConfig AppConfig;
#else
  typedef class FileConfig     AppConfig;
#endif

//@}

//@}
#endif  //_APPCONF_H

