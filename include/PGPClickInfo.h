///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   PGPClickInfo.h: declaration of ClickableInfo
// Purpose:     PGPClickableInfo is for "(inter)active" PGP objects which can
//              appear in MessageView
// Author:      Xavier Nodet
// Modified by:
// Created:     13.12.02 (extracted from viewfilt/PGP.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2002 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_PGPCLICKINFO_H_
#define _M_PGPCLICKINFO_H_

#include "ClickInfo.h"

// ----------------------------------------------------------------------------
// ClickablePGPInfo: an icon in message viewer containing PGP information
// ----------------------------------------------------------------------------

class ClickablePGPInfo : public ClickableInfo
{
public:
   ClickablePGPInfo(MessageView *msgView,
                    const String& label,
                    const String& bmpName,
                    const wxColour& colour);
   virtual ~ClickablePGPInfo();

   // implement the base class pure virtuals
   virtual String GetLabel() const;

   virtual void OnLeftClick(const wxPoint&) const { ShowDetails(); }
   virtual void OnRightClick(const wxPoint& pt) const;
   virtual void OnDoubleClick(const wxPoint&) const { ShowDetails(); }

   // show the details about this PGP info object to the user (menu command)
   void ShowDetails() const;

   // show the raw text of the PGP message (menu command)
   void ShowRawText() const;

   // get the bitmap and the colour to show in the viewer
   wxBitmap GetBitmap() const;
   wxColour GetColour() const;

   // used by PGPFilter only
   void SetRaw(const String& textRaw) { m_textRaw = textRaw; }
   void SetLog(MCryptoEngineOutputLog *log)
      { ASSERT_MSG( !m_log, _T("SetLog() called twice?") ); m_log = log; }

private:
   // the kind of object (e.g. "good signature")
   String m_label;

   // the name of the bitmap shown by this object
   String m_bmpName;

   // the raw text of the PGP message
   String m_textRaw;

   // the colour to show this object in (only for text viewer)
   wxColour m_colour;

   // the log output (we own this object and will delete it)
   MCryptoEngineOutputLog *m_log;
};

// ----------------------------------------------------------------------------
// A bunch of classes trivially deriving from ClickablePGPInfo
// ----------------------------------------------------------------------------

class PGPInfoGoodSig : public ClickablePGPInfo
{
public:
   PGPInfoGoodSig(MessageView *msgView, const String& from)
      : ClickablePGPInfo(msgView,
                         wxString::Format( _("Good PGP signature%s"), 
                                           (from.empty() ? 
                                                 _T("") :
                                                 wxString::Format(
                                                    _T(" from \"%s\"") , from.c_str()) ).c_str() ),
                         _T("pgpsig_good"),
                         *wxGREEN) { }
};

class PGPInfoExpiredSig : public ClickablePGPInfo
{
public:
   PGPInfoExpiredSig(MessageView *msgView, const String& from)
      : ClickablePGPInfo(msgView,
                         wxString::Format( _("Expired PGP signature%s"), 
                                           (from.empty() ? 
                                                 _T("") :
                                                 wxString::Format(
                                                    _T(" from \"%s\"") , from.c_str()) ).c_str() ),
                         _T("pgpsig_exp"),
                         wxColour(0, 255, 255)) { }
};

class PGPInfoUntrustedSig : public ClickablePGPInfo
{
public:
   PGPInfoUntrustedSig(MessageView *msgView, const String& from)
      : ClickablePGPInfo(msgView,
                         wxString::Format( _("PGP Signature from untrusted key%s"), 
                                           (from.empty() ? 
                                                 _T("") :
                                                 wxString::Format(
                                                    _T(" \"%s\"") , from.c_str()) ).c_str() ),
                         _T("pgpsig_untrust"),
                         wxColour(255, 128, 0)) { }
};

class PGPInfoBadSig : public ClickablePGPInfo
{
public:
   PGPInfoBadSig(MessageView *msgView, const String& from)
      : ClickablePGPInfo(msgView,
                         wxString::Format( _("Bad PGP signature%s"), 
                                           (from.empty() ? 
                                                 _T("") :
                                                 wxString::Format(
                                                    _T(" from \"%s\"") , from.c_str()) ).c_str() ),
                         _T("pgpsig_bad"),
                         *wxRED) { }
};


class PGPInfoKeyNotFoundSig : public ClickablePGPInfo
{
public:
   PGPInfoKeyNotFoundSig(MessageView *msgView, const String& from)
      : ClickablePGPInfo(msgView,
                         wxString::Format( _("PGP public key not found%s"), 
                                           (from.empty() ? 
                                                 _T("") :
                                                 wxString::Format(
                                                    _T(" from \"%s\"") , from.c_str()) ).c_str() ),
                         _T("pgpsig_bad"),
                         wxColour(145, 145, 145)) { }
};

class PGPInfoGoodMsg : public ClickablePGPInfo
{
public:
   PGPInfoGoodMsg(MessageView *msgView)
      : ClickablePGPInfo(msgView,
                         _("Decrypted PGP message"),
                         _T("pgpmsg_ok"),
                         *wxGREEN) { }
};

class PGPInfoBadMsg : public ClickablePGPInfo
{
public:
   PGPInfoBadMsg(MessageView *msgView)
      : ClickablePGPInfo(msgView,
                         _("Encrypted PGP message"),
                         _T("pgpmsg_bad"),
                         *wxRED) { }
};


#endif
