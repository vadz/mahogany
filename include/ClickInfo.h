///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ClickInfo.h: declaration of ClickableInfo
// Purpose:     ClickableInfo is an ABC for "(inter)active" objects which can
//              appear in MessageView
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02 (extracted from MessageView.h)
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CLICKINFO_H_
#define _M_CLICKINFO_H_

class MessageView;
class wxPoint;

// ----------------------------------------------------------------------------
// ClickableInfo: anything which the user can click on in the MessageView
// ----------------------------------------------------------------------------

class ClickableInfo
{
public:
   /**
      @name Ctor and dtor
    */
   //@{

   /// ctor takes the MessageView we're associated with
   ClickableInfo(MessageView *msgView) : m_msgView(msgView) { }

   /// virtual dtor for any base class
   virtual ~ClickableInfo() { }

   //@}

   /**
      @name Accessors
    */
   //@{

   /// get the label to be shown in the viewer
   virtual String GetLabel() const = 0;

   //@}

   /**
      @name Processing mouse events

      All functions get the click coordinates in the m_msgView->GetWindow()
      client coordinates.
    */
   //@{

   /// process left click
   virtual void OnLeftClick(const wxPoint& pt) const = 0;

   /// process right click
   virtual void OnRightClick(const wxPoint& pt) const = 0;

   /// process double (left) click
   virtual void OnDoubleClick(const wxPoint& pt) const = 0;

   //@}

protected:
   MessageView * const m_msgView;

   DECLARE_NO_COPY_CLASS(ClickableInfo)
};

#endif // _M_CLICKINFO_H_

