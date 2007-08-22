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

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "strutil.h"
#  include "guidef.h"

#  include "Profile.h"

#  include "gui/wxMApp.h"      // for PrepareForPrinting()
// windows.h defines ERROR
#ifdef __WINE__
#  undef   ERROR
#endif // __WINE__
#  include "MHelp.h"
#  include "sysutil.h"
#  include "gui/wxIconManager.h"
#  include "Mdefaults.h"

#  include <wx/filedlg.h>
#endif //USE_PCH

#include "gui/wxOptionsDlg.h"
#include "MTextStyle.h"
#include "MModule.h"

#include "MessageView.h"
#include "MessageViewer.h"
#include "ViewFilter.h"

#include "mail/MimeDecode.h"
#include "MailFolderCC.h" // needed to properly include MessageCC.h
#include "MessageCC.h"
#include "MimePartCC.h"
#include "MimePartVirtual.h"
#include "FolderView.h"
#include "ASMailFolder.h"
#include "MFolder.h"
#include "gui/wxMenuDefs.h"
#include "gui/wxMDialogs.h"

#include "wx/persctrl.h"
#include "XFace.h"
#include "Collect.h"
#include "ColourNames.h"

#include "Composer.h"

#include "modules/MCrypt.h"
#include "PGPClickInfo.h"

#include "ClickAtt.h"
#include "MimeDialog.h"

#include <wx/file.h>            // for wxFile
#include <wx/mimetype.h>      // for wxFileType::MessageParameters
#include <wx/process.h>
#include <wx/mstream.h>
#include <wx/fontutil.h>
#include <wx/tokenzr.h>
#include <wx/fs_mem.h>

#ifdef OS_UNIX
   #include <sys/stat.h>

   #include <wx/dcps.h> // for wxThePrintSetupData
#endif

M_LIST_OWN(VirtualMimePartsList, MimePart);

// ----------------------------------------------------------------------------
// options we use here
// ----------------------------------------------------------------------------

extern const MOption MP_AUTOCOLLECT_INCOMING;
extern const MOption MP_AUTOCOLLECT_ADB;
extern const MOption MP_AUTOCOLLECT_SENDER;
extern const MOption MP_AUTOCOLLECT_NAMED;
extern const MOption MP_HIGHLIGHT_URLS;
extern const MOption MP_INCFAX_DOMAINS;
extern const MOption MP_INCFAX_SUPPORT;
extern const MOption MP_INLINE_GFX;
extern const MOption MP_INLINE_GFX_EXTERNAL;
extern const MOption MP_INLINE_GFX_SIZE;
extern const MOption MP_MAX_MESSAGE_SIZE;
extern const MOption MP_MSGVIEW_ALLOW_HTML;
extern const MOption MP_MSGVIEW_ALLOW_IMAGES;
extern const MOption MP_MSGVIEW_AUTO_VIEWER;
extern const MOption MP_MSGVIEW_DEFAULT_ENCODING;
extern const MOption MP_MSGVIEW_HEADERS;
extern const MOption MP_MSGVIEW_PREFER_HTML;
extern const MOption MP_MSGVIEW_VIEWER;
extern const MOption MP_MVIEW_TITLE_FMT;
extern const MOption MP_MVIEW_FONT;
extern const MOption MP_MVIEW_FONT_DESC;
extern const MOption MP_MVIEW_FONT_SIZE;
extern const MOption MP_MVIEW_ATTCOLOUR;
extern const MOption MP_MVIEW_BGCOLOUR;
extern const MOption MP_MVIEW_FGCOLOUR;
extern const MOption MP_MVIEW_HEADER_NAMES_COLOUR;
extern const MOption MP_MVIEW_HEADER_VALUES_COLOUR;
extern const MOption MP_MVIEW_URLCOLOUR;
extern const MOption MP_PGP_COMMAND;
extern const MOption MP_PLAIN_IS_TEXT;
extern const MOption MP_REPLY_QUOTE_SELECTION;
extern const MOption MP_RFC822_IS_TEXT;
extern const MOption MP_SHOWHEADERS;
extern const MOption MP_SHOW_XFACES;
extern const MOption MP_TIFF2PS;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_GFX_NOT_INLINED;
extern const MPersMsgBox *M_MSGBOX_ASK_URL_BROWSER;

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
// user.
class ProcessInfo
{
public:
   ProcessInfo(wxProcess *process,
               int pid,
               const String& errormsg)
      : m_errormsg(errormsg)
   {
      ASSERT_MSG( process && pid, _T("invalid process in ProcessInfo") );

      m_process = process;
      m_pid = pid;
   }

   ~ProcessInfo()
   {
      delete m_process;
   }

   // get the pid of our process
   int GetPid() const { return m_pid; }

   // get the error message
   const String& GetErrorMsg() const { return m_errormsg; }

   // don't delete wxProcess object (must be called before destroying this
   // object if the external process is still running)
   void Detach() { m_process->Detach(); m_process = NULL; }

private:
   String         m_errormsg; // error message to give if launch failed
   wxProcess     *m_process;  // wxWindows process info
   int            m_pid;      // pid of the process
};

// a simple event forwarder
class ProcessEvtHandler : public wxEvtHandler
{
public:
   ProcessEvtHandler(MessageView *msgView) { m_msgView = msgView; }

private:
   void OnProcessTermination(wxProcessEvent& event)
   {
      m_msgView->HandleProcessTermination(event.GetPid(), event.GetExitCode());
   }

   MessageView *m_msgView;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(ProcessEvtHandler) 
};

// the message parameters for the MIME type manager
class MailMessageParameters : public wxFileType::MessageParameters
{
public:
   MailMessageParameters(const wxString& filename,
                         const wxString& mimetype,
                         const MimePart *part)
      : wxFileType::MessageParameters(filename, mimetype)
      {
         m_mimepart = part;
      }

   virtual wxString GetParamValue(const wxString& name) const;

private:
   const MimePart *m_mimepart;
};

// a wxFileSystem providing access to MIME parts data
class MIMEFSHandler : public wxMemoryFSHandler
{
public:
   virtual bool CanOpen(const wxString& location)
   {
      return GetProtocol(location) == _T("cid");
   }
};

// ----------------------------------------------------------------------------
// TransparentFilter: the filter which doesn't filter anything but simply
//                    shows the text in the viewer.(always the last one in
//                    filter chain)
// ----------------------------------------------------------------------------

class TransparentFilter : public ViewFilter
{
public:
   TransparentFilter(MessageView *msgView) : ViewFilter(msgView, NULL, true)
   {
      m_isInBody = false;
   }

   virtual void ProcessURL(const String& text,
                           const String& url,
                           MessageViewer *viewer)
   {
      if ( m_isInBody )
         m_msgView->OnBodyText(text);

      viewer->InsertURL(text, url);
   }

   virtual void StartText()
   {
      m_isInBody = true;
   }

   virtual void EndText()
   {
      m_isInBody = false;
   }

protected:
   virtual void DoProcess(String& text,
                          MessageViewer *viewer,
                          MTextStyle& style)
   {
      if ( m_isInBody )
         m_msgView->OnBodyText(text);

      viewer->InsertText(text, style);
   }

   bool m_isInBody;
};

// ----------------------------------------------------------------------------
// ViewFilterNode: a node in the linked list of all the filters we use
// ----------------------------------------------------------------------------

class ViewFilterNode
{
public:
   ViewFilterNode(ViewFilter *filter,
                  int prio,
                  const String& name,
                  const String& desc,
                  ViewFilterNode *next)
      : m_name(name),
        m_desc(desc)
   {
      m_filter = filter;
      m_prio = prio;
      m_next = next;
   }

   ~ViewFilterNode()
   {
      delete m_filter;
      delete m_next;
   }

   ViewFilter *GetFilter() const { return m_filter; }
   int GetPriority() const { return m_prio; }
   const String& GetName() const { return m_name; }
   const String& GetDescription() const { return m_desc; }
   ViewFilterNode *GetNext() const { return m_next; }

   void SetNext(ViewFilterNode *next)
   {
      m_filter->m_next = next->m_filter;
      m_next = next;
   }

private:
   ViewFilter *m_filter;
   int m_prio;
   String m_name,
          m_desc;
   ViewFilterNode *m_next;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

static inline
String GetFileNameForMIME(Message *message, int partNo)
{
   return message->GetMimePart(partNo)->GetFilename();
}

// common part of GetAllAvailableViewers() and GetAllAvailableFilters()
static
size_t GetAllAvailablePlugins(const wxChar *iface,
                              wxArrayString *names,
                              wxArrayString *descs,
                              wxArrayInt *states = NULL)
{
   CHECK( names && descs, 0, _T("NULL pointer in GetAllAvailablePlugins()") );

   names->Empty();
   descs->Empty();
   if ( states )
      states->Empty();

   MModuleListing *listing = MModule::ListAvailableModules(iface);
   if ( !listing )
   {
      return 0;
   }

   // found some plugins
   const size_t count = listing->Count();
   for ( size_t n = 0; n < count; n++ )
   {
      const MModuleListingEntry& entry = (*listing)[n];

      names->Add(entry.GetName());
      descs->Add(entry.GetShortDescription());

      // dirty hack: if we have states != NULL we know we're called from
      // GetAllAvailableFilters()
      if ( states )
      {
         ViewFilterFactory * const filterFactory
            = (ViewFilterFactory *)MModule::LoadModule(entry.GetName());
         if ( filterFactory )
         {
            states->Add(filterFactory->GetDefaultState());
            filterFactory->DecRef();
         }
         else
         {
            FAIL_MSG( _T("failed to create ViewFilterFactory") );

            states->Add(false);
         }
      }
   }

   listing->DecRef();

   return count;
}

// ----------------------------------------------------------------------------
// ProcessEvtHandler
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ProcessEvtHandler, wxEvtHandler)
   EVT_END_PROCESS(-1, ProcessEvtHandler::OnProcessTermination)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// MailMessageParameters
// ----------------------------------------------------------------------------

wxString
MailMessageParameters::GetParamValue(const wxString& name) const
{
   // look in Content-Type parameters
   String value = m_mimepart->GetParam(name);

   if ( value.empty() )
   {
      // try Content-Disposition parameters (should we?)
      value = m_mimepart->GetDispositionParam(name);
   }

   if ( value.empty() )
   {
      // if all else failed, call the base class

      // typedef is needed for VC++ -- otherwise you get a compile error --
      // but with it you get this warning
      #ifdef _MSC_VER
         #pragma warning(disable:4097)
      #endif

      typedef wxFileType::MessageParameters BaseMessageParameters;
      value = BaseMessageParameters::GetParamValue(name);

      #ifdef _MSC_VER
         #pragma warning(default:4097)
      #endif
   }

   return value;
}

// ----------------------------------------------------------------------------
// MessageView::AllProfileValues
// ----------------------------------------------------------------------------

MessageView::AllProfileValues::AllProfileValues()
{
   // init everything to some default values, even if they're not normally
   // used before ReadAllSettings() is called it is still better not to leave
   // junk in the struct fields
   fontFamily = -1;
   fontSize = -1;

   showHeaders =
   inlineRFC822 =
   inlinePlainText =
   showFaces =
   highlightURLs =
   autoViewer =
   preferHTML =
   allowHTML =
   allowImages = false;

   inlineGFX = -1;
   showExtImages = false;

   autocollect = M_ACTION_NEVER;
}

