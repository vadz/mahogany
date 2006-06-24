///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/ClickURL.cpp implementations of ClickableURL
// Purpose:     ClickableURL is a ClickableInfo which corresponds to an URL
//              embedded in the message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.12.02 (extracted from MessageView.cpp and wxMessageView.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 2001-2002 Mahogany Team
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"
   #include "Mdefaults.h"
   #include "guidef.h"
   #include "MApplication.h"

   #include <wx/menu.h>
   #include <wx/dataobj.h>
   #ifdef OS_WIN // cygwin and mingw
      #include <wx/msw/registry.h>
   #endif
#endif //USE_PCH

#include "gui/wxMDialogs.h"
#include "gui/wxOptionsDlg.h"

#include "ClickURL.h"
#include "MessageView.h"
#include "gui/wxMenuDefs.h"

#include "Address.h"
#include "Composer.h"
#include "Collect.h"

#include <wx/clipbrd.h>

#ifdef OS_UNIX
   #include <sys/stat.h>
#endif

class MPersMsgBox;

// ----------------------------------------------------------------------------
// menu item ids
// ----------------------------------------------------------------------------

enum
{
   WXMENU_URL_BEGIN,
   WXMENU_URL_OPEN,
   WXMENU_URL_OPEN_NEW,
   WXMENU_URL_COMPOSE,
   WXMENU_URL_REPLYTO,
   WXMENU_URL_FORWARDTO,
   WXMENU_ADD_TO_ADB,
   WXMENU_ADD_TO_WHITELIST,
   WXMENU_ADD_DOMAIN_TO_WHITELIST,
   WXMENU_URL_COPY,
   WXMENU_URL_END
};

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_ASK_URL_BROWSER;

// ----------------------------------------------------------------------------
// UrlPopup: the popup menu used with URLs in the message view
// ----------------------------------------------------------------------------

class UrlPopup : public wxMenu
{
public:
   UrlPopup(const ClickableURL *clickableURL);

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   const ClickableURL * const m_clickableURL;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(UrlPopup)
};

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_BROWSER;
extern const MOption MP_BROWSER_INNW;
extern const MOption MP_BROWSER_ISNS;
extern const MOption MP_WHITE_LIST;

// ============================================================================
// UrlPopup implementation
// ============================================================================

BEGIN_EVENT_TABLE(UrlPopup, wxMenu)
   EVT_MENU(-1, UrlPopup::OnCommandEvent)
END_EVENT_TABLE()

UrlPopup::UrlPopup(const ClickableURL *clickableURL)
        : m_clickableURL(clickableURL)
{
   // set a descriptive title
   String title;
   if ( m_clickableURL->IsMail() )
   {
      title = _("EMail address");
   }
   else // !mailto
   {
      const String& url = m_clickableURL->GetUrl();
      title = url.BeforeFirst(':').Upper();
      if ( title.length() == url.length() )
      {
         // no ':' in the URL, so it must be HTTP by default
         title = _T("HTTP");
      }

      title += _(" url");
   }

   SetTitle(title);

   // create the menu
   if ( m_clickableURL->IsMail() )
   {
      Append(WXMENU_URL_COMPOSE, _("&Write to"));
      Append(WXMENU_URL_REPLYTO, _("&Reply to"));
      Append(WXMENU_URL_FORWARDTO, _("&Forward to"));
      AppendSeparator();
      Append(WXMENU_ADD_TO_ADB, _("&Add to address book..."));
      Append(WXMENU_ADD_TO_WHITELIST, _("Add to &whitelist"));
      Append(WXMENU_ADD_DOMAIN_TO_WHITELIST, _("Add &domain to whitelist"));
   }
   else // !mailto
   {
      Append(WXMENU_URL_OPEN, _("&Open"));
      Append(WXMENU_URL_OPEN_NEW, _("Open in &new window"));
   }

   AppendSeparator();
   Append(WXMENU_URL_COPY, _("&Copy to clipboard"));
}

