///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/SpamOptionsPage.h
// Purpose:     SpamOptionsPage is a wxOptionsPageDynamic used in spam options dialog
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-12-04
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file gui/SpamOptionsPage.h
   @brief Declaration of SpamOptionsPage class.

   This header is private to spam modules and exists mainly to avoid that all
   SpamFilter clients depend on gui/wxOptionsPageDynamic.h header.
 */

#ifndef _M_GUI_SPAMOPTIONSPAGE_H_
#define _M_GUI_SPAMOPTIONSPAGE_H_

#include "gui/wxOptionsPage.h"

/**
   SpamOptionsPage provides additional methods for serialization to/from
   strings.
 */
class SpamOptionsPage : public wxOptionsPageDynamic
{
public:
   SpamOptionsPage(wxNotebook *parent, Profile *profile, String *params)
      : wxOptionsPageDynamic(parent)
   {
      m_profile = profile;
      m_params = params;
   }

   virtual bool TransferDataToWindow()
   {
      if ( m_params )
      {
         WriteParamsToProfile();
      }

      return wxOptionsPageDynamic::TransferDataToWindow();
   }

   virtual bool TransferDataFromWindow()
   {
      if ( !wxOptionsPageDynamic::TransferDataFromWindow() )
         return false;

      if ( m_params )
      {
         ReadParamsFromProfile();
      }

      return true;
   }

protected:
   virtual void WriteParamsToProfile() { }
   virtual void ReadParamsFromProfile() { }

   Profile *m_profile;
   String *m_params;
};

#endif // _M_GUI_SPAMOPTIONSPAGE_H_