bool
MessageView::AllProfileValues::operator==(const AllProfileValues& other) const
{
   #define CMP(x) (x == other.x)

   return CMP(msgViewer) && CMP(BgCol) && CMP(FgCol) && CMP(AttCol) &&
          CMP(HeaderNameCol) && CMP(HeaderValueCol) &&
          CMP(fontDesc) &&
          (!fontDesc.empty() || (CMP(fontFamily) && CMP(fontSize))) &&
          CMP(showHeaders) && CMP(inlineRFC822) && CMP(inlinePlainText) &&
          CMP(inlineGFX) && CMP(showExtImages) &&
          CMP(highlightURLs) && (!highlightURLs || CMP(UrlCol)) &&
          // even if these fields are different, they don't change our
          // appearance, so ignore them for the purpose of this comparison
#if 0
          CMP(autocollect) && CMP(autocollectSenderOnly) &&
          CMP (autocollectNamed) CMP(autocollectBookName) &&
#endif // 0
          CMP(showFaces) &&
          CMP(autoViewer) &&
          (!autoViewer || CMP(preferHTML) && CMP(allowHTML) && CMP(allowImages));

   #undef CMP
}

wxFont MessageView::AllProfileValues::GetFont(wxFontEncoding encoding) const
{
   wxFont font(CreateFontFromDesc(fontDesc, fontSize, fontFamily));

   // assume that wxFONTENCODING_DEFAULT (US-ASCII) text can be shown in any
   // encoding
   if ( encoding != wxFONTENCODING_DEFAULT )
   {
      if ( !font.Ok() )
         font = *wxNORMAL_FONT;

      font.SetEncoding(encoding);
   }

   return font;
}

// ============================================================================
// MessageView implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MessageView creation
// ----------------------------------------------------------------------------

MessageView::MessageView()
{
   Init();
}

void
MessageView::Init(wxWindow *parent, Profile *profile)
{
   m_profile = profile;
   if ( m_profile )
      m_profile->IncRef();

   ResetViewer(parent);

   m_usingDefViewer = true;
}

void
MessageView::Init()
{
   m_profile = NULL;
   m_asyncFolder = NULL;
   m_mailMessage = NULL;
   m_viewer =
   m_viewerOld = NULL;
   m_filters = NULL;
   m_virtualMimeParts = NULL;
   m_cidsInMemory = NULL;

   m_uid = UID_ILLEGAL;
   m_encodingUser = wxFONTENCODING_DEFAULT;
   m_encodingAuto = wxFONTENCODING_SYSTEM;

   m_evtHandlerProc = NULL;

   RegisterForEvents();
}

MessageView::~MessageView()
{
   delete m_virtualMimeParts;

   delete m_cidsInMemory;

   UnregisterForEvents();

   DetachAllProcesses();
   delete m_evtHandlerProc;

   SafeDecRef(m_mailMessage);
   SafeDecRef(m_asyncFolder);
   SafeDecRef(m_profile);

   delete m_filters;
   delete m_viewer;
   delete m_viewerOld;
}

// ----------------------------------------------------------------------------
// loading and managing the viewers
// ----------------------------------------------------------------------------

/* static */
size_t MessageView::GetAllAvailableViewers(wxArrayString *names,
                                           wxArrayString *descs)
{
   return GetAllAvailablePlugins(MESSAGE_VIEWER_INTERFACE, names, descs);
}

void
MessageView::SetViewer(MessageViewer *viewer,
                       const String& viewerName,
                       wxWindow *parent)
{
   if ( !viewer )
   {
      // having a dummy viewer which simply doesn't do anything is
      // simpler/cleaner than inserting "if ( m_viewer )" checks everywhere
      viewer = CreateDefaultViewer();

      ASSERT_MSG( viewer, _T("must have default viewer, will crash without it!") );

      m_usingDefViewer = true;

      // make sure that the viewer name is empty if we're using the dummy one:
      // this is how we can determine whether we have a real viewer or not
      m_ProfileValues.msgViewer.clear();
   }
   else // we have a real viewer now
   {
      m_usingDefViewer = false;
   }

   MessageViewer *viewerOld = m_viewer;

   DoSetViewer(viewer, viewerName, parent);

   delete viewerOld;
}

void
MessageView::DoSetViewer(MessageViewer *viewer,
                         const String& viewerName,
                         wxWindow *parent)
{
   viewer->Create(this, parent ? parent : GetWindow()->GetParent());

   OnViewerChange(m_viewer, viewer, viewerName);

   m_viewer = viewer;
   m_viewerName = viewerName;
}

void MessageView::RestoreOldViewer()
{
   if ( m_viewerOld )
   {
      SetViewer(m_viewerOld, m_viewerNameOld);
      m_viewerOld = NULL;
      m_viewerNameOld.clear();
   }
}

/**
   Load the viewer by name.

   If the viewer with this name can't be loaded and another viewer is
   available, return its name in nameAlt parameter if it's not NULL: this
   allows to use a fallback viewer to show at least something if the configured
   viewer is not available.

   @param name of the viewer to load
   @param nameAlt output parameter for the available viewer name if the given
                  one couldn't be loaded
   @return the viewer (to be deleted by caller) or NULL
 */
static MessageViewer *LoadViewer(const String& name, String *nameAlt = NULL)
{
   MessageViewer *viewer = NULL;

   MModuleListing *
      listing = MModule::ListAvailableModules(MESSAGE_VIEWER_INTERFACE);

   const size_t countViewers = listing ? listing->Count() : 0;
   if ( !countViewers )
   {
      wxLogError(_("No message viewer plugins found. It will be "
                   "impossible to view any messages.\n"
                   "\n"
                   "Please check that Mahogany plugins are available."));
   }
   else // got listing of viewer factories
   {
      MModule *viewerFactory = MModule::LoadModule(name);

      // failed to load the configured viewer
      if ( !viewerFactory )
      {
         // try to find alternative
         if ( nameAlt )
         {
            for ( size_t n = 0; n < countViewers; n++ )
            {
               String nameViewer = (*listing)[n].GetName();
               if ( nameViewer != name )
               {
                  *nameAlt = nameViewer;
                  break;
               }
            }
         }
      }
      else // loaded ok
      {
         viewer = ((MessageViewerFactory *)viewerFactory)->Create();
         viewerFactory->DecRef();
      }
   }

   if ( listing )
      listing->DecRef();

   return viewer;
}

void
MessageView::CreateViewer()
{
   String name = m_ProfileValues.msgViewer;
   if ( name.empty() )
      name = GetStringDefault(MP_MSGVIEW_VIEWER);

   String nameAlt;
   MessageViewer *viewer = LoadViewer(name, &nameAlt);
   if ( !viewer )
   {
      wxLogWarning(_("Failed to load the configured message viewer '%s'.\n"
                     "\n"
                     "Reverting to the available message viewer '%s'."),
                   name.c_str(), nameAlt.c_str());

      viewer = LoadViewer(name = nameAlt);
   }

   if ( !viewer )
   {
      wxLogError(_("Failed to load any message viewer.\n"
                   "\n"
                   "Message preview will not work!"));
   }

   SetViewer(viewer, name);
}

// ----------------------------------------------------------------------------
// view filters
// ----------------------------------------------------------------------------

/* static */
size_t
MessageView::GetAllAvailableFilters(wxArrayString *names,
                                    wxArrayString *labels,
                                    wxArrayInt *states)
{
   return GetAllAvailablePlugins(VIEW_FILTER_INTERFACE, names, labels, states);
}

void
MessageView::InitializeViewFilters()
{
   CHECK_RET( !m_filters, _T("InitializeViewFilters() called twice?") );

   // always insert the terminating, "do nothing", filter at the end
   m_filters = new ViewFilterNode
                   (
                     new TransparentFilter(this),
                     ViewFilter::Priority_Lowest,
                     wxEmptyString,
                     wxEmptyString,
                     NULL
                   );

   MModuleListing *listing =
      MModule::ListAvailableModules(VIEW_FILTER_INTERFACE);

   if ( listing  )
   {
      const size_t count = listing->Count();
      for ( size_t n = 0; n < count; n++ )
      {
         const MModuleListingEntry& entry = (*listing)[n];

         const String& name = entry.GetName();
         ViewFilterFactory * const filterFactory
            = (ViewFilterFactory *)MModule::LoadModule(name);
         if ( filterFactory )
         {
            // create the node for the new filter
            int prio = filterFactory->GetPriority();

            if ( prio < ViewFilter::Priority_Lowest )
            {
               ERRORMESSAGE(( _("Incorrect message view filter priority %d "
                                "for the filter \"%s\""),
                              (int)prio, name.c_str() ));

               prio = ViewFilter::Priority_Lowest;
            }

            // find the right place to insert the new filter into
            for ( ViewFilterNode *node = m_filters, *nodePrev = NULL;
                  node;
                  nodePrev = node, node = node->GetNext() )
            {
               if ( prio >= node->GetPriority() )
               {
                  // create the new filter
                  ViewFilter *
                     filter = filterFactory->Create(this, node->GetFilter());

                  // and insert it here
                  ViewFilterNode *
                     nodeNew = new ViewFilterNode
                                   (
                                       filter,
                                       prio,
                                       name,
                                       wxGetTranslation(filterFactory->GetDescription()),
                                       node
                                   );

                  if ( !nodePrev )
                     m_filters = nodeNew;
                  else
                     nodePrev->SetNext(nodeNew);

                  // finally, enable/disable it initially as configured
                  Profile *profile = GetProfile();

                  int enable = profile->readEntryFromHere(name, -1);
                  if ( enable != -1 )
                  {
                     filter->Enable(enable != 0);
                  }

                  break;
               }
            }

            filterFactory->DecRef();
         }
      }

      listing->DecRef();
   }
}

void
MessageView::UpdateViewFiltersState()
{
   if ( !m_filters )
   {
      // no filters loaded yet
      return;
   }

   Profile *profile = GetProfile();

   // we never change the status of the last filter (transparent one), so stop
   // at one before last
   for ( ViewFilterNode *node = m_filters;
         node->GetNext();
         node = node->GetNext() )
   {
      int enable = profile->readEntryFromHere(node->GetName(), -1);
      if ( enable != -1 )
      {
         node->GetFilter()->Enable(enable != 0);
      }
   }
}

void
MessageView::CreateViewMenu()
{
   if ( !m_filters )
   {
      InitializeViewFilters();
   }
}

bool
MessageView::GetFirstViewFilter(String *name,
                                String *desc,
                                bool *enabled,
                                void **cookie)
{
   CHECK( cookie, false, _T("NULL cookie in GetFirstFilter") );

   *cookie = m_filters;

   return GetNextViewFilter(name, desc, enabled, cookie);
}

bool
MessageView::GetNextViewFilter(String *name,
                               String *desc,
                               bool *enabled,
                               void **cookie)
{
   CHECK( name && desc && enabled && cookie, false,
          _T("NULL parameter in GetNextFilter") );

   if ( !*cookie )
      return false;

   const ViewFilterNode * const filter = (ViewFilterNode *)(*cookie);

   // we shouldn't return the TransparentFilter because it is an implementation
   // detail and can't be turned off (nor does it make sense) anyhow
   //
   // as the TransparentFilter has empty name and is always the last one in the
   // priority queue we know that if we encounter it we may stop
   if ( filter->GetName().empty() )
      return false;

   *name = filter->GetName();
   *desc = filter->GetDescription();
   *enabled = filter->GetFilter()->IsEnabled();
   *cookie = filter->GetNext();

   return true;
}

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

wxWindow *MessageView::GetWindow() const
{
   return m_viewer->GetWindow();
}

wxFrame *MessageView::GetParentFrame() const
{
   return GetFrame(GetWindow());
}

String MessageView::GetFolderName() const
{
   CHECK( m_mailMessage->GetFolder(), _T("unknown"),
          _T("no folder in MessageView?") );

   return m_mailMessage->GetFolder()->GetName();
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

Profile *MessageView::GetProfile() const
{
   // always return something non NULL
   return m_profile ? m_profile : mApplication->GetProfile();
}

// ----------------------------------------------------------------------------
// MessageView events
// ----------------------------------------------------------------------------

void
MessageView::RegisterForEvents()
{
   // register with the event manager
   m_regCookieOptionsChange = MEventManager::Register(*this, MEventId_OptionsChange);
   ASSERT_MSG( m_regCookieOptionsChange, _T("can't register for options change event"));
   m_regCookieASFolderResult = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookieASFolderResult, _T("can't reg folder view with event manager"));
}

