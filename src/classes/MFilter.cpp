///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/MFilter.cpp - implements MFilter and related classes
// Purpose:     non GUI filter classes responsible for storing filters in
//              profile
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     2000
// CVS-ID:      $Id$
// Copyright:   (c) 2000-2001 Mahogany Team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#   pragma implementation "MFilter.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Profile.h"
#   include "strutil.h"

#   include <wx/dynarray.h>        // for WX_DECLARE_OBJARRAY
#endif // USE_PCH

#include "MFilter.h"
#include "MFolder.h"
#include "modules/Filters.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// profile entries names
//
// NB: don't forget to modify MFilter::Copy() when adding more entries here
#define MP_FILTER_NAME     _T("Name")
#define MP_FILTER_RULE     _T("Rule")
#define MP_FILTER_GUIDESC  _T("Settings")

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

struct MFDialogComponent
{
   bool            m_Inverted;
   MFDialogTest    m_Test;
   MFDialogLogical m_Logical;
   MFDialogTarget  m_Target;
   String          m_Argument;

   /// reads settings from a string
   bool ReadSettings(String *str);
   /// writes settings to a string
   String WriteSettings();
   /// attempt to parse filter rule string
   bool ReadSettingsFromRule(String & str);

   /// Writes a rule
   String WriteTest();

   bool operator==(const MFDialogComponent& other) const
   {
      return m_Inverted == other.m_Inverted &&
             m_Test == other.m_Test &&
             m_Logical == other.m_Logical &&
             m_Target == other.m_Target &&
             m_Argument == other.m_Argument;
   }

   bool operator!=(const MFDialogComponent& other) const
   {
      return !(*this == other);
   }
};

bool
MFDialogComponent::ReadSettings(String *str)
{
   // VZ: what's wrong with using sscanf()??
   bool success;
   long number = strutil_readNumber(*str, &success);
   if(!success)
      return FALSE;
   m_Logical = (MFDialogLogical) number;

   number = strutil_readNumber(*str, &success);
   if(!success)
      return FALSE;
   m_Inverted = number != 0;

   number = strutil_readNumber(*str, &success);
   if(! success)
      return FALSE;
   m_Test = (MFDialogTest)number;

   m_Argument = strutil_readString(*str, &success);
   if(! success)
      return FALSE;

   number = strutil_readNumber(*str);
   if(!success)
      return FALSE;
   m_Target = (MFDialogTarget) number;

   return TRUE;
}

String
MFDialogComponent::WriteSettings(void)
{
   return wxString::Format(_T("%d %d %d \"%s\" %d"),
                           (int) m_Logical,
                           (int) m_Inverted,
                           (int) m_Test,
                           strutil_escapeString(m_Argument).c_str(),
                           (int) m_Target);
}

static
const wxChar * ORC_T_Names[] =
{
   // NB: if the string is terminated with a '(' a matching ')' will be
   //     automatically added when constructing the filter expression
   _T("1"),                 // ORC_T_Always
   _T("matchi("),           // ORC_T_Match
   _T("containsi("),        // ORC_T_Contains
   _T("match("),            // ORC_T_MatchC
   _T("contains("),         // ORC_T_ContainsC
   _T("matchregex("),       // ORC_T_MatchRegExC
   _T("size() > "),         // ORC_T_LargerThan
   _T("size() < "),         // ORC_T_SmallerThan
   _T("(now()-date()) > "), // ORC_T_OlderThan
   _T("(now()-date()) < "), // ORC_T_NewerThan
   _T("isspam("),           // ORC_T_IsSpam
   _T("python("),           // ORC_T_Python
   _T("matchregexi("),      // ORC_T_MatchRegEx
   _T("score() > "),        // ORC_T_ScoreAbove
   _T("score() < "),        // ORC_T_ScoreBelow
   _T("istome()"),          // ORC_T_IsToMe
   _T("hasflag("),          // ORC_T_HasFlag
   _T("isfromme()"),        // ORC_T_IsFromMe
   NULL
};

/// this array tells us if the tests need arguments
#define ORC_F_NeedsTarget   0x01
#define ORC_F_NeedsArg      0x02
#define ORC_F_Unimplemented 0x80

