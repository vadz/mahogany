///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Mdefaults.h: Mahogany options and their default values
// Purpose:     functions for reading the value of an option from Profile
// Author:      Karsten Ballüder
// Modified by: Vadim Zeitlin at 22.08.01 to use MOption class
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef MDEFAULTS_H
#define MDEFAULTS_H

#include "Mcallbacks.h"
#include "Mversion.h"
#include "Profile.h"

/** @name The sections of the configuration file. */
///@{

/**
  The section in the global configuration file used for storing
  profiles (without trailing '/')
*/
#ifndef M_PROFILE_CONFIG_SECTION
#  ifdef OS_WIN
#     define   M_PROFILE_CONFIG_SECTION   "/Profiles"
#     define   M_IDENTITY_CONFIG_SECTION  "/Ids"
#     define   M_FILTERS_CONFIG_SECTION   "/Filters"
#     define   M_FRAMES_CONFIG_SECTION    "/Frames"
#     define   M_TEMPLATES_CONFIG_SECTION "/Templates"
#  else  // Unix
#     define   M_PROFILE_CONFIG_SECTION   "/M/Profiles"
#     define   M_IDENTITY_CONFIG_SECTION  "/M/Ids"
#     define   M_FILTERS_CONFIG_SECTION   "/M/Filters"
#     define   M_FRAMES_CONFIG_SECTION    "/M/Frames"
#     define   M_TEMPLATES_CONFIG_SECTION "/M/Templates"
#  endif // Unix/Win

   // these are used for remote IMAP sync: we have to use the same format for
   // the remote settings and we choose to use Unix one
#  define   M_PROFILE_CONFIG_SECTION_UNIX   "/M/Profiles"
#  define   M_IDENTITY_CONFIG_SECTION_UNIX  "/M/Ids"
#  define   M_FILTERS_CONFIG_SECTION_UNIX   "/M/Filters"
#  define   M_FRAMES_CONFIG_SECTION_UNIX    "/M/Frames"
#  define   M_TEMPLATES_CONFIG_SECTION_UNIX "/M/Templates"
#endif

/**
  The section in the global configuration file used for storing
  folder profiles (without trailing '/')
*/
#ifndef M_FOLDER_CONFIG_SECTION
#  define   M_FOLDER_CONFIG_SECTION   M_PROFILE_CONFIG_SECTION
#endif

/**
  The section in the global configuration file used for storing
  control settings.
*/
#define M_SETTINGS_CONFIG_SECTION       "Settings"

/// the root path for all ADB editor config entries
#define ADB_CONFIG_PATH "AdbEditor"

/** @name Keys where the template for messages of given type is stored */
//@{
#define MP_TEMPLATE_NEWMESSAGE   "NewMessage"
#define MP_TEMPLATE_NEWARTICLE   "NewArticle"
#define MP_TEMPLATE_REPLY        "Reply"
#define MP_TEMPLATE_FOLLOWUP     "Followup"
#define MP_TEMPLATE_FORWARD      "Forward"
//@}

//@}

// under Windows 12pt font looks too huge, letters are like in a book for
// children
#ifdef OS_WIN
#  define DEFAULT_FONT_SIZE 10l
#else
#  define DEFAULT_FONT_SIZE 12l
#endif

/** @name User level: novice, intermidiate, expert */
//@{
enum
{
   /// minimal user level, prefer simplicity over flexibility
   M_USERLEVEL_NOVICE,
   /// advanced user, more configurability features available
   M_USERLEVEL_ADVANCED,
   /// expert user, all features available
   M_USERLEVEL_EXPERT,
   /// for the developers themselves
   M_USERLEVEL_WIZARD
};
//@}

/** @name Levels of  interaction, do something or not?

    NB: these values can't be changed as they are written to (and read from)
        the profile and so this would create a compatibility problem
 */
//@{
enum
{
   /// never do this action
   M_ACTION_NEVER,
   /// ask user if he wants it
   M_ACTION_PROMPT,
   /// don't ask user, do it
   M_ACTION_ALWAYS
};
//@}

/// the positions of the standard folders in the tree
enum MFolderIndex
{
   // system folders
   MFolderIndex_Inbox,
   MFolderIndex_NewMail,
   MFolderIndex_SentMail,
   MFolderIndex_Trash,
   MFolderIndex_Outbox,
   MFolderIndex_Draft,

   // initial servers
   MFolderIndex_IMAP,
   MFolderIndex_POP,
   MFolderIndex_NNTP,

   // anything else goes here
   MFolderIndex_Max
};

// ----------------------------------------------------------------------------
// MOptionValue
// ----------------------------------------------------------------------------

class MOptionValue
{
public:
   MOptionValue() { m_kind = None; }

   void Set(long number)
   {
      m_kind = Number;
      m_number = number;
   }

   void Set(const String& s)
   {
      m_kind = Text;
      m_string = s;
   }