void MessageView::UnregisterForEvents()
{
   MEventManager::DeregisterAll(&m_regCookieASFolderResult,
                                &m_regCookieOptionsChange,
                                NULL);
}

bool
MessageView::OnMEvent(MEventData& event)
{
   if ( event.GetId() == MEventId_OptionsChange )
   {
      OnOptionsChange((MEventOptionsChangeData &)event);
   }
   else if ( event.GetId() == MEventId_ASFolderResult )
   {
      OnASFolderResultEvent((MEventASFolderResultData &)event);
   }

   return true;
}

void MessageView::OnOptionsChange(MEventOptionsChangeData& event)
{
   // if we don't have a viewer, we don't have to react to this event now -
   // we'll do it later when we create the viewer
   if ( !HasRealViewer() )
      return;

   // first of all, are we interested in this profile or not?
   Profile *profileChanged = event.GetProfile();
   if ( !profileChanged || !profileChanged->IsAncestor(GetProfile()) )
   {
      // it's some profile which has nothing to do with us
      return;
   }

   switch ( event.GetChangeKind() )
   {
      case MEventOptionsChangeData::Apply:
      case MEventOptionsChangeData::Ok:
      case MEventOptionsChangeData::Cancel:
         if ( UpdateProfileValues() )
            Update();
         break;

      default:
         FAIL_MSG(_T("unknown options change event"));
   }
}

void
MessageView::OnASFolderResultEvent(MEventASFolderResultData &event)
{
   ASMailFolder::Result *result = event.GetResult();
   if ( result->GetUserData() == this )
   {
      switch(result->GetOperation())
      {
         case ASMailFolder::Op_GetMessage:
         {
            Message *mptr =
               ((ASMailFolder::ResultMessage *)result)->GetMessage();

            if ( mptr )
            {
               // have we got the message we had asked for?
               if ( mptr->GetUId() == m_uid )
               {
                  DoShowMessage(mptr);
               }
               //else: not, it is some other one

               mptr->DecRef();
            }
         }
         break;

         default:
            FAIL_MSG(_T("Unexpected async result event"));
      }
   }

   result->DecRef();
}

// ----------------------------------------------------------------------------
// MessageView options
// ----------------------------------------------------------------------------

bool
MessageView::UpdateProfileValues()
{
   AllProfileValues settings;
   ReadAllSettings(&settings);

   // we do not call DecRef() on this one
   Profile *profile = GetProfile();

   bool updateViewer = false;
   for ( ViewFilterNode *filterNode = m_filters;
         filterNode;
         filterNode = filterNode->GetNext() )
   {
      if ( filterNode->GetFilter()->UpdateOptions(profile) )
      {
         updateViewer = true;
      }
   }

   if ( updateViewer || settings != m_ProfileValues )
   {
      bool recreateViewer = settings.msgViewer != m_ProfileValues.msgViewer;

      // update options first so that CreateViewer() creates the correct
      // viewer
      m_ProfileValues = settings;

      if ( recreateViewer )
      {
         CreateViewer();
      }
      else // use the same viewer
      {
         // but update it
         if ( m_uid != UID_ILLEGAL && m_asyncFolder )
         {
            (void)m_asyncFolder->GetMessage(m_uid, this);
         }
      }

      return true;
   }
   //else: nothing significant changed

   // but still reassign to update all options
   m_ProfileValues = settings;

   return false;
}

void
MessageView::ReadAllSettings(AllProfileValues *settings)
{
   Profile *profile = GetProfile();
   CHECK_RET( profile, _T("MessageView::ReadAllSettings: no profile") );

   // a macro to make setting many colour options less painful
   #define GET_COLOUR_FROM_PROFILE(col, name) \
      ReadColour(&col, profile, MP_MVIEW_ ## name)

   #define GET_COLOUR_FROM_PROFILE_IF_NOT_FG(which, name) \
      GET_COLOUR_FROM_PROFILE(col, name); \
      if ( col != settings->FgCol ) \
         settings->which = col

   GET_COLOUR_FROM_PROFILE(settings->FgCol, FGCOLOUR);
   GET_COLOUR_FROM_PROFILE(settings->BgCol, BGCOLOUR);

   wxColour col; // used by the macro
   GET_COLOUR_FROM_PROFILE_IF_NOT_FG(AttCol, ATTCOLOUR);
   GET_COLOUR_FROM_PROFILE_IF_NOT_FG(UrlCol, URLCOLOUR);
   GET_COLOUR_FROM_PROFILE_IF_NOT_FG(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE_IF_NOT_FG(HeaderValueCol, HEADER_VALUES_COLOUR);

   #undef GET_COLOUR_FROM_PROFILE
   #undef GET_COLOUR_FROM_PROFILE_IF_NOT_FG

   settings->msgViewer = READ_CONFIG_TEXT(profile, MP_MSGVIEW_VIEWER);

   settings->fontDesc = READ_CONFIG_TEXT(profile, MP_MVIEW_FONT_DESC);
   if ( settings->fontDesc.empty() )
   {
      settings->fontFamily = GetFontFamilyFromProfile(profile, MP_MVIEW_FONT);
      settings->fontSize = READ_CONFIG(profile, MP_MVIEW_FONT_SIZE);
   }

   settings->showHeaders = READ_CONFIG_BOOL(profile, MP_SHOWHEADERS);
   settings->inlinePlainText = READ_CONFIG_BOOL(profile, MP_PLAIN_IS_TEXT);
   settings->inlineRFC822 = READ_CONFIG_BOOL(profile, MP_RFC822_IS_TEXT);

   // we set inlineGFX to 0 if we don't inline graphics at all and to the
   // max size limit of graphics to show inline otherwise (-1 if no limit)
   settings->inlineGFX = READ_CONFIG(profile, MP_INLINE_GFX);
   if ( settings->inlineGFX )
   {
      settings->inlineGFX = READ_CONFIG(profile, MP_INLINE_GFX_SIZE);

      settings->showExtImages = READ_CONFIG_BOOL(profile, MP_INLINE_GFX_EXTERNAL);
   }

   settings->autocollect = (MAction)(long)READ_CONFIG(profile, MP_AUTOCOLLECT_INCOMING);
   settings->autocollectSenderOnly =  READ_CONFIG(profile, MP_AUTOCOLLECT_SENDER);
   settings->autocollectNamed =  READ_CONFIG(profile, MP_AUTOCOLLECT_NAMED);
   settings->autocollectBookName = READ_CONFIG_TEXT(profile, MP_AUTOCOLLECT_ADB);
   settings->showFaces = READ_CONFIG_BOOL(profile, MP_SHOW_XFACES);

   settings->autoViewer = READ_CONFIG_BOOL(profile, MP_MSGVIEW_AUTO_VIEWER);
   settings->preferHTML = READ_CONFIG_BOOL(profile, MP_MSGVIEW_PREFER_HTML);
   settings->allowHTML = READ_CONFIG_BOOL(profile, MP_MSGVIEW_ALLOW_HTML);
   settings->allowImages = READ_CONFIG_BOOL(profile, MP_MSGVIEW_ALLOW_IMAGES);

   settings->highlightURLs = READ_CONFIG_BOOL(profile, MP_HIGHLIGHT_URLS);

   // update the parents menu as the show headers option might have changed
   OnShowHeadersChange();

   m_viewer->UpdateOptions();

   // update the filters state
   UpdateViewFiltersState();
}

// ----------------------------------------------------------------------------
// MessageView headers processing
// ----------------------------------------------------------------------------

void
MessageView::ShowHeader(const String& name,
                        const String& valueOrig,
                        wxFontEncoding encHeader)
{
   m_viewer->ShowHeaderName(name);

   String value(valueOrig);
   wxFontEncoding encReal = encHeader;
   if ( encHeader != wxFONTENCODING_SYSTEM )
   {
      // convert the string to an encoding we can show, if needed
      EnsureAvailableTextEncoding(&encReal, &value);
   }

   // don't highlight URLs in headers which contain message IDs or other things
   // which look just like the URLs but, in fact, are not ones
   bool highlightURLs = m_ProfileValues.highlightURLs;
   String nameUpper(name.Upper());
   if ( highlightURLs )
   {
      if ( nameUpper == _T("IN-REPLY-TO") ||
            nameUpper == _T("REFERENCES") ||
             nameUpper == _T("CONTENT-ID") ||
              nameUpper == _T("MESSAGE-ID") ||
               nameUpper == _T("RESENT-MESSAGE-ID") )
      {
         highlightURLs = false;
      }
   }

   do
   {
      String before,
             url,
             urlText;

      if ( highlightURLs )
      {
         before = strutil_findurl(value, url);

         // for headers (usually) containing addresses with personal names try
         // to detect the personal names as well
         if ( *url.c_str() == _T('<') &&
               (nameUpper == _T("FROM") ||
                 nameUpper == _T("TO") ||
                  nameUpper == _T("CC") ||
                   nameUpper == _T("BCC") ||
                    nameUpper == _T("RESENT-FROM") ||
                     nameUpper == _T("RESENT-TO")) )
         {
            urlText = url;

            // try to find the personal name as well by going backwards
            // until we reach the previous address
            bool inQuotes = false,
                 stop = false;

            while ( !before.empty() )
            {
               wxChar ch = before.Last();
               switch ( ch )
               {
                  case _T('"'):
                     inQuotes = !inQuotes;
                     break;

                  case _T(','):
                  case _T(';'):
                     if ( !inQuotes )
                        stop = true;
               }

               if ( stop )
                  break;

               urlText.insert((size_t)0, (size_t)1, ch);
               before.erase(before.length() - 1);
            }
         }
         else // not a mail address
         {
            // use the URL itself as label
            urlText = url;
         }
      }
      else // no URL highlighting
      {
         before = value;
         value.clear();
      }

      // do this even if "before" is empty if we have to change the
      // encoding (it will affect the URL following it)
      if ( !before.empty() || encReal != wxFONTENCODING_SYSTEM )
      {
         m_viewer->ShowHeaderValue(before, encReal);
      }

      if ( !url.empty() )
      {
         m_viewer->ShowHeaderURL(urlText, url);
      }
   }
   while ( !value.empty() );

   m_viewer->EndHeader();
}

void
MessageView::ShowHeaders()
{
   m_viewer->StartHeaders();

   // if wanted, display all header lines
   if ( m_ProfileValues.showHeaders )
   {
      HeaderIterator headers(m_mailMessage->GetHeader());

      String name,
             value;
      while ( headers.GetNext(&name, &value, HeaderIterator::MultiLineOk) )
      {
         ShowHeader(name, value, wxFONTENCODING_SYSTEM);
      }
   }
   else
   {
      // retrieve all headers at once instead of calling Message::GetHeaderLine()
      // many times: this is incomparably faster with remote servers (one round
      // trip to server is much less expensive than a dozen of them!)

      // all the headers the user configured
      wxArrayString headersUser =
         strutil_restore_array(READ_CONFIG(GetProfile(), MP_MSGVIEW_HEADERS));

      // X-Face is handled separately
#ifdef HAVE_XFACES
      if ( m_ProfileValues.showFaces )
      {
         headersUser.Insert(_T("X-Face"), 0);
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
         return;
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
         static const wxChar *envelopHeadersNames[] =
         {
            _T("From"),
            _T("To"),
            _T("Cc"),
            _T("Bcc"),
            _T("Subject"),
            _T("Date"),
            _T("Newsgroups"),
            _T("Message-Id"),
            _T("In-Reply-To"),
            _T("References"),
         };

         ASSERT_MSG( EnvelopHeader_Max == WXSIZEOF(envelopHeadersNames),
                     _T("forgot to update something - should be kept in sync!") );

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
         ASSERT_MSG( nNonEnv == countNonEnvHeaders, _T("logic error") );

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
               FAIL_MSG( _T("logic error") );

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
                        default: FAIL_MSG( _T("forgot to add header here") );
                        case EnvelopHeader_From: mat = MAT_FROM; break;
                        case EnvelopHeader_To: mat = MAT_TO; break;
                        case EnvelopHeader_Cc: mat = MAT_CC; break;
                        case EnvelopHeader_Bcc: mat = MAT_BCC; break;
                     }

                     value = m_mailMessage->GetAddressesString(mat);
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
                  FAIL_MSG( _T("unknown envelop header") );
            }

            // extract encoding info from it
            wxFontEncoding enc;
            headerValues.Add(MIME::DecodeHeader(value, &enc));
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
         const wxString& xfaceString = headerValues[n++];
         if ( xfaceString.length() > 20 )
         // FIXME it was > 2, i.e. \r\n. Although if(uncompface(data) < 0) in
         // XFace.cpp should catch illegal data, it is not the case. For example,
         // for "X-Face: nope" some nonsense was displayed. So we use 20 for now.
         {
            // valid X-Faces are always ASCII, so don't bother if conversion
            // fails
            const wxCharBuffer xfaceBuf(xfaceString.ToAscii());
            if ( xfaceBuf )
            {
               XFace *xface = new XFace;
               xface->CreateFromXFace(xfaceBuf);

               char **xfaceXpm;
               if ( xface->CreateXpm(&xfaceXpm) )
               {
                  m_viewer->ShowXFace(wxBitmap(xfaceXpm));

                  wxIconManager::FreeImage(xfaceXpm);
               }

               delete xface;
            }
         }
      }

      // show the headers using the correct encoding now
      wxFontEncoding encInHeaders = wxFONTENCODING_SYSTEM;
      for ( ; n < countHeaders; n++ )
      {
         wxString value = headerValues[n];
         if ( value.empty() )
         {
            // don't show empty headers at all
            continue;
         }

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
            if ( m_encodingUser != wxFONTENCODING_DEFAULT )
            {
               // use the user specified encoding if none specified in the header
               // itself
               encHeader = m_encodingUser;
            }
            else if ( m_encodingAuto != wxFONTENCODING_SYSTEM )
            {
               encHeader = m_encodingAuto;
            }
         }

#if 0
         // does not work - EnsureAvailableTextEncoding for UTF-8 always succeeds
         if ( !EnsureAvailableTextEncoding(&encHeader, &value) )
#endif
         {
            // special handling for the UTF-7|8 if it's not supported natively
            if ( encHeader == wxFONTENCODING_UTF8 ||
                  encHeader == wxFONTENCODING_UTF7 )
            {
               encHeader = ConvertUTFToMB(&value, encHeader);
            }
         }

         // show the header and mark the URLs in it
         const String& name = headerNames[n];

         // there can be more than one line in each header in which case we
         // show each line of the value on a separate line -- although this is
         // wrong because there could be a single header with a multiline value
         // as well, this is the best we can do considering that GetHeaderLine
         // concatenates the values of all headers with the same name anyhow
         wxArrayString values = wxStringTokenize(value, _T("\n"));

         const size_t linesCount = values.GetCount();
         for ( size_t line = 0; line < linesCount; ++line )
         {
            ShowHeader(name, values[line], encHeader);
         }
      }

      // NB: some broken mailers don't create correct "Content-Type" header,
      //     but they may yet give us the necessary info in the other headers
      //     so we assume the header encoding as the default encoding for the
      //     body
      m_encodingAuto = encInHeaders;
   }

   m_viewer->EndHeaders();
}

