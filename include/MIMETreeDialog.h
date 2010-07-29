///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MIMETreeDialog.h
// Purpose:     dialog allowing the user to view all attachments
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2004-08-13
// CVS-ID:      $Id$
// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_MIMETREEDIALOG_H_
#define _M_MIMETREEDIALOG_H_

class MessageView;
class MimePart;

class WXDLLIMPEXP_FWD_CORE wxFrame;

/**
   Show the dialog showing all MIME parts to user.

   @param part the MIME part (usually top level one) to show
   @param parent the parent window for the dialog
   @param msgView the message view if the dialog should allow the user to
                  use popup menus to open/save MIME parts (not used for
                  anything else)
 */
extern void
ShowMIMETreeDialog(const MimePart *part,
                   wxFrame *parent,
                   MessageView *msgView = NULL);

#endif // _M_MIMETREEDIALOG_H_


