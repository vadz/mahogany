///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/MessageView.cpp - non GUI logic of msg viewing
// Purpose:     MessageView class does everything related to the message
//              viewing not related to GUI - the rest is done by a GUI viewer
//              which is just an object implementing MessageViewer interface
//              and which is responsible for the GUI
// Author:      Vadim Zeitlin (based on gui/MessageView.cpp by Karsten)
// Modified by:
// Created:     24.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Mahogany Team
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#   pragma implementation "MessageView.h"
#endif

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"

#  include "PathFinder.h"
#  include "Profile.h"

#  include "MFrame.h"

#  include <wx/menu.h>
#endif //USE_PCH

#include "Mdefaults.h"
#include "MHelp.h"
#include "Message.h"
#include "FolderView.h"
#include "ASMailFolder.h"
#include "MFolder.h"
#include "MDialogs.h"
#include "MessageView.h"
#include "XFace.h"
#include "miscutil.h"
#include "sysutil.h"

#include "MessageTemplate.h"
#include "Composer.h"

#include "gui/wxIconManager.h"

#include <wx/dynarray.h>
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/fontmap.h>
#include <wx/clipbrd.h>
#include <wx/process.h>

#include <ctype.h>  // for isspace
#include <time.h>   // for time stamping autocollected addresses

#ifdef OS_UNIX
#   include <sys/stat.h>
#endif

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

// make the first letter of the string capitalized and all the others of lower
// case, it looks nicer like this when presented to the user
static String NormalizeString(const String& s)
{
   String norm;
   if ( !s.empty() )
   {
      norm << String(s[0]).Upper() << String(s.c_str() + 1).Lower();
   }

   return norm;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Data about a process (external viewer typically) we launched: these objects
// are created by LaunchProcess() and deleted when the viewer process
// terminates. If it terminates with non 0 exit code, errormsg is given to the
// user. The tempfile is the name of a temp file containing the data passedto
// the viewer (or NULL if none) and will be removed after viewer terminates.
class ProcessInfo
{
public:
   ProcessInfo(wxProcess *process,

               int pid,
               const String& errormsg,
               const String& tempfilename)
      : m_errormsg(errormsg)
   {
      ASSERT_MSG( process && pid, "invalid process in ProcessInfo" );

      m_process = process;
      m_pid = pid;

      if ( !tempfilename.IsEmpty() )
         m_tempfile = new MTempFileName(tempfilename);
      else
         m_tempfile = NULL;
   }

   ~ProcessInfo()
   {
      if ( m_process )
         delete m_process;
      if ( m_tempfile )
         delete m_tempfile;
   }

   // get the pid of our process
   int GetPid() const { return m_pid; }

   // get the error message
   const String& GetErrorMsg() const { return m_errormsg; }

   // don't delete wxProcess object (must be called before destroying this
   // object if the external process is still running)
   void Detach() { m_process->Detach(); m_process = NULL; }

   // return the pointer to temp file object (may be NULL)
   MTempFileName *GetTempFile() const { return m_tempfile; }

private:
   String         m_errormsg; // error message to give if launch failed
   wxProcess     *m_process;  // wxWindows process info
   int            m_pid;      // pid of the process
   MTempFileName *m_tempfile; // the temp file (or NULL if none)
};

// the popup menu invoked by clicking on an attachment in the mail message
class MimePopup : public wxMenu
{
public:
   MimePopup(MessageView *parent, int partno)
   {
      // init member vars
      m_PartNo = partno;
      m_MessageView = parent;

      // create the menu items
      Append(WXMENU_MIME_INFO, _("&Info"));
      AppendSeparator();
      Append(WXMENU_MIME_OPEN, _("&Open"));
      Append(WXMENU_MIME_OPEN_WITH, _("Open &with..."));
      Append(WXMENU_MIME_SAVE, _("&Save..."));
      Append(WXMENU_MIME_VIEW_AS_TEXT, _("&View as text"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   MessageView *m_MessageView;
   int m_PartNo;

   DECLARE_EVENT_TABLE()
};

// the popup menu used with URLs
class UrlPopup : public wxMenu
{
public:
   UrlPopup(MessageView *parent, const String& url)
      : m_url(url)
   {
      m_MessageView = parent;

      SetTitle(NormalizeString(url.BeforeFirst(':')) + _(" url"));
      Append(WXMENU_URL_OPEN, _("&Open"));
      Append(WXMENU_URL_OPEN_NEW, _("Open in &new window"));
      Append(WXMENU_URL_COPY, _("&Copy to clipboard"));
   }

   // callbacks
   void OnCommandEvent(wxCommandEvent &event);

private:
   MessageView *m_MessageView;
   String m_url;

   DECLARE_EVENT_TABLE()
};

// the message parameters for the MIME type manager
class MailMessageParameters : public wxFileType::MessageParameters
{
public:
   MailMessageParameters(const wxString& filename,
         const wxString& mimetype,
         Message *mailMessage,
         int part)
      : wxFileType::MessageParameters(filename, mimetype)
      {
         m_mailMessage = mailMessage;
         m_part = part;
      }

   virtual wxString GetParamValue(const wxString& name) const;

private:
   Message *m_mailMessage;
   int m_part;
};

// the var expander for message view frame title
//
// TODO: the code should be reused with VarExpander in wxComposeView.cpp!
class MsgVarExpander : public MessageTemplateVarExpander
{
public:
   MsgVarExpander(Message *msg) { m_msg = msg; SafeIncRef(m_msg); }
   virtual ~MsgVarExpander() { SafeDecRef(m_msg); }

   virtual bool Expand(const String& category,
                       const String& Name,
                       const wxArrayString& arguments,
                       String *value) const
   {
      if ( !m_msg )
         return false;

      // we only understand fields in the unnamed/default category
      if ( !category.empty() )
         return false;

      String name = Name.Lower();
      if ( name == "from" )
         *value = m_msg->From();
      else if ( name == "subject" )
         *value = m_msg->Subject();
      else if ( name == "date" )
         *value = m_msg->Date();
      else
         return false;

      return true;
   }

private:
    Message *m_msg;
};

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MimePopup, wxMenu)
   EVT_MENU(-1, MimePopup::OnCommandEvent)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(UrlPopup, wxMenu)
   EVT_MENU(-1, UrlPopup::OnCommandEvent)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================


static String
GetParameter(Message *msg, int partno, const String &param)
{
   String value;
   if ( !msg->ExpandParameter(msg->GetParameters(partno), param, &value) )
      (void)msg->ExpandParameter(msg->GetDisposition(partno), param, &value);

   return value;
}


static
String GetFileNameForMIME(Message *message, int partNo)
{
   String fileName = GetParameter(message, partNo, "FILENAME");
   if ( fileName.empty() )
      fileName = GetParameter(message, partNo,"NAME");

   return fileName;
}


void
MimePopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_MIME_INFO:
         m_MessageView->MimeInfo(m_PartNo);
         break;

      case WXMENU_MIME_OPEN:
         m_MessageView->MimeHandle(m_PartNo);
         break;

      case WXMENU_MIME_OPEN_WITH:
         m_MessageView->MimeOpenWith(m_PartNo);
         break;

      case WXMENU_MIME_VIEW_AS_TEXT:
         m_MessageView->MimeViewText(m_PartNo);
         break;

      case WXMENU_MIME_SAVE:
         m_MessageView->MimeSave(m_PartNo);
         break;
   }
}

wxString
MailMessageParameters::GetParamValue(const wxString& name) const
{
   const MessageParameterList &plist = m_mailMessage->GetParameters(m_part);
   MessageParameterList::iterator i;
   for ( i = plist.begin(); i != plist.end(); i++) {
      if ( name.CmpNoCase((*i)->name) == 0 ) {
         // found
         return (*i)->value;
      }
   }

   const MessageParameterList &dlist = m_mailMessage->GetDisposition(m_part);
   for ( i = dlist.begin(); i != dlist.end(); i++) {
      if ( name.CmpNoCase((*i)->name) == 0 ) {
         // found
         return (*i)->value;
      }
   }

   // if all else failed, call the base class

   // typedef is needed for VC++ 5.0 - otherwise you get a compile error!
   typedef wxFileType::MessageParameters BaseMessageParameters;
   return BaseMessageParameters::GetParamValue(name);
}

// ----------------------------------------------------------------------------
// UrlPopup
// ----------------------------------------------------------------------------

void
UrlPopup::OnCommandEvent(wxCommandEvent &event)
{
   switch ( event.GetId() )
   {
      case WXMENU_URL_OPEN:
         m_MessageView->OpenURL(m_url, FALSE);
         break;

      case WXMENU_URL_OPEN_NEW:
         m_MessageView->OpenURL(m_url, TRUE);
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
               wxTheClipboard->SetData(new wxTextDataObject(m_url));
            }
         }
         break;

      default:
         FAIL_MSG( "unexpected URL popup menu command" );
         break;
   }
}
// ----------------------------------------------------------------------------
// MessageView::AllProfileValues
// ----------------------------------------------------------------------------