// ----------------------------------------------------------------------------
// MessageView text part processing
// ----------------------------------------------------------------------------

void MessageView::ShowTextPart(const MimePart *mimepart)
{
   // as we're going to need its contents, we'll have to download it: check if
   // it is not too big before doing this
   if ( !CheckMessagePartSize(mimepart) )
   {
      // don't download this part
      return;
   }

   ShowText(mimepart->GetTextContent(), mimepart->GetTextEncoding());
}

void MessageView::ShowText(String textPart, wxFontEncoding textEnc)
{
   // get the encoding of the text
   wxFontEncoding encPart;
   if( m_encodingUser != wxFONTENCODING_DEFAULT )
   {
      // user-specified encoding overrides everything
      encPart = m_encodingUser;
   }
   else // determine the encoding ourselves
   {
      encPart = textEnc;

      if ( encPart == wxFONTENCODING_UTF8 || encPart == wxFONTENCODING_UTF7 )
      {
         // show UTF-8|7, not env. encoding in Language menu
         m_encodingAuto = encPart;

         // convert from UTF-8|7 to environment's default encoding
         encPart = ConvertUTFToMB(&textPart, encPart);
      }
      else if ( encPart == wxFONTENCODING_SYSTEM ||
                encPart == wxFONTENCODING_DEFAULT )
      {
         // use the encoding of the last part which had it
         encPart = m_encodingAuto;
      }
      else if ( m_encodingAuto == wxFONTENCODING_SYSTEM )
      {
         // remember the encoding for the next parts
         m_encodingAuto = encPart;
      }
   }

   if ( encPart == wxFONTENCODING_SYSTEM )
   {
      // autodetecting encoding didn't work, use the fall back encoding, if any
      if ( m_encodingUser != wxFONTENCODING_DEFAULT )
      {
         encPart = (wxFontEncoding)(long)
                     READ_CONFIG(GetProfile(), MP_MSGVIEW_DEFAULT_ENCODING);
      }
   }

   // init the style we're going to use
   bool fontSet = false;
   MTextStyle style;
   if ( encPart != wxFONTENCODING_SYSTEM )
   {
      if ( EnsureAvailableTextEncoding(&encPart, &textPart) )
      {
         wxFont font = m_ProfileValues.GetFont(encPart);

         style.SetFont(font);

         fontSet = true;
      }
      //else: don't change font -- no such encoding anyhow
   }

   // we need to reset the font and the colour because they may have been
   // changed by the headers
   if ( !fontSet )
      style.SetFont(m_ProfileValues.GetFont());

   style.SetTextColour(m_ProfileValues.FgCol);

   // show the text by passing it to all the registered view filters
   if ( !m_filters )
   {
      InitializeViewFilters();
   }

   ViewFilter *filter = m_filters->GetFilter();
   CHECK_RET( filter, _T("no view filters at all??") );

   filter->StartText();
   filter->Process(textPart, m_viewer, style);
}

// ----------------------------------------------------------------------------
// MessageView attachments and images handling
// ----------------------------------------------------------------------------

void MessageView::ShowAttachment(const MimePart *mimepart)
{
   // get the icon for the attachment using its MIME type and filename
   // extension (if any)
   wxString mimeFileName = mimepart->GetFilename();
   wxIcon icon = mApplication->GetIconManager()->
                     GetIconFromMimeType(mimepart->GetType().GetFull(),
                                         mimeFileName.AfterLast('.'));

   m_viewer->InsertAttachment(icon, GetClickableInfo(mimepart));
}

void MessageView::ShowImage(const MimePart *mimepart)
{
   // first of all, can we show it inline at all?
   bool showInline = m_viewer->CanInlineImages();

   if ( showInline )
   {
      switch ( m_ProfileValues.inlineGFX )
      {
         default:
            // check that the size of the image is less than configured
            // maximum
            if ( mimepart->GetSize() > 1024*(size_t)m_ProfileValues.inlineGFX )
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
                     GetParentFrame(),
                     MDIALOG_YESNOTITLE,
                     M_DLG_NO_DEFAULT,
                     M_MSGBOX_GFX_NOT_INLINED
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

         case 0:
            // never inline
            showInline = false;
            break;

         case -1:
            // always inline
            break;
      }
   }

   if ( showInline )
   {
      unsigned long len;
      const void *data = mimepart->GetContent(&len);

      if ( !data )
      {
         wxLogError(_("Cannot get attachment content."));

         return;
      }

      wxMemoryInputStream mis(data, len);
      wxImage img(mis);
      if ( !img.Ok() )
      {
#ifdef OS_UNIX
         // try loading via wxIconManager which can use ImageMagik to do the
         // conversion
         MTempFileName tmpFN;
         if ( tmpFN.IsOk() )
         {
            String filename = tmpFN.GetName();
            MimeSave(mimepart, filename);

            bool ok;
            img =  wxIconManager::LoadImage(filename, &ok, true);
            if ( !ok )
               showInline = false;
         }
#else // !OS_UNIX
         showInline = false;
#endif // OS_UNIX/!OS_UNIX
      }

      if ( showInline )
      {
         m_viewer->InsertImage(img, GetClickableInfo(mimepart));
      }
   }

   if ( !showInline )
   {
      // show as an attachment then
      ShowAttachment(mimepart);
   }
}

/* static */
ClickableInfo *MessageView::GetClickableInfo(const MimePart *mimepart) const
{
   return new ClickableAttachment(const_cast<MessageView *>(this), mimepart);
}

// ----------------------------------------------------------------------------
// global MIME structure parsing
// ----------------------------------------------------------------------------

void
MessageView::ShowPart(const MimePart *mimepart)
{
   size_t partSize = mimepart->GetSize();
   if ( partSize == 0 )
   {
      // ignore empty parts but warn user as it might indicate a problem
      wxLogStatus(GetParentFrame(),
                  _("Skipping the empty MIME part #%s."),
                  mimepart->GetPartSpec().c_str());

      return;
   }

   MimeType type = mimepart->GetType();

   String mimeType = type.GetFull();

   String fileName = mimepart->GetFilename();

   MimeType::Primary primaryType = type.GetPrimary();

   // deal with unknown MIME types: some broken mailers use unregistered
   // primary MIME types - try to do something with them here (even though
   // breaking the neck of the author of the software which generated them
   // would be more satisfying)
   if ( primaryType > MimeType::OTHER )
   {
      wxString typeName = type.GetType();

      if ( typeName == _T("OCTET") )
      {
         // I have messages with "Content-Type: OCTET/STREAM", convert them
         // to "APPLICATION/OCTET-STREAM"
         primaryType = MimeType::APPLICATION;
      }
      else
      {
         wxLogDebug(_T("Invalid MIME type '%s'!"), typeName.c_str());
      }
   }

   // let's guess a little if we have generic APPLICATION MIME type, we may
   // know more about this file type from local sources
   if ( primaryType == MimeType::APPLICATION )
   {
      // get the MIME type for the files of this extension
      wxString ext = fileName.AfterLast('.');
      if ( !ext.empty() )
      {
         wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
         wxFileType *ft = mimeManager.GetFileTypeFromExtension(ext);
         if(ft)
         {
            wxString mt;
            ft->GetMimeType(&mt);
            delete ft;

            if(wxMimeTypesManager::IsOfType(mt,_T("image/*")))
               primaryType = MimeType::IMAGE;
            else if(wxMimeTypesManager::IsOfType(mt,_T("audio/*")))
               primaryType = MimeType::AUDIO;
            else if(wxMimeTypesManager::IsOfType(mt,_T("video/*")))
               primaryType = MimeType::VIDEO;
         }
      }
   }

   m_viewer->StartPart();

   // if the disposition is set to attachment we force the part to be shown
   // as an attachment by disabling all the other heuristics
   const bool isAttachment = mimepart->IsAttachment();

   // check for images before checking for more general viewer-specific part
   // support as CanViewerProcessPart() returns for images if the viewer
   // supports their inline display, but images should be inserted with
   // ShowImage() and not InsertRawContents()
   if ( primaryType == MimeType::IMAGE )
   {
      ShowImage(mimepart);
   }
   else if ( !isAttachment && CanViewerProcessPart(mimepart) )
   {
      // as we're going to need its contents, we'll have to download it: check
      // if it is not too big before doing this
      if ( CheckMessagePartSize(mimepart) )
      {
         unsigned long len;
         const char * const
            data = static_cast<const char *>(mimepart->GetContent(&len));

         if ( !data )
         {
            wxLogError(_("Cannot get attachment content."));
         }
         else
         {
            wxString s;
#if wxUSE_UNICODE
            if ( mimepart->GetType().IsText() )
            {
               s = wxCSConv(mimepart->GetTextEncoding()).cMB2WC(data, len, NULL);
            }

            if ( s.empty() )
#endif // wxUSE_UNICODE
               s = wxString::From8BitData(data, len);

            m_viewer->InsertRawContents(s);
         }
      }
      //else: skip this part
   }
   else if ( !isAttachment &&
             ((mimeType == _T("TEXT/PLAIN") &&
               (fileName.empty() || m_ProfileValues.inlinePlainText)) ||
              (primaryType == MimeType::MESSAGE &&
                m_ProfileValues.inlineRFC822)) )
   {
      ShowTextPart(mimepart);
   }
   else // attachment
   {
      ShowAttachment(mimepart);
   }

   m_viewer->EndPart();
}

