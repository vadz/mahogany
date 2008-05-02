///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/AddressExpander.h
// Purpose:     declaration of AddressExpander class
// Author:      Vadim Zeitlin
// Created:     2007-01-08
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2007 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/**
   @file gui/AddressExpander.h
   @brief Declaration of AddressExpander helper.

   AddressExpander can be used with any control which needs to expand email
   addresses entered into it by user by looking them up in the address books.
 */

#ifndef _M_GUI_ADDRESSEXPANDER_H_
#define _M_GUI_ADDRESSEXPANDER_H_

#include "RecipientType.h"

#ifndef USE_PCH
#   include "Profile.h"        // for Profile_obj
#endif // USE_PCH

class WXDLLIMPEXP_FWD_CORE wxFrame;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

/**
   AddressExpander is not a control but can be associated with an existing
   control to add support for expanding addresses in it.
 */
class AddressExpander
{
public:
   /**
      Create an AddressExpander associated with the given text entry zone.

      @param text the associated text control
      @param profile for expansion options
    */
   AddressExpander(wxTextCtrl *text, Profile *profile);

   /**
      Calls AdbExpandAllRecipients() for the control contents.

      Doesn't modify the control contents.
    */
   int ExpandAll(wxArrayString& addresses,
                 wxArrayInt& rcptTypes,
                 String *subject) const;

   /**
      Replaces the last address entered into the control with its expansion.

      @param subject filled in with the subject if the recipient contains it
      @return type of the recipient if explicitly specified, Recipient_None if
              none and Recipient_Max if nothing was expanded
    */
   RecipientType ExpandLast(String *subject);

private:
   // return the parent window for ADB functions
   wxFrame *GetParentFrame() const;


   wxTextCtrl *m_text;
   Profile_obj m_profile;

   DECLARE_NO_COPY_CLASS(AddressExpander)
};

#endif // _M_GUI_ADDRESSEXPANDER_H_