void
UrlPopup::OnCommandEvent(wxCommandEvent &event)
{
   const int id = event.GetId();
   switch ( id )
   {
      case WXMENU_URL_OPEN:
      case WXMENU_URL_OPEN_NEW:
         m_clickableURL->OpenInBrowser
                        (
                           id == WXMENU_URL_OPEN ? URLOpen_Default
                                                 : URLOpen_New_Window
                        );
         break;

      case WXMENU_URL_COMPOSE:
         m_clickableURL->OpenAddress();
         break;

      case WXMENU_URL_REPLYTO:
      case WXMENU_URL_FORWARDTO:
         {
            MessageView *msgview = m_clickableURL->GetMessageView();

            MailFolder::Params params;
            params.msgview = msgview;

            Composer *cv = (id == WXMENU_URL_REPLYTO
                              ? MailFolder::ReplyMessage
                              : MailFolder::ForwardMessage)
                                (
                                  msgview ? msgview->GetMessage() : NULL,
                                  params,
                                  m_clickableURL->GetProfile(),
                                  msgview ? msgview->GetWindow() : NULL,
                                  NULL
                                );

            if ( cv )
            {
               cv->SetAddresses(m_clickableURL->GetUrl());
            }
         }
         break;

      case WXMENU_ADD_TO_ADB:
         m_clickableURL->AddToAddressBook();
         break;

      case WXMENU_ADD_TO_WHITELIST:
      case WXMENU_ADD_DOMAIN_TO_WHITELIST:
         {
            AddressList_obj addrList(m_clickableURL->GetUrl());
            Address *addr = addrList->GetFirst();
            if ( !addr )
            {
               wxLogError(_("Failed to parse address \"%s\""),
                          m_clickableURL->GetUrl().c_str());
               break;
            }

            ASSERT_MSG( !addrList->HasNext(addr), "more than one address?" );

            const String str = id == WXMENU_ADD_DOMAIN_TO_WHITELIST
                                 ? addr->GetDomain()
                                 : addr->GetEMail();

            // this profile shouldn't be DecRef()'d, don't use Profile_obj
            Profile *profile = m_clickableURL->GetProfile();

            wxFrame *frame = m_clickableURL->GetMessageView()->GetParentFrame();

            wxArrayString whitelist(strutil_restore_array(
                                       READ_CONFIG(profile, MP_WHITE_LIST)));
            if ( whitelist.Index(str) != wxNOT_FOUND )
            {
               wxLogStatus(frame, _("\"%s\" is already in the white list"),
                           str.c_str());
               break;
            }

            whitelist.Add(str);

            profile->writeEntry(MP_WHITE_LIST, strutil_flatten_array(whitelist));

            wxLogStatus(frame, _("Added \"%s\" to the white list"), str.c_str());
         }
         break;

      case WXMENU_URL_COPY:
         {
            wxClipboardLocker lockClip;
            if ( !lockClip )
            {
               wxLogError(_("Failed to lock clipboard, URL not copied."));
            }
            else
            {
               wxTheClipboard->UsePrimarySelection();
               wxTheClipboard->SetData(new
                     wxTextDataObject(m_clickableURL->GetUrl()));
            }
         }
         break;

      default:
         FAIL_MSG( _T("unexpected URL popup menu command") );
         break;
   }
}

// ============================================================================
// ClickableURL implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ClickableURL ctor
// ----------------------------------------------------------------------------

ClickableURL::ClickableURL(MessageView *msgView, const String& url)
            : ClickableInfo(msgView)
{
   m_isMail = Unknown;

   // URL taken from the text may contain the line breaks if it had been
   // broken so remove them -- and also scan the string for ':' and '@' while
   // doing it
   bool hadColon = false;
   m_url.reserve(url.length());
   for ( const wxChar *p = url.c_str(); *p; p++ )
   {
      switch ( *p )
      {
         case _T('\r'):
         case _T('\n'):
            // ignore
            break;

         case _T('@'):
            if ( !hadColon )
            {
               // we don't have an explicit scheme, assume email address
               m_isMail = Yes;
            }
            // fall through

         case _T(':'):
            hadColon = true;

         default:
            m_url += *p;
      }
   }
}

