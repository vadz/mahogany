/*-*- c++ -*-********************************************************
 * MFilter - a class for managing filter config settings            *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef MFILTER_H
#define MFILTER_H

#ifdef __GNUG__
#   pragma interface "MFilter.h"
#endif

/**@name Some commonly used enums for the different parts of a simple
   dialog-constructed filter rule. */
//@{
/// Enum holding the different possible matching filter actions:
enum MFDialogTest
{
   ORC_T_Always = 0,
   ORC_T_Match,
   ORC_T_Contains,
   ORC_T_MatchC,
   ORC_T_ContainsC,
   ORC_T_MatchRegExC,
   ORC_T_LargerThan,
   ORC_T_SmallerThan,
   ORC_T_OlderThan,
   ORC_T_NewerThan,
   ORC_T_IsSpam,
   ORC_T_Python,
   ORC_T_MatchRegEx,
   ORC_T_ScoreAbove,
   ORC_T_ScoreBelow,
   ORC_T_Max
};
   
enum MFDialogTarget
{
   ORC_W_Invalid = -1,
   ORC_W_Subject = 0,
   ORC_W_Header,
   ORC_W_From,
   ORC_W_Body,
   ORC_W_Message,
   ORC_W_To,
   ORC_W_Sender,
   ORC_W_Max
};

enum MFDialogAction
{
   OAC_T_Invalid = -1,
   OAC_T_Delete = 0,
   OAC_T_CopyTo,
   OAC_T_MoveTo,
   OAC_T_Expunge,
   OAC_T_MessageBox,
   OAC_T_LogEntry,
   OAC_T_Python,
   OAC_T_ChangeScore,
   OAC_T_SetColour,
   OAC_T_Uniq,
   OAC_T_Max
};

enum MFDialogLogical
{
   ORC_L_None = -1,
   ORC_L_Or = 0,
   ORC_L_And = 1,
   ORC_L_Max
};
//@}

/** This is a set of dialog settings representing a filter rule. A
    single filter rule consists of one or more Tests connected by
    logical operators, and a single Action to be applied. */
class MFDialogSettings : public MObject
{
public:
   /// The number of tests in the rule:
   virtual size_t CountTests() const = 0;
   /// Return the n-th test:
   virtual MFDialogTest GetTest(size_t n) const = 0;
   /// Is the n-th test inverted?
   virtual bool IsInverted(size_t n) const = 0;
   /// Return the n-th logical operator, i.e. the one after the n-th test:
   virtual MFDialogLogical GetLogical(size_t n) const = 0;
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
                        String argument = ""
      ) = 0;

   /// This produces a filter rule for this set of settings
   virtual String WriteRule(void) const = 0;
};

class MFilter : public MObjectRC
{
public:
   static MFilter * CreateFromProfile(const String &name);
   
   /// return the filter rule string
   virtual String GetFilterRule(void) const = 0;
   /// return the dialog settings for this filter if possible, or NULL
   virtual MFDialogSettings * GetDialogSettings(void) const = 0;
};

#endif
