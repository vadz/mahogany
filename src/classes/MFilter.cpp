/*-*- c++ -*-********************************************************
 * MFilter - a class for managing filter config settings            *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifdef __GNUG__
#   pragma implementation "MFilter.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include  "Mpch.h"

#ifndef USE_PCH
#   include "Mcommon.h"
#   include "Profile.h"
#   include "strutil.h"
#   include "Mdefaults.h"
#endif

#include "MFilter.h"



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
   return wxString::Format("%d %d %d \"%s\" %d",
                           (int) m_Logical,
                           (int) m_Inverted,
                           (int) m_Test,
                           strutil_escapeString(m_Argument).c_str(),
                           (int) m_Target);
}

static
const char * ORC_T_Names[] =
{
   "1", // ORC_T_Always = 0,
   "matchi(", // ORC_T_Match,
   "containsi(", // ORC_T_Contains,
   "match(", // ORC_T_MatchC,
   "contains(", // ORC_T_ContainsC,
   "matchregex(", // ORC_T_MatchRegExC,
   "size() > ", // ORC_T_LargerThan,
   "size() < ", // ORC_T_SmallerThan,
   "(now()-date()) > ", // ORC_T_OlderThan,
   "(now()-date()) < ", // ORC_T_NewerThan,
   "isspam()", // ORC_T_IsSpam,
   "python(", // ORC_T_Python,
   "matchregexi(", // ORC_T_MatchRegEx,
   "score() > ", // ORC_T_ScoreAbove,
   "score() < ", // ORC_T_ScoreBelow,
   NULL
};

/// this array tells us if the tests need arguments
#define ORC_F_NeedsTarget   0x01
#define ORC_F_NeedsArg      0x02

static unsigned char ORC_T_Flags[] =
{
   0, // ORC_T_Always = 0,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_Match,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_Contains,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_MatchC,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_ContainsC,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_MatchRegExC,
      ORC_F_NeedsArg, // ORC_T_LargerThan,
      ORC_F_NeedsArg, // ORC_T_SmallerThan,
      ORC_F_NeedsArg, // ORC_T_OlderThan,
      ORC_F_NeedsArg, // ORC_T_NewerThan,
      0, // ORC   _T_IsSpam,
      ORC_F_NeedsArg, // ORC_T_Python,
      ORC_F_NeedsTarget|ORC_F_NeedsArg, // ORC_T_MatchRegEx,
      ORC_F_NeedsArg, // ORC_T_ScoreAbove,
      ORC_F_NeedsArg, // ORC_T_ScoreBelow
      };

static
const char * ORC_W_Names[] =
{
   "subject()", "header()", "from()", "body()",
   "message()", "to()", "header(\"Sender\")",
   "recipients()",
   NULL
};

static const char * OAC_T_Names[] = 
{
   "delete(", "copy(", "move(", "expunge(",
      "message(", "log(", "python(", "addscore(",
      "setcolour(", "uniq(", "print(", NULL
};

#define OAC_F_NeedsArg   0x01
static unsigned char OAC_T_Flags[] =
{
   0, OAC_F_NeedsArg, OAC_F_NeedsArg, 0,
      OAC_F_NeedsArg, OAC_F_NeedsArg, OAC_F_NeedsArg, OAC_F_NeedsArg,
      OAC_F_NeedsArg, 0 , 0
};

String
MFDialogComponent::WriteTest(void)
{
   ASSERT( WXSIZEOF(ORC_T_Names) == ORC_T_Max + 1);
   ASSERT( WXSIZEOF(ORC_T_Flags) == ORC_T_Max);
   ASSERT( WXSIZEOF(ORC_W_Names) == ORC_W_Max + 1);

   String program;
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
   case ORC_L_None:
      break;
   default:
      ASSERT(0); // not possible
   }
   program << '(';
   if(m_Inverted)
      program << '!';
         
   if(m_Test < 0  || m_Test >= ORC_T_Max)
   {
      ASSERT_MSG(0, "Illegal test - must not happen");
      return "";
   }
   
   bool needsTarget = ORC_T_Flags[m_Test] & ORC_F_NeedsTarget;
   bool needsArgument = (ORC_T_Flags[m_Test] & ORC_F_NeedsArg) != 0;
   program << ORC_T_Names[m_Test];
   if(needsTarget)
   {
      if(m_Target != ORC_W_Illegal && m_Target < ORC_W_Max)
         program << ORC_W_Names[ m_Target ];
      else
      { ASSERT_MSG(0,"This must not happen!"); return ""; }
   }
   if(needsTarget && needsArgument)
      program << ',';
   if(needsArgument)
   {
      program << '"' << m_Argument << '"';
   }
   if(needsTarget || needsArgument)
      program << ')'; // end of function call
   program << ')';
   return program;
}

bool
MFDialogComponent::ReadSettingsFromRule(String & rule)
{
   ASSERT( WXSIZEOF(ORC_T_Names) == ORC_T_Max + 1);
   ASSERT( WXSIZEOF(ORC_T_Flags) == ORC_T_Max);
   ASSERT( WXSIZEOF(ORC_W_Names) == ORC_W_Max + 1);

   const char *cptr = rule.c_str();

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
      if(strncmp(cptr, ORC_T_Names[i], strlen(ORC_T_Names[i])) == 0)
      {
         cptr += strlen(ORC_T_Names[i]);
         m_Test = (MFDialogTest) i;
         break;
      }
   if(m_Test == ORC_T_Illegal)
      return FALSE;
   bool needsTarget = ORC_T_Flags[m_Test] & ORC_F_NeedsTarget;
   bool needsArgument = (ORC_T_Flags[m_Test] & ORC_F_NeedsArg) != 0;
   m_Target = ORC_W_Illegal;
   if(needsTarget)
   {
      for(size_t i = 0; ORC_W_Names[i]; i++)
         if(strncmp(cptr, ORC_W_Names[i], strlen(ORC_W_Names[i])) == 0)
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
   
   m_Argument = "";
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

#include <wx/dynarray.h>
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
                        String argument = ""
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
   if( strncmp(cptr, what, strlen(what)) == 0) \
     cptr += strlen(what); \
   else \
     return FALSE \

String
MFDialogSettingsImpl::WriteAction(void) const
{
   String program;
   if(m_Action < 0 || m_Action >= OAC_T_Max)
   {
      ASSERT_MSG(0,"illegal action - must not happen");
      return "";
   }
   bool needsArgument = OAC_T_Flags[m_Action] & OAC_F_NeedsArg;
   program << OAC_T_Names[m_Action];
   if(needsArgument)
      program << '"' << m_ActionArgument << '"';
   program << ");";
   return program;
}

bool
MFDialogSettingsImpl::ReadSettingsFromRule(const String & rule)
{
   const char *cptr = rule.c_str();
   bool rc;

   MATCH_FAIL("if("); // now inside first test

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

   MATCH_FAIL("){");

   for(size_t i = 0; OAC_T_Names[i] ; i++)
      if(strncmp(cptr, OAC_T_Names[i], strlen(OAC_T_Names[i])) == 0)
      {
         cptr += strlen(OAC_T_Names[i]);
         m_Action = (MFDialogAction) i;
         break;
      }
   bool needsArgument = OAC_T_Flags[m_Action] & OAC_F_NeedsArg;
   if(needsArgument)
   {
      if(*cptr != '"') return FALSE;
      tmp = cptr;
      m_ActionArgument = strutil_readString(tmp, &rc);
      if(! rc)
         return FALSE;
   }
   else
      m_ActionArgument = "";
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
   return String::Format("%d \"%s\"",
                         (int) m_Action,
                         strutil_escapeString(m_ActionArgument).c_str());
}

String
MFDialogSettingsImpl::WriteRule(void) const
{
   ASSERT(m_Tests.Count() > 0);
   String program = "if(";
   
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
         m_Name = p->readEntry("Name", "");
         m_Settings = NULL;
         // we now prefer to parse the filter code:
         m_Rule = p->readEntry("Rule", "");
         if( m_Rule[0] )
         {
            MFDialogSettingsImpl *control = new MFDialogSettingsImpl;
            bool rc = control->ReadSettingsFromRule(m_Rule);
            control->DecRef();
            if(rc)
               m_SettingsStr = ""; // don't need them
            else
               m_SettingsStr = p->readEntry("Settings", "");
         }
         else
            m_SettingsStr = p->readEntry("Settings", "");
            m_dirty = false;
      }
   
   virtual ~MFilterFromProfile()
      {
         if ( m_dirty )
         {
            // write the values to the profile
            m_Profile->writeEntry("Name", m_Name);
            if ( m_Settings )
            {
               // if we have matching dialog settings, we prefer to
               // write them as they are more compact in the config file
               m_Profile->writeEntry("Settings", m_Settings->WriteSettings());
            }
            else
            {
               (void) m_Profile->DeleteEntry("Settings");
               m_Profile->writeEntry("Rule", m_Rule);
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
   Profile *p = Profile::CreateFilterProfile("");
   bool rc = FALSE;
   if(p)
   {
      rc = p->DeleteGroup(name);
      p->DecRef();
   }
   return rc;
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
   return MObjectRC::DebugDump() + " name " + m_Name;
}

#endif // DEBUG