// ----------------------------------------------------------------------------
// ClickableURL accessors
// ----------------------------------------------------------------------------

inline
Profile *ClickableURL::GetProfile() const
{
   return m_msgView->GetProfile();
}

bool ClickableURL::IsMail() const
{
   if ( m_isMail == Unknown )
   {
      // treat mail urls separately: we handle them ourselves
      wxString protocol = m_url.BeforeFirst(':');

      // protocol is either really the protocol or the whole URL if it didn't
      // contain ':'
      m_isMail = protocol == _T("mailto") ? Yes : No;
      if ( m_isMail == Yes )
      {
         // leave only the mail address (7 == strlen("mailto:"))
         const_cast<ClickableURL *>(this)->m_url.erase(0, 7);
      }
      else if ( protocol.length() == m_url.length() &&
                  m_url.find('@') != String::npos )
      {
         // a bare mail address
         m_isMail = Yes;
      }
   }

   return m_isMail == Yes;
}

const String& ClickableURL::GetUrl() const
{
   // discard "mailto:" if needed
   if ( m_isMail == Unknown )
      IsMail();

   return m_url;
}

String ClickableURL::GetLabel() const
{
   return GetUrl();
}

// ----------------------------------------------------------------------------
// ClickableURL operations
// ----------------------------------------------------------------------------

