///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/SpamOptions.cpp - Spam options dialog
// Purpose:     Splits spam options off wxFilterDialogs.cpp
// Author:      Robert Vazan
// Modified by:
// Created:     05 September 2003
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
   #pragma implementation "wxSpamOptions.h"
#endif

#include "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "strutil.h"
#   include "miscutil.h"
#   include "Profile.h"
#   include "MApplication.h"
#endif // USE_PCH

#include <stdlib.h>

#include "MFilterLang.h"
#include "gui/wxOptionsPage.h"
#include "gui/wxSpamOptions.h"
#include "gui/wxOptionsDlg.h"
#include "pointers.h"

DECLARE_REF_COUNTER(Profile)

/**
   A fixed-size array.
 */
class BoundArrayCommon
{
public:
   BoundArrayCommon() : m_size(0) {}

   size_t Size() const { return m_size; }

protected:
   size_t m_size;
};

#define BOUND_ARRAY(type,name) \
   class name : public BoundArrayCommon \
   { \
   public: \
      typedef type HostType; \
   \
      name() : m_array(NULL) {} \
      ~name() { Destroy(); } \
   \
      void Initialize(size_t count); \
      type *Get() { return m_array; } \
      type& At(size_t offset); \
      type& operator[](size_t offset) { return At(offset); } \
   \
   private: \
      void Destroy(); \
   \
      type *m_array; \
   }

#define IMPLEMENT_BOUND_ARRAY(name) \
   void name::Destroy() { delete[] m_array; } \
   \
   void name::Initialize(size_t count) \
   { \
      ASSERT( !m_array ); \
      m_array = new name::HostType[m_size = count]; \
   } \
   \
   name::HostType& name::At(size_t offset) \
   { \
      ASSERT( offset < m_size ); \
      return m_array[offset]; \
   }

class SpamOption
{
public:
   virtual bool DefaultValue() const = 0;
   virtual const wxChar *Token() const = 0;
   virtual const wxChar *ProfileHackName() const = 0;
   virtual const wxChar *Title() const = 0;
   
   bool m_active;
};


class SpamOptionAssassin : public SpamOption
{
public:
   virtual bool DefaultValue() const { return true; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_SPAMASSASSIN; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamAssassin"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Been tagged as spam by Spam&Assassin"); }
};


class SpamOption8Bit : public SpamOption
{
public:
   virtual bool DefaultValue() const { return true; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_SUBJ8BIT; }
   virtual const wxChar *ProfileHackName() const
      { return _T("Spam8BitSubject"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Too many &8 bit characters in subject"); }
};


class SpamOptionCaps : public SpamOption
{
public:
   virtual bool DefaultValue() const { return true; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_SUBJCAPS; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamCapsSubject"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Only &capitals in subject"); }
};


class SpamOptionJunkSubject : public SpamOption
{
public:
   virtual bool DefaultValue() const { return true; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_SUBJENDJUNK; }
   virtual const wxChar *ProfileHackName() const
      { return _T("JunkEndSubject"); }
   virtual const wxChar *Title() const
      { return gettext_noop("&Junk at end of subject"); }
};


class SpamOptionKorean : public SpamOption
{
public:
   virtual bool DefaultValue() const { return true; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_KOREAN; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamKoreanCharset"); }
   virtual const wxChar *Title() const
      { return gettext_noop("&Korean charset"); }
};


class SpamOptionXAuth : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_XAUTHWARN; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamXAuthWarning"); }
   virtual const wxChar *Title() const
      { return gettext_noop("X-Authentication-&Warning header"); }
};


class SpamOptionReceived : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_RECEIVED; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamReceived"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Suspicious \"&Received\" headers"); }
};


class SpamOptionHtml : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_HTML; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamHtml"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Only &HTML content"); }
};


class SpamOptionMime : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_MIME; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamMime"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Unusual &MIME structure"); }
};


class SpamOptionWhiteList : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_WHITE_LIST; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpameWhiteList"); }
   virtual const wxChar *Title() const
      { return gettext_noop("No match in &whitelist"); }
};


#ifdef USE_RBL

