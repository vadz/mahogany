// -*- c++ -*- //////////////////////////////////////////////////////////////
// Name:        wx/phone.cpp
// Purpose:     Telephone support
// Author:      karsten Ballüder
// Modified by:
// Created:     04.10.99
// RCS-ID:      $Id$
// Copyright:   (c) Karsten Ballüder
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifdef    __GNUG__
#   pragma implementation "phone.h"
#endif

#include "wx/phone.h"
#include <ctype.h>

#define ISNUMBER(ch) ( isalnum(ch) || ch == '.' || ch == '*' || ch == '#' || isspace(ch))
                                                                   

wxPhoneManager::wxPhoneManager(const wxString & local_areacode, const wxString & local_int_code)
{
   m_AreaCode = local_areacode;
   m_IntCode = local_int_code;
   m_KeepAreaZero = FALSE;
}

      
wxPhoneManager::~wxPhoneManager()
{
}

/** Converts a number to the normalised form:
    +int-areacode-number-ext
    preserving dots for pauses.
*/

wxString
wxPhoneManager::NormaliseNumber(wxString const &inumber)
{
   wxString number = inumber;
   number.Trim(); number.Trim(TRUE); // remove whitespace

   wxString norm;
   size_t idx;

   // Write international code +int
   norm << '+';
   if(number[(size_t)0] == '+')
   {
      idx = 1;
      while(ISNUMBER(number[idx]))
      {
         if(! isspace(number[idx]))
            norm << number[idx];
         idx++;
      }
      number = number.Right(number.Length()-idx);
   }
   else if (number.Left(m_IntPrefix.length()) == m_IntPrefix)
   // starts with our local int dialling prefix
   {
      idx = m_IntPrefix.length();
      while(ISNUMBER(number[idx]))
      {
         if(! isspace(number[idx]))
            norm << number[idx];
         idx++;
      }
      number = number.Right(number.Length()-idx);
   }
   else // no international prefix, write ours:
   {
      norm << m_IntCode;
   }
   norm << '-';

   /* Now, try to find the area code in the phone number. We recognise 
      either:
      (areacode)local
      areacode-local
      areacode/local
   */
   bool brackets = FALSE;
   idx = 0;
   if(number[(size_t)0] == '(')
   {
      brackets = TRUE;
      idx++;
   }
   while(ISNUMBER(number[idx]))
   {
      if(! isspace(number[idx]))
         norm << number[idx];
      idx++;
   }
   number = number.Right(number.Length()-idx);
   number.Trim();

   idx = 0;
   // Yes, there is a local part, too:
   if(number.Length() > 0)
   {
      norm << '-';
      while(number[idx])
      {
         if( ! isspace(number[idx]))
            norm << number[idx];
         idx++;
      }
   }
   return norm;
}

