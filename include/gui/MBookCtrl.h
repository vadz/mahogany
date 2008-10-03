///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/listbook.h
// Purpose:     definition of MBookCtrl
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-11
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file gui/listbook.h
   @brief Defines MBookCtrl typedef.

   We use wxListbook instead of wxNotebook if it is available but on some
   platforms it isn't, so we always use MBookCtrl typedef as these
   classes have the same interface and so can be used interchangeably.
*/

#ifndef M_GUI_MBOOKCTRL_H_
#define M_GUI_MBOOKCTRL_H_

#if wxUSE_LISTBOOK
   class WXDLLIMPEXP_FWD_CORE wxListbook;
   typedef wxListbook MBookCtrl;

   class wxPListbook;
   typedef wxPListbook MPBookCtrl;

   #define M_EVT_BOOK_PAGE_CHANGED wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED

   #define MBookEvent wxListbookEvent
   #define MBookEventHandler wxListbookEventHandler
#else
   class WXDLLIMPEXP_FWD_CORE wxNotebook;
   typedef wxNotebook MBookCtrl;

   class wxPNotebook;
   typedef wxPNotebook MPBookCtrl;

   #define M_EVT_BOOK_PAGE_CHANGED wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED

   #define MBookEvent wxNotebookEvent
   #define MBookEventHandler wxNotebookEventHandler
#endif

#endif // M_GUI_MBOOKCTRL_H_

