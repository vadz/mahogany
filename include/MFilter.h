/*-*- c++ -*-********************************************************
 * MFilter - a class for managing filter config settings            *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef _MFILTER_H
#define _MFILTER_H

#ifdef __GNUG__
#   pragma interface "MFilter.h"
#endif

#ifndef USE_PCH
#   include "Profile.h"
#endif

#include "MObject.h"

class WXDLLEXPORT wxWindow;

class FilterRule;
class MFolder;
class MModule_Filters;

/**@name Some commonly used enums for the different parts of a simple
   dialog-constructed filter rule. */
//@{

/** Enum holding the different possible matching filter actions.
    Do not change the order of these without changing MFilter.cpp
    where corresponding strings are defined.

    Actually, never change the order or remove any of these values,
    their numeric values are used to store filter settings.
*/
enum MFDialogTest
{
   ORC_T_Illegal = -1,
   ORC_T_Always  = 0,      // test is always true
   ORC_T_Match,            // argument matches the target (case insensitive)
   ORC_T_Contains,         // target contains the argument (case insensitive)
   ORC_T_MatchC,           // as Match but case sensitive
   ORC_T_ContainsC,        // as Contains but case sensitive
   ORC_T_MatchRegExC,      // as RegEx but case sensitive
   ORC_T_LargerThan,       // size of the message is larger than argument
   ORC_T_SmallerThan,      //                        smaller
   ORC_T_OlderThan,        // date of the message is older than argument
   ORC_T_NewerThan,        //                        newer
   ORC_T_IsSpam,           // doesn't pass some heuristic SPAM tests
   ORC_T_Python,           // return value of Python script
   ORC_T_MatchRegEx,       // target matches RE in argument
   ORC_T_ScoreAbove,       // score of the message is greater than argument
   ORC_T_ScoreBelow,       //                         less
   ORC_T_IsToMe,           // the message is addressed to one of my addresses
   ORC_T_HasFlag,          // flag is set for message
   ORC_T_Max
};

enum MFDialogHasFlag
{
   ORC_MF_Illegal = -1,
   ORC_MF_Unseen  = 0, // MailFolder::MSG_STAT_SEEN (inverted)
   ORC_MF_Deleted,     // MailFolder::MSG_STAT_DELETED
   ORC_MF_Answered,    // MailFolder::MSG_STAT_ANSWERED
// ORC_MF_Searched,    // MailFolder::MSG_STAT_SEARCHED
   ORC_MF_Important,   // MailFolder::MSG_STAT_FLAGGED
   ORC_MF_Recent,      // MailFolder::MSG_STAT_RECENT
   ORC_MF_Max
};

enum MFDialogTarget
{
   ORC_W_Illegal = -1,
   ORC_W_Subject = 0,
   ORC_W_Header,
   ORC_W_From,
   ORC_W_Body,
   ORC_W_Message,
   ORC_W_To,
   ORC_W_Sender,
   ORC_W_Recipients,
   ORC_W_Max
};

enum MFDialogAction
{
   OAC_T_Illegal = -1,
   OAC_T_Delete = 0,
   OAC_T_CopyTo,
   OAC_T_MoveTo,
   OAC_T_Expunge,
   OAC_T_MessageBox,
   OAC_T_LogEntry,
   OAC_T_Python,
   OAC_T_AddScore,
   OAC_T_SetColour,
   OAC_T_Zap,
   OAC_T_Print,
   OAC_T_SetFlag,
   OAC_T_ClearFlag,
   OAC_T_SetScore,
   OAC_T_Max
};

enum MFDialogSetFlag
{
   OAC_MF_Illegal = -1,
   OAC_MF_Unseen  = 0, // MailFolder::MSG_STAT_SEEN (inverted)
   OAC_MF_Deleted,     // MailFolder::MSG_STAT_DELETED
   OAC_MF_Answered,    // MailFolder::MSG_STAT_ANSWERED
// OAC_MF_Searched,    // MailFolder::MSG_STAT_SEARCHED
   OAC_MF_Important,   // MailFolder::MSG_STAT_FLAGGED
// OAC_MF_Recent,      // MailFolder::MSG_STAT_RECENT (can't set)
   OAC_MF_Max
};

enum MFDialogLogical
{
   ORC_L_None = -1,
   ORC_L_Or = 0,
   ORC_L_And = 1,
   ORC_L_Max
};

/// return true if this filter test requires an argument
extern bool FilterTestNeedsArgument(MFDialogTest test);

/// return true if this filter test requires the target to operate on
extern bool FilterTestNeedsTarget(MFDialogTest test);

/// return true if this filter test is implemented
extern bool FilterTestImplemented(MFDialogTest test);

/// return true if this filter action requires an argument
extern bool FilterActionNeedsArg(MFDialogAction action);

/// return true if this filter action can use the colour popup
extern bool FilterActionUsesColour(MFDialogAction action);

/// return true if this filter action can use the folder popup
extern bool FilterActionUsesFolder(MFDialogAction action);

/// return true if this filter action sets a message flag
extern bool FilterActionMsgFlag(MFDialogAction action);

/// return true if this filter action is implemented
extern bool FilterActionImplemented(MFDialogAction action);

//@}

/** This is a set of dialog settings representing a filter rule. A
    single filter rule consists of one or more Tests connected by
    logical operators, and a single Action to be applied. */
class MFDialogSettings : public MObjectRC
{
public:
   /// create an object implementing this interface
   static MFDialogSettings *Create();