bool
MessageView::ProcessAllNestedParts(const MimePart *mimepart,
                                   MimePartAction action)
{
   bool rc = true;

   const MimePart *partChild = mimepart->GetNested();
   while ( partChild )
   {
      if ( !ProcessPart(partChild, action) )
         rc = false;

      partChild = partChild->GetNext();
   }

   return rc;
}

bool
MessageView::ProcessAlternativeMultiPart(const MimePart *mimepart,
                                         MimePartAction action)
{
   // find the best subpart we can show
   //
   // normally we'd have to iterate from end as the best
   // representation (i.e. the most faithful) is the last one
   // according to RFC 2046, but as we only have forward pointers
   // we iterate from the start - not a big deal
   const MimePart *partChild = mimepart->GetNested();

   const MimePart *partBest = partChild;
   while ( partChild )
   {
      String mimetype = partChild->GetType().GetFull();

      if ( mimetype == _T("TEXT/PLAIN") || CanViewerProcessPart(partChild) )
      {
         // remember this one as the best so far
         partBest = partChild;
      }

      partChild = partChild->GetNext();
   }

   // show just the best one
   CHECK(partBest, false, _T("No part can be displayed !"));

   // the content of an alternative is not necessarily a single
   // part, so process it as well.
   return ProcessPart(partBest, action);
}

bool
MessageView::ProcessRelatedMultiPart(const MimePart *mimepart,
                                     MimePartAction action)
{
   // see RFC 2387: http://www.faqs.org/rfcs/rfc2387.html

   // we need to find the start part: it is specified using the "start"
   // parameter but usually is just the first one as this parameter is optional
   const String cidStart = mimepart->GetParam("start");

   const MimePart *partChild = mimepart->GetNested();

   const MimePart *partStart = cidStart.empty() ? partChild : NULL;

   while ( partChild )
   {
      // if we're just testing, it's enough to find the start part, it's the
      // only one which counts
      if ( action == Part_Test && partStart )
         break;

      // FIXME: this manual parsing is ugly and probably wrong, should add
      //        a method to MimePart for this...
      const String headers = partChild->GetHeaders();

      static const char *CONTENT_ID = "content-id: ";
      static const size_t CONTENT_ID_LEN = 12; // strlen(CONTENT_ID)

      size_t posCID = headers.Lower().find(CONTENT_ID);
      if ( posCID != String::npos )
      {
         posCID += CONTENT_ID_LEN;

         const size_t posEOL = headers.find_first_of("\r\n", posCID);

         String cid;
         cid.assign(headers, posCID, posEOL - posCID);

         if ( cid == cidStart )
         {
            if ( partStart )
            {
               wxLogDebug(_T("Duplicate CIDs in multipart/related message"));
            }

            partStart = partChild;
         }

         // if we're going to display the part now we need to store all the
         // parts except the start part in memory so that the start part could
         // use them
         if ( !cid.empty() && action == Part_Show && partChild != partStart )
         {
            StoreMIMEPartData(partChild, cid);
         }
      }

      partChild = partChild->GetNext();
   }

   CHECK( partStart, false, _T("No start part in multipart/related") );

   return ProcessPart(partStart, action);
}

bool
MessageView::ProcessSignedMultiPart(const MimePart *mimepart)
{
   if ( mimepart->GetParam(_T("protocol")) != _T("application/pgp-signature") )
   {
      // unknown signature protocol, don't try to interpret it
      return false;
   }

   static const wxChar *
      sigIgnoredMsg = gettext_noop("\n\nSignature will be ignored");

   MimePart * const signedPart = mimepart->GetNested();
   MimePart * const signaturePart = signedPart->GetNext();
   if ( !signedPart || !signaturePart )
   {
      wxLogWarning(String(_("This message pretends to be signed but "
                            "doesn't have the correct MIME structure.")) +
                   wxGetTranslation(sigIgnoredMsg));

      return false;
   }

   // Let's not be too strict on what we receive
   //CHECK_RET( (signedPart->GetTransferEncoding() == MIME_ENC_7BIT ||
   //            signedPart->GetTransferEncoding() == MIME_ENC_BASE64 ||
   //            signedPart->GetTransferEncoding() == MIME_ENC_QUOTEDPRINTABLE),
   //           _T("Signed part should be 7 bits"));
   //CHECK_RET( signaturePart->GetNext() == 0, _T("Signature should be the last part") );
   //CHECK_RET( signaturePart->GetNested() == 0, _T("Signature should not have nested parts") );

   if ( signaturePart->GetType().GetFull() != _T("APPLICATION/PGP-SIGNATURE") )
   {
      wxLogWarning(String(_("Signed message signature does not have a "
                            "\"application/pgp-signature\" type.")) +
                   wxGetTranslation(sigIgnoredMsg));

      return false;
   }

   MCryptoEngineFactory * const factory
      = (MCryptoEngineFactory *)MModule::LoadModule(_T("PGPEngine"));
   CHECK( factory, false, _T("failed to create PGPEngineFactory") );

   MCryptoEngine *pgpEngine = factory->Get();
   MCryptoEngineOutputLog *log = new MCryptoEngineOutputLog(GetWindow());

   // the signature is applied to both the headers and the body of the
   // message and it is done after encoding the latter so we need to get
   // it in the raw form
   String signedText = signedPart->GetHeaders() +
                           signedPart->GetRawContentAsString();

   MCryptoEngine::Status status = pgpEngine->VerifyDetachedSignature
                                             (
                                                signedText,
                                                signaturePart->GetTextContent(),
                                                log
                                             );
   ClickablePGPInfo *pgpInfo = ClickablePGPInfo::CreateFromSigStatusCode
                               (
                                 status,
                                 this,
                                 log
                               );

   ProcessPart(signedPart);

   pgpInfo->SetLog(log);

   ShowText(_T("\r\n"));

   m_viewer->InsertClickable(pgpInfo->GetBitmap(),
                             pgpInfo,
                             pgpInfo->GetColour());

   ShowText(_T("\r\n"));

   factory->DecRef();

   return true;
}

bool
MessageView::ProcessEncryptedMultiPart(const MimePart *mimepart)
{
   if ( mimepart->GetParam(_T("protocol")) != _T("application/pgp-encrypted") )
   {
      // unknown encryption protocol, don't try to interpret it
      return false;
   }

   MimePart * const controlPart = mimepart->GetNested();
   MimePart * const encryptedPart = controlPart->GetNext();

   if ( !controlPart || !encryptedPart )
   {
      wxLogError(_("This message pretends to be encrypted but "
                   "doesn't have the correct MIME structure."));
      return false;
   }

   // We could also test some more features:
   // - that the control part actually has an "application/pgp-encrypted" type
   // - that it contains a "Version: 1" field (and only that)

   if ( encryptedPart->GetType().GetFull() != _T("APPLICATION/OCTET-STREAM") )
   {
      wxLogError(_("The actual encrypted data part does not have a "
                   "\"application/octet-stream\" type, "
                   "ignoring it."));
      return false;
   }

   String encryptedData = encryptedPart->GetRawContentAsString();

   MCryptoEngineFactory * const factory
      = (MCryptoEngineFactory *)MModule::LoadModule(_T("PGPEngine"));
   CHECK( factory, false, _T("failed to create PGPEngineFactory") );

   MCryptoEngine* pgpEngine = factory->Get();

   MCryptoEngineOutputLog *log = new MCryptoEngineOutputLog(GetWindow());

   String decryptedData;
   MCryptoEngine::Status status =
         pgpEngine->Decrypt(encryptedData, decryptedData, log);

   ClickablePGPInfo *pgpInfo = 0;
   switch ( status )
   {
      case MCryptoEngine::OK:
         pgpInfo = new PGPInfoGoodMsg(this);

         {
            MimePartVirtual *mpv = new MimePartVirtual(decryptedData);
            AddVirtualMimePart(mpv);
            ProcessPart(mpv);
         }
         break;

      default:
         wxLogError(_("Decrypting the PGP message failed."));
         // fall through

      // if the user cancelled decryption, don't complain about it
      case MCryptoEngine::OPERATION_CANCELED_BY_USER:
         // using unmodified text is not very helpful here anyhow so
         // simply replace it with an icon
         pgpInfo = new PGPInfoBadMsg(this);
   }

   pgpInfo->SetLog(log);

   ShowText(_T("\r\n"));

   m_viewer->InsertClickable(pgpInfo->GetBitmap(),
                             pgpInfo,
                             pgpInfo->GetColour());

   ShowText(_T("\r\n"));

   factory->DecRef();

   return true;
}

bool
MessageView::ProcessMultiPart(const MimePart *mimepart,
                              const String& subtype,
                              MimePartAction action)
{
   // TODO: support for DIGEST

   bool processed = false;
   if ( subtype == _T("ALTERNATIVE") )
   {
      processed = ProcessAlternativeMultiPart(mimepart, action);
   }
   else if ( subtype == _T("RELATED") )
   {
      processed = ProcessRelatedMultiPart(mimepart, action);
   }
   else if ( subtype == _T("SIGNED") )
   {
      switch ( action )
      {
         case Part_Show:
            processed = ProcessSignedMultiPart(mimepart);
            break;

         case Part_Test:
            // all viewers can display signatures
            processed = true;
            break;

         default:
            FAIL_MSG( _T("unknown MIME part processing action") );
      }
   }
   else if ( subtype == _T("ENCRYPTED") )
   {
      switch ( action )
      {
         case Part_Show:
            processed = ProcessEncryptedMultiPart(mimepart);
            break;

         case Part_Test:
            // FIXME: this is wrong, we should examine the parts of the
            //        encrypted message
            processed = true;
            break;

         default:
            FAIL_MSG( _T("unknown MIME part processing action") );
      }
   }
   //else: process all unknown as MIXED (according to the RFC 2047)

   if ( !processed )
      processed = ProcessAllNestedParts(mimepart, action);

   return processed;
}