static unsigned char ORC_T_Flags[] =
{
   0,                                  // ORC_T_Always
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_Match
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_Contains
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_MatchC
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_ContainsC
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_MatchRegExC
   ORC_F_NeedsArg,                     // ORC_T_LargerThan
   ORC_F_NeedsArg,                     // ORC_T_SmallerThan
   ORC_F_NeedsArg,                     // ORC_T_OlderThan
   ORC_F_NeedsArg,                     // ORC_T_NewerThan
   ORC_F_NeedsArg,                     // ORC_T_IsSpam
#ifndef USE_PYTHON
   ORC_F_Unimplemented|
#endif
   ORC_F_NeedsArg,                     // ORC_T_Python
   ORC_F_NeedsTarget|ORC_F_NeedsArg,   // ORC_T_MatchRegEx
#ifndef USE_HEADER_SCORE
   ORC_F_Unimplemented|
#endif
   ORC_F_NeedsArg,                     // ORC_T_ScoreAbove
#ifndef USE_HEADER_SCORE
   ORC_F_Unimplemented|
#endif
   ORC_F_NeedsArg,                     // ORC_T_ScoreBelow
   0,                                  // ORC_T_IsToMe
   ORC_F_NeedsArg,                     // ORC_T_HasFlag
   0                                   // ORC_T_IsFromMe
};