   /// The number of tests in the rule:
   virtual size_t CountTests() const = 0;
   /// Return the n-th test:
   virtual MFDialogTest GetTest(size_t n) const = 0;
   /// Is the n-th test inverted?
   virtual bool IsInverted(size_t n) const = 0;
   /// Return the n-th logical operator, i.e. the one after the n-th test:
   virtual MFDialogLogical GetLogical(size_t n) const = 0;
   /// Return where to apply the n-th test
   virtual MFDialogTarget GetTestTarget(size_t n) const = 0;
   /// Return the n-th test's argument if any:
   virtual String GetTestArgument(size_t n) const = 0;
   /// Return the action component
   virtual MFDialogAction GetAction() const = 0;
   /// Return the action argument if any
   virtual String GetActionArgument() const = 0;

      /** Add a new test component:
          @param l logical connection to previous rule (ignored for first)
          @param isInverted TRUE if test is to be inverted
          @param test the actual test enum
          @param target where to apply the test to
          @param argument empty string or optional string argument
      */
   virtual void AddTest(MFDialogLogical l,
                        bool isInverted,
                        MFDialogTest test,
                        MFDialogTarget target,
                        String argument = _T("")
      ) = 0;

   /// sets the action and its argument
   virtual void SetAction(MFDialogAction action, const String& arg) = 0;

   /// This produces a filter rule for this set of settings
   virtual String WriteRule(void) const = 0;

   /// Compare 2 objects
   virtual bool operator==(const MFDialogSettings& other) const = 0;
};

/// filter description: it is either MFDialogSettings or a filter rule as is
class MFilterDesc
{
public:
   /// ctor creates an uninitialized object, one of Set() below must be used
   MFilterDesc() { m_settings = NULL; }

   /// assignment operator
   MFilterDesc& operator=(const MFilterDesc& o)
   {
      SafeDecRef(m_settings);
      DoCopy(o);
      return *this;
   }
   /// copy ctor
   MFilterDesc(const MFilterDesc& other) { DoCopy(other); }

   /// dtor
  ~MFilterDesc() { SafeDecRef(m_settings); }

   /// will take ownership of the pointer and will delete it itself
   void Set(MFDialogSettings *settings)
      { SafeDecRef(m_settings); m_settings = settings; m_program.clear(); }

   /// from a filter rule
   void Set(const String& program)
   {
      if ( m_settings )
      {
         m_settings->DecRef();
         m_settings = NULL;
      }

      m_program = program;
   }

   /// set the name of the filter
   void SetName(const String& name) { m_name = name; }

   /// returns TRUE if we can be represented as MFDialogSettings
   bool IsSimple() const { return m_settings != NULL; }

   /// returns TRUE if we're uninitialized
   bool IsEmpty() const { return !IsSimple() && !m_program; }

   /// only can be called if IsSimple()
   MFDialogSettings *GetSettings() /* logically */ const
   {
      MFDialogSettings *settings = m_settings;
      settings->IncRef();
      return settings;
   }

   /// should only be called for !IsSimple() filters
   const String& GetProgram() const { return m_program; }

   /// return the rule program, can be called for any filters
   String GetRule() const
   {
      return IsSimple() ? m_settings->WriteRule() : m_program;
   }

   /// returns the name of the filter
   const String& GetName() const { return m_name; }

   /// compare 2 filters
   bool operator==(const MFilterDesc& desc)
   {
      if ( m_name != desc.m_name )
         return false;

      if ( m_settings )
      {
         if ( !desc.m_settings )
            return false;
         else
            return *m_settings == *desc.m_settings;
      }
      else
      {
         if ( desc.m_settings )
            return false;
         else
            return m_program == desc.m_program;
      }
   }

   bool operator!=(const MFilterDesc& desc) { return !(*this == desc); }

private:
   void DoCopy(const MFilterDesc& other)
   {
      m_name = other.m_name;
      m_program = other.m_program;
      m_settings = other.m_settings;
      SafeIncRef(m_settings);
   }

   MFDialogSettings *m_settings;
   String            m_program,
                     m_name;
};

/// class encapsulating an entire filter
class MFilter : public MObjectRC
{
public:
   /// create the filter by name
   static MFilter *CreateFromProfile(const String &name);

   /// delete the filter by name
   static bool DeleteFromProfile(const String &name);

   /// copy one filter to another (names must be different)
   static bool Copy(const String& nameSrc, const String& nameDst);

   /// get the list of all defined filters
   static wxArrayString GetAllFilters()
   {
      return Profile::GetAllFilters();
   }

   /// return the filter descrption
   virtual MFilterDesc GetDesc(void) const = 0;

   /// set the filter parameters
   virtual void Set(const MFilterDesc& desc) = 0;
};

/// smart reference to MFilter
BEGIN_DECLARE_AUTOPTR(MFilter)
   public:
      MFilter_obj(const String& name)
      {
         m_ptr = MFilter::CreateFromProfile(name);
      }
END_DECLARE_AUTOPTR();

/**
  Returns the filter program for the given folder. If no filters are specified
  or if an error occured, returns NULL. Otherwise the returned pointer must be
  DecRef()d by caller

  @param mfolder the folder to get filters for (can't be NULL)
  @return filter rule configured for this folder or NULL
 */
extern FilterRule *GetFilterForFolder(const MFolder *mfolder);

#endif // _MFILTER_H