class SpamOptionRbl : public SpamOption
{
public:
   virtual bool DefaultValue() const { return false; }
   virtual const wxChar *Token() const
      { return SPAM_TEST_RBL; }
   virtual const wxChar *ProfileHackName() const
      { return _T("SpamIsInRBL"); }
   virtual const wxChar *Title() const
      { return gettext_noop("Been &blacklisted by RBL"); }
};

#endif // USE_RBL


class SpamOptionManagerBody : public SpamOptionManager
{
public:
   SpamOptionManagerBody();
   
   virtual void FromString(const String &source);
   virtual String ToString();
   
   virtual bool ShowDialog(wxFrame *parent);
   
private:
   void BuildConfigValues();
   void BuildFieldInfo();
   
   void SetDefaults();
   void SetFalse();
   
   void WriteProfile(Profile *profile);
   void ReadProfile(Profile *profile);
   void DeleteProfile(Profile *profile);

   size_t ConfigEntryCount() const { return ms_count+1; }
   
   BOUND_ARRAY(ConfigValueNone,ConfigValueArray);
   ConfigValueArray m_configValues;
   BOUND_ARRAY(wxOptionsPage::FieldInfo,FieldInfoArray);
   FieldInfoArray m_fieldInfo;

   SpamOptionAssassin m_checkSpamAssassin;
   SpamOption *PickAssassin() { return &m_checkSpamAssassin; }
   
   SpamOption8Bit m_check8bit;
   SpamOption *Pick8bit() { return &m_check8bit; }
   
   SpamOptionCaps m_checkCaps;
   SpamOption *PickCaps() { return &m_checkCaps; }
   
   SpamOptionJunkSubject m_checkJunkSubj;
   SpamOption *PickJunkSubject() { return &m_checkJunkSubj; }
   
   SpamOptionKorean m_checkKorean;
   SpamOption *PickKorean() { return &m_checkKorean; }
   
   SpamOptionXAuth m_checkXAuthWarn;
   SpamOption *PickXAuthWarn() { return &m_checkXAuthWarn; }
   
   SpamOptionReceived m_checkReceived;
   SpamOption *PickReceived() { return &m_checkReceived; }
   
   SpamOptionHtml m_checkHtml;
   SpamOption *PickHtml() { return &m_checkHtml; }
   
   SpamOptionMime m_checkMime;
   SpamOption *PickMime() { return &m_checkMime; }
   
   SpamOptionWhiteList m_whitelist;
   SpamOption *PickWhiteList() { return &m_whitelist; }
   
#ifdef USE_RBL
   SpamOptionRbl m_checkRBL;
   SpamOption *PickRBL() { return &m_checkRBL; }
#endif // USE_RBL

   typedef SpamOption *(SpamOptionManagerBody::*PickMember)();
   static const PickMember ms_members[];
   static const size_t ms_count;
   
   class Iterator
   {
   public:
      Iterator(SpamOptionManagerBody *container)
         : m_container(container), m_index(0) {}
      
      SpamOption *operator->()
      {
         return (m_container->*SpamOptionManagerBody::ms_members[m_index])();
      }
      void operator++() { if ( !IsEnd() ) m_index++; }
      bool IsEnd() { return m_index == SpamOptionManagerBody::ms_count; }
      size_t Index() { return m_index; }
         
   private:
      SpamOptionManagerBody *m_container;
      size_t m_index;
   };

   friend class SpamOptionManagerBody::Iterator;
};

const SpamOptionManagerBody::PickMember SpamOptionManagerBody::ms_members[] =
{
   &SpamOptionManagerBody::PickAssassin,
   &SpamOptionManagerBody::Pick8bit,
   &SpamOptionManagerBody::PickCaps,
   &SpamOptionManagerBody::PickJunkSubject,
   &SpamOptionManagerBody::PickKorean,
   &SpamOptionManagerBody::PickXAuthWarn,
   &SpamOptionManagerBody::PickReceived,
   &SpamOptionManagerBody::PickHtml,
   &SpamOptionManagerBody::PickMime,
   &SpamOptionManagerBody::PickWhiteList,
#ifdef USE_RBL
   &SpamOptionManagerBody::PickRBL,
#endif // USE_RBL
};

const size_t SpamOptionManagerBody::ms_count
   = WXSIZEOF(SpamOptionManagerBody::ms_members);


