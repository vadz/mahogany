///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/listbook.h
// Purpose:     definition of wxListOrNoteBook
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-07-11
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

/**
   @file gui/listbook.h
   @brief Defines wxListOrNoteBook typedef.

   We use wxListbook instead of wxNotebook if it is available but on some
   platforms it isn't, so we always use wxListOrNoteBook typedef as these
   classes have the same interface and so can be used interchangeably.
*/

#ifndef _M_GUI_LISTBOOK_H_
#define _M_GUI_LISTBOOK_H_

#if wxUSE_LISTBOOK
   class WXDLLEXPORT wxListbook;
   typedef wxListbook wxListOrNoteBook;
#else
   class WXDLLEXPORT wxNotebook;
   typedef wxNotebook wxListOrNoteBook;
#endif

#endif // _M_GUI_LISTBOOK_H_