bool MessageView::AllProfileValues::operator==(const AllProfileValues& other)
{
   #define CMP(x) (x == other.x)

   return CMP(BgCol) && CMP(FgCol) && CMP(UrlCol) &&
          CMP(QuotedCol[0]) && CMP(QuotedCol[1]) && CMP(QuotedCol[2]) &&
          CMP(quotedColourize) && CMP(quotedCycleColours) &&
          CMP(quotedMaxWhitespace) && CMP(quotedMaxAlpha) &&
          CMP(HeaderNameCol) && CMP(HeaderValueCol) &&
          CMP(fontFamily) && CMP(fontSize) &&
          CMP(showHeaders) && CMP(inlineRFC822) && CMP(inlinePlainText) &&
          CMP(highlightURLs) && CMP(inlineGFX) &&
          CMP(browser) && CMP(browserInNewWindow) &&
          CMP(autocollect) && CMP(autocollectNamed) &&
          CMP(autocollectBookName) &&
#ifdef OS_UNIX
          CMP(browserIsNS) && CMP(afmpath) &&
#endif // Unix
          CMP(showFaces);

   #undef CMP
}

// ----------------------------------------------------------------------------
// MessageView
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MessageView, wxLayoutWindow)
   // process termination notification
   EVT_END_PROCESS(-1, MessageView::OnProcessTermination)

   // mouse click processing
   EVT_MENU(WXLOWIN_MENU_RCLICK, MessageView::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_LCLICK, MessageView::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_DBLCLICK, MessageView::OnMouseEvent)

   // menu & toolbars
   EVT_MENU(-1, MessageView::OnMenuCommand)
   EVT_TOOL(-1, MessageView::OnMenuCommand)
   EVT_CHAR(MessageView::OnChar)
END_EVENT_TABLE()

void
MessageView::SetLanguage(int id)
{
   wxFontEncoding encoding = GetEncodingFromMenuCommand(id);
   SetEncoding(encoding);
}

void
MessageView::OnChar(wxKeyEvent& event)
{
   // FIXME: this should be more intelligent, i.e. use the
   // wxlayoutwindow key bindings:
   if(m_WrapMargin > 0 &&
      event.KeyCode() == 'q' || event.KeyCode() == 'Q')
   {
      // temporarily allow editing to enable manual word wrap:
      SetEditable(TRUE);
      event.Skip();
      SetEditable(FALSE);
      SetCursorVisibility(1);
   }
   else
      event.Skip();
}

void
MessageView::MessageView(wxWindow *parent)
{
   m_mailMessage = NULL;
   m_Parent = parent;
   m_uid = UID_ILLEGAL;
   m_encodingUser = wxFONTENCODING_SYSTEM;
   SetMouseTracking();
   SetParentProfile(fv ? fv->GetProfile() : NULL);

   wxFrame *p = GetFrame(this);
   if ( p )
   {
      wxStatusBar *statusBar = p->GetStatusBar();
      if ( statusBar )
      {
         // we don't edit the message, so the cursor coordinates are useless
         SetStatusBar(statusBar, 0, -1);
      }
   }
   RegisterForEvents();
}


MessageView::MessageView(wxFolderView *fv,
                             wxWindow *parent,
                             bool show)
             : wxLayoutWindow(parent)
{
   m_asyncFolder = NULL;
   m_Profile = NULL;
   Create(fv,parent);
   Show(show);
}

MessageView::MessageView(ASMailFolder *folder,
                             long num,
                             wxFolderView *fv,
                             wxWindow *parent,
                             bool show)
             : wxLayoutWindow(parent)
{
   m_asyncFolder = folder;
   SafeIncRef(m_asyncFolder);
   m_Profile = NULL;
   Create(fv,parent);
   if(folder)
      ShowMessage(folder,num);
   Show(show);
}

MessageView::~MessageView()
{
   if ( m_regCookieOptionsChange )
      MEventManager::Deregister(m_regCookieOptionsChange);
   if(m_regCookieASFolderResult)
      MEventManager::Deregister(m_regCookieASFolderResult);

   size_t procCount = m_processes.GetCount();
   for ( size_t n = 0; n < procCount; n++ )
   {
      ProcessInfo *info = m_processes[n];
      info->Detach();

      MTempFileName *tempfile = info->GetTempFile();
      if ( tempfile )
      {
         String tempFileName = tempfile->GetName();
         wxLogWarning(_("Temporary file '%s' left because it is still in "
                        "use by an external process"), tempFileName.c_str());
      }

      delete info;
   }

   if(m_mailMessage) m_mailMessage->DecRef();
   if(m_Profile) m_Profile->DecRef();
   SafeDecRef(m_asyncFolder);
}

void
MessageView::RegisterForEvents()
{
   // register with the event manager
   m_regCookieOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_regCookieOptionsChange, "can't register for options change event");
   m_regCookieASFolderResult = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookieASFolderResult, "can't reg folder view with event manager");
}

/// Tell it a new parent profile - in case folder changed.
void
MessageView::SetParentProfile(Profile *profile)
{
   SafeDecRef(m_Profile);

   if(profile)
   {
      m_Profile = profile;
      m_Profile->IncRef();
   }
   else
   {
      m_Profile = Profile::CreateEmptyProfile();
   }

   UpdateProfileValues();

   Clear();
}

void
MessageView::UpdateProfileValues()
{
   ReadAllSettings(&m_ProfileValues);
}

