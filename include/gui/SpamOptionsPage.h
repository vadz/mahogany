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
   SpamOptionsPage is currently empty, but it was used before and is left in
   case it may become useful in the future.

   This class is used for the option pages created by the spam filters in their
   CreateOptionPage() and shown in the spam filters options dialog.
 */
class SpamOptionsPage : public wxOptionsPageDynamic
{
public:
   SpamOptionsPage(wxNotebook *parent, Profile *profile)
      : wxOptionsPageDynamic(parent)
   {
   }
};

#endif // _M_GUI_SPAMOPTIONSPAGE_H_