IMPLEMENT_BOUND_ARRAY(SpamOptionManagerBody::ConfigValueArray)
IMPLEMENT_BOUND_ARRAY(SpamOptionManagerBody::FieldInfoArray)

SpamOptionManagerBody::SpamOptionManagerBody()
{
   BuildConfigValues();
   BuildFieldInfo();
}

void SpamOptionManagerBody::SetDefaults()
{
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      option->m_active = option->DefaultValue();
   }
}

void SpamOptionManagerBody::SetFalse()
{
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      option->m_active = false;
   }
}

void SpamOptionManagerBody::FromString(const String &source)
{
   if ( source.empty() )
   {
      SetDefaults();
      return;
   }
   
   SetFalse();
   
   const wxArrayString tests = strutil_restore_array(source);
   
   size_t token;
   for ( token = 0; token < tests.GetCount(); token++ )
   {
      const wxString& actual = tests[token];
      
      for ( SpamOptionManagerBody::Iterator option(this);
         !option.IsEnd(); ++option )
      {
         if ( actual == option->Token() )
         {
            option->m_active = true;
            goto found;
         }
      }
      wxLogDebug(_T("Unknown spam test \"%s\""), actual.c_str());
found:
      ;
   }
}

String SpamOptionManagerBody::ToString()
{
   String result;
   
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      if ( option->m_active )
      {
         if ( !result.empty() )
            result += ':';
      
         result += option->Token();
      }
   }
   
   return result;
}

void SpamOptionManagerBody::WriteProfile(Profile *profile)
{
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      profile->writeEntry(option->ProfileHackName(), option->m_active);
   }
}

void SpamOptionManagerBody::ReadProfile(Profile *profile)
{
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      option->m_active = profile->readEntry(
         option->ProfileHackName(), 0l) != 0l;
   }
}

void SpamOptionManagerBody::DeleteProfile(Profile *profile)
{
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      profile->DeleteEntry(option->ProfileHackName());
   }
}

void SpamOptionManagerBody::BuildConfigValues()
{
   m_configValues.Initialize(ConfigEntryCount());
   
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      ConfigValueDefault &value = m_configValues[option.Index()+1];
      value = ConfigValueDefault(
         option->ProfileHackName(),option->DefaultValue());
   }
}

void SpamOptionManagerBody::BuildFieldInfo()
{
   m_fieldInfo.Initialize(ConfigEntryCount());
   
   m_fieldInfo[0].label
      = gettext_noop("Mahogany may use several heuristic tests to detect spam.\n"
                     "Please choose the ones you'd like to be used by checking\n"
                     "the corresponding entries.\n"
                     "\n"
                     "So the message is considered to be spam if it has...");
   m_fieldInfo[0].flags = wxOptionsPage::Field_Message;
   m_fieldInfo[0].enable = -1;
   
   for ( SpamOptionManagerBody::Iterator option(this);
      !option.IsEnd(); ++option )
   {
      wxOptionsPage::FieldInfo &info = m_fieldInfo[option.Index()+1];
      info.label = option->Title();
      info.flags = wxOptionsPage::Field_Bool;
      info.enable = -1;
   }
}

bool SpamOptionManagerBody::ShowDialog(wxFrame *parent)
{
   // we use the global app profile to pass the settings to/from the options
   // page because like this we can reuse the options page classes easily
   Profile *profile = mApplication->GetProfile();

   // transfer data to dialog
   WriteProfile(profile);

   const wxOptionsPageDesc page(
         // title and image
         gettext_noop("Details of spam detection"),
         _T("spam"),
   
         // help id (TODO)
         -1,
   
         // the fields description
         m_fieldInfo.Get(),
         m_configValues.Get(),
         ConfigEntryCount()
      );
   
   bool status = ShowCustomOptionsDialog(page, profile, parent);
   
   if ( status )
      ReadProfile(profile);

   // don't keep this stuff in profile, we don't use it except here
   DeleteProfile(profile);
   
   return status;
}

DEFINE_REF_COUNTER(SpamOptionManager)

/*static*/ RefCounter<SpamOptionManager> SpamOptionManager::Create()
{
   RefCounter<SpamOptionManager> pointer(new SpamOptionManagerBody);
   return pointer;
}