bool
MessageView::ProcessPart(const MimePart *mimepart, MimePartAction action)
{
   CHECK( mimepart, false, _T("MessageView::ProcessPart: NULL mimepart") );

   const MimeType type = mimepart->GetType();
   switch ( type.GetPrimary() )
   {
      case MimeType::MULTIPART:
         return ProcessMultiPart(mimepart, type.GetSubType(), action);

      case MimeType::MESSAGE:
         if ( m_ProfileValues.inlineRFC822 )
         {
            return ProcessAllNestedParts(mimepart, action);
         }
         //else: fall through and show it as attachment

      case MimeType::TEXT:
      case MimeType::APPLICATION:
      case MimeType::AUDIO:
      case MimeType::IMAGE:
      case MimeType::VIDEO:
      case MimeType::MODEL:
      case MimeType::OTHER:
      case MimeType::CUSTOM1:
      case MimeType::CUSTOM2:
      case MimeType::CUSTOM3:
      case MimeType::CUSTOM4:
      case MimeType::CUSTOM5:
      case MimeType::CUSTOM6:
         switch ( action )
         {
            case Part_Show:
               // a simple part, show it (ShowPart() decides how exactly)
               ShowPart(mimepart);
               return true;

            case Part_Test:
               {
                  const MimeType type = mimepart->GetType();
                  return (type.GetPrimary() == MimeType::IMAGE &&
                           m_viewer->CanInlineImages()) ||
                              m_viewer->CanProcess(type.GetFull());
               }

            default:
               FAIL_MSG( _T("unknown MIME part processing action") );
         }
         break;

      default:
         FAIL_MSG( _T("unknown MIME type") );
   }

   return false;
}

void
MessageView::AddVirtualMimePart(MimePart *mimepart)
{
   if ( !m_virtualMimeParts )
      m_virtualMimeParts = new VirtualMimePartsList;

   m_virtualMimeParts->push_back(mimepart);
}

bool MessageView::StoreMIMEPartData(const MimePart *part, const String& cidOrig)
{
   unsigned long len;
   const void *data = part->GetContent(&len);

   if ( !data )
      return false;

   if ( !m_cidsInMemory )
   {
      // one-time initialization
      m_cidsInMemory = new wxArrayString;

      static bool s_mimeHandlerInitialized = false;
      if ( !s_mimeHandlerInitialized )
      {
         s_mimeHandlerInitialized = true;

         wxFileSystem::AddHandler(new MIMEFSHandler);
      }
   }

   // cid could be quoted with <...>, unquote it then as it's referenced
   // without the quotes in the other parts
   CHECK( !cidOrig.empty(), false, "empty CID not allowed" );

   String cid;
   if ( *cidOrig.begin() == '<' && *cidOrig.rbegin() == '>' )
      cid.assign(cidOrig, 1, cidOrig.length() - 2);
   else
      cid = cidOrig;

   m_cidsInMemory->Add(cid);
   MIMEFSHandler::AddFile(cid, data, len);

   return true;
}

void MessageView::ClearMIMEPartDataStore()
{
   if ( !m_cidsInMemory || m_cidsInMemory->empty() )
      return;

   const size_t count = m_cidsInMemory->size();
   for ( size_t n = 0; n < count; n++ )
   {
      MIMEFSHandler::RemoveFile((*m_cidsInMemory)[n]);
   }

   m_cidsInMemory->clear();
}

// ----------------------------------------------------------------------------
// finding the best viewer for the current message
// ----------------------------------------------------------------------------

// bit flags returned by GetPartContent()
enum
{
   PartContent_Text = 0x01,
   PartContent_Image = 0x02,
   PartContent_HTML = 0x04,

   // this one is set when we have images only used by HTML part of the message
   PartContent_HTMLImage = 0x08
};

int
MessageView::DeterminePartContent(const MimePart *mimepart)
{
   int contents = 0;

   // if the part is explicitely an attachment we show it as attachment anyhow
   // so its real contents don't matter
   if ( !mimepart->IsAttachment() )
   {
      const MimeType type = mimepart->GetType();
      const String subtype = type.GetSubType();
      switch ( type.GetPrimary() )
      {
         case MimeType::MESSAGE:
            if ( !m_ProfileValues.inlineRFC822 )
               break;
            //else: fall through and handle it as embedded multipart

         case MimeType::MULTIPART:
            {
               // the automatic viewer determination logic is the same for
               // multipart/alternative and multipart/mixed and we don't really
               // support anything else for now (except multipart/related which
               // is accounted for below)
               for ( const MimePart *partChild = mimepart->GetNested();
                     partChild;
                     partChild = partChild->GetNext() )
               {
                  contents |= DeterminePartContent(partChild);
               }

               if ( type.GetSubType() == _T("RELATED") )
               {
                  // this is, of course, just a particular cas but a very
                  // common one so we handle the situation when we have
                  // multipart/related with an HTML part and an image part
                  // specially: this almost certainly means that the images can
                  // be seen *only* when HTML is viewed
                  if ( (contents & PartContent_HTML) &&
                           (contents & PartContent_Image) )
                  {
                     contents |= PartContent_HTMLImage;
                  }
               }
            }
            break;

         case MimeType::TEXT:
            if ( subtype == _T("PLAIN") )
               contents |= PartContent_Text;
            else if ( subtype == _T("HTML") )
               contents |= PartContent_HTML;
            break;

         case MimeType::IMAGE:
            contents |= PartContent_Image;
            break;

         default:
            wxFAIL_MSG( _T("Unknown MIME part type") );
            // fall through

         case MimeType::APPLICATION:
         case MimeType::AUDIO:
         case MimeType::VIDEO:
         case MimeType::MODEL:
         case MimeType::OTHER:
         case MimeType::CUSTOM1:
         case MimeType::CUSTOM2:
         case MimeType::CUSTOM3:
         case MimeType::CUSTOM4:
         case MimeType::CUSTOM5:
         case MimeType::CUSTOM6:
            // these parts don't influence the viewer choice
            break;
      }
   }

   return contents;
}

void
MessageView::AutoAdjustViewer(const MimePart *mimepart)
{
   // find out which viewer we want to use for this message
   String viewerName;
   const int contents = DeterminePartContent(mimepart);
   if ( contents & PartContent_HTML )
   {
      // use HTML viewer when the user prefers HTML to plain text or when there
      // is nothing but HTML or when we have images embedded in HTML (so they
      // won't be shown if we don't use the HTML viewer) and we do want to see
      // images inline
      if ( m_ProfileValues.preferHTML ||
            (!(contents & PartContent_Text) && m_ProfileValues.allowHTML) ||
               ((contents & PartContent_HTMLImage) &&
                  m_ProfileValues.allowImages) )
      {
         viewerName = _T("HtmlViewer");
      }
   }

   if ( viewerName.empty() )
   {
      if ( contents & PartContent_Image )
      {
         if ( m_ProfileValues.allowImages )
         {
            // this is the best viewer for showing images currently
            viewerName = _T("LayoutViewer");
         }
      }
   }
   //else: viewerName is already HTML and can show images anyhow


   if ( viewerName == m_viewerName )
   {
      // nothing to do, we already use the correct viewer
      return;
   }

   if ( viewerName.empty() || viewerName == m_viewerNameOld )
   {
      // either we don't have any specific viewer to set, in which case we need
      // to just restore the user-chosen one if any, or we need the viewer
      // which had been previously replaced
      RestoreOldViewer();
   }
   else // we need to switch to another viewer
   {
      // don't update the message contents here: we're called from Update()
      // which is going to do it anyhow
      ChangeViewerWithoutUpdate(viewerName);
   }
}

bool MessageView::ChangeViewerWithoutUpdate(const String& viewerName)
{
   ASSERT_MSG( !viewerName.empty(), _T("empty viewer name") );

   MessageViewer *viewer = LoadViewer(viewerName);
   if ( !viewer )
   {
      wxLogWarning(_("Viewer \"%s\" couldn't be set."), viewerName.c_str());
      return false;
   }

   if ( m_viewerOld )
   {
      // replace the last automatically set viewer
      SetViewer(viewer, viewerName);
   }
   else // we're using the original user-chosen viewer, keep it as "old"
   {
      // we can't use SetViewer() because it deletes the current viewer
      // and we want to keep it as m_viewerOld and restore it later
      m_viewerNameOld = m_viewerName;
      m_viewerOld = m_viewer;
      DoSetViewer(viewer, viewerName);
   }

   return true;
}

bool MessageView::ChangeViewer(const String& viewerName)
{
   if ( !ChangeViewerWithoutUpdate(viewerName) )
      return false;

   // redisplay the message in the new viewer
   DisplayMessageInViewer();

   return true;
}

// ----------------------------------------------------------------------------
// showing a new message
// ----------------------------------------------------------------------------

void
MessageView::Update()
{
   if ( m_mailMessage )
   {
      m_uid = m_mailMessage->GetUId();

      const MimePart *mimepart = m_mailMessage->GetTopMimePart();
      CHECK_RET( mimepart, _T("No MIME part to show?") );

      // use the encoding of the first body part as the default encoding for
      // the headers - this cares for the (horribly broken) mailers which send
      // 8 bit stuff in the headers instead of using RFC 2047
      m_encodingAuto = mimepart->GetTextEncoding();

      // adjust the viewer to the current message contents
      if ( m_ProfileValues.autoViewer )
         AutoAdjustViewer(mimepart);
      else
         RestoreOldViewer();
   }
   else // no message
   {
      m_uid = UID_ILLEGAL;
      m_encodingAuto = wxFONTENCODING_SYSTEM;
   }

   // do display it
   DisplayMessageInViewer();


   if ( m_mailMessage )
   {
      // if user selects the language from the menu, m_encodingUser is set
      wxFontEncoding encoding;
      if ( m_encodingUser != wxFONTENCODING_DEFAULT )
      {
         encoding = m_encodingUser;
      }
      else if ( m_encodingAuto != wxFONTENCODING_SYSTEM )
      {
         encoding = m_encodingAuto;
      }
      else
      {
         encoding = wxFONTENCODING_DEFAULT;
      }

      // update the menu of the frame containing us to show the encoding used
      CheckLanguageInMenu(GetParentFrame(), encoding);
   }
}

void
MessageView::DisplayMessageInViewer()
{
   if ( m_virtualMimeParts )
      m_virtualMimeParts->clear();

   m_textBody.clear();

   ClearMIMEPartDataStore();

   m_viewer->Clear();

   if ( !m_mailMessage )
   {
      // no message to display, but still call Update() after Clear()
      m_viewer->Update();
      return;
   }

   //#define PROFILE_VIEWER
#ifdef PROFILE_VIEWER
   wxStopWatch timeViewer;
#endif

   // show the headers first
   ShowHeaders();

   // then the body ...
   m_viewer->StartBody();

   // ... by recursively showing all the parts
   ProcessPart(m_mailMessage->GetTopMimePart());

   m_viewer->EndBody();

#ifdef PROFILE_VIEWER
   wxLogStatus(_T("Message shown in %ldms"), timeViewer.Time());
#endif
}

// ----------------------------------------------------------------------------
// MIME attachments menu commands
// ----------------------------------------------------------------------------

