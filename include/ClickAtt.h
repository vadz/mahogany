///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ClickAtt.h: declaration of ClickableAttachment
// Purpose:     ClickableAttachment is a ClickableInfo which corresponds to an
//              attachment (i.e. MIME part) of the message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CLICKATT_H_
#define _M_CLICKATT_H_

#include "ClickInfo.h"

class MessageView;
class MimePart;

class WXDLLEXPORT wxWindow;

class ClickableAttachment : public ClickableInfo
{
public:
   /// create a clickable object associated with this MIME part
   ClickableAttachment(MessageView *msgView, const MimePart *mimepart);

   /// return a descriptive label for this MIME part
   static String GetLabelFor(const MimePart *mimepart);

   // implement base class pure virtuals
   virtual String GetLabel() const;

   virtual void OnLeftClick(const wxPoint& pt) const;
   virtual void OnRightClick(const wxPoint& pt) const;
   virtual void OnDoubleClick(const wxPoint& pt) const;

   // show the popup menu for this window/at this point
   //
   // this one is for wxMsgCmdProc convenience
   void ShowPopupMenu(wxWindow *window, const wxPoint& pt) const;

private:
   // the MIME part we're associated with
   const MimePart * const m_mimepart;

   DECLARE_NO_COPY_CLASS(ClickableAttachment)
};

#endif // _M_CLICKATT_H_