   long GetNumberValue() const
   {
      ASSERT_MSG( m_kind == Number, "MOptionValue type mismatch" );

      return m_number;
   }

   bool GetBoolValue() const
   {
      return GetNumberValue() != 0;
   }

   String GetTextValue() const
   {
      ASSERT_MSG( m_kind == Text, "MOptionValue type mismatch" );

      return m_string;
   }

   operator long() const { return GetNumberValue(); }
   operator String() const { return GetTextValue(); }

private:
   long m_number;
   String m_string;

   // type of the value we stor
   enum
   {
      None,
      Number,
      Text
   } m_kind;
};

inline bool operator==(const MOptionValue& v, long l)
{
   return l == v.GetNumberValue();
}

inline bool operator==(long l, const MOptionValue& v)
{
   return l == v.GetNumberValue();
}

inline bool operator!=(const MOptionValue& v, long l)
{
   return l != v.GetNumberValue();
}

inline bool operator!=(long l, const MOptionValue& v)
{
   return l != v.GetNumberValue();
}

inline bool operator==(const MOptionValue& v, const String& s)
{
   return s == v.GetTextValue();
}

inline bool operator==(const String& s, const MOptionValue& v)
{
   return v == s;
}

inline bool operator!=(const MOptionValue& v, const String& s)
{
   return s != v.GetTextValue();
}

inline bool operator!=(const String& s, const MOptionValue& v)
{
   return v != s;
}

inline String operator+(const MOptionValue& v, const String& s)
{
   return v.GetTextValue() + s;
}

inline void operator+=(String& s, const MOptionValue& v)
{
   s += v.GetTextValue();
}

// ----------------------------------------------------------------------------
// MOption
// ----------------------------------------------------------------------------

// We use MOption to avoid having to put all names and values in this file.
// Although this was how it used to be done, it becamse unmaintanable as
// adding any option (or even modifying the default value for it) resulted in
// the recompilation of [almost] all files.
//
// So now we keep the values themselves in a separate file and force the user
// code to forward declare the options it uses itself. This is more trouble
// than just including a header, but is much better from header
// interdependcies point of view

/// an opaque handle to an option
class MOption
{
public:
   MOption();

   inline const char *GetName() const;

   // implicit conversions required for backwards compatibility
   inline operator const char *() const;
   inline operator String() const;

   // for internal use only!
   int GetId() const { return m_id; }

private:
   int m_id;
};

// ----------------------------------------------------------------------------
// functions to get option info
// ----------------------------------------------------------------------------

/// get the name of the option in profile
extern const char *GetOptionName(const MOption opt);

/// is it a numeric option or a string one?
extern bool IsNumeric(const MOption opt);

/// get the default value of a numeric option
extern long GetNumericDefault(const MOption opt);

/// get the default value of a string option
extern const char *GetStringDefault(const MOption opt);

// ----------------------------------------------------------------------------
// MOption inline methods implementation (couldn't be done before)
// ----------------------------------------------------------------------------

inline const char *MOption::GetName() const { return GetOptionName(*this); }
inline MOption::operator const char *() const { return GetOptionName(*this); }
inline MOption::operator String() const { return GetOptionName(*this); }

inline String operator+(const String& s, const MOption& opt)
{
   return s + opt.GetName();
}

// ----------------------------------------------------------------------------
// get the options value
// ----------------------------------------------------------------------------

extern MOptionValue GetOptionValue(Profile *profile, const MOption opt);

extern long GetNumericOptionValue(Profile *profile, const MOption opt);

// from the given profile
#define READ_CONFIG(profile, opt) GetOptionValue(profile, opt)

// from the main profile
#define READ_APPCONFIG(opt) GetOptionValue(mApplication->GetProfile(), opt)

// with (sometimes) required cast to bool
#define READ_CONFIG_BOOL(profile, opt) \
   (GetNumericOptionValue(profile, opt) != 0l)
#define READ_APPCONFIG_BOOL(opt) \
   READ_CONFIG_BOOL(mApplication->GetProfile(), opt)

// with (sometimes) required cast to string
#define READ_CONFIG_TEXT(profile, opt) READ_CONFIG(profile, opt).GetTextValue()
#define READ_APPCONFIG_TEXT(opt) READ_APPCONFIG(opt).GetTextValue()

// this allows to use READ_CONFIG() in the old way, i.e. just define 2 macros
// FOO and FOO_D and call READ_CONFIG_MOD(FOO) - this is currently used in the
// modules as they can't define their own MOption values hence the synonym (We
// can later replace READ_CONFIG_MOD with something better, so keep the name
// separate)
#define READ_CONFIG_PRIVATE(p, opt) p->readEntry(opt, opt##_D)
#define READ_CONFIG_MOD(p, opt) p->readEntry(opt, opt##_D)

#endif // MDEFAULTS_H

