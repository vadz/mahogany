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
   bool success;
   long number = strutil_readNumber(*str, &success);
   if(success && number != -1)
      m_Logical = (MFDialogLogical) number;
   else
      return FALSE;
   number = strutil_readNumber(*str, &success);
   if(success)
      m_Inverted = number != 0;
   else
      return FALSE;
   number = strutil_readNumber(*str, &success);
   if(! success) return FALSE;
   m_Test = (MFDialogTest)number;
   m_Argument = strutil_readString(*str, &success);
   if(! success) return FALSE;
   number = strutil_readNumber(*str);
   if(success)
      m_Target = (MFDialogTarget) number;
   else
      return FALSE;
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

String
MFDialogComponent::WriteTest(void)
{
   String program;
   // This returns the bit to go into an if between the brackets:
   // if ( .............. )
   switch(m_Logical)
   {
   case ORC_L_Or: // OR:
      program << "|";
      break;
   case ORC_L_And: // AND:
      program << "&";
      break;
   case ORC_L_None:
      break;
   default:
      ASSERT(0); // not possible
   }
   program << '(';
   if(m_Inverted)
      program << '!';
         
   bool needsTarget = true;
   bool needsArgument = true;
   switch(m_Test)
   {
   case ORC_T_Always:
      needsTarget = false;
      needsArgument = false;
      program << '1'; // true
      break;
   case ORC_T_Match:
      program << "matchi("; break;
   case ORC_T_Contains:
      program << "containsi("; break;
   case ORC_T_MatchC:
      program << "match("; break;
   case ORC_T_ContainsC:
      program << "contains("; break;
   case ORC_T_MatchRegExC:
      program << "matchregex("; break;
   case ORC_T_MatchRegEx:
      program << "matchregexi("; break;
   case ORC_T_Python:
      program << "python(";
      needsTarget = false;
      break;
   case ORC_T_LargerThan:
   case ORC_T_SmallerThan:
   case ORC_T_OlderThan:
   case ORC_T_ScoreAbove:
   case ORC_T_ScoreBelow:
      needsArgument = true;
      needsTarget = false;
      switch(m_Test)
      {
      case ORC_T_ScoreAbove:
         program << "score() > "; break;
      case ORC_T_ScoreBelow:
         program << "score() < "; break;
      case ORC_T_LargerThan:
         program << "size() > "; break;
      case ORC_T_SmallerThan:
         program << "size() < "; break;
      case ORC_T_OlderThan:
         program << "(date()-now()) > "; break;
      case ORC_T_NewerThan:
         program << "(date()-now()) < "; break;
      default:
         ;
      }
      break;
   case ORC_T_IsSpam:
      program << "isspam()";
      needsTarget = false;
      needsArgument = false;
      break;
   default:
      wxASSERT_MSG(0,"unknown rule"); // FIXME: give meaningful error message
   }
   if(needsTarget)
   {
      switch(m_Target)
      {
      case ORC_W_Subject:
         program << "subject()"; break;
      case ORC_W_Header:
         program << "header()"; break;
      case ORC_W_From:
         program << "from()"; break;
      case ORC_W_Body:
         program << "body()"; break;
      case ORC_W_Message:
         program << "message()"; break;
      case ORC_W_To:
         program << "to()"; break;
      case ORC_W_Sender:
         program << "header(\"Sender\")"; break;
      case ORC_W_Invalid:
      case ORC_W_Max:
         ASSERT(0); // must not happen
      };
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
   
   bool ReadSettings(const String & str);
   String WriteSettings(void) const;
   String WriteActionSettings(void) const;
   
   String WriteAction(void) const;
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

bool
MFDialogSettingsImpl::ReadSettings(const String & istr)
{
   String str = istr;
   bool rc;

   size_t n = strutil_readNumber(str, &rc);
   if(n < 1)
      return FALSE;
   for(size_t i = 0; i < n && rc; i++)
   {
      MFDialogComponent c;
      rc = c.ReadSettings(&str);
      if(rc)
         m_Tests.Add(c);
   }
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
      str << m_Tests[i].WriteSettings();
   str << ' ' << WriteActionSettings();
   return str;
}
   
String
MFDialogSettingsImpl::WriteAction(void) const
{
   String program;
   bool needsArgument = true;
   switch(m_Action)
   {
   case OAC_T_Delete:
      program << "delete("; needsArgument = false; break;
   case OAC_T_CopyTo:
      program << "copy("; break;
   case OAC_T_MoveTo:
      program << "move("; break;
   case OAC_T_Expunge:
      program << "expunge("; needsArgument = false; break;
   case OAC_T_MessageBox:
      program << "message("; break;
   case OAC_T_LogEntry:
      program << "log("; break;
   case OAC_T_Python:
      program << "python("; break;
   case OAC_T_ChangeScore:
      program << "addscore("
              << m_ActionArgument;
      needsArgument = false;
      break;
   case OAC_T_SetColour:
      program << "setcolour("; break;
   case OAC_T_Uniq:
      program << "uniq("; needsArgument = false; break;
   case OAC_T_Invalid:
   case OAC_T_Max:
      ASSERT_MSG(0,"Invalid action type");
   }
   if(needsArgument)
      program << '"' << m_ActionArgument << '"';
   program << ");";
   return program;
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
      if ( fd.IsSimple() )
      {
         SafeDecRef(m_Settings);
         m_Settings = (MFDialogSettingsImpl *)fd.GetSettings();
      }
      else
      {
         m_Rule = fd.GetProgram();
      }

      m_dirty = true;
   }
   
   static MFilterFromProfile * Create(Profile *p)
      { return new MFilterFromProfile(p); }
   
protected:
   MFilterFromProfile(Profile *p)
      {
         m_Profile = p;
         m_Name = p->readEntry("Name", "");
         m_SettingsStr = p->readEntry("Settings", "");
         m_Rule = p->readEntry("Rule", "");
         m_Settings = NULL;
         m_dirty = false;
         UpdateSettings();
      }
   
   virtual ~MFilterFromProfile()
      {
         if ( m_dirty )
         {
            // write the values to the profile
            m_Profile->writeEntry("Name", m_Name);
            if ( m_Settings )
               m_Profile->writeEntry("Settings", m_Settings->WriteSettings());
            else
               m_Profile->writeEntry("Rule", m_Rule);
         }

         SafeDecRef(m_Settings);
         m_Profile->DecRef();
      }
   
private:
   void UpdateSettings(void)
      {
         m_Settings = new MFDialogSettingsImpl;
         if( m_SettingsStr.Length() == 0
             ||  m_Settings->ReadSettings(m_SettingsStr) == FALSE)
         {
            delete m_Settings;
            m_Settings = NULL;
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
   FAIL_MSG("Karsten, please implement this");

   return false;
}

