///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ClickURL.h: declaration of ClickableURL
// Purpose:     ClickableURL is a ClickableInfo which corresponds to an URL
//              embedded in the message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_CLICKURL_H_
#define _M_CLICKURL_H_

#include "ClickInfo.h"

class MessageView;
class wxPoint;
class Profile;

/// options for OpenInBrowser() (these are bit masks)
enum
{
   URLOpen_Default = 0,
   URLOpen_Other   = 1
};

// ----------------------------------------------------------------------------
// ClickableURL: an URL in MessageView
// ----------------------------------------------------------------------------

class ClickableURL : public ClickableInfo
{
public:
   /// create a clickable object associated with this (possibly multiline) URL
   ClickableURL(MessageView *msgView, const String& url);

   // implement base class pure virtuals
   virtual String GetLabel() const;

   virtual void OnLeftClick() const;
   virtual void OnRightClick(const wxPoint& pt) const;

   /// @name Accessors
   //@{

   // returns true if this is a mail URL
   //
   // NB: if the URL starts with "mailto:", the prefix is removed so this
   //     method may modify m_url as a side effect
   bool IsMail() const;

   /// returns the URL string we represent
   const String& GetUrl() const;

   /// get the profile we use (the profile of MessageView in fact), do NOT
   /// DecRef() it!
   Profile *GetProfile() const;

   //@}

   /// @name Operations
   //@{

   /// open the URL (maybe in a new browser window)
   void OpenInBrowser(int options = URLOpen_Default) const;

   /// open an address, i.e. start writing message to it normally
   void OpenAddress() const;

   /// add an address to the address book
   void AddToAddressBook() const;

   //@}

private:
   // the URL itself
   String m_url;

   // the flag returned by IsMail(), shouldn't be used directly
   mutable enum
   {
      Unknown = -1,
      No,
      Yes
   } m_isMail;

   DECLARE_NO_COPY_CLASS(ClickableURL)
};

#endif // _M_CLICKURL_H_

