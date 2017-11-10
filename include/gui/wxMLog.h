///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxMLog.h
// Purpose:     wxMLog is a custom wxLog target showing the messages in a less
//              intrusive way by using the active wxInfoBar or showing them in
//              the default way if there is no active one currently
// Author:      Vadim Zeitlin
// Created:     2017-09-30
// Copyright:   (c) 2017 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_MLOG_H_
#define _M_MLOG_H_

#include <wx/log.h>

class wxInfoBarBase;

// This class is never used directly, you need to use wxMLogTargetSetter below
// to set the given wxInfoBar object as the target of log messages.
class wxMLog : public wxLogGui
{
public:
   // Install an object of this class as current wxLog target.
   //
   // Normally should be called exactly once, on program startup.
   static void Activate();

protected:
   virtual void DoLogRecord(wxLogLevel level,
                            const wxString& msg,
                            const wxLogRecordInfo& info);

private:
   // Ctor is private, use static Activate() to actually create the object.
   wxMLog() = default;

   // This method is private and can be called only by wxMLogTargetSetter.
   void SetInfoBarToUse(wxInfoBarBase* infobar);

   // The unique currently used wxMLog.
   static wxMLog* ms_MLog;

   // SetInfoBarToUse() must be called to change this from outside this class.
   wxInfoBarBase* m_activeInfoBar = nullptr;

   // The last shown message using the same infobar as is currently used.
   wxString m_lastMsg;

   // The level of the last shown message, only used if m_lastMsg is not empty.
   wxLogLevel m_lastShownLevel = wxLOG_Max;

   friend class wxMLogTargetSetter;

   wxMLog(const wxMLog&);
   wxMLog& operator=(const wxMLog&);
};

// Small RAII helper class ensuring that the active info bar is reset in its
// dtor.
class wxMLogTargetSetter
{
public:
   explicit wxMLogTargetSetter(wxInfoBarBase* infobar)
      : m_infobarOrig(wxMLog::ms_MLog ? wxMLog::ms_MLog->m_activeInfoBar : NULL)
   {
      if ( wxMLog::ms_MLog )
         wxMLog::ms_MLog->SetInfoBarToUse(infobar);
   }

   ~wxMLogTargetSetter()
   {
      if ( wxMLog::ms_MLog )
         wxMLog::ms_MLog->SetInfoBarToUse(m_infobarOrig);
   }

private:
   wxInfoBarBase* const m_infobarOrig;

   wxMLogTargetSetter(const wxMLogTargetSetter&);
   wxMLogTargetSetter& operator=(const wxMLogTargetSetter&);
};

#endif // _M_MLOG_H_
