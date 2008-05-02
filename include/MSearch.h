///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MSearch.h
// Purpose:     various search-related stuff
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.07.02 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MSEARCH_H_
#define _MSEARCH_H_

class MFolder;
class Profile;
class WXDLLIMPEXP_FWD_CORE wxWindow;

/**
  Search criterium for searching folders for certain messages.
 */
struct SearchCriterium
{
public:
   enum Type
   {
      SC_ILLEGAL = -1,
      SC_FULL = 0,
      SC_BODY,
      SC_HEADER,
      SC_SUBJECT,
      SC_TO,
      SC_FROM,
      SC_CC
   };

   /// which part of the message to search
   Type m_What;

   /// what to search for
   String m_Key;

   /// invert the criterium?
   bool m_Invert;

   /// the array of the names of folders to search
   wxArrayString m_Folders;

   SearchCriterium() { m_What = SC_ILLEGAL; m_Invert = false; }
};

/**
  Show the dialog allowing the user to specify the search criteria. If this
  function returns true, the caller should continue with searching, otherwise
  the search is cancelled. In the former case, crit out parameter contains not
  only what to search, but also where to search for it (m_Folders names array)

  @param crit the search criterium filled by this function
  @param profile the profile to use for the last search values
  @param folder the default folder to search in
  @param parent the parent window for the dialog
  @return true if ok, false if the dialog was cancelled
 */
extern
bool
ConfigureSearchMessages(SearchCriterium *crit,
                        Profile *profile,
                        const MFolder *folder,
                        wxWindow *parent);

#endif // _MSEARCH_H_

