///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/AddressExpander.cpp
// Purpose:     AddressExpander implementation
// Author:      Vadim Zeitlin
// Created:     2007-01-08
// CVS-ID:      $Id: wxComposeView.cpp 7203 2007-01-08 00:09:32Z vadz $
// Copyright:   (c) 1998-2007 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include <wx/textctrl.h>        // for wxTextCtrl
#endif

#include "adb/AdbManager.h"

#include "gui/AddressExpander.h"

// ============================================================================
// AddressExpander implementation
// ============================================================================

AddressExpander::AddressExpander(wxTextCtrl *text, Profile *profile)
               : m_profile(profile)
{
   SafeIncRef(profile);

   m_text = text;

   // TODO: connect to TAB and Enter events to handle them automatically?
}

wxFrame *AddressExpander::GetParentFrame() const
{
   return (wxFrame *)wxGetTopLevelParent(m_text);
}

int
AddressExpander::ExpandAll(wxArrayString& addresses,
                           wxArrayInt& rcptTypes,
                           String *subject) const
{
   return AdbExpandAllRecipients
          (
            m_text->GetValue(),
            addresses,
            rcptTypes,
            subject,
            m_profile,
            GetParentFrame()
          );
}

RecipientType AddressExpander::ExpandLast(String *subject)
{
   // find the last address
   String text = m_text->GetValue();
   const size_t len = text.length();
   size_t start = 0;
   bool quoted = false;
   for ( size_t n = 0; n < len; n++ )
   {
      switch ( (wxChar)text[n] )
      {
         case '\\':
            // skip the next character, even if it's a quote or separator
            n++;
            break;

         case '"':
            quoted = !quoted;
            break;

         case ',':
         case ';':
            if ( !quoted )
            {
               // keep the space if there is one following the separator
               if ( n < len - 1 && text[n + 1] == ' ' )
                  n++;

               start = n;
            }
            break;
      }
   }

   // if we didn't find the final quote, skip over the initial one too
   if ( quoted )
      start++;

   if ( start == len )
      return Recipient_Max; // nothing to expand

   // preserve the part before the address start
   String textBefore;
   textBefore.assign(text, 0, start);
   text.erase(0, start);
   text.Trim(false /* from left */);

   // expand the address
   RecipientType rcptType =
      AdbExpandSingleRecipient(&text, subject, m_profile, GetParentFrame());

   if ( rcptType != Recipient_Max )
   {
      m_text->SetValue(textBefore + text);
      m_text->SetInsertionPointEnd();
   }

   return rcptType;
}