// show information about an attachment
void
MessageView::MimeInfo(const MimePart *mimepart)
{
   MimeType type = mimepart->GetType();

   String message;
   message << _("MIME type: ") << type.GetFull() << '\n';

   String desc = mimepart->GetDescription();
   if ( !desc.empty() )
      message << '\n' << _("Description: ") << desc << '\n';

   message << _("Size: ");
   size_t lines;
   if ( type.IsText() && (lines = mimepart->GetNumberOfLines()) != 0 )
   {
      message << strutil_ltoa(lines) << _(" lines");
   }
   else
   {
      message << strutil_ltoa(mimepart->GetSize()) << _(" bytes");
   }

   message << '\n';

   // param name and value (used in 2 loops below)
   wxString name, value;

   // debug output with all parameters
   const MessageParameterList &plist = mimepart->GetParameters();
   MessageParameterList::iterator plist_it;
   if ( !plist.empty() )
   {
      message += _("\nParameters:\n");
      for ( plist_it = plist.begin(); plist_it != plist.end(); plist_it++ )
      {
         name = plist_it->name;
         message << NormalizeString(name) << _T(": ");

         // filenames are case-sensitive, don't modify them
         value = plist_it->value;
         if ( name.CmpNoCase(_T("name")) != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   // now output disposition parameters too
   String disposition = mimepart->GetDisposition();
   const MessageParameterList& dlist = mimepart->GetDispositionParameters();

   if ( !strutil_isempty(disposition) )
      message << _("\nDisposition: ") << disposition.Lower() << '\n';

   if ( !dlist.empty() )
   {
      message += _("\nDisposition parameters:\n");
      for ( plist_it = dlist.begin(); plist_it != dlist.end(); plist_it++ )
      {
         name = plist_it->name;
         message << NormalizeString(name) << _T(": ");

         value = plist_it->value;
         if ( name.CmpNoCase(_T("filename")) != 0 )
         {
            value.MakeLower();
         }

         message << value << '\n';
      }
   }

   String title;
   title << _("Attachment #") << mimepart->GetPartSpec();

   MDialog_Message(message, GetParentFrame(), title);
}

// open as an embedded mail message
void MessageView::MimeOpenAsMessage(const MimePart *mimepart)
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
   ...
   msg->DecRef();
#else // 1
   bool ok = false;
   wxString filename;
   if ( wxGetTempFileName(_T("Mtemp"), filename) )
   {
      if ( MimeSaveAsMessage(mimepart, filename) )
      {
         wxString name;
         name.Printf(_("Attached message '%s'"),
                     mimepart->GetFilename().c_str());

         MFolder_obj mfolder(MFolder::CreateTempFile(name, filename));

         if ( mfolder )
         {
            ASMailFolder *asmf = ASMailFolder::OpenFolder(mfolder);
            if ( asmf )
            {
               // FIXME: assume UID of the first message in a new MBX folder
               //        is always 1
               ShowMessageViewFrame(GetParentFrame(), asmf, 1);

               ok = true;

               asmf->DecRef();
            }
         }
         else
         {
            // note that if we succeeded with folder creation, it will
            // delete the file itself (because of MF_FLAGS_TEMPORARY)
            wxRemoveFile(filename);
         }
      }
   }

   if ( !ok )
   {
      wxLogError(_("Failed to open attached message."));
   }
#endif // 0/1
}

void MessageView::MimeDoOpen(const String& command, const String& filename)
{
   if ( command.empty() )
   {
      wxLogWarning(_("No command to open file \"%s\"."), filename.c_str());
      return;
   }

   // see HandleProcessTermination() for the explanation of "possibly"
   wxString errmsg;
   errmsg.Printf(_("External viewer \"%s\" possibly failed"), command.c_str());
   (void)LaunchProcess(command, errmsg, filename);
}

// open (execute) a message attachment
void
MessageView::MimeHandle(const MimePart *mimepart)
{
   // we'll need this filename later
   wxString filenameOrig = mimepart->GetFilename();

   MimeType type = mimepart->GetType();

   String mimetype = type.GetFull();
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   if ( wxMimeTypesManager::IsOfType(mimetype, _T("APPLICATION/OCTET-STREAM")) )
   {
      // special handling of "APPLICATION/OCTET-STREAM": this is the default
      // MIME type for all binary attachments and many e-mail clients don't
      // use the correct type (e.g. IMAGE/GIF) always leaving this one as
      // default. Try to guess a better MIME type ourselves from the file
      // extension.
      if ( !filenameOrig.empty() )
      {
         wxString ext;
         wxSplitPath(filenameOrig, NULL, NULL, &ext);

         if ( !ext.empty() )
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
   if ( wxMimeTypesManager::IsOfType(mimetype, _T("MESSAGE/*")) )
   {
      MimeOpenAsMessage(mimepart);

      return;
   }

   String filename;
   if ( !wxGetTempFileName(_T("Mtemp"), filename) )
   {
      wxLogError(_("Failed to open the attachment."));

      delete fileType;
      return;
   }

   wxString ext;
   wxSplitPath(filenameOrig, NULL, NULL, &ext);

   // get the standard extension for such files if there is no real one
   if ( fileType != NULL && ext.empty() )
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
         wxLogDebug(_T("Warning: stale temp file '%s' will be left."),
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      if ( !ext.empty() )
      {
         // normally the extension should always already have the leading dot
         // but check for it just in case
         if ( ext[0u] != wxFILE_SEP_EXT )
            filename += wxFILE_SEP_EXT;

         filename += ext;
      }
   }

   MailMessageParameters params(filename, mimetype, mimepart);

   // have we already saved the file to disk?
   bool saved = false;

#ifdef OS_UNIX
   Profile * const profile = GetProfile();

   /* For IMAGE/TIFF content, check whether it comes from one of the
      fax domains. If so, change the mimetype to "IMAGE/TIFF-G3" and
      proceed in the usual fashion. This allows the use of a special
      image/tiff-g3 mailcap entry. */
   if ( READ_CONFIG(profile,MP_INCFAX_SUPPORT) &&
        (wxMimeTypesManager::IsOfType(mimetype, _T("IMAGE/TIFF"))
         || wxMimeTypesManager::IsOfType(mimetype, _T("APPLICATION/OCTET-STREAM"))))
   {
      const wxArrayString faxdomains(
            strutil_restore_array(READ_CONFIG(profile, MP_INCFAX_DOMAINS)));

      bool isfax = false;
      wxString fromline = m_mailMessage->From();
      strutil_tolower(fromline);

      for( wxArrayString::const_iterator i = faxdomains.begin();
           i != faxdomains.end();
           ++i )
      {
         wxString domain = *i;
         strutil_tolower(domain);
         if ( fromline.Find(domain) != -1 )
            isfax = true;
      }

      if(isfax
         && MimeSave(mimepart, filename))
      {
         wxLogDebug(_T("Detected image/tiff fax content."));
         // use TIFF2PS command to create a postscript file, open that
         // one with the usual ps viewer
         String filenamePS = filename.BeforeLast('.') + _T(".ps");
         String command;
         command.Printf(READ_CONFIG_TEXT(profile,MP_TIFF2PS),
                        filename.c_str(), filenamePS.c_str());
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
         filename = filenamePS;
         mimetype = _T("application/postscript");
         if(fileType) delete fileType;
         fileType = mimeManager.GetFileTypeFromMimeType(mimetype);

         // proceed as usual
         MailMessageParameters new_params(filename, mimetype, mimepart);
         params = new_params;
         saved = true; // use this file instead!
      }
   }
#endif // Unix

   // We must save the file before actually calling GetOpenCommand()
   if( !saved )
   {
      if ( !MimeSave(mimepart, filename) )
      {
         wxLogError(_("Failed to open the attachment."));

         delete fileType;
         return;
      }

      saved = TRUE;
   }

   String command;
   if ( (fileType == NULL) || !fileType->GetOpenCommand(&command, params) )
   {
      // unknown MIME type, ask the user for the command to use
      MimeOpenWith(mimepart);
   }
   else // got the command to use
   {
      MimeDoOpen(command, filename);
   }

   delete fileType;
}

void
MessageView::MimeOpenWith(const MimePart *mimepart)
{
   // we'll need this filename later
   wxString filenameOrig = mimepart->GetFilename();

   MimeType type = mimepart->GetType();

   String mimetype = type.GetFull();
   wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();

   wxFileType *fileType = NULL;
   fileType = mimeManager.GetFileTypeFromMimeType(mimetype);

   String filename = wxGetTempFileName(_T("Mtemp"));

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
         wxLogDebug(_T("Warning: stale temp file '%s' will be left."),
                    filename.c_str());
      }

      filename = path + wxFILE_SEP_PATH + name;
      filename << '.' << ext;
   }

   // get the command to open this attachment
   bool openAsMsg = false;
   String command = GetCommandForMimeType(GetWindow(), mimetype, &openAsMsg);
   if ( command.empty() )
   {
      // cancelled by user
      return;
   }

   if ( openAsMsg )
   {
      MimeOpenAsMessage(mimepart);
   }
   else // open using external command
   {
      if ( MimeSave(mimepart, filename) )
      {
         // expand the command and use it
         MailMessageParameters params(filename, mimetype, mimepart);
         command = wxFileType::ExpandCommand(command, params);

         MimeDoOpen(command, filename);
      }
   }
}

bool
MessageView::MimeSaveAsMessage(const MimePart *mimepart, const wxChar *filename)
{
   unsigned long len;
   const void *content = mimepart->GetContent(&len);
   if ( !content )
   {
      wxLogError(_("Cannot get embedded message content."));

      return false;
   }

   if ( !MailFolder::SaveMessageAsMBOX(filename, content, len) )
   {
      wxLogError(_("Failed to save embedded message to a temporary file."));

      return false;
   }

   return true;
}

bool
MessageView::MimeSave(const MimePart *mimepart,const wxChar *ifilename)
{
   String filename;

   if ( strutil_isempty(ifilename) )
   {
      filename = mimepart->GetFilename();

      wxString path, name, ext;
      wxSplitPath(filename, &path, &name, &ext);

      filename = wxPSaveFileSelector
                 (
                     GetParentFrame(),
                     "MimeSave",
                     _("Save attachment as:"),
                     NULL /* no default path */, name, ext,
                     NULL /* default filters */,
                     wxFILEDLG_USE_FILENAME
                 );
   }
   else
      filename = ifilename;

   if ( !filename )
   {
      // no filename and user cancelled the dialog
      return false;
   }

   if ( mimepart->GetType().GetPrimary() == MimeType::MESSAGE )
   {
      // saving the messages is special, we have a separate function for
      // this as it's also done from elsewhere
      return MimeSaveAsMessage(mimepart, filename);
   }

   unsigned long len;
   const void *content = mimepart->GetContent(&len);
   if ( !content )
   {
      wxLogError(_("Cannot get attachment content."));
   }
   else
   {
      wxFile out(filename, wxFile::write);
      bool ok = out.IsOpened();

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
            wxLogStatus(GetParentFrame(), _("Wrote %lu bytes to file '%s'"),
                        len, filename.c_str());
         }

         return true;
      }
   }

   wxLogError(_("Could not save the attachment."));

   return false;
}

void
MessageView::MimeViewText(const MimePart *mimepart)
{
   const String content = mimepart->GetTextContent();
   if ( !content.empty() )
   {
      String title;
      title << _("Attachment #") << mimepart->GetPartSpec();

      // add the filename if any
      String filename = mimepart->GetFilename();
      if ( !filename.empty() )
      {
         title << _T(" ('") << filename << _T("')");
      }

      MDialog_ShowText(GetParentFrame(), title, content, _T("MimeView"));
   }
   else
   {
      wxLogError(_("Failed to view the attachment."));
   }
}

// ----------------------------------------------------------------------------
// MessageView menu command processing
// ----------------------------------------------------------------------------

bool
MessageView::DoMenuCommand(int id)
{
   CHECK( GetFolder(), false, _T("no folder in message view?") );

   Profile *profile = GetProfile();

   switch ( id )
   {
      case WXMENU_EDIT_COPY:
         m_viewer->Copy();
         break;

      case WXMENU_EDIT_SELECT_ALL:
         m_viewer->SelectAll();
         break;

      case WXMENU_EDIT_FIND:
         if ( m_uid != UID_ILLEGAL )
         {
            String text;
            if ( MInputBox(&text,
                           _("Find text"),
                           _("   Find:"),
                           GetParentFrame(),
                           _T("MsgViewFindString")) )
            {
               if ( !m_viewer->Find(text) )
               {
                  wxLogStatus(GetParentFrame(),
                              _("'%s' not found"),
                              text.c_str());
               }
            }
         }
         break;

      case WXMENU_EDIT_FINDAGAIN:
         if ( m_uid != UID_ILLEGAL && !m_viewer->FindAgain() )
         {
            wxLogStatus(GetParentFrame(), _("No more matches"));
         }
         break;

      case WXMENU_HELP_CONTEXT:
         mApplication->Help(MH_MESSAGE_VIEW, GetParentFrame());
         break;

      case WXMENU_MSG_TOGGLEHEADERS:
         if ( m_uid != UID_ILLEGAL )
         {
            m_ProfileValues.showHeaders = !m_ProfileValues.showHeaders;
            profile->writeEntry(MP_SHOWHEADERS, m_ProfileValues.showHeaders);
            OnShowHeadersChange();
            Update();
         }
         break;

      default:
         if ( WXMENU_CONTAINS(LANG, id) && (id != WXMENU_LANG_SET_DEFAULT) )
         {
            SetLanguage(id);
         }
         else if ( WXMENU_CONTAINS(VIEW_FILTERS, id) )
         {
            OnToggleViewFilter(id);
         }
         else if ( WXMENU_CONTAINS(VIEW_VIEWERS, id) )
         {
            OnSelectViewer(id);
         }
         else
         {
            // not handled
            return false;
         }
   }

   // message handled
   return true;
}