void ClickableURL::OpenInBrowser(int options) const
{
   bool inNewWindow = (options & URLOpen_New_Window) != 0;

   wxFrame *frame = m_msgView->GetParentFrame();
   wxLogStatus(frame, _("Opening URL '%s'..."), m_url.c_str());

   MBusyCursor bc;

   // the command to execute
   wxString command;

   bool bOk = false;

   String browser = READ_CONFIG(GetProfile(), MP_BROWSER);
   if ( browser.empty() )
   {
#ifdef OS_WIN
      // ShellExecute() always opens in the same window,
      // so do it manually for new window
      if ( inNewWindow )
      {
         wxRegKey key(wxRegKey::HKCR, m_url.BeforeFirst(':') + "\\shell\\open");
         if ( key.Exists() )
         {
            wxRegKey keyDDE(key, "DDEExec");
            if ( keyDDE.Exists() )
            {
               wxString ddeTopic = wxRegKey(keyDDE, "topic");

               // we only know the syntax of WWW_OpenURL DDE request
               if ( ddeTopic == "WWW_OpenURL" )
               {
                  wxString ddeCmd = keyDDE;

                  // this is a bit naive but should work as -1 can't appear
                  // elsewhere in the DDE topic, normally
                  if ( ddeCmd.Replace("-1", "0",
                                      FALSE /* only first occurence */) == 1 )
                  {
                     // and also replace the parameters
                     if ( ddeCmd.Replace("%1", m_url, FALSE) == 1 )
                     {
                        // magic incantation understood by wxMSW
                        command << "WX_DDE#"
                                << wxRegKey(key, "command").QueryDefaultValue() << '#'
                                << wxRegKey(keyDDE, "application").QueryDefaultValue()
                                << '#' << ddeTopic << '#'
                                << ddeCmd;
                     }
                  }
               }
            }
         }
      }

      if ( !command.empty() )
      {
         wxString errmsg;
         errmsg.Printf(_("Could not launch browser: '%s' failed."),
                       command.c_str());
         bOk = m_msgView->LaunchProcess(command, errmsg);
      }
      else // easy case: open in the same window
      {
#if !defined(__CYGWIN__) && !defined(__MINGW32__) // FIXME ShellExecute() is defined in <w32api/shellapi.h>, how to include it?
         bOk = (int)ShellExecute(NULL, "open", m_url,
                                 NULL, NULL, SW_SHOWNORMAL ) > 32;
#endif
      }
#else  // Unix
      // propose to choose program for opening URLs
      if ( MDialog_YesNoDialog
           (
            _("No command configured to view URLs.\n"
              "Would you like to choose one now?"),
            frame,
            MDIALOG_YESNOTITLE,
            M_DLG_YES_DEFAULT,
            M_MSGBOX_ASK_URL_BROWSER
           )
         )
      {
         ShowOptionsDialog();
      }

      browser = READ_CONFIG_TEXT(GetProfile(), MP_BROWSER);
      if ( browser.empty() )
      {
         wxLogError(_("No command configured to view URLs."));
         bOk = false;
      }
#endif // Win/Unix
   }
   else // browser setting non empty, use it
   {
#ifdef OS_UNIX
      if ( READ_CONFIG(GetProfile(), MP_BROWSER_ISNS) ) // try re-loading first
      {
         wxString lockfile;
         wxGetHomeDir(&lockfile);
         if ( !wxEndsWithPathSeparator(lockfile) )
            lockfile += DIR_SEPARATOR;
         lockfile << _T(".netscape") << DIR_SEPARATOR << _T("lock");
         struct stat statbuf;

         // cannot use wxFileExists here, because it's a link pointing to a
         // non-existing location!
         if ( lstat(lockfile.mb_str(), &statbuf) == 0 )
         {
            command << browser << _T(" -remote openURL(") << m_url;
            if ( inNewWindow )
            {
               command << _T(",new-window)");
            }
            else
            {
               command << _T(")");
            }
            wxString errmsg;
            errmsg.Printf(_("Could not launch browser: '%s' failed."),
                          command.c_str());
            bOk = m_msgView->LaunchProcess(command, errmsg);
         }
      }
#endif // Unix
      // either not netscape or ns isn't running or we have non-UNIX
      if(! bOk)
      {
         command = browser;
         command << _T(' ') << m_url;

         wxString errmsg;
         errmsg.Printf(_("Couldn't launch browser: '%s' failed"),
                       command.c_str());

         bOk = m_msgView->LaunchProcess(command, errmsg);
      }
   }

   if ( bOk )
   {
      wxLogStatus(frame, _("Opening URL '%s'... done."), m_url.c_str());
   }
   else
   {
      wxLogStatus(frame, _("Opening URL '%s' failed."), m_url.c_str());
   }
}

void ClickableURL::OpenAddress() const
{
   Composer *cv = Composer::CreateNewMessage(GetProfile());

   CHECK_RET( cv, _T("creating new message failed?") );

   cv->SetAddresses(m_url);
   cv->InitText();
}

void ClickableURL::AddToAddressBook() const
{
   wxArrayString addresses;
   addresses.Add(m_url);

   InteractivelyCollectAddresses(addresses,
                                 READ_APPCONFIG(MP_AUTOCOLLECT_ADB),
                                 m_msgView->GetFolderName(),
                                 m_msgView->GetParentFrame());
}

// ----------------------------------------------------------------------------
// ClickableURL click handlers
// ----------------------------------------------------------------------------

void ClickableURL::OnLeftClick(const wxPoint& /* pt */) const
{
   if ( IsMail() )
   {
      OpenAddress();
   }
   else // non mailto URL
   {
      OpenInBrowser(READ_CONFIG_BOOL(GetProfile(), MP_BROWSER_INNW)
                     ? URLOpen_New_Window
                     : URLOpen_Default);
   }
}

void ClickableURL::OnDoubleClick(const wxPoint& pt) const
{
   // no special action for double clicking
   OnLeftClick(pt);
}

void ClickableURL::OnRightClick(const wxPoint& pt) const
{
   UrlPopup menu(this);

   m_msgView->GetWindow()->PopupMenu(&menu, pt);
}

