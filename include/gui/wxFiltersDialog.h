///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxFiltersDialog.h - filter-related constants/functions
// Purpose:     this file doesn't deal with filters themselves, but with our
//              representation of them (how we store them in profile...)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     25.05.00 (extracted from gui/wxFiltersDialog.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _WXFILTERSDIALOG_H_
#define _WXFILTERSDIALOG_H_

#ifdef __GNUG__
   #pragma interface "Mdnd.h"
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum ORC_Types_Enum
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

enum ORC_Where_Enum
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

enum OAC_Types_Enum
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

enum ORC_Logical_Enum
{
   ORC_L_None = -1,
   ORC_L_Or = 0,
   ORC_L_And,
   ORC_L_Max
};

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

/// configure the filters
extern
bool ConfigureFilterRules(Profile *profile, wxWindow *parent);

/// write the filter to the given profile
extern
bool SaveSimpleFilter(Profile *profile,
                      const wxString& name,
                      ORC_Types_Enum cond,
                      ORC_Where_Enum condWhere,
                      const wxString& condWhat,
                      OAC_Types_Enum action,
                      const wxString& actionArg,
                      wxString *program = NULL);

#endif // _WXFILTERSDIALOG_H_