void
MessageView::DoMouseCommand(int id, const ClickableInfo *ci, const wxPoint& pt)
{
   // ignore mouse clicks if we're inside wxYield()
   if ( !mApplication->AllowBgProcessing() )
      return;

   CHECK_RET( ci, _T("MessageView::DoMouseCommand(): NULL ClickableInfo") );

   switch ( id )
   {
      case WXMENU_LAYOUT_LCLICK:
         ci->OnLeftClick(pt);
         break;

      case WXMENU_LAYOUT_RCLICK:
         ci->OnRightClick(pt);
         break;

      case WXMENU_LAYOUT_DBLCLICK:
         ci->OnDoubleClick(pt);
         break;

      default:
         FAIL_MSG(_T("unknown mouse action"));
   }
}

void
MessageView::SetLanguage(int id)
{
   wxFontEncoding encoding = GetEncodingFromMenuCommand(id);
   SetEncoding(encoding);
}

void
MessageView::SetEncoding(wxFontEncoding encoding)
{
   // use wxFONTENCODING_DEFAULT instead!
   ASSERT_MSG( encoding != wxFONTENCODING_SYSTEM,
               _T("invalid encoding in MessageView::SetEncoding()") );

   m_encodingUser = encoding;

   Update();
}

void MessageView::ResetUserEncoding()
{
   // if the user had manually set the encoding for the old message, we
   // revert back to the default encoding for the new message (otherwise it is
   // already wxFONTENCODING_DEFAULT and the line below does nothing at all)
   m_encodingUser = wxFONTENCODING_DEFAULT;
}

// ----------------------------------------------------------------------------
// MessageView selecting the shown message
// ----------------------------------------------------------------------------

void
MessageView::SetFolder(ASMailFolder *asmf)
{
   if ( asmf == m_asyncFolder )
      return;

   if ( m_asyncFolder )
      m_asyncFolder->DecRef();

   m_asyncFolder = asmf;

   if ( m_asyncFolder )
      m_asyncFolder->IncRef();

   if ( m_asyncFolder )
   {
      // use the settings for this folder now
      if ( m_profile )
         m_profile->DecRef();

      m_profile = m_asyncFolder->GetProfile();

      if ( m_profile )
         m_profile->IncRef();

      UpdateProfileValues();
   }
   else // no folder
   {
      // forget the viewer to restore: it won't make sense for the new folder
      if ( m_viewerOld )
      {
         delete m_viewerOld;
         m_viewerOld = NULL;
         m_viewerNameOld.clear();
      }

      // revert to the default viewer if we don't have any folder any more
      ResetViewer();

      // make sure the viewer will be recreated the next time we are called
      // with a valid folder - if we didn't do it, the UpdateProfileValues()
      // wouldn't do anything if the same folder was reopened, for example
      m_ProfileValues.msgViewer.clear();
   }

   ResetUserEncoding();
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
      ResetUserEncoding();

      // file request, our DoShowMessage() will be called later
      m_uid = uid;
      (void)m_asyncFolder->GetMessage(uid, this);

      // Reset the status bar, so that some information pertaining
      // to the previous message does not stay there...
      wxFrame *frame = GetParentFrame();
      CHECK_RET( frame, _T("message view without parent frame?") );
      frame->SetStatusText(wxEmptyString);
   }
}

bool
MessageView::CheckMessageOrPartSize(unsigned long size, bool part) const
{
   unsigned long maxSize = (unsigned long)READ_CONFIG(GetProfile(),
                                                      MP_MAX_MESSAGE_SIZE);

   // translate to Kb before comparing with MP_MAX_MESSAGE_SIZE which is in Kb
   size /= 1024;
   if ( size <= maxSize )
   {
      // it's ok, don't ask
      return true;
   }
   //else: big message, ask

   wxString msg;
   msg.Printf(_("The selected message%s is %u Kbytes long which is "
                "more than the current threshold of %d Kbytes.\n"
                "\n"
                "Do you still want to download it?"),
              part ? _(" part") : "", size, maxSize);

   return MDialog_YesNoDialog(msg, GetParentFrame());
}

bool
MessageView::CheckMessagePartSize(const MimePart *mimepart) const
{
   // only check for IMAP here: for POP/NNTP we had already checked it in
   // CheckMessageSize() below and the local folders are fast
   return (m_asyncFolder->GetType() != MF_IMAP) ||
               CheckMessageOrPartSize(mimepart->GetSize(), true);
}

bool
MessageView::CheckMessageSize(const Message *message) const
{
   // we check the size here only for POP3 and NNTP because the local folders
   // are supposed to be fast and for IMAP we don't retrieve the whole message
   // at once so we'd better ask only if we're going to download it
   //
   // for example, if the message has text/plain part of 100 bytes and
   // video/mpeg one of 2Mb, we don't want to ask the user before downloading
   // 100 bytes (and then, second time, before downloading 2b!)
   MFolderType folderType = m_asyncFolder->GetType();
   return (folderType != MF_POP && folderType != MF_NNTP) ||
               CheckMessageOrPartSize(message->GetSize(), false);
}

void
MessageView::DoShowMessage(Message *mailMessage)
{
   CHECK_RET( mailMessage, _T("no message to show in MessageView") );
   CHECK_RET( m_asyncFolder, _T("no folder in MessageView::DoShowMessage()") );

   if ( !CheckMessageSize(mailMessage) )
   {
      // too big, don't show it
      return;
   }

   mailMessage->IncRef();

   // ok, make this our new current message
   SafeDecRef(m_mailMessage);

   m_mailMessage = mailMessage;
   m_uid = mailMessage->GetUId();

   // have we not seen the message before?
   if ( !(m_mailMessage->GetStatus() & MailFolder::MSG_STAT_SEEN) )
   {
      MailFolder *mf = m_mailMessage->GetFolder();
      CHECK_RET( mf, _T("mail message without associated folder?") );

      // mark it as seen if we can
      if ( mf->CanSetFlag(MailFolder::MSG_STAT_SEEN) )
      {
         mf->SetMessageFlag(m_uid, MailFolder::MSG_STAT_SEEN, true);
      }
      //else: read only folder

      // autocollect the addresses from it if configured
      if ( m_ProfileValues.autocollect != M_ACTION_NEVER )
      {
         AutoCollectAddresses(m_mailMessage,
                              m_ProfileValues.autocollect,
                              m_ProfileValues.autocollectSenderOnly != 0,
                              m_ProfileValues.autocollectNamed != 0,
                              m_ProfileValues.autocollectBookName,
                              GetFolderName(),
                              GetParentFrame());
      }
   }

   Update();
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

String MessageView::GetText() const
{
   // return selection if we have any and if the option to reply to selected
   // text hasn't been disabled by the user
   String text;
   if ( READ_CONFIG(GetProfile(), MP_REPLY_QUOTE_SELECTION) )
   {
      text = m_viewer->GetSelection();
   }

   // test for m_textBody is important to avoid problems in the code below
   if ( !text.empty() || m_textBody.empty() )
      return text;

   // trim trailing empty lines, it is annoying to have to delete them manually
   // when replying
   wxString::const_iterator p = m_textBody.end() - 1;
   while ( *p == _T('\r') || *p == _T('\n') )
      p--;

   const_cast<String &>(m_textBody).erase(p - m_textBody.begin() + 1);

   return m_textBody;
}

// ----------------------------------------------------------------------------
// printing
// ----------------------------------------------------------------------------

bool
MessageView::Print()
{
   return m_viewer->Print();
}

void
MessageView::PrintPreview()
{
   m_viewer->PrintPreview();
}

// ----------------------------------------------------------------------------
// external processes
// ----------------------------------------------------------------------------

bool
MessageView::RunProcess(const String& command)
{
   wxLogStatus(GetParentFrame(),
               _("Calling external viewer '%s'"),
               command.c_str());
   return wxExecute(command, true) == 0;
}

ProcessEvtHandler *
MessageView::GetEventHandlerForProcess()
{
   if ( !m_evtHandlerProc )
   {
      m_evtHandlerProc = new ProcessEvtHandler(this);
   }

   return m_evtHandlerProc;
}

bool
MessageView::LaunchProcess(const String& command,
                           const String& errormsg,
                           const String& filename)
{
   wxLogStatus(GetParentFrame(),
               _("Calling external viewer '%s'"),
               command.c_str());

   wxProcess *process = new wxProcess(GetEventHandlerForProcess());
   int pid = wxExecute(command, false, process);
   if ( !pid )
   {
      delete process;

      if ( !errormsg.empty() )
         wxLogError(_T("%s."), errormsg.c_str());

      return false;
   }

   if ( pid != -1 )
   {
      ProcessInfo *procInfo = new ProcessInfo(process, pid, errormsg);

      m_processes.Add(procInfo);
      m_tempFiles.Add(filename);
   }

   return true;
}

void
MessageView::HandleProcessTermination(int pid, int exitcode)
{
   // find the corresponding entry in m_processes
   size_t n,
          procCount = m_processes.GetCount();
   for ( n = 0; n < procCount; n++ )
   {
      if ( m_processes[n]->GetPid() == pid )
         break;
   }

   CHECK_RET( n != procCount, _T("unknown process terminated!") );

   ProcessInfo *info = m_processes[n];
   if ( exitcode != 0 )
   {
      // don't make this a wxLogError because too many Windows apps (most
      // annoyingly, Acrobat Reader which is used very often to view PDFs) do
      // exit with non zero exit code if they simply forward the file via DDE
      // to another (already running) instance instead of showing it themselves
      //
      // so just warn the user about it...
      wxLogStatus(GetParentFrame(),
                  _("%s (non null exit code %d)"),
                  info->GetErrorMsg().c_str(),
                  exitcode);
   }

   m_processes.RemoveAt(n);
   delete info;
}

void MessageView::DetachAllProcesses()
{
   size_t n;

   // delete all process info objects, we don't need notifications about
   // process termination any more
   const size_t procCount = m_processes.GetCount();
   for ( n = 0; n < procCount; n++ )
   {
      ProcessInfo *info = m_processes[n];
      info->Detach();

      delete info;
   }

   m_processes.Empty();


   // also delete the temporary files used by them
   bool closedAll = true;
   const size_t tmpCount = m_tempFiles.GetCount();
   for ( n = 0; n < tmpCount; n++ )
   {
      const wxString& fn = m_tempFiles[n];
      if ( wxRemove(fn) != 0 )
      {
         wxLogSysError(_("Failed to delete file \"%s\""), fn.c_str());
         closedAll = false;
      }
   }

   if ( !closedAll )
   {
      wxLogWarning(_("Some of the temporary files couldn't have been removed."));
   }

   m_tempFiles.Empty();
}

// ----------------------------------------------------------------------------
// MessageView scrolling
// ----------------------------------------------------------------------------

/// scroll down one line
bool
MessageView::LineDown()
{
   return m_viewer->LineDown();
}

/// scroll up one line:
bool
MessageView::LineUp()
{
   return m_viewer->LineUp();
}

/// scroll down one page:
bool
MessageView::PageDown()
{
   return m_viewer->PageDown();
}

/// scroll up one page:
bool
MessageView::PageUp()
{
   return m_viewer->PageUp();
}