void
MessageView::ReadAllSettings(AllProfileValues *settings)
{
   Profile *profile = GetProfile();
   CHECK_RET( profile, "MessageView::ReadAllSettings: no profile" );

   // We also use this to set all values to be read to speed things up:
   #define GET_COLOUR_FROM_PROFILE(which, name) \
      GetColourByName(&settings->which, \
                      READ_CONFIG(profile, MP_MVIEW_##name), \
                      MP_MVIEW_##name##_D)

   GET_COLOUR_FROM_PROFILE(FgCol, FGCOLOUR);
   GET_COLOUR_FROM_PROFILE(BgCol, BGCOLOUR);
   GET_COLOUR_FROM_PROFILE(UrlCol, URLCOLOUR);
   GET_COLOUR_FROM_PROFILE(QuotedCol[0], QUOTED_COLOUR1);
   GET_COLOUR_FROM_PROFILE(QuotedCol[1], QUOTED_COLOUR2);
   GET_COLOUR_FROM_PROFILE(QuotedCol[2], QUOTED_COLOUR3);
   GET_COLOUR_FROM_PROFILE(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderValueCol, HEADER_VALUES_COLOUR);

   #undef GET_COLOUR_FROM_PROFILE

   settings->quotedColourize = READ_CONFIG(profile, MP_MVIEW_QUOTED_COLOURIZE) != 0;
   settings->quotedCycleColours = READ_CONFIG(profile, MP_MVIEW_QUOTED_CYCLE_COLOURS) != 0;
   settings->quotedMaxWhitespace = READ_CONFIG(profile, MP_MVIEW_QUOTED_MAXWHITESPACE) != 0;
   settings->quotedMaxAlpha = READ_CONFIG(profile,MP_MVIEW_QUOTED_MAXALPHA);

   static const int fontFamilies[] =
   {
      wxDEFAULT,
      wxDECORATIVE,
      wxROMAN,
      wxSCRIPT,
      wxSWISS,
      wxMODERN,
      wxTELETYPE
   };

   long idx = READ_CONFIG(profile, MP_MVIEW_FONT);
   if ( idx < 0 || idx >= WXSIZEOF(fontFamilies) )
   {
      FAIL_MSG( "corrupted config data" );

      idx = 0;
   }

   settings->fontFamily = fontFamilies[idx];
   settings->fontSize = READ_CONFIG(profile,MP_MVIEW_FONT_SIZE);
   settings->showHeaders = READ_CONFIG(profile,MP_SHOWHEADERS) != 0;
   settings->inlinePlainText = READ_CONFIG(profile,MP_PLAIN_IS_TEXT) != 0;
   settings->inlineRFC822 = READ_CONFIG(profile,MP_RFC822_IS_TEXT) != 0;
   settings->highlightURLs = READ_CONFIG(profile,MP_HIGHLIGHT_URLS) != 0;

   // we set inlineGFX to -1 if we don't inline graphics at all and to the
   // max size limit of graphics to show inline otherwise (0 if no limit)
   settings->inlineGFX = READ_CONFIG(profile, MP_INLINE_GFX)
                         ? READ_CONFIG(profile, MP_INLINE_GFX_SIZE)
                         : -1;

   settings->browser = READ_CONFIG(profile, MP_BROWSER);
   settings->browserInNewWindow = READ_CONFIG(profile, MP_BROWSER_INNW) != 0;
   settings->autocollect =  READ_CONFIG(profile, MP_AUTOCOLLECT);
   settings->autocollectNamed =  READ_CONFIG(profile, MP_AUTOCOLLECT_NAMED);
   settings->autocollectBookName = READ_CONFIG(profile, MP_AUTOCOLLECT_ADB);
   settings->showFaces = READ_CONFIG(profile, MP_SHOW_XFACES) != 0;

   // these settings are used under Unix only
#ifdef OS_UNIX
   settings->browserIsNS = READ_CONFIG(profile, MP_BROWSER_ISNS) != 0;
   settings->afmpath = READ_APPCONFIG(MP_AFMPATH);
#endif // Unix

#ifndef OS_WIN
   SetFocusFollowMode(READ_CONFIG(profile,MP_FOCUS_FOLLOWSMOUSE) != 0);
#endif
   SetWrapMargin(READ_CONFIG(profile, MP_VIEW_WRAPMARGIN));

   // update the parents menu as the show headers option might have changed
   UpdateShowHeadersInMenu();
}

void
MessageView::Clear()
{
   m_viewer->Clear();
   m_viewer->Update();

   m_uid = UID_ILLEGAL;
   if ( m_mailMessage )
   {
      m_mailMessage->DecRef();
      m_mailMessage = NULL;
   }
}

void
MessageView::SetEncoding(wxFontEncoding encoding)
{
   m_encodingUser = encoding;

   Update();

   if ( READ_CONFIG(m_Profile, MP_MSGVIEW_AUTO_ENCODING) )
   {
      // don't keep it for the other messages, just for this one
      m_encodingUser = wxFONTENCODING_SYSTEM;
   }
}

// ----------------------------------------------------------------------------
// MessageView headers processing
// ----------------------------------------------------------------------------

wxFontEncoding
MessageView::ShowHeaders()
{
   // if wanted, display all header lines
   if ( m_ProfileValues.showHeaders )
   {
      m_viewer->ShowRawHeaders(m_mailMessage->GetHeader());
   }

   // retrieve all headers at once instead of calling Message::GetHeaderLine()
   // many times: this is incomparably faster with remote servers (one round
   // trip to server is much less expensive than a dozen of them!)

   // all the headers the user configured
   wxArrayString headersUser =
      strutil_restore_array(':', READ_CONFIG(m_Profile, MP_MSGVIEW_HEADERS));

   // X-Face is handled separately
#ifdef HAVE_XFACES
   if ( m_ProfileValues.showFaces )
   {
      headersUser.Insert("X-Face", 0);
   }
#endif // HAVE_XFACES

   size_t countHeaders = headersUser.GetCount();

   // stupidly, MP_MSGVIEW_HEADERS_D is terminated with a ':' so there
   // is a dummy empty header at the end - just ignore it for compatibility
   // with existing config files
   if ( countHeaders && headersUser.Last().empty() )
   {
      countHeaders--;
   }

   if ( !countHeaders )
   {
      // no headers at all, don't waste time below
      return wxFONTENCODING_SYSTEM;
   }

   // these headers can be taken from the envelope instead of retrieving them
   // from server, so exclude them from headerNames which is the array of
   // headers we're going to retrieve from server
   //
   // the trouble is that we want to keep the ordering of headers correct,
   // hence all the contorsions below: we rmemeber the indices and then inject
   // them back into the code processing headers in the loop
   enum EnvelopHeader
   {
      EnvelopHeader_From,
      EnvelopHeader_To,
      EnvelopHeader_Cc,
      EnvelopHeader_Bcc,
      EnvelopHeader_Subject,
      EnvelopHeader_Date,
      EnvelopHeader_Newsgroups,
      EnvelopHeader_MessageId,
      EnvelopHeader_InReplyTo,
      EnvelopHeader_References,
      EnvelopHeader_Max
   };

   size_t n;

   // put the stuff into the array to be able to use Index() below: a bit
   // silly but not that much as it also gives us index checking in debug
   // builds
   static wxArrayString envelopHeaders;
   if ( envelopHeaders.IsEmpty() )
   {
      // init it on first use
      static const char *envelopHeadersNames[] =
      {
         "From",
         "To",
         "Cc",
         "Bcc",
         "Subject",
         "Date",
         "Newsgroups",
         "Message-Id",
         "In-Reply-To",
         "References",
      };

      ASSERT_MSG( EnvelopHeader_Max == WXSIZEOF(envelopHeadersNames),
                  "forgot to update something - should be kept in sync!" );

      for ( n = 0; n < WXSIZEOF(envelopHeadersNames); n++ )
      {
         envelopHeaders.Add(envelopHeadersNames[n]);
      }
   }

   // the index of the envelop headers if we have to show it, -1 otherwise
   int envelopIndices[EnvelopHeader_Max];
   for ( n = 0; n < EnvelopHeader_Max; n++ )
   {
      envelopIndices[n] = wxNOT_FOUND;
   }

   // a boolean array, in fact
   wxArrayInt headerIsEnv;
   headerIsEnv.Alloc(countHeaders);

   wxArrayString headerNames;

   size_t countNonEnvHeaders = 0;
   for ( n = 0; n < countHeaders; n++ )
   {
      const wxString& h = headersUser[n];

      // we don't need to retrieve envelop headers
      int index = envelopHeaders.Index(h, false /* case insensitive */);
      if ( index == wxNOT_FOUND )
      {
         countNonEnvHeaders++;
      }

      headerIsEnv.Add(index != wxNOT_FOUND);
      headerNames.Add(h);
   }

   // any non envelop headers to retrieve?
   size_t nNonEnv;
   wxArrayInt headerNonEnvEnc;
   wxArrayString headerNonEnvValues;
   if ( countNonEnvHeaders )
   {
      const char **headerPtrs = new const char *[countNonEnvHeaders + 1];

      // have to copy the headers into a temp buffer unfortunately
      for ( nNonEnv = 0, n = 0; n < countHeaders; n++ )
      {
         if ( !headerIsEnv[n] )
         {
            headerPtrs[nNonEnv++] = headerNames[n].c_str();
         }
      }

      // did their number change from just recounting?
      ASSERT_MSG( nNonEnv == countNonEnvHeaders, "logic error" );

      headerPtrs[countNonEnvHeaders] = NULL;

      // get them all at once
      headerNonEnvValues = m_mailMessage->GetHeaderLines(headerPtrs,
                                                         &headerNonEnvEnc);

      delete [] headerPtrs;
   }

   // combine the values of the headers retrieved above with those of the
   // envelop headers into one array
   wxArrayInt headerEncodings;
   wxArrayString headerValues;
   for ( nNonEnv = 0, n = 0; n < countHeaders; n++ )
   {
      if ( headerIsEnv[n] )
      {
         int envhdr = envelopHeaders.Index(headerNames[n]);
         if ( envhdr == wxNOT_FOUND )
         {
            // if headerIsEnv, then it must be in the array
            FAIL_MSG( "logic error" );

            continue;
         }

         // get the raw value
         String value;
         switch ( envhdr )
         {
            case EnvelopHeader_From:
            case EnvelopHeader_To:
            case EnvelopHeader_Cc:
            case EnvelopHeader_Bcc:
               {
                  MessageAddressType mat;
                  switch ( envhdr )
                  {
                     default: FAIL_MSG( "forgot to add header here ");
                     case EnvelopHeader_From: mat = MAT_FROM; break;
                     case EnvelopHeader_To: mat = MAT_TO; break;
                     case EnvelopHeader_Cc: mat = MAT_CC; break;
                     case EnvelopHeader_Bcc: mat = MAT_BCC; break;
                  }

                  String name;
                  String addr = m_mailMessage->Address(name, mat);
                  value = GetFullEmailAddress(name, addr);
               }
               break;

            case EnvelopHeader_Subject:
               value = m_mailMessage->Subject();
               break;

            case EnvelopHeader_Date:
               // don't read the header line directly because Date() function
               // might return date in some format different from RFC822 one
               value = m_mailMessage->Date();
               break;

            case EnvelopHeader_Newsgroups:
               value = m_mailMessage->GetNewsgroups();
               break;

            case EnvelopHeader_MessageId:
               value = m_mailMessage->GetId();
               break;

            case EnvelopHeader_InReplyTo:
               value = m_mailMessage->GetInReplyTo();
               break;

            case EnvelopHeader_References:
               value = m_mailMessage->GetReferences();
               break;


            default:
               FAIL_MSG( "unknown envelop header" );
         }

         // extract encoding info from it
         wxFontEncoding enc;
         headerValues.Add(MailFolder::DecodeHeader(value, &enc));
         headerEncodings.Add(enc);
      }
      else // non env header
      {
         headerValues.Add(headerNonEnvValues[nNonEnv]);
         headerEncodings.Add(headerNonEnvEnc[nNonEnv]);

         nNonEnv++;
      }
   }

   // for the loop below: we start it at 0 normally but at 1 if we have an
   // X-Face as we don't want to show it verbatim ...
   n = 0;

   // ... instead we show an icon for it
   if ( m_ProfileValues.showFaces )
   {
      wxString xfaceString = headerValues[n++];
      if ( xfaceString.length() > 2 )   //\r\n
      {
         XFace *xface = new XFace;
         xface->CreateFromXFace(xfaceString.c_str());

         char **xfaceXpm;
         if ( xface->CreateXpm(&xfaceXpm) )
         {
            DoShowXFace(wxBitmap(xfaceXpm));

            wxIconManager::FreeImage(xfaceXpm);
         }

         delete xface;
      }
   }

   // show the headers using the correct encoding now
   wxFontEncoding encInHeaders = wxFONTENCODING_SYSTEM;
   for ( ; n < countHeaders; n++ )
   {
      wxFontEncoding encHeader = (wxFontEncoding)headerEncodings[n];

      // remember the encoding that we have found in the headers: some mailers
      // are broken in so weird way that they use correct format for the
      // headers but fail to specify charset parameter in Content-Type (the
      // best of the Web mailers do this - the worst/normal just send 8 bit in
      // the headers too)
      if ( encHeader != wxFONTENCODING_SYSTEM )
      {
         // we deal with them by supposing that the body has the same encoding
         // as the headers by default, so we remember encInHeaders here and
         // use it later when showing the body
         encInHeaders = encHeader;
      }
      else // no encoding in the header
      {
         // use the user specified encoding if none specified in the header
         // itself
         if ( m_encodingUser != wxFONTENCODING_SYSTEM )
            encHeader = m_encodingUser;
      }

      m_viewer->ShowHeaders(headerNames[n], headerValues[n], encHeader);
   }

   return encInHeaders;
}

// ----------------------------------------------------------------------------
// MessageView text part processing
// ----------------------------------------------------------------------------

size_t
MessageView::GetQuotedLevel(const char *text) const
{
   size_t qlevel = strutil_countquotinglevels
                   (
                     text,
                     m_ProfileValues.quotedMaxWhitespace,
                     m_ProfileValues.quotedMaxAlpha
                   );

   static const qlevelMax = WXSIZEOF(AllProfileValues::QuotedCol);

   // note that qlevel is counted from 1, really, as 0 means unquoted and that
   // GetQuoteColour() relies on this
   if ( qlevel > qlevelMax )
   {
      if ( m_ProfileValues.quotedCycleColours )
      {
         // cycle through the colours: use 1st level colour for qlevelMax
         qlevel = (qlevel - 1) % qlevelMax + 1;
      }
      else
      {
         // use the same colour for all levels deeper than max
         qlevel = qlevelMax;
      }
   }

   return qlevel;
}

wxColour MessageView::GetQuoteColour(size_t qlevel) const
{
   if ( qlevel-- == 0 )
   {
      return m_ProfileValues.FgCol;
   }

   CHECK( qlevel < WXSIZEOF(AllProfileValues::QuotedCol), wxNullColour,
          "MessageView::GetQuoteColour(): invalid quoting level" );

   return m_ProfileValues.QuotedCol[qlevel];
}

void MessageView::ShowTextPart(wxFontEncoding encBody,
                               size_t nPart)
{
   // get the encoding of the text
   wxFontEncoding encPart;
   if ( m_encodingUser != wxFONTENCODING_SYSTEM )
   {
      // user-specified encoding overrides everything
      encPart = m_encodingUser;
   }
   else if ( READ_CONFIG(m_Profile, MP_MSGVIEW_AUTO_ENCODING) )
   {
      encPart = m_mailMessage->GetTextPartEncoding(nPart);
      if ( encPart == wxFONTENCODING_SYSTEM )
      {
         // use the encoding of the last part which had it
         encPart = encBody;
      }
      else if ( encBody == wxFONTENCODING_SYSTEM )
      {
         // remember the encoding for the next parts
         encBody = encPart;
      }
   }
   else
   {
      // autodetecting encoding is disabled, don't use any
      encPart = wxFONTENCODING_SYSTEM;
   }

   unsigned long len;
   String textPart = m_mailMessage->GetPartContent(nPart, &len);

   TextStyle style;
   if ( encPart != wxFONTENCODING_SYSTEM )
   {
      wxFont font(
                  m_ProfileValues.fontSize,
                  m_ProfileValues.fontFamily,
                  wxNORMAL,
                  wxNORMAL,
                  FALSE,   // not underlined
                  "",      // no specific face name
                  encPart
                 );
      style.SetFont(font);
   }

   if ( m_ProfileValues.highlightURLs || m_ProfileValues.quotedColourize )
   {
      String url;
      String before;

      size_t level = GetQuotedLevel(textPart);
      style.SetTextColour(GetQuoteColour(level));

      do
      {
         // extract the first URL into url string and put all preceding
         // text into before, textPart is updated to contain only the text
         // after the URL
         before = strutil_findurl(textPart, url);

         if ( m_ProfileValues.quotedColourize )
         {
            size_t line_from = 0,
                   line_lng = before.length();
            for ( size_t line_pos = 0; line_pos < line_lng; line_pos++ )
            {
               if ( before[line_pos] == '\n' )
               {
                  level = GetQuotedLevel(before.c_str() + line_pos + 1);

                  m_viewer->InsertText(before.Mid(line_from,
                                                 line_pos - line_from + 1),
                                       style);

                  style.SetTextColour(GetQuoteColour(level));
                  line_from = line_pos + 1;
               }
            }

            if ( line_from < line_lng-1 )
            {
               m_viewer->InsertText(before.Mid(line_from,
                                               line_lng - line_from),
                                    style);
            }
         }
         else // no quoted text colourizing
         {
            m_viewer->InsertText(before, style);
         }

         if ( !strutil_isempty(url) )
         {
            if ( m_ProfileValues.highlightURLs )
            {
               m_viewer->InsertURL(url);
            }
            else
            {
               m_viewer->InsertText(url, style);
            }
         }
      }
      while( !strutil_isempty(textPart) );
   }
   else // no URL highlighting, no quoting text colourizing
   {
      m_viewer->InsertText(textPart, style);
   }
}

// ----------------------------------------------------------------------------
// MessageView attachments and images handling
// ----------------------------------------------------------------------------

void MessageView::ShowAttachment(size_t nPart,
                                 const String& mimeType,
                                 size_t partSize)
{
   // get the icon for the attachment using its MIME type and filename
   // extension (if any)
   wxString mimeFileName = GetFileNameForMIME(m_mailMessage, nPart);
   wxIcon icon = mApplication->GetIconManager()->
                     GetIconFromMimeType(mimeType,
                                         mimeFileName.AfterLast('.'));

   m_viewer->InsertAttachment(icon,
                              GetClickableInfo(nPart, mimeFileName, partSize));
}

void MessageView::ShowImage(size_t nPart,
                            const String& mimeType,
                            size_t partSize)
{
   bool showInline = true;

   switch ( m_ProfileValues.inlineGFX )
   {
      default:
         // check that the size of the image is less than configured
         // maximum
         if ( partSize > 1024*m_ProfileValues.inlineGFX )
         {
            wxString msg;
            msg.Printf
                (
                  _("An image embedded in this message is bigger "
                    "than the currently configured limit of %luKb.\n"
                    "\n"
                    "Would you still like to see it?\n"
                    "\n"
                    "You can change this setting in the \"Message "
                    "View\" page of the preferences dialog to 0 if "
                    "you want to always show the images inline."),
                  (unsigned long)m_ProfileValues.inlineGFX
                );

            if ( MDialog_YesNoDialog
                 (
                  msg,
                  GetFrame(this),
                  MDIALOG_YESNOTITLE,
                  false, // [No] default
                  GetPersMsgBoxName(M_MSGBOX_GFX_NOT_INLINED)
                 ) )
            {
               // will inline
               break;
            }

         }
         else
         {
            // will inline
            break;
         }

         // fall through

      case -1:
         // never inline
         showInline = false;
         break;

      case 0:
         // always inline
         break;
   }

   if ( showInline )
   {
      bool ok = false;

      MTempFileName tmpFN;
      if ( tmpFN.IsOk() )
      {
         MimeSave(nPart, tmpFN.GetName());

         wxImage img =  wxIconManager::LoadImage(filename, &ok, true);

         if ( ok )
         {
            wxString mimeFileName = GetFileNameForMIME(m_mailMessage, nPart);

            m_viewer->InsertImage
                      (
                        img.ConvertToBitmap(),
                        GetClickableInfo(nPart, mimeFileName, partSize)
                      );
         }
      }

      if ( !ok )
         showInline = false;
   }

   if ( !showInline )
   {
      // show as an attachment then
      ShowAttachment(nPart, mimeType, partSize);
   }
}

ClickableInfo *MessageView::GetClickableInfo(size_t nPart,
                                             const String& mimeFileName.
                                             size_t partSize) const
{
   wxString label = mimeFileName;
   if ( !label.empty() )
      label << " : ";

   label << mimeType << ", " << strutil_ultoa(partSize) << _(" bytes");

   return new ClickableInfo(nPart, label);
}

// ----------------------------------------------------------------------------
// MessageView::Update
// ----------------------------------------------------------------------------

void
MessageView::Update(void)
{
   m_viewer->Clear();

   if(! m_mailMessage)  // no message to display
      return;

   m_uid = m_mailMessage->GetUId();

   // deal with the headers first
   wxFontEncoding encInHeaders = ShowHeaders();

   // get/guess the body encoding
   wxFontEncoding encBody;
   if ( m_encodingUser != wxFONTENCODING_SYSTEM )
   {
      // user-specified encoding overrides everything
      encBody = m_encodingUser;
   }
   else // auto-detect
   {
      // FIXME how to get the params of the top level message?
#if 0
      encBody = m_mailMessage->GetTextPartEncoding(-1);
      if ( encBody == wxFONTENCODING_SYSTEM )
#endif
      {
         // some broken mailers don't create correct "Content-Type" header,
         // but they may yet give us the necessary info in the other headers
         encBody = encInHeaders;
      }
   }

   m_viewer->StartBody();

   // this var stores the MIME spec of the MESSAGE/RFC822 we're currently in,
   // it is empty if we're outside any embedded message
   String specMsg;

   // iterate over all parts
   int countParts = m_mailMessage->CountParts();
   for ( int nPart = 0; nPart < countParts; nPart++ )
   {
      String spec = m_mailMessage->GetPartSpec(nPart);

      // FIXME: there is a bug with this code as it only remembers the last
      //        embedded message and so will break if we have 2 embedded
      //        RFC822s - it will forget about the enclosing one after leaving
      //        the inner message
      //
      //        it's true that it happens rarely enough, but we should still
      //        maintain a stack of msg specs here instead of having only one
      //        variable...

      if ( !specMsg.empty() )
      {
         // check if this is a part of MULTIPART or MESSAGE message,
         // distinguish between the 2 as we do show MULTIPART subparts (even
         // though we should only do it for MIXED and DIGEST, probably, and
         // only show one part for ALTERNATIVE - TODO), but we don't show
         // MESSAGE subparts unless we're configured to inline them
         if ( spec.StartsWith(specMsg) )
         {
            // this is a part of embedded message
            if ( !m_ProfileValues.inlineRFC822 )
            {
               // don't show it
               continue;
            }
            //else: still show it as we're configured to do so
         }
         else
         {
            // we've finished with the embedded message
            specMsg.clear();
         }
      }
      //else: not part of an embedded message

      int t = m_mailMessage->GetPartType(nPart);
      if ( t == Message::MSG_TYPEMESSAGE )
      {
         // remember the part spec of the last embedded message found, used
         // above
         specMsg = spec;

         if ( m_ProfileValues.inlineRFC822 )
         {
            // we don't show the message itself, just its subparts
            continue;
         }
         //else: show the embedded message as an icon
      }

      size_t partSize = m_mailMessage->GetPartSize(nPart);
      if ( partSize == 0 )
      {
         // ignore empty parts but warn user as it might indicate a problem
         wxLogStatus(GetFrame(this),
                     _("Skipping the empty MIME part #%d."), nPart);

         continue;
      }

      String mimeType = m_mailMessage->GetPartMimeType(nPart);
      strutil_tolower(mimeType);
      String fileName = GetFileNameForMIME(m_mailMessage,nPart);

      String disposition;
      (void) m_mailMessage->GetDisposition(nPart,&disposition);
      strutil_tolower(disposition);

      // let's guess a little if we have unknown encoding such as
      // APPLICATION/OCTET_STREAM
      if ( t == Message::MSG_TYPEAPPLICATION )
      {
         wxString ext = fileName.AfterLast('.');
         wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
         wxFileType *ft = mimeManager.GetFileTypeFromExtension(ext);
         if(ft)
         {
            wxString mt;
            ft->GetMimeType(&mt);
            delete ft;
            if(wxMimeTypesManager::IsOfType(mt,"image/*"))
               t = Message::MSG_TYPEIMAGE;
            else if(wxMimeTypesManager::IsOfType(mt,"audio/*"))
               t = Message::MSG_TYPEAUDIO;
            else if(wxMimeTypesManager::IsOfType(mt,"video/*"))
               t = Message::MSG_TYPEVIDEO;
         }
      }

      m_viewer->StartPart();

      // if the disposition is set to attachment we force the part to be shown
      // as an attachment
      bool isAttachment = disposition == "attachment";

      // first check for viewer specific formats, next for text, then for
      // images and finally show all the rest as generic attachment

      if ( !isAttachment && m_viewer->CanProcess(mimeType) )
      {
         m_viewer->InsertRawContents();
      }
      else if ( !isAttachment &&
                ((mimeType == "text/plain" &&
                  (fileName.empty() || m_ProfileValues.inlinePlainText)) ||
                 (t == Message::MSG_TYPEMESSAGE &&
                   m_ProfileValues.inlineRFC822)) )
      {
         ShowTextPart(encBody, nPart);
      }
      else if ( !isAttachment &&
                  (t == Message::MSG_TYPEIMAGE && m_ProfileValues.inlineGFX) )
      {
         ShowImage(nPart, mimeType, partSize);
      }
      else // attachment
      {
         ShowAttachment(nPart, mimeType, partSize);
      }

      m_viewer->EndPart();
   }

   m_viewer->EndBody();

   // update the menu of the frame containing us to show the encoding used
   CheckLanguageInMenu(this, encBody == wxFONTENCODING_SYSTEM
                              ? wxFONTENCODING_DEFAULT
                              : encBody);
}

// ----------------------------------------------------------------------------
// MIME attachments menu commands
// ----------------------------------------------------------------------------

// show information about an attachment
void
MessageView::MimeInfo(int mimeDisplayPart)
{
   String message;
   message << _("MIME type: ")
           << m_mailMessage->GetPartMimeType(mimeDisplayPart)
           << '\n';

   String desc = m_mailMessage->GetPartDesc(mimeDisplayPart);
   if ( !desc.empty() )
      message << '\n' << _("Description: ") << desc << '\n';

   message << _("Size: ")
           << strutil_ltoa(m_mailMessage->GetPartSize(mimeDisplayPart, true));

   // as we passed true to GetPartSize() above, it will return size in lines
   // for the text messages (and in bytes for everything else)
   Message::ContentType type = m_mailMessage->GetPartType(mimeDisplayPart);
   if(type == Message::MSG_TYPEMESSAGE || type == Message::MSG_TYPETEXT)
      message << _(" lines");
   else
      message << _(" bytes");
   message << '\n';

   // param name and value (used in 2 loops below)
   wxString name, value;

   // debug output with all parameters
   const MessageParameterList &plist = m_mailMessage->GetParameters(mimeDisplayPart);
   MessageParameterList::iterator plist_it;
   if ( !plist.empty() )
   {
      message += _("\nParameters:\n");
      for ( plist_it = plist.begin(); plist_it != plist.end(); plist_it++ )
      {
         name = (*plist_it)->name;
         message << NormalizeString(name) << ": ";

         // filenames are case-sensitive, don't modify them
         value = (*plist_it)->value;
         if ( name.CmpNoCase("name") != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   // now output disposition parameters too
   String disposition;
   const MessageParameterList& dlist =
      m_mailMessage->GetDisposition(mimeDisplayPart,&disposition);

   if ( !strutil_isempty(disposition) )
      message << _("\nDisposition: ") << disposition.Lower() << '\n';

   if ( !dlist.empty() )
   {
      message += _("\nDisposition parameters:\n");
      for ( plist_it = dlist.begin(); plist_it != dlist.end(); plist_it++ )
      {
         name = (*plist_it)->name;
         message << NormalizeString(name) << ": ";

         value = (*plist_it)->value;
         if ( name.CmpNoCase("filename") != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   String title;
   title << _("MIME information for attachment #") << mimeDisplayPart;
   MDialog_Message(message, this, title);
}

// open (execute) a message attachment
void
MessageView::MimeHandle(int mimeDisplayPart)
{
   // we'll need this filename later
   wxString filenameOrig = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);

   String mimetype = m_mailMessage->GetPartMimeType(mimeDisplayPart);
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   if ( wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM") )
   {
      // special handling of "APPLICATION/OCTET-STREAM": this is the default
      // MIME type for all binary attachments and many e-mail clients don't
      // use the correct type (e.g. IMAGE/GIF) always leaving this one as
      // default. Try to guess a better MIME type ourselves from the file
      // extension.
      if ( !filenameOrig.IsEmpty() )
      {
         wxString ext;
         wxSplitPath(filenameOrig, NULL, NULL, &ext);

         if ( !ext.IsEmpty() )
            fileType = mimeManager.GetFileTypeFromExtension(ext);
      }
   }

   if ( !fileType )
   {
      // non default MIME type (so use it) or couldn't get the MIME type from
      // extension
      fileType = mimeManager.GetFileTypeFromMimeType(mimetype);
   }

   // First, we check for those contents that we handle in M itself:

   // handle internally MESSAGE/*
   if ( wxMimeTypesManager::IsOfType(mimetype, "MESSAGE/*") )
   {
#if 0
      // It's a pity, but creating a MessageCC from a string doesn't
      // quite work yet. :-(
      unsigned long len;
      char const *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
      if( !content )
      {
         wxLogError(_("Cannot get attachment content."));
         return;
      }
      Message *msg = Message::Create(content, 1);
      wxMessageViewFrame * f = new wxMessageViewFrame(NULL, 1, NULL, m_Parent);
      f->ShowMessage(msg);
      f->SetTitle(mimetype);
      msg->DecRef();
#else // 1
      bool ok = false;
      char *filename = wxGetTempFileName("Mtemp");
      if ( MimeSave(mimeDisplayPart, filename) )
      {
         wxString name;
         name.Printf(_("Attached message '%s'"),
                     GetFileNameForMIME(m_mailMessage, mimeDisplayPart).c_str());

         MFolder_obj mfolder = MFolder::CreateTemp
                               (
                                 name,
                                 MF_FILE, 0,
                                 filename
                               );

         if ( mfolder )
         {
            ASMailFolder *mf = ASMailFolder::OpenFolder(mfolder);
            if ( mf )
            {
               wxMessageViewFrame * f =
                  new wxMessageViewFrame(mf, 1, NULL, m_Parent);
               f->SetTitle(mimetype);

               ok = true;

               mf->DecRef();
            }
         }
      }

      if ( !ok )
      {
         wxLogError(_("Failed to open attached message."));
      }

      wxRemoveFile(filename);
#endif // 0/1

      return;
   }

   String
      filename = wxGetTempFileName("Mtemp"),
      filename2 = "";

   wxString ext;
   wxSplitPath(filenameOrig, NULL, NULL, &ext);
   // get the standard extension for such files if there is no real one
   if ( fileType != NULL && !ext)
   {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) && exts.GetCount() )
      {
         ext = exts[0u];
      }
   }

   // under Windows some programs will do different things depending on the
   // extensions of the input file (case in point: WinZip), so try to choose a
   // correct one
   wxString path, name, extOld;
   wxSplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      filename << '.' << ext;
   }

   MailMessageParameters params(filename, mimetype,
                                m_mailMessage, mimeDisplayPart);

   // We might fake a file, so we need this:
   bool already_saved = false;

#ifdef OS_UNIX
   /* For IMAGE/TIFF content, check whether it comes from one of the
      fax domains. If so, change the mimetype to "IMAGE/TIFF-G3" and
      proceed in the usual fashion. This allows the use of a special
      image/tiff-g3 mailcap entry. */
   if ( READ_CONFIG(m_Profile,MP_INCFAX_SUPPORT) &&
        (wxMimeTypesManager::IsOfType(mimetype, "IMAGE/TIFF")
         || wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM")))
   {
      kbStringList faxdomains;
      char *faxlisting = strutil_strdup(READ_CONFIG(m_Profile,
                                                    MP_INCFAX_DOMAINS));
      strutil_tokenise(faxlisting, ":;,", faxdomains);
      delete [] faxlisting;
      bool isfax = false;
      wxString domain;
      wxString fromline = m_mailMessage->From();
      strutil_tolower(fromline);

      for(kbStringList::iterator i = faxdomains.begin();
          i != faxdomains.end(); i++)
      {
         domain = **i;
         strutil_tolower(domain);
         if(fromline.Find(domain) != -1)
            isfax = true;
      }

      if(isfax
         && MimeSave(mimeDisplayPart,filename))
      {
         wxLogDebug("Detected image/tiff fax content.");
         // use TIFF2PS command to create a postscript file, open that
         // one with the usual ps viewer
         filename2 = filename.BeforeLast('.') + ".ps";
         String command;
         command.Printf(READ_CONFIG(m_Profile,MP_TIFF2PS),
                        filename.c_str(), filename2.c_str());
         // we ignore the return code, because next viewer will fail
         // or succeed depending on this:
         //system(command);  // this produces a postscript file on  success
         RunProcess(command);
         // We cannot use launch process, as it doesn't wait for the
         // program to finish.
         //wxString msg;
         //msg.Printf(_("Running '%s' to create Postscript file failed."), command.c_str());
         //(void)LaunchProcess(command, msg );

         wxRemoveFile(filename);
         filename = filename2;
         mimetype = "application/postscript";
         if(fileType) delete fileType;
         fileType = mimeManager.GetFileTypeFromMimeType(mimetype);
         // proceed as usual
         MailMessageParameters new_params(filename, mimetype,
                                          m_mailMessage,
                                          mimeDisplayPart);
         params = new_params;
         already_saved = true; // use this file instead!
      }
   }
#endif // Unix

   // We must save the file before actually calling GetOpenCommand()
   if( !already_saved )
   {
      MimeSave(mimeDisplayPart,filename);
      already_saved = TRUE;
   }
   String command;
   if ( (fileType == NULL) || !fileType->GetOpenCommand(&command, params) )
   {
      // unknown MIME type, ask the user for the command to use
      String prompt;
      prompt.Printf(_("Please enter the command to handle '%s' data:"),
                    mimetype.c_str());
      if ( !MInputBox(&command, _("Unknown MIME type"), prompt,
                      this, "MimeHandler") )
      {
         // cancelled by user
         return;
      }

      if ( command.IsEmpty() )
      {
         wxLogWarning(_("Do not know how to handle data of type '%s'."),
                      mimetype.c_str());
      }
      else
      {
         // the command must contain exactly one '%s' format specificator!
         String specs = strutil_extract_formatspec(command);
         if ( specs.IsEmpty() )
         {
            // at least the filename should be there!
            command += " %s";
         }

         // do expand it
         command = wxFileType::ExpandCommand(command, params);

         // TODO save this command to mailcap!
      }
      //else: empty command means try to handle it internally
   }

   if ( fileType )
      delete fileType;

   if ( ! command.IsEmpty() )
   {
      if(already_saved || MimeSave(mimeDisplayPart,filename))
      {
         wxString errmsg;
         errmsg.Printf(_("Error opening attachment: command '%s' failed"),
                       command.c_str());
         (void)LaunchProcess(command, errmsg, filename);
      }
   }
}

void
MessageView::MimeOpenWith(int mimeDisplayPart)
{
   // we'll need this filename later
   wxString filenameOrig = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);

   String mimetype = m_mailMessage->GetPartMimeType(mimeDisplayPart);
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   fileType = mimeManager.GetFileTypeFromMimeType(mimetype);

   String filename = wxGetTempFileName("Mtemp");

   wxString ext;
   wxSplitPath(filenameOrig, NULL, NULL, &ext);
   // get the standard extension for such files if there is no real one
   if ( fileType != NULL && !ext )
   {
      wxArrayString exts;
      if ( fileType->GetExtensions(exts) && exts.GetCount() )
      {
         ext = exts[0u];
      }
   }

   // under Windows some programs will do different things depending on the
   // extensions of the input file (case in point: WinZip), so try to choose a
   // correct one
   wxString path, name, extOld;
   wxSplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      filename << '.' << ext;
   }

   MailMessageParameters params(filename, mimetype,
                                m_mailMessage, mimeDisplayPart);

   String command;
   // ask the user for the command to use
   String prompt;
   prompt.Printf(_("Please enter the command to handle '%s' data:"),
                 mimetype.c_str());
   if ( !MInputBox(&command, _("Open with"), prompt,
                   this, "MimeHandler") )
   {
      // cancelled by user
      return;
   }

   if ( command.IsEmpty() )
   {
      wxLogWarning(_("Do not know how to handle data of type '%s'."),
                   mimetype.c_str());
   }
   else
   {
      // the command must contain exactly one '%s' format specificator!
      String specs = strutil_extract_formatspec(command);
      if ( specs.IsEmpty() )
      {
         // at least the filename should be there!
         command += " %s";
      }

      // do expand it
      command = wxFileType::ExpandCommand(command, params);

   }

   if ( ! command.IsEmpty() )
   {
      if ( MimeSave(mimeDisplayPart,filename) )
      {
         wxString errmsg;
         errmsg.Printf(_("Error opening attachment: command '%s' failed"),
                       command.c_str());
         (void)LaunchProcess(command, errmsg, filename);
      }
   }
}


bool
MessageView::MimeSave(int mimeDisplayPart,const char *ifilename)
{
   String filename;

   if ( strutil_isempty(ifilename) )
   {
      filename = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);
      wxString path, name, ext;
      wxSplitPath(filename, &path, &name, &ext);

      filename = wxPFileSelector("MimeSave",_("Save attachment as:"),
                                 NULL, // no default path
                                 name, ext,
                                 NULL, wxFILEDLG_USE_FILENAME | wxSAVE | wxOVERWRITE_PROMPT, this);
   }
   else
      filename = ifilename;

   if ( !filename )
   {
      // no filename and user cancelled the dialog
      return false;
   }

   unsigned long len;
   const char *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
   if( !content )
   {
      wxLogError(_("Cannot get attachment content."));
   }
   else
   {
      wxFile out(filename, wxFile::write);
      if ( out.IsOpened() )
      {
         bool ok = true;

         // when saving messages to a file we need to "From stuff" them to
         // make them readable in a standard mail client (including this one)
         if ( m_mailMessage->GetPartType(mimeDisplayPart) ==
               Message::MSG_TYPEMESSAGE )
         {
            // standard prefix
            String fromLine = "From ";

            // find the from address
            const MessageParameterList& params =
               m_mailMessage->GetParameters(mimeDisplayPart);
            MessageParameterList::iterator i;
            for ( i = params.begin(); i != params.end(); i++ )
            {
               if ( (*i)->name == "From" )
               {
                  // found, now extract just the address
                  fromLine += Message::GetEMailFromAddress((*i)->value);
               }
            }

            if ( i == params.end() )
            {
               // this shouldn't normally happen, but if it does just make it
               // up (FIXME: now it always happens!)
               wxLogDebug("Couldn't find from header in the message");

               fromLine += "MAHOGANY-DUMMY-SENDER";
            }

            fromLine += ' ';

            // time stamp
            time_t t;
            time(&t);
            fromLine += ctime(&t);

            ok = out.Write(fromLine);
         }

         if ( ok )
         {
            // write the body
            ok = out.Write(content, len) == len;
         }

         if ( ok )
         {
            // only display in interactive mode
            if ( strutil_isempty(ifilename) )
            {
               wxLogStatus(GetFrame(this), _("Wrote %lu bytes to file '%s'"),
                           len, filename.c_str());
            }

            return true;
         }
      }
   }

   wxLogError(_("Could not save the attachment."));

   return false;
}

void
MessageView::MimeViewText(int mimeDisplayPart)
{
   unsigned long len;
   const char *content = m_mailMessage->GetPartContent(mimeDisplayPart, &len);
   if ( content )
   {
      String title;
      title.Printf(_("Attachment #%d"), mimeDisplayPart);

      // add the filename if any
      String filename = GetFileNameForMIME(m_mailMessage, mimeDisplayPart);
      if ( !filename.empty() )
      {
         title << " ('" << filename << "')";
      }

      MDialog_ShowText(this, title, content, "MimeView");
   }
   else
   {
      wxLogError(_("Failed to view the attachment."));
   }
}

// ----------------------------------------------------------------------------
// URL handling
// ----------------------------------------------------------------------------

void MessageView::PopupURLMenu(const String& url, const wxPoint& pt)
{
   UrlPopup menu(this, url);
   PopupMenu(&menu, pt);
}

void MessageView::OpenURL(const String& url, bool inNewWindow)
{
   wxFrame *frame = GetFrame(this);
   wxLogStatus(frame, _("Opening URL '%s'..."), url.c_str());

   wxBusyCursor bc;

   // the command to execute
   wxString command;

   bool bOk;
   if ( m_ProfileValues.browser.IsEmpty() )
   {
#ifdef OS_WIN
      // ShellExecute() always opens in the same window,
      // so do it manually for new window
      if ( inNewWindow )
      {
         wxRegKey key(wxRegKey::HKCR, url.BeforeFirst(':') + "\\shell\\open");
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
                     if ( ddeCmd.Replace("%1", url, FALSE) == 1 )
                     {
                        // magic incantation understood by wxMSW
                        command << "WX_DDE#"
                                << wxRegKey(key, "command") << '#'
                                << wxRegKey(keyDDE, "application") << '#'
                                << ddeTopic << '#'
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
         bOk = LaunchProcess(command, errmsg);
      }
      else // easy case: open in the same window
      {
         bOk = (int)ShellExecute(NULL, "open", url,
                                 NULL, NULL, SW_SHOWNORMAL ) > 32;
      }

      if ( !bOk )
      {
         wxLogSysError(_("Cannot open URL '%s'"), url.c_str());
      }
# else  // Unix
      // propose to choose program for opening URLs
      if (
         MDialog_YesNoDialog
         (
            _("No command configured to view URLs.\n"
              "Would you like to choose one now?"),
            frame,
            MDIALOG_YESNOTITLE,
            true,
            GetPersMsgBoxName(M_MSGBOX_ASK_URL_BROWSER)
            )
         )
         ShowOptionsDialog();

      if ( m_ProfileValues.browser.IsEmpty() )
      {
         wxLogError(_("No command configured to view URLs."));
         bOk = false;
      }
#endif // Win/Unix
   }
   else
   {
      // not empty, user provided his script - use it
      bOk = false;

#ifdef OS_UNIX
      if ( m_ProfileValues.browserIsNS ) // try re-loading first
      {
         wxString lockfile;
         wxGetHomeDir(&lockfile);
         lockfile << WXEXTHELP_SEPARATOR << ".netscape/lock";
         struct stat statbuf;

         if(lstat(lockfile.c_str(), &statbuf) == 0)
         // cannot use wxFileExists, because it's a link pointing to a
         // non-existing location      if(wxFileExists(lockfile))
         {
            command << m_ProfileValues.browser
                    << " -remote openURL(" << url;
            if ( inNewWindow )
            {
               command << ",new-window)";
            }
            else
            {
               command << ")";
            }
            wxString errmsg;
            errmsg.Printf(_("Could not launch browser: '%s' failed."),
                          command.c_str());
            bOk = LaunchProcess(command, errmsg);
         }
      }
#endif // Unix
      // either not netscape or ns isn't running or we have non-UNIX
      if(! bOk)
      {
         command = m_ProfileValues.browser;
         command << ' ' << url;

         wxString errmsg;
         errmsg.Printf(_("Couldn't launch browser: '%s' failed"),
                       command.c_str());

         bOk = LaunchProcess(command, errmsg);
      }

      if ( bOk )
         wxLogStatus(frame, _("Opening URL '%s'... done."),
                     url.c_str());
      else
         wxLogStatus(frame, _("Opening URL '%s' failed."),
                     url.c_str());
   }
}

// ----------------------------------------------------------------------------
// event handling
// ----------------------------------------------------------------------------

bool
MessageView::OnMEvent(MEventData& ev)
{
   if ( ev.GetId() == MEventId_OptionsChange )
   {
      MEventOptionsChangeData& event = (MEventOptionsChangeData &)ev;

      // first of all, are we interested in this profile or not?
      Profile *profileChanged = event.GetProfile();
      if ( !profileChanged || !profileChanged->IsAncestor(m_Profile) )
      {
         // it's some profile which has nothing to do with us
         return true;
      }

      AllProfileValues settings;
      ReadAllSettings(&settings);
      if ( settings == m_ProfileValues )
      {
         // nothing changed
         return true;
      }

      switch ( event.GetChangeKind() )
      {
         case MEventOptionsChangeData::Apply:
         case MEventOptionsChangeData::Ok:
         case MEventOptionsChangeData::Cancel:
            // update everything
            m_ProfileValues = settings;
            break;

         default:
            FAIL_MSG("unknown options change event");
      }

      Update();
   }
   else if( ev.GetId() == MEventId_ASFolderResult )
   {
      OnASFolderResultEvent((MEventASFolderResultData &) ev);
   }

   return true;
}

void
MessageView::OnMouseEvent(wxCommandEvent &event)
{
   ClickableInfo *ci;

   wxLayoutObject *obj;
   obj = (wxLayoutObject *)event.GetClientData();
   ci = (ClickableInfo *)obj->GetUserData();
   if(ci)
   {
      switch( ci->GetType() )
      {
         case ClickableInfo::CI_ICON:
         {
            switch ( event.GetId() )
            {
               case WXLOWIN_MENU_RCLICK:
                  // show the menu
                  PopupMenu(new MimePopup(this, ci->GetPart()), m_ClickPosition);
                  break;

               case WXLOWIN_MENU_LCLICK:
                  // for now, do the same thing as double click: perhaps the
                  // left button behaviour should be configurable?

               case WXLOWIN_MENU_DBLCLICK:
                  // open
                  MimeHandle(ci->GetPart());
                  break;

               default:
                  FAIL_MSG("unknown mouse action");
            }
         }
         break;

         case ClickableInfo::CI_URL:
         {
            wxString url = ci->GetUrl();

            // treat mail urls separately:
            wxString protocol = url.BeforeFirst(':');
            if ( protocol == "mailto" )
            {
               Composer *cv = Composer::CreateNewMessage(m_Profile);

               cv->SetAddresses(ci->GetUrl().Right(ci->GetUrl().Length()-7));
               cv->InitText();
               break;
            }

            if ( event.GetId() == WXLOWIN_MENU_RCLICK )
            {
               PopupURLMenu(url, m_ClickPosition);
            }
            else // left or double click
            {
               OpenURL(url, m_ProfileValues.browserInNewWindow);
            }
         }
         break;

         default:
            FAIL_MSG("unknown embedded object type");
      }

      ci->DecRef();
   }
}

bool
MessageView::DoMenuCommand(int id)
{
   if ( m_uid == UID_ILLEGAL )
      return false;

   CHECK( GetFolder(), false, "no folder in message view?" );

   UIdArray msgs;
   msgs.Add(m_uid);

   switch ( id )
   {
      case WXMENU_MSG_FIND:
         Find("");
         break;

      case WXMENU_MSG_REPLY:
         MailFolder::ReplyMessage(m_mailMessage,
                                  MailFolder::Params(),
                                  m_Profile,
                                  GetFrame(this));
         break;

      case WXMENU_MSG_FOLLOWUP:
         MailFolder::ReplyMessage(m_mailMessage,
                                  MailFolder::Params(MailFolder::REPLY_FOLLOWUP),
                                  m_Profile,
                                  GetFrame(this));
         break;
      case WXMENU_MSG_FORWARD:
         MailFolder::ForwardMessage(m_mailMessage,
                                    MailFolder::Params(),
                                    m_Profile,
                                    GetFrame(this));
         break;

      case WXMENU_MSG_SAVE_TO_FOLDER:
         GetFolder()->SaveMessagesToFolder(&msgs, GetFrame(this));
         break;

      case WXMENU_MSG_SAVE_TO_FILE:
         GetFolder()->SaveMessagesToFile(&msgs, GetFrame(this));
         break;

      case WXMENU_MSG_DELETE:
         GetFolder()->DeleteMessages(&msgs);
         break;

      case WXMENU_MSG_UNDELETE:
         GetFolder()->UnDeleteMessages(&msgs);
         break;

      case WXMENU_MSG_PRINT:
         Print();
         break;

      case WXMENU_MSG_PRINT_PREVIEW:
         PrintPreview();
         break;

#ifdef USE_PS_PRINTING
      case WXMENU_MSG_PRINT_PS:
         break;
#endif

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(MH_MESSAGE_VIEW,this);
         break;

      case WXMENU_MSG_TOGGLEHEADERS:
         m_ProfileValues.showHeaders = !m_ProfileValues.showHeaders;
         m_Profile->writeEntry(MP_SHOWHEADERS, m_ProfileValues.showHeaders);
         UpdateShowHeadersInMenu();
         Update();
         break;

      case WXMENU_MSG_SAVEADDRESSES:
         {
            MailFolder *mf = GetFolder()->GetMailFolder();
            CHECK( mf, false, "message preview without folder?" );

            Message *msg = mf->GetMessage(m_uid);
            if ( !msg )
            {
               FAIL_MSG( "no message in message view?" );
            }
            else
            {
               wxSortedArrayString addressesSorted;
               if ( !msg->ExtractAddressesFromHeader(addressesSorted) )
               {
                  // very strange
                  wxLogWarning(_("Selected message doesn't contain any valid addresses."));
               }

               wxArrayString addresses = strutil_uniq_array(addressesSorted);
               if ( !addresses.IsEmpty() )
               {
                  InteractivelyCollectAddresses
                  (
                     addresses,
                     READ_APPCONFIG(MP_AUTOCOLLECT_ADB),
                     mf->GetName(),
                     (MFrame *)GetFrame(this)
                  );
               }

               msg->DecRef();
            }

            mf->DecRef();
         }
         break;

      case WXMENU_EDIT_PASTE:
         Paste();
         Refresh();
         break;

      case WXMENU_EDIT_COPY:
         Copy();
         break;

      case WXMENU_EDIT_CUT:
         Cut();
         Refresh();
         break;

      default:
         if ( WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT) )
         {
            SetLanguage(id);
            break;
         }

         // not handled
         return false;
   }

   // message handled
   return true;
}

void
MessageView::ShowMessage(UIdType uid)
{
   if ( m_uid == uid )
      return;

   if ( uid == UID_ILLEGAL || !m_asyncFolder )
   {
      // don't show anything
      Clear();
   }
   else
   {
      // file request, our ShowMessage(Message *) will be called later
      (void)m_asyncFolder->GetMessage(uid, this);
   }
}

void
MessageView::ShowMessage(Message *mailMessage)
{
   CHECK_RET( mailMessage, "no message to show in MessageView" );

   mailMessage->IncRef();

   // size is measured in KBytes
   unsigned long size = mailMessage->GetSize() / 1024,
                 maxSize = (unsigned long)READ_CONFIG(m_Profile,
                                                      MP_MAX_MESSAGE_SIZE);

   if ( size > maxSize )
   {
      MailFolder *mf = mailMessage->GetFolder();
      CHECK_RET( mf, "message without folder?" );

      // local folders are supposed to be fast
      if ( !IsLocalQuickFolder(mf->GetType()) )
      {
         wxString msg;
         msg.Printf(_("The selected message is %u Kbytes long which is "
                      "more than the current threshold of %d Kbytes.\n"
                      "\n"
                      "Do you still want to download it?"),
                    size, maxSize);
         if ( !MDialog_YesNoDialog(msg, this) )
         {
            // don't do anything
            mailMessage->DecRef();
            return;
         }
      }
   }

   // ok, make this our new current message
   SafeDecRef(m_mailMessage);

   m_mailMessage = mailMessage;
   m_uid = mailMessage->GetUId();

   //have we not seen the message before?
   if( GetFolder() &&
     ! (m_mailMessage->GetStatus() & MailFolder::MSG_STAT_SEEN))
   {
      // mark it as seen
      m_mailMessage->GetFolder()->
        SetMessageFlag(m_uid, MailFolder::MSG_STAT_SEEN, true);

      // autocollect the address:
      /* FIXME for now it's here, should go somewhere else: */
      if ( m_ProfileValues.autocollect )
      {
         String addr, name;
         addr = m_mailMessage->Address(name, MAT_REPLYTO);

         String folderName = m_mailMessage->GetFolder() ?
            m_mailMessage->GetFolder()->GetName() : String(_("unknown"));

         AutoCollectAddresses(addr, name,
                              m_ProfileValues.autocollect,
                              m_ProfileValues.autocollectNamed != 0,
                              m_ProfileValues.autocollectBookName,
                              folderName,
                              (MFrame *)GetFrame(this));
         addr = m_mailMessage->Address(name, MAT_FROM);
         AutoCollectAddresses(addr, name,
                              m_ProfileValues.autocollect,
                              m_ProfileValues.autocollectNamed != 0,
                              m_ProfileValues.autocollectBookName,
                              folderName,
                              (MFrame *)GetFrame(this));
      }
   }

   Update();
}

// ----------------------------------------------------------------------------
// printing
// ----------------------------------------------------------------------------

bool
MessageView::Print(bool interactive)
{
   return m_viewer->Print();
}

void
MessageView::PrintPreview(void)
{
   m_viewer->PrintPreview();
}

// ----------------------------------------------------------------------------
// external processes
// ----------------------------------------------------------------------------

bool
MessageView::RunProcess(const String& command)
{
   wxLogStatus(GetFrame(this),
               _("Calling external viewer '%s'"),
               command.c_str());
   return wxExecute(command, true) == 0;
}

bool
MessageView::LaunchProcess(const String& command,
                             const String& errormsg,
                             const String& filename)
{
   wxLogStatus(GetFrame(this),
               _("Calling external viewer '%s'"),
               command.c_str());

   wxProcess *process = new wxProcess(this);
   int pid = wxExecute(command, false, process);
   if ( !pid )
   {
      delete process;

      if ( !errormsg.IsEmpty() )
         wxLogError("%s.", errormsg.c_str());

      return false;
   }

   if ( pid != -1 )
   {
      ProcessInfo *procInfo = new ProcessInfo(process, pid, errormsg, filename);

      m_processes.Add(procInfo);
   }

   return true;
}

void
MessageView::OnProcessTermination(wxProcessEvent& event)
{
   // find the corresponding entry in m_processes
   size_t n, procCount = m_processes.GetCount();
   for ( n = 0; n < procCount; n++ )
   {
      if ( m_processes[n]->GetPid() == event.GetPid() )
         break;
   }

   CHECK_RET( n != procCount, "unknown process terminated!" );

   ProcessInfo *info = m_processes[n];
   if ( event.GetExitCode() != 0 )
   {
      wxLogError(_("%s (external viewer exited with non null exit code)"),
                 info->GetErrorMsg().c_str());
   }

#if 0
   MTempFileName *tempfile = info->GetTempFile();
   if ( tempfile )
   {
      // tell it that it's ok to delete the temp file
      //FIXME: Ok() tells it not to delete it!tempfile->Ok();
   }
#endif

   m_processes.RemoveAt(n);
   delete info;
}

// ----------------------------------------------------------------------------
// async result event processing
// ----------------------------------------------------------------------------

void
MessageView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   ASMailFolder::Result *result = event.GetResult();
   if (result->GetUserData() == this )
   {
      switch(result->GetOperation())
      {
         case ASMailFolder::Op_GetMessage:
         {
            /* The only situation where we receive a Message, is if we
               want to open it in a separate viewer. */
            Message *mptr =
               ((ASMailFolder::ResultMessage *)result)->GetMessage();

            if(mptr && mptr->GetUId() != m_uid)
            {
               ShowMessage(mptr);
               wxFrame *frame = GetFrame(this);
               if ( wxDynamicCast(frame, wxMessageViewFrame) )
               {
                  wxString fmt = READ_CONFIG(m_Profile, MP_MVIEW_TITLE_FMT);
                  MsgVarExpander expander(mptr);
                  frame->SetTitle(ParseMessageTemplate(fmt, expander));
               }
            }
            SafeDecRef(mptr);
         }
         break;

         default:
            FAIL_MSG("Unexpected async result event");
      }
   }

   result->DecRef();
}

void
MessageView::UpdateShowHeadersInMenu()
{
   wxFrame *frame = GetFrame(GetWindow());
   CHECK_RET( frame, "message view without parent frame?" );

   wxMenuBar *mbar = frame->GetMenuBar();
   CHECK_RET( mbar, "message view frame without menu bar?" );

   mbar->Check(WXMENU_MSG_TOGGLEHEADERS, m_ProfileValues.showHeaders);
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void MessageView::EmulateKeyPress(int keycode)
{
   wxKeyEvent event;
   event.m_keyCode = keycode;

#ifdef __WXGTK__
   wxScrolledWindow::OnChar(event);
#else
   wxScrolledWindow::HandleOnChar(event);
#endif
}

/// scroll down one line:
void
MessageView::LineDown(void)
{
   EmulateKeyPress(WXK_DOWN);
}

/// scroll down one page:
void
MessageView::PageDown(void)
{
   EmulateKeyPress(WXK_PAGEDOWN);
}

/// scroll up one page:
void
MessageView::PageUp(void)
{
   EmulateKeyPress(WXK_PAGEUP);
}

// ----------------------------------------------------------------------------
// wxMessageViewFrame
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxMessageViewFrame, wxMFrame)
   EVT_SIZE(wxMessageViewFrame::OnSize)
   EVT_MENU(-1, wxMessageViewFrame::OnCommandEvent)
   EVT_TOOL(-1, wxMessageViewFrame::OnCommandEvent)
END_EVENT_TABLE()

wxMessageViewFrame::wxMessageViewFrame(ASMailFolder *folder,
                                       long num,
                                       wxFolderView *fv,
                                       MWindow  *parent,
                                       const String& name)
                  : wxMFrame(name.empty() ? String(_("Mahogany: Message View"))
                                          : name, parent)
{
   m_MessageView = NULL;

   AddFileMenu();
   AddEditMenu();
   AddMessageMenu();
   AddLanguageMenu();

   // add a toolbar to the frame
   // NB: the buttons must have the same ids as the menu commands
   AddToolbarButtons(CreateToolBar(), WXFRAME_MESSAGE);
   CreateStatusBar(2);
   static const int s_widths[] = { -1, 70 };
   SetStatusWidths(WXSIZEOF(s_widths), s_widths);

   Show(true);
   m_MessageView = new MessageView(folder, num, fv, this);
   wxSizeEvent se; // unused
   OnSize(se);
}

void
wxMessageViewFrame::OnCommandEvent(wxCommandEvent &event)
{
   int id = event.GetId();
   if(! m_MessageView->DoMenuCommand(id))
   {
      switch(id)
      {
      case WXMENU_MSG_EXPUNGE:
      case WXMENU_MSG_SELECTALL:
      case WXMENU_MSG_DESELECTALL:
         if(m_MessageView->GetFolderView())
            m_MessageView->GetFolderView()->OnCommandEvent(event);
         break;

      default:
         wxMFrame::OnMenuCommand(event.GetId());
      }
   }
}

void
wxMessageViewFrame::OnSize( wxSizeEvent & WXUNUSED(event) )
{
   int x, y;
   GetClientSize( &x, &y );
   if(m_MessageView)
      m_MessageView->SetSize(0,0,x,y);
}

IMPLEMENT_DYNAMIC_CLASS(wxMessageViewFrame, wxMFrame)