bool FilterTestNeedsArgument(MFDialogTest test)
{
   CHECK( test >= 0 && (unsigned)test < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter test") );

   return (ORC_T_Flags[test] & ORC_F_NeedsArg) != 0;
}

bool FilterTestNeedsTarget(MFDialogTest test)
{
   CHECK( test >= 0 && (unsigned)test < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter test") );

   return (ORC_T_Flags[test] & ORC_F_NeedsTarget) != 0;
}

bool FilterTestImplemented(MFDialogTest test)
{
   CHECK( test >= 0 && (unsigned)test < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter test") );

   return (ORC_T_Flags[test] & ORC_F_Unimplemented) == 0;
}

static
const wxChar * ORC_W_Names[] =
{
   _T("subject()"),             // ORC_W_Subject
   _T("header()"),              // ORC_W_Header
   _T("from()"),                // ORC_W_From
   _T("body()"),                // ORC_W_Body
   _T("message()"),             // ORC_W_Message
   _T("to()"),                  // ORC_W_To
   _T("header(\"Sender\")"),    // ORC_W_Sender
   _T("recipients()"),          // ORC_W_Recipients
   NULL
};

static const wxChar * OAC_T_Names[] =
{
   _T("delete("),       // OAC_T_Delete
   _T("copy("),         // OAC_T_CopyTo
   _T("move("),         // OAC_T_MoveTo
   _T("expunge("),      // OAC_T_Expunge
   _T("message("),      // OAC_T_MessageBox
   _T("log("),          // OAC_T_LogEntry
   _T("python("),       // OAC_T_Python
   _T("addscore("),     // OAC_T_AddScore
   _T("setcolour("),    // OAC_T_SetColour
   _T("zap("),          // OAC_T_Zap
   _T("print("),        // OAC_T_Print
   _T("setflag("),      // OAC_T_SetFlag
   _T("clearflag("),    // OAC_T_ClearFlag
   _T("setscore("),     // OAC_T_SetScore
   _T("nop("),          // OAC_T_NOP
   NULL
};

#define OAC_F_NeedsArg      0x01
#define OAC_F_Colour        0x02
#define OAC_F_Folder        0x04
#define OAC_F_MsgFlag       0x08
#define OAC_F_Unimplemented 0x80
static unsigned char OAC_T_Flags[] =
{
   0,                            // OAC_T_Delete
   OAC_F_NeedsArg|OAC_F_Folder,  // OAC_T_CopyTo
   OAC_F_NeedsArg|OAC_F_Folder,  // OAC_T_MoveTo
   0,                            // OAC_T_Expunge
   OAC_F_NeedsArg,               // OAC_T_MessageBox
   OAC_F_NeedsArg,               // OAC_T_LogEntry
#ifndef USE_PYTHON
   OAC_F_Unimplemented|
#endif
   OAC_F_NeedsArg,               // OAC_T_Python
#ifndef USE_HEADER_SCORE
   OAC_F_Unimplemented|
#endif
   OAC_F_NeedsArg,               // OAC_T_AddScore
   OAC_F_NeedsArg|OAC_F_Colour,  // OAC_T_SetColour
   0,                            // OAC_T_Zap
   0,                            // OAC_T_Print
   OAC_F_NeedsArg|OAC_F_MsgFlag, // OAC_T_SetFlag
   OAC_F_NeedsArg|OAC_F_MsgFlag, // OAC_T_ClearFlag
#ifndef USE_HEADER_SCORE
   OAC_F_Unimplemented|
#endif
   OAC_F_NeedsArg,               // OAC_T_SetScore
   0,                            // OAC_T_NOP
};

wxCOMPILE_TIME_ASSERT( WXSIZEOF(ORC_T_Names) == ORC_T_Max + 1,
                       MismatchInCritArrays );

wxCOMPILE_TIME_ASSERT( WXSIZEOF(ORC_T_Flags) == ORC_T_Max,
                       MismatchInCritFlags );

wxCOMPILE_TIME_ASSERT( WXSIZEOF(ORC_W_Names) == ORC_W_Max + 1,
                       MismatchInTargetArray );

wxCOMPILE_TIME_ASSERT( WXSIZEOF(OAC_T_Names) == OAC_T_Max + 1,
                       MismatchInActionNames );

wxCOMPILE_TIME_ASSERT( WXSIZEOF(OAC_T_Flags) == OAC_T_Max,
                       MismatchInActionFlags );


bool FilterActionNeedsArg(MFDialogAction action)
{
   CHECK( action >= 0 && (unsigned)action < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter action") );

   return (OAC_T_Flags[action] & OAC_F_NeedsArg) != 0;
}

bool FilterActionUsesColour(MFDialogAction action)
{
   CHECK( action >= 0 && (unsigned)action < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter action") );

   return (OAC_T_Flags[action] & OAC_F_Colour) != 0;
}

bool FilterActionUsesFolder(MFDialogAction action)
{
   CHECK( action >= 0 && (unsigned)action < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter action") );

   return (OAC_T_Flags[action] & OAC_F_Folder) != 0;
}

bool FilterActionMsgFlag(MFDialogAction action)
{
   CHECK( action >= 0 && (unsigned)action < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter action") );

   return (OAC_T_Flags[action] & OAC_F_MsgFlag) != 0;
}

bool FilterActionImplemented(MFDialogAction action)
{
   CHECK( action >= 0 && (unsigned)action < WXSIZEOF(ORC_T_Flags), false,
          _T("invalid filter action") );

   return (OAC_T_Flags[action] & OAC_F_Unimplemented) == 0;
}

String
MFDialogComponent::WriteTest(void)
{
   String program;

   CHECK( m_Test >= 0 && m_Test < ORC_T_Max, program,
          _T("illegal filter test") );

   // This returns the bit to go into an if between the brackets:
   // if ( .............. )
   switch(m_Logical)
   {
      case ORC_L_Or: // OR:
         program << '|';
         break;

      case ORC_L_And: // AND:
         program << '&';
         break;

      default:
         FAIL_MSG( _T("unknown logical filter operation") );

      case ORC_L_None:
         break;
   }

   program << '(';
   if ( m_Inverted )
      program << '!';


   program << ORC_T_Names[m_Test];

   bool needsClosingParen = program.Last() == '(';

   bool needsTarget = FilterTestNeedsTarget(m_Test);
   if(needsTarget)
   {
      if(m_Target != ORC_W_Illegal && m_Target < ORC_W_Max)
         program << ORC_W_Names[ m_Target ];
      else
      {
         FAIL_MSG(_T("This must not happen!"));
         return _T("");
      }
   }

   bool needsArgument = FilterTestNeedsArgument(m_Test);
   if(needsTarget && needsArgument)
      program << ',';
   if(needsArgument)
   {
      // use strutil_escapeString() here to prevent misparsing the quotes
      // embedded into m_Argument as string terminator characters
      program << '"' << strutil_escapeString(m_Argument) << '"';
   }
   if ( needsClosingParen )
      program << ')'; // end of function call
   program << ')';

   return program;
}

bool
MFDialogComponent::ReadSettingsFromRule(String & rule)
{
   const wxChar *cptr = rule.c_str();

   if(*cptr == '|')
   {
      m_Logical = ORC_L_Or;
      cptr++;
   }
   else if(*cptr == '&')
   {
      m_Logical = ORC_L_And;
      cptr++;
   }
   else
      m_Logical = ORC_L_None;

   if(*cptr++ != '(')
      return FALSE;

   if(*cptr == '!')
   {
      m_Inverted = TRUE;
      cptr++;
   }
   else
      m_Inverted = FALSE;
   // now we need to find the test to be applied:
   m_Test = ORC_T_Illegal;
   for(size_t i = 0; ORC_T_Names[i]; i++)
      if(wxStrncmp(cptr, ORC_T_Names[i], wxStrlen(ORC_T_Names[i])) == 0)
      {
         cptr += wxStrlen(ORC_T_Names[i]);
         m_Test = (MFDialogTest) i;
         break;
      }
   if(m_Test == ORC_T_Illegal)
      return FALSE;
   bool needsTarget = FilterTestNeedsTarget(m_Test);
   bool needsArgument = FilterTestNeedsArgument(m_Test);
   m_Target = ORC_W_Illegal;
   if(needsTarget)
   {
      for(size_t i = 0; ORC_W_Names[i]; i++)
         if(wxStrncmp(cptr, ORC_W_Names[i], wxStrlen(ORC_W_Names[i])) == 0)
         {
            m_Target = (MFDialogTarget) i;
            break;
         }
      if(m_Target == ORC_W_Illegal)
         return FALSE;
   }
   // comma between target and argument:
   if(needsTarget && needsArgument
      && *cptr++ != ',')
      return FALSE;

   m_Argument = _T("");
   if(needsArgument)
   {
      if(*cptr != '"') return FALSE;
      String tmp = *cptr;
      bool success;
      m_Argument = strutil_readString(tmp, &success);
      if(! success) return FALSE;
   }
   if(*cptr++ != ')')
      return FALSE;
   // assing remaining bit
   rule = cptr;
   return TRUE;
}

WX_DECLARE_OBJARRAY(MFDialogComponent, MFDComponentArray);
#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(MFDComponentArray);


/** This is a set of dialog settings representing a filter rule. A
    single filter rule consists of one or more Tests connected by
    logical operators, and a single Action to be applied. */
class MFDialogSettingsImpl : public MFDialogSettings
{
public:
   /// The number of tests in the rule:
   virtual size_t CountTests() const
      { return m_Tests.Count(); }

   /// Return the n-th test:
   virtual MFDialogTest GetTest(size_t n) const
      {
         MOcheck();
         return m_Tests[n].m_Test;
      }
   /// Is the n-th test inverted?
   virtual bool IsInverted(size_t n) const
      {
         MOcheck();
         return m_Tests[n].m_Inverted;
      }

   /// Return the n-th logical operator, i.e. the one after the n-th test:
   virtual MFDialogLogical GetLogical(size_t n) const
      {
         MOcheck();
         return m_Tests[n].m_Logical;
      }

   /// Return the n-th test's argument if any:
   virtual String GetTestArgument(size_t n) const
      {
         MOcheck();
         return m_Tests[n].m_Argument;
      }

   virtual MFDialogTarget GetTestTarget(size_t n) const
      {
         MOcheck();
         return m_Tests[n].m_Target;
      }

   /// Return the action component
   virtual MFDialogAction GetAction() const
      {
         MOcheck();
         return m_Action;
      }
   /// Return the action argument if any
   virtual String GetActionArgument() const
      {
         MOcheck();
         return m_ActionArgument;
      }
   /// Add a new test component:
   virtual void AddTest(MFDialogLogical l,
                        bool isInverted,
                        MFDialogTest test,
                        MFDialogTarget target,
                        String argument = _T("")
      )
      {
         MOcheck();
         MFDialogComponent c;
         c.m_Inverted = isInverted;
         c.m_Logical = (m_Tests.Count() == 0) ? ORC_L_None : l;
         c.m_Test = test;
         c.m_Target = target;
         c.m_Argument = argument;
         m_Tests.Add(c);
      }

   virtual void SetAction(MFDialogAction action, const String& arg)
   {
      m_Action = action;
      m_ActionArgument = arg;
   }

   virtual String WriteRule(void) const;

   virtual bool operator==(const MFDialogSettings& other) const;

   /// attempt to parse filter rule string
   bool ReadSettingsFromRule(const String & str);

   bool ReadSettings(const String & str);
   String WriteSettings(void) const;
   String WriteActionSettings(void) const;

   String WriteAction(void) const;

   MOBJECT_DEBUG(MFDialogSettingsImpl)

private:
   MFDComponentArray m_Tests;
   MFDialogAction    m_Action;
   String            m_ActionArgument;
};

bool
MFDialogSettingsImpl::operator==(const MFDialogSettings& o) const
{
   // FIXME urgh
   const MFDialogSettingsImpl& other = (const MFDialogSettingsImpl &)o;

   size_t count = m_Tests.GetCount();
   if ( count != other.m_Tests.GetCount() )
      return false;

   for ( size_t n = 0; n < count; n++ )
   {
      if ( m_Tests[n] != other.m_Tests[n] )
         return false;
   }

   return m_Action == other.m_Action &&
          m_ActionArgument == other.m_ActionArgument;
}

#define MATCH_FAIL(what) \
   if( wxStrncmp(cptr, what, wxStrlen(what)) == 0) \
     cptr += wxStrlen(what); \
   else \
     return FALSE \

String
MFDialogSettingsImpl::WriteAction(void) const
{
   String program;
   if(m_Action < 0 || m_Action >= OAC_T_Max)
   {
      ASSERT_MSG(0, _T("illegal action - must not happen"));
      return _T("");
   }
   bool needsArgument = FilterActionNeedsArg(m_Action);
   program << OAC_T_Names[m_Action];
   if(needsArgument)
      program << '"' << m_ActionArgument << '"';
   program << _T(");");
   return program;
}

bool
MFDialogSettingsImpl::ReadSettingsFromRule(const String & rule)
{
   const wxChar *cptr = rule.c_str();
   bool rc;

   MATCH_FAIL(_T("if(")); // now inside first test

   String tmp = cptr;
   do
   {
      MFDialogComponent c;
      rc = c.ReadSettingsFromRule(tmp);
      if(rc)
         m_Tests.Add(c);
   }while(rc);
   if(m_Tests.Count() == 0)
      return FALSE; // could not find any test
   cptr = tmp.c_str();

   MATCH_FAIL(_T("){"));

   for(size_t i = 0; OAC_T_Names[i] ; i++)
      if(wxStrncmp(cptr, OAC_T_Names[i], wxStrlen(OAC_T_Names[i])) == 0)
      {
         cptr += wxStrlen(OAC_T_Names[i]);
         m_Action = (MFDialogAction) i;
         break;
      }
   bool needsArgument = FilterActionNeedsArg(m_Action);
   if(needsArgument)
   {
      if(*cptr != '"') return FALSE;
      tmp = cptr;
      m_ActionArgument = strutil_readString(tmp, &rc);
      if(! rc)
         return FALSE;
   }
   else
      m_ActionArgument = _T("");
   return TRUE; // we    made it
}

bool
MFDialogSettingsImpl::ReadSettings(const String & istr)
{
   String str = istr;
   bool rc;

   long n = strutil_readNumber(str, &rc);
   for(long i = 0; i < n && rc ; i++)
   {
      MFDialogComponent c;
      rc = c.ReadSettings(&str);
      if(rc)
         m_Tests.Add(c);
   }
   if(m_Tests.Count() == 0)
      return FALSE;

   // now read the action settings:
   long a = strutil_readNumber(str, &rc);
   if(! rc) return FALSE;
   m_Action = (MFDialogAction) a;
   m_ActionArgument = strutil_readString(str, &rc);
   return rc;
}

String
MFDialogSettingsImpl::WriteSettings(void) const
{
   String str;
   str << m_Tests.Count() << ' ';
   for(size_t i = 0; i < m_Tests.Count(); i++)
      str << m_Tests[i].WriteSettings() << ' ';
   str << WriteActionSettings();
   return str;
}

String
MFDialogSettingsImpl::WriteActionSettings(void) const
{
   return String::Format(_T("%d \"%s\""),
                         (int) m_Action,
                         strutil_escapeString(m_ActionArgument).c_str());
}

String
MFDialogSettingsImpl::WriteRule(void) const
{
   ASSERT(m_Tests.Count() > 0);
   String program = _T("if(");

   for(size_t i = 0; i < m_Tests.Count(); i++)
      program << m_Tests[i].WriteTest();

   program << ')'
           << '{'
           << WriteAction()
           << '}';
   return program;
}

/* static */
MFDialogSettings *MFDialogSettings::Create()
{
   return new MFDialogSettingsImpl;
}

// ----------------------------------------------------------------------------
// MFilterFromProfile
// ----------------------------------------------------------------------------

class MFilterFromProfile : public MFilter
{
public:
   virtual MFilterDesc GetDesc(void) const
   {
      MFilterDesc fd;
      fd.SetName(m_Name);
      ((MFilterFromProfile *)this)->UpdateSettings(); // const_cast
      if ( m_Settings )
      {
         m_Settings->IncRef();
         fd.Set(m_Settings);
      }
      else
      {
         fd.Set(m_Rule);
      }

      return fd;
   }

   virtual void Set(const MFilterDesc& fd)
   {
      m_Name = fd.GetName();
      SafeDecRef(m_Settings);
      if ( fd.IsSimple() )
      {
         m_Settings = (MFDialogSettingsImpl *)fd.GetSettings();
         m_Rule.clear();
      }
      else
      {
         m_Settings = NULL;
         m_Rule = fd.GetProgram();
      }

      m_dirty = true;
   }

   static MFilterFromProfile * Create(Profile *p)
      { return new MFilterFromProfile(p); }

   MOBJECT_DEBUG(MFilter)

protected:
   MFilterFromProfile(Profile *p)
      {
         m_Profile = p;
         m_Name = p->readEntry(MP_FILTER_NAME, _T(""));
         m_Settings = NULL;

         // use the filter program if we have it
         m_Rule = p->readEntry(MP_FILTER_RULE, _T(""));
         if( !m_Rule.empty() )
         {
            // try to parse the rule
            MFDialogSettingsImpl *settings = new MFDialogSettingsImpl;
            if ( !settings->ReadSettingsFromRule(m_Rule) )
            {
               settings->DecRef();
            }
            else
            {
               m_Settings = settings;
            }
         }

         if ( m_Rule.empty() )
         {
            // use the GUI settings if no rule
            m_SettingsStr = p->readEntry(MP_FILTER_GUIDESC, _T(""));
         }

         m_dirty = false;
      }

   virtual ~MFilterFromProfile()
      {
         if ( m_dirty )
         {
            // write the values to the profile
            m_Profile->writeEntry(MP_FILTER_NAME, m_Name);
            if ( m_Settings )
            {
               // if we have matching dialog settings, we prefer to
               // write them as they are more compact in the config file
               m_Profile->writeEntry(MP_FILTER_GUIDESC,
                                     m_Settings->WriteSettings());
            }
            else
            {
               if ( m_Profile->HasEntry(MP_FILTER_GUIDESC) )
               {
                  (void) m_Profile->DeleteEntry(MP_FILTER_GUIDESC);
               }

               m_Profile->writeEntry(MP_FILTER_RULE, m_Rule);
            }
         }
         SafeDecRef(m_Settings);
         m_Profile->DecRef();
      }

private:
   void UpdateSettings(void)
      {
         // old style settings:
         if ( m_SettingsStr.Length() != 0 )
         {
            // parse the settings string
            m_Settings = new MFDialogSettingsImpl;
            if ( !m_Settings->ReadSettings(m_SettingsStr) )
            {
               // oops, failed...
               m_Settings->DecRef();
               m_Settings = NULL;
            }
            else
               m_Rule = m_Settings->WriteRule();
         }
         else // no settings stored but rule code:
         {
            m_Settings = new MFDialogSettingsImpl;
            bool rc = m_Settings->ReadSettingsFromRule(m_Rule);
            if(!rc)
            {
               m_Settings->DecRef();
               m_Settings = NULL;
            }
         }
      }

   Profile *m_Profile;
   String m_Name;
   String m_Rule;
   String m_SettingsStr;
   MFDialogSettingsImpl *m_Settings;
   bool m_dirty;
};

/* static */
MFilter *
MFilter::CreateFromProfile(const String &name)
{
   Profile * p = Profile::CreateFilterProfile(name);
   return MFilterFromProfile::Create(p);
}

/* static */
bool
MFilter::DeleteFromProfile(const String& name)
{
   // get parent of all filters
   Profile_obj p(Profile::CreateFilterProfile(_T("")));

   bool rc;
   if ( p )
   {
      rc = p->DeleteGroup(name);
   }
   else
   {
      rc = false;
   }

   return rc;
}

// MFilter::Copy helper: copies entry from one profile to another if it's
// present, returns false if an error occured
static bool CopyFilterEntry(Profile *profileSrc,
                            Profile *profileDst,
                            const String& entry)
{
   String value = profileSrc->readEntry(entry, _T(""));
   if ( !value.empty() )
   {
      if ( !profileDst->writeEntry(entry, value) )
         return false;
   }

   return true;
}

/* static */
bool
MFilter::Copy(const String& nameSrc, const String& nameDst)
{
   // GUI code is supposed to check for this
   CHECK( nameSrc != nameDst, false, _T("can't copy filter over itself") );

   Profile_obj profileSrc(Profile::CreateFilterProfile(nameSrc));
   Profile_obj profileDst(Profile::CreateFilterProfile(nameDst));

   bool rc = profileSrc && profileDst;
   if ( rc )
   {
      // change the name
      profileDst->writeEntry(MP_FILTER_NAME, nameDst);
   }

   // copy other entries as is
   if ( rc )
      rc = CopyFilterEntry(profileSrc, profileDst, MP_FILTER_RULE);
   if ( rc )
      rc = CopyFilterEntry(profileSrc, profileDst, MP_FILTER_GUIDESC);

   if ( !rc )
   {
      ERRORMESSAGE((_("Failed to copy filter '%s' to '%s'")));

      if ( profileDst )
      {
         // don't leave incomplete entries
         (void)DeleteFromProfile(nameDst);
      }

      return false;
   }

   return true;
}

// ----------------------------------------------------------------------------
// debugging helpers
// ----------------------------------------------------------------------------

#ifdef DEBUG

String
MFDialogSettingsImpl::DebugDump() const
{
   return MObjectRC::DebugDump();
}

String
MFilterFromProfile::DebugDump() const
{
   return MObjectRC::DebugDump() + _T(" name ") + m_Name;
}

#endif // DEBUG

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

extern FilterRule *
GetFilterForFolder(const MFolder *folder)
{
   CHECK( folder, NULL, _T("GetFilterForFolder: NULL parameter") );

   // build a single program from all filter rules:
   String filterString;
   wxArrayString filters = folder->GetFilters();
   size_t countFilters = filters.GetCount();
   for ( size_t nFilter = 0; nFilter < countFilters; nFilter++ )
   {
      MFilter_obj filter(filters[nFilter]);
      MFilterDesc fd = filter->GetDesc();
      filterString += fd.GetRule();
   }

   // do we have any filters?
   if ( filterString.empty() )
   {
      // no, nothing to do
      return NULL;
   }

   MModule_Filters *filterModule = MModule_Filters::GetModule();
   if ( !filterModule )
   {
      wxLogWarning(_("Filter module couldn't be loaded."));

      return NULL;
   }

   // compile the filter rule into the real filter
   FilterRule *filterRule = filterModule->GetFilter(filterString);

   // filterRule holds a reference to the filterModule if it was successfully
   // been created, otherwise we don't need filterModule anyhow
   filterModule->DecRef();

   if ( !filterRule )
   {
      wxLogError(_("Error parsing filter '%s' for folder '%s'"),
                 filterString.c_str(), folder->GetFullName().c_str());
   }

   return filterRule;
}
