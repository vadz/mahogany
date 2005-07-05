///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ConfigPrivate.h
// Purpose:     various internal classes used by Profile and AllConfigSources
// Author:      Vadim Zeitlin
// Creatd:      2005-07-04 (extracted from Profile.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2005 Vadim Zeitlin <vadim@zeitlins.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CONFIGPRIVATE_H_
#define _M_CONFIGPRIVATE_H_

#include "ConfigSourcesAll.h"

/**
   LookupData is just a small struct holding a key and a value of either long
   or string type.

   It is used to avoid duplicating the same (sometimes complicated) logic for
   the long/string overloads of our readEntry() functions. The same could be
   achieved by using templates but LookupData approach works just fine for
   now.
 */
class LookupData
{
public:
   enum Type { LD_LONG, LD_STRING };

   LookupData(const String& key, const String& def)
      : m_Key(key), m_Str(def)
   {
      m_Type = LD_STRING;
      m_Found = Profile::Read_Default;
   }

   LookupData(const String& key, long def)
      : m_Key(key)
   {
      m_Type = LD_LONG;
      m_Key = key;
      m_Long = def;
      m_Found = Profile::Read_Default;
   }

   // default copy ctor, assignment operator and dtor are ok


   // don't compare m_Found field here, just the data
   bool operator==(const LookupData& other)
   {
      if ( other.m_Type != m_Type || other.m_Key != m_Key )
         return false;

      return m_Type == LD_LONG ? other.m_Long == m_Long
                               : other.m_Str == m_Str;
   }


   Type GetType(void) const { return m_Type; }
   const String & GetKey(void) const { return m_Key; }
   const String & GetString(void) const
      { ASSERT(m_Type == LD_STRING); return m_Str; }
   long GetLong(void) const
      { ASSERT(m_Type == LD_LONG); return m_Long; }
   Profile::ReadResult GetFound(void) const
      {
         return m_Found;
      }
   void SetResult(const String &str)
      {
         ASSERT(m_Type == LD_STRING);
         m_Str = str;
      }
   void SetResult(long l)
      {
         ASSERT(m_Type == LD_LONG);
         m_Long = l;
      }
   void SetFound(Profile::ReadResult found)
      {
         m_Found = found;
      }

   long *GetLongPtr() { ASSERT(m_Type == LD_LONG); return &m_Long; }
   String *GetStringPtr() { ASSERT(m_Type == LD_STRING); return &m_Str; }

private:
   Type m_Type;
   String m_Key;
   String m_Str;
   long   m_Long;
   Profile::ReadResult m_Found;
};

/**
  ProfileEnumDataImpl is used to store state data while iterating over
  entries/subgroups of a profile.

  The user code only sees EnumData, this class is 100% internal.

  It is implemented in Profile.cpp.
 */
class ProfileEnumDataImpl
{
public:
   // default ctor, copy ctor, dtor and assignment operators are ok

   // must be called (by AllConfigSources) before calling GetNextXXX()
   void Init(const String& path,
             AllConfigSources::List::iterator begin,
             AllConfigSources::List::iterator end)
   {
      m_path = path;
      m_current = begin;
      m_end = end;
      m_started = false;
   }

   // we provide 2 methods doing exactly the same thing for the entries and
   // groups but for each instance of ProfileEnumDataImpl object only one of
   // them should be called as we keep only one state (we don't care for what)
   bool GetNextEntry(String& s) { return DoGetNext(s, Entry); }
   bool GetNextGroup(String& s) { return DoGetNext(s, Group); }

private:
   enum What { Entry, Group };

   // common part of GetNextXXX()s
   bool DoGetNext(String& s, What what);


   // the current and one beyond last config source we're iterating over
   AllConfigSources::List::iterator m_current,
                                    m_end;

   // the path of the key whose entries/groups we're enumerating
   String m_path;

   // the array of the entries/groups we had already encountered
   wxSortedArrayString m_namesSeen;

   // the cookie used by ConfigSource::GetFirst/NextXXX()
   ConfigSource::EnumData m_cookie;

   // had we already called GetFirst() for m_current?
   bool m_started;
};

#endif // _M_CONFIGPRIVATE_H_

