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

#  include "Mcclient.h"          // only for rfc822_base64()

#  include <wx/filedlg.h>
#endif //USE_PCH

#include "gui/wxOptionsDlg.h"
#include "MTextStyle.h"
#include "MModule.h"

#include "MessageView.h"
#include "MessageViewer.h"
#include "ViewFilter.h"

#include "mail/MimeDecode.h"
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
#include <wx/mstream.h>
#include <wx/scopeguard.h>

#ifdef OS_UNIX
   #include <sys/stat.h>

   #include <wx/dcps.h> // for wxThePrintSetupData
#endif

M_LIST_OWN(VirtualMimePartsList, MimePart);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

static const char *FACE_HEADER = "Face";
static const char *XFACE_HEADER = "X-Face";

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
extern const MOption MP_RFC822_DECORATE;
extern const MOption MP_RFC822_SHOW_HEADERS;
extern const MOption MP_SHOWHEADERS;
extern const MOption MP_SHOW_XFACES;
extern const MOption MP_TIFF2PS;

// ----------------------------------------------------------------------------
// persistent msgboxes we use here
// ----------------------------------------------------------------------------

extern const MPersMsgBox *M_MSGBOX_GFX_NOT_INLINED;

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

// recode the contents of a string in the specified destination encoding,
// assuming that the current contents was obtained from the source encoding
#if wxUSE_UNICODE

static void
RecodeText(String *text, wxFontEncoding encSrc, wxFontEncoding encDst)
{
   // recode the text to the user-specified encoding
   const wxCharBuffer textMB(text->mb_str(wxCSConv(encSrc)));
   CHECK_RET( textMB, "string not in source encoding?" );

   *text = MIME::DecodeText(textMB, textMB.length(), encDst);
}

#else // !wxUSE_UNICODE

static inline void RecodeText(String *, wxFontEncoding, wxFontEncoding)
{
   // do nothing in ANSI build: the string always contains the same bytes,
   // independently of the encoding being used
}

#endif // wxUSE_UNICODE/!wxUSE_UNICODE

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Information present in headers which needs to be shown in some special way
struct ViewableInfoFromHeaders
{
   // the contents of Face header if non-empty
   wxString face;

#ifdef HAVE_XFACES
   // the contents of X-Face header if non-empty
   wxString xface;
#endif // HAVE_XFACES
};

// Data about a process (external viewer typically) we launched: these objects
// are created by LaunchProcess() and deleted when the viewer process
// terminates. If it terminates with non 0 exit code, errormsg is given to the
// user.
class ProcessInfo
{
public:
   ProcessInfo(wxProcess *process,
               int pid,
               const String& errormsg,
               const String& filename)
      : m_errormsg(errormsg),
        m_filename(filename)
   {
      ASSERT_MSG( process, "must have valid process in ProcessInfo ctor" );
      ASSERT_MSG( pid, "must have valid PID in ProcessInfo ctor" );
      ASSERT_MSG( !filename.empty(),
                  "must have non empty file name in ProcessInfo ctor" );

      m_process = process;
      m_pid = pid;
   }

   /**
       Remove the temporary file used by the corresponding child process.

       This is normally called by dtor but can be called explicitly before
       destroying the object to check the return value which will be false only
       if we failed to delete the file.
    */
   bool RemoveTempFile()
   {
      bool rc = true;
      if ( !m_filename.empty() && wxRemove(m_filename) != 0 )
      {
         wxLogSysError(_("Failed to delete temporary file \"%s\""),
                       m_filename);
         rc = false;
      }

      // In any case, don't try to delete it again from dtor.
      m_filename.clear();

      return rc;
   }

   ~ProcessInfo()
   {
      RemoveTempFile();

      delete m_process;
   }

   // get the pid of our process
   int GetPid() const { return m_pid; }

   // get the error message
   const String& GetErrorMsg() const { return m_errormsg; }

private:
   String         m_errormsg; // error message to give if launch failed
   String         m_filename; // temporary file to delete after child exit
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
      return GetProtocol(location) == "cid";
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
size_t GetAllAvailablePlugins(const char *iface,
                              wxArrayString *names,
                              wxArrayString *descs,
                              wxArrayInt *states = NULL)
{
   CHECK( names && descs, 0, "NULL pointer in GetAllAvailablePlugins()" );

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
            FAIL_MSG( "failed to create ViewFilterFactory" );

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
   inlineEmbedded =
   decorateEmbedded =
   showEmbeddedHeaders =
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
          CMP(showHeaders) &&
          CMP(inlineEmbedded) && (!inlineEmbedded ||
            (CMP(decorateEmbedded) && CMP(showEmbeddedHeaders))) &&
          CMP(inlinePlainText) &&
          CMP(inlineGFX) && CMP(showExtImages) &&
          CMP(highlightURLs) && (!highlightURLs || CMP(UrlCol)) &&
          // even if these fields are different, they don't change our
          // appearance, so ignore them for the purpose of this comparison
#if 0
          CMP(autocollect) && CMP(autocollectSenderOnly) &&
          CMP (autocollectNamed) CMP(autocollectBookName) &&
#endif // 0
#ifdef HAVE_XFACES
          CMP(showFaces) &&
#endif // HAVE_XFACES
          CMP(autoViewer) &&
          (!autoViewer || (CMP(preferHTML) && CMP(allowHTML) && CMP(allowImages)));

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
   m_nullFilter = NULL;
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

      ASSERT_MSG( viewer, "must have default viewer, will crash without it!" );

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
                   name, nameAlt);

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
   CHECK_RET( !m_filters, "InitializeViewFilters() called twice?" );

   // always insert the terminating, "do nothing", filter at the end
   m_nullFilter = new TransparentFilter(this);
   m_filters = new ViewFilterNode
                   (
                     m_nullFilter,
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
                              (int)prio, name ));

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
   CHECK( cookie, false, "NULL cookie in GetFirstFilter" );

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
          "NULL parameter in GetNextFilter" );

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
   CHECK( m_mailMessage->GetFolder(), "unknown", "no folder in MessageView?" );

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
   ASSERT_MSG( m_regCookieOptionsChange, "can't register for options change event");
   m_regCookieASFolderResult = MEventManager::Register(*this, MEventId_ASFolderResult);
   ASSERT_MSG( m_regCookieASFolderResult, "can't reg folder view with event manager");
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
         FAIL_MSG("unknown options change event");
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
            FAIL_MSG("Unexpected async result event");
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
   CHECK_RET( profile, "MessageView::ReadAllSettings: no profile" );

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
   settings->inlineEmbedded = READ_CONFIG_BOOL(profile, MP_RFC822_IS_TEXT);
   settings->decorateEmbedded =
      READ_CONFIG_BOOL(profile, MP_RFC822_DECORATE);
   settings->showEmbeddedHeaders =
      READ_CONFIG_BOOL(profile, MP_RFC822_SHOW_HEADERS);

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
#ifdef HAVE_XFACES
   settings->showFaces = READ_CONFIG_BOOL(profile, MP_SHOW_XFACES);
#endif // HAVE_XFACES

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
   // show the header name -- this is simple as it's always ASCII
   m_viewer->ShowHeaderName(name);


   // next deal with the encoding to use for the header value (and as using
   // correct encoding can involve re-encoding the text, make a copy of it)
   String value(valueOrig);
   if ( encHeader == wxFONTENCODING_SYSTEM )
   {
      if ( m_encodingUser != wxFONTENCODING_DEFAULT )
      {
         // use the user specified encoding if none specified in the header
         // itself
         encHeader = m_encodingUser;

         RecodeText(&value, wxFONTENCODING_ISO8859_1, m_encodingUser);
      }
      else if ( m_encodingAuto != wxFONTENCODING_SYSTEM )
      {
         encHeader = m_encodingAuto;
      }
   }

#if !wxUSE_UNICODE
   // special handling for the UTF-7|8 if it's not supported natively
   if ( encHeader == wxFONTENCODING_UTF8 ||
         encHeader == wxFONTENCODING_UTF7 )
   {
      encHeader = ConvertUTFToMB(&value, encHeader);
   }

   if ( encHeader != wxFONTENCODING_SYSTEM )
   {
      // convert the string to an encoding we can show, if needed
      EnsureAvailableTextEncoding(&encHeader, &value);
   }
#endif // !wxUSE_UNICODE


   // don't highlight URLs in headers which contain message IDs or other things
   // which look just like the URLs but, in fact, are not ones
   bool highlightURLs = m_ProfileValues.highlightURLs;
   String nameUpper(name.Upper());
   if ( highlightURLs )
   {
      if ( nameUpper == "IN-REPLY-TO" ||
            nameUpper == "REFERENCES" ||
             nameUpper == "CONTENT-ID" ||
              nameUpper == "MESSAGE-ID" ||
               nameUpper == "RESENT-MESSAGE-ID" )
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
         if ( *url.c_str() == '<' &&
               (nameUpper == "FROM" ||
                 nameUpper == "TO" ||
                  nameUpper == "CC" ||
                   nameUpper == "BCC" ||
                    nameUpper == "RESENT-FROM" ||
                     nameUpper == "RESENT-TO") )
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
                  case '"':
                     inQuotes = !inQuotes;
                     break;

                  case ',':
                  case ';':
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
      if ( !before.empty() || encHeader != wxFONTENCODING_SYSTEM )
      {
         m_viewer->ShowHeaderValue(before, encHeader);
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
MessageView::ShowAllHeaders(ViewableInfoFromHeaders *vi)
{
   HeaderIterator headers(m_mailMessage->GetHeader());

   String name,
          value;
   wxFontEncoding enc;
   while ( headers.GetNextDecoded(&name, &value, &enc,
                                  HeaderIterator::MultiLineOk) )
   {
      if ( m_ProfileValues.showFaces )
      {
         if ( wxStricmp(name, FACE_HEADER) == 0 )
            vi->face = value;
#ifdef HAVE_XFACES
         else if ( wxStricmp(name, XFACE_HEADER) == 0 )
            vi->xface = value;
#endif // HAVE_XFACES
      }

      ShowHeader(name, value, enc);
   }
}

void
MessageView::ShowMatchingHeaders(HeaderIterator headerIterator,
                                 const wxArrayString& headersToShow,
                                 ViewableInfoFromHeaders *vi)
{
   wxArrayString headerNames,
                 headerValues;
   wxArrayInt headerEncodings;
   size_t countHeaders = headerIterator.GetAllDecoded
                         (
                           &headerNames,
                           &headerValues,
                           &headerEncodings,
                           HeaderIterator::DuplicatesOk |
                           HeaderIterator::MultiLineOk
                         );

   const size_t countToShow = headersToShow.size();
   for ( size_t n = 0; n < countToShow; n++ )
   {
      const String& hdr = headersToShow[n];

      // as we use HeaderIterator::DuplicatesOk above we can have multiple
      // occurrences of this header so loop until we find all of them
      for ( ;; )
      {
         int pos = wxNOT_FOUND;
         if ( hdr.find_first_of("*?") == String::npos )
         {
            pos = headerNames.Index(hdr, false /* case-insensitive */);
         }
         else // this header is a wildcard
         {
            for ( size_t m = 0; m < countHeaders; m++ )
            {
               if ( headerNames[m].Matches(hdr) )
               {
                  pos = m;
                  break;
               }
            }
         }

         if ( pos == wxNOT_FOUND )
            break;

         ShowHeader(headerNames[pos],
                    headerValues[pos],
                    static_cast<wxFontEncoding>(headerEncodings[pos]));

         // we shouldn't show the same header more than once and this could
         // happen when using the wildcards (e.g. if we show "X-Foo" and "X-*"
         // headers and didn't remove "X-Foo" after showing it, it would be
         // also found during the second pass)
         headerNames.RemoveAt(pos);
         headerValues.RemoveAt(pos);
         headerEncodings.RemoveAt(pos);
         countHeaders--;
      }
   }
}

void
MessageView::ShowSelectedHeaders(const wxArrayString& headersUser_,
                                 ViewableInfoFromHeaders *vi)
{
   // retrieve all headers at once instead of calling Message::GetHeaderLine()
   // many times: this is incomparably faster with remote servers (one round
   // trip to server is much less expensive than a dozen of them!)

   wxArrayString headersUser(headersUser_);

   // Face and X-Face headers need to be retrieved if we want to show faces
   if ( m_ProfileValues.showFaces )
   {
#ifdef HAVE_XFACES
      headersUser.Insert(XFACE_HEADER, 0);
#endif // HAVE_XFACES
      headersUser.Insert(FACE_HEADER, 0);
   }

   size_t countHeaders = headersUser.GetCount();

   // these headers can be taken from the envelope instead of retrieving them
   // from server, so exclude them from headerNames which is the array of
   // headers we're going to retrieve from server
   //
   // the trouble is that we want to keep the ordering of headers correct,
   // hence all the contortions below: we remember the indices and then
   // inject them back into the code processing headers in the loop
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

   // a boolean array (in spite of its name) telling us, for each header we
   // show, whether it's present in the envelop or not
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
                     default:
                        FAIL_MSG( "forgot to add header here" );
                        // fall through

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
               FAIL_MSG( "unknown envelop header" );
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

   // for the loop below: we start it at 0 normally but at 1 or 2 if we had
   // inserted Face/X-Face headers above as we don't want to show them verbatim
   n = 0;

   if ( m_ProfileValues.showFaces )
   {
      vi->face = headerValues[n++];
#ifdef HAVE_XFACES
      vi->xface = headerValues[n++];
#endif // HAVE_XFACES
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

      // show the header and mark the URLs in it
      const String& name = headerNames[n];

      // there can be more than one line in each header in which case we
      // show each line of the value on a separate line -- although this is
      // wrong because there could be a single header with a multiline value
      // as well, this is the best we can do considering that GetHeaderLine
      // concatenates the values of all headers with the same name anyhow
      wxArrayString values = wxStringTokenize(value, "\n");

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

void
MessageView::ShowFace(const wxString& faceString)
{
   // according to the spec at http://quimby.gnus.org/circus/face/ the Face
   // header must be less than 966 after folding the lines
   if ( faceString.length() > 966 )
   {
      wxLogDebug("Message \"%s\" Face header is too long, ignored.",
                 m_mailMessage->Subject());
      return;
   }

   // TODO: for now we use rfc822_base64() instead of wxBase64Decode() as we
   //       still support wx 2.8 which doesn't have the latter but we should
   //       replace this code with wx equivalent in the future
#if 0
   #include <wx/base64.h>

   const wxMemoryBuffer faceBuf(wxBase64Decode(faceString));
   unsigned long faceLen = faceBuf.GetDataLen();
   if ( !faceLen )
#endif

   unsigned long faceLen;
   void *faceData = rfc822_base64(UCHAR_CAST(faceString.char_str()),
                                  faceString.length(),
                                  &faceLen);
   if ( !faceData )
   {
      wxLogDebug("Message \"%s\" Face header is not valid base64, ignored.",
                 m_mailMessage->Subject());
      return;
   }

   wxON_BLOCK_EXIT1( fs_give, &faceData );

   wxMemoryInputStream is(faceData, faceLen);
   wxImage face;
   if ( !face.LoadFile(is, wxBITMAP_TYPE_PNG) )
   {
      wxLogDebug("Message \"%s\" Face header is corrupted, ignored.",
                 m_mailMessage->Subject());
      return;
   }

   if ( face.GetWidth() != 48 || face.GetHeight() != 48 )
   {
      wxLogDebug("Message \"%s\" Face header has non-standard size.",
                 m_mailMessage->Subject());
   }

   m_viewer->ShowXFace(face);
}

#ifdef HAVE_XFACES

void
MessageView::ShowXFace(const wxString& xfaceString)
{
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

#endif // HAVE_XFACES

void
MessageView::ShowInfoFromHeaders(const ViewableInfoFromHeaders& vi)
{
   // normally there will never be both X-Face and Face in the same message but
   // check for the latter first so that we give it priority (it has higher
   // resolution and colours) if this does happen
   if ( !vi.face.empty() )
      ShowFace(vi.face);
#ifdef HAVE_XFACES
   else if ( !vi.xface.empty() )
      ShowXFace(vi.xface);
#endif // HAVE_XFACES
}

String
MessageView::GetHeaderNamesToDisplay() const
{
   // get all the headers we need to display
   wxString headers = READ_CONFIG(GetProfile(), MP_MSGVIEW_HEADERS);

   // ignore trailing colon as it would result in a dummy empty name after
   // splitting (and unfortunately this extra colon can occur as some old M
   // versions had it in the default value of MP_MSGVIEW_HEADERS by mistake)
   if ( !headers.empty() && *headers.rbegin() == ':' )
      headers.erase(headers.length() - 1);

   return headers;
}

void
MessageView::ShowHeaders()
{
   m_viewer->StartHeaders();

   ViewableInfoFromHeaders vi;

   // if wanted, display all header lines
   if ( m_ProfileValues.showHeaders )
   {
      ShowAllHeaders(&vi);
   }
   else // show just the selected headers
   {
      const String headers = GetHeaderNamesToDisplay();
      if ( !headers.empty() )
      {
         const wxArrayString headersArray = strutil_restore_array(headers);

         // if we are using wildcards we need to examine all the headers anyhow
         // so just do it, otherwise we can retrieve just the headers we need
         // (this can make a huge difference, the default headers typically are
         // ~100 bytes while the entire message header is commonly 2-3KB and
         // sometimes more and, even more importantly, we can already have the
         // envelope headers and if this is all we display we can avoid another
         // trip to server completely)
         if ( headers.find_first_of("*?") != String::npos )
         {
            ShowMatchingHeaders(m_mailMessage->GetHeaderIterator(),
                                headersArray, &vi);
         }
         else
         {
            ShowSelectedHeaders(headersArray, &vi);
         }
      }
   }

   // display anything requiring special treatment that we found in the headers
   ShowInfoFromHeaders(vi);

   m_viewer->EndHeaders();
}

// ----------------------------------------------------------------------------
// MessageView text part processing
// ----------------------------------------------------------------------------

void MessageView::ShowTextPart(const MimePart *mimepart)
{
   // as we're going to need its contents, we'll have to download it: check if
   // it is not too big before doing this
   if ( CheckMessagePartSize(mimepart) )
   {
      ShowText(mimepart->GetTextContent(), mimepart->GetTextEncoding());
   }
   else // part too big to be shown inline
   {
      // display it as an attachment instead
      ShowAttachment(mimepart);
   }
}

void MessageView::ShowText(String textPart, wxFontEncoding textEnc)
{
   // get the encoding of the text
   wxFontEncoding encPart;
   if ( m_encodingUser != wxFONTENCODING_DEFAULT )
   {
      // user-specified encoding overrides everything
      encPart = m_encodingUser;

      RecodeText(&textPart, textEnc, m_encodingUser);
   }
   else // determine the encoding ourselves
   {
      encPart = textEnc;

#if !wxUSE_UNICODE
      if ( encPart == wxFONTENCODING_UTF8 || encPart == wxFONTENCODING_UTF7 )
      {
         m_encodingAuto = encPart;

         // convert from UTF-8|7 to environment's default encoding
         encPart = ConvertUTFToMB(&textPart, encPart);
      }
      else
#endif // wxUSE_UNICODE
      if ( encPart == wxFONTENCODING_SYSTEM ||
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
      encPart = (wxFontEncoding)(long)
                  READ_CONFIG(GetProfile(), MP_MSGVIEW_DEFAULT_ENCODING);
   }

   // init the style we're going to use
   MTextStyle style;
#if !wxUSE_UNICODE
   bool fontSet = false;
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
#endif // !wxUSE_UNICODE
      style.SetFont(m_ProfileValues.GetFont());

   style.SetTextColour(m_ProfileValues.FgCol);

   // show the text by passing it to all the registered view filters
   if ( !m_filters )
   {
      InitializeViewFilters();
   }

   // This is a hack to avoid a problem with some filters (notably the
   // quoting/URL detection one) taking worse than O(N) time to execute making
   // them impractical to use for large messages. So we don't use any filters
   // at all for such "large" messages. A better solution is discussed in #937
   // but would need significantly more effort.
   ViewFilter* const
      filter = CheckMessageOrPartSize(textPart.length(), Check_NonInteractive)
                  ? m_filters->GetFilter()
                  : m_nullFilter;

   CHECK_RET( filter, "no view filters at all??" );

   filter->StartText();
   filter->Process(textPart, m_viewer, style);
}

// ----------------------------------------------------------------------------
// MessageView embedded messages display
// ----------------------------------------------------------------------------

// TODO: make this stuff configurable, e.g. by having an option specifying the
//       template to use and using MessageTemplateVarExpander here

void MessageView::ShowEmbeddedMessageSeparator()
{
   if ( m_ProfileValues.decorateEmbedded )
      ShowTextLine("\r\n" + wxString(80, '_'));
}

void MessageView::ShowEmbeddedMessageStart(const MimePart& part)
{
   ShowEmbeddedMessageSeparator();

   if ( !m_ProfileValues.showEmbeddedHeaders )
      return;

   const wxArrayString
      displayHeaders = strutil_restore_array(GetHeaderNamesToDisplay());
   if ( !displayHeaders.empty() )
   {
      m_viewer->StartHeaders();

      // we need to get the headers of the embedded message, which are not the
      // headers of MESSAGE/RFC822 part (they're basically limited to just
      // "Content-Type: message/rfc822") nor the headers of the nested part
      unsigned long len = 0;
      const char * const hdrStart = (const char *)part.GetRawContent(&len);

      // headers are separated from the body by a blank line
      const char * const hdrEnd = strstr(hdrStart, "\r\n\r\n");
      if ( hdrEnd )
         len = hdrEnd - hdrStart + 2; // leave "\r\n" terminating last header

      ViewableInfoFromHeaders vi;
      ShowMatchingHeaders(String(hdrStart, len), displayHeaders, &vi);

      ShowInfoFromHeaders(vi);

      m_viewer->EndHeaders();
   }
}

void MessageView::ShowEmbeddedMessageEnd(const MimePart& part)
{
   ShowEmbeddedMessageSeparator();
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
                  mimepart->GetPartSpec());

      return;
   }

   const MimeType type = mimepart->GetType();

   const String mimeType = type.GetFull();

   const String fileName = mimepart->GetFilename();

   MimeType::Primary primaryType = type.GetPrimary();

   // deal with unknown MIME types: some broken mailers use unregistered
   // primary MIME types - try to do something with them here (even though
   // breaking the neck of the author of the software which generated them
   // would be more satisfying)
   if ( primaryType > MimeType::OTHER )
   {
      wxString typeName = type.GetType();

      if ( typeName == "OCTET" )
      {
         // I have messages with "Content-Type: OCTET/STREAM", convert them
         // to "APPLICATION/OCTET-STREAM"
         primaryType = MimeType::APPLICATION;
      }
      else
      {
         wxLogDebug("Invalid MIME type '%s'!", typeName);
      }
   }

   // let's guess a little if we have generic APPLICATION MIME type, we may
   // know more about this file type from local sources
   if ( primaryType == MimeType::APPLICATION )
   {
      // get the MIME type for the files of this extension
      wxString ext = wxFileName(fileName).GetExt();
      if ( !ext.empty() )
      {
         wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
         wxFileType *ft = mimeManager.GetFileTypeFromExtension(ext);
         if(ft)
         {
            wxString mt;
            ft->GetMimeType(&mt);
            delete ft;

            if(wxMimeTypesManager::IsOfType(mt,"image/*"))
               primaryType = MimeType::IMAGE;
            else if(wxMimeTypesManager::IsOfType(mt,"audio/*"))
               primaryType = MimeType::AUDIO;
            else if(wxMimeTypesManager::IsOfType(mt,"video/*"))
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
               wxFontEncoding enc = m_encodingUser == wxFONTENCODING_DEFAULT
                                       ? mimepart->GetTextEncoding()
                                       : m_encodingUser;
               s = wxCSConv(enc).cMB2WC(data, len, NULL);
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
             ((mimeType == "TEXT/PLAIN" &&
               (fileName.empty() || m_ProfileValues.inlinePlainText))) )
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

      if ( mimetype == "TEXT/PLAIN" || CanViewerProcessPart(partChild) )
      {
         // remember this one as the best so far
         partBest = partChild;
      }

      partChild = partChild->GetNext();
   }

   // show just the best one
   CHECK(partBest, false, "No part can be displayed !");

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
               wxLogDebug("Duplicate CIDs in multipart/related message");
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

   CHECK( partStart, false, "No start part in multipart/related" );

   return ProcessPart(partStart, action);
}

bool
MessageView::ProcessSignedMultiPart(const MimePart *mimepart)
{
   if ( mimepart->GetParam("protocol") != "application/pgp-signature" )
   {
      // unknown signature protocol, don't try to interpret it
      return false;
   }

   static const char *
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
   //           "Signed part should be 7 bits");
   //CHECK_RET( signaturePart->GetNext() == 0, "Signature should be the last part" );
   //CHECK_RET( signaturePart->GetNested() == 0, "Signature should not have nested parts" );

   if ( signaturePart->GetType().GetFull() != "APPLICATION/PGP-SIGNATURE" )
   {
      wxLogWarning(String(_("Signed message signature does not have a "
                            "\"application/pgp-signature\" type.")) +
                   wxGetTranslation(sigIgnoredMsg));

      return false;
   }

   MCryptoEngineFactory * const factory
      = (MCryptoEngineFactory *)MModule::LoadModule("PGPEngine");
   CHECK( factory, false, "failed to create PGPEngineFactory" );

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
                                 pgpEngine,
                                 status,
                                 this,
                                 log
                               );

   ProcessPart(signedPart);

   pgpInfo->SetLog(log);

   ShowTextLine();

   m_viewer->InsertClickable(pgpInfo->GetBitmap(),
                             pgpInfo,
                             pgpInfo->GetColour());

   ShowTextLine();

   factory->DecRef();

   return true;
}

bool
MessageView::ProcessEncryptedMultiPart(const MimePart *mimepart)
{
   if ( mimepart->GetParam("protocol") != "application/pgp-encrypted" )
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

   if ( encryptedPart->GetType().GetFull() != "APPLICATION/OCTET-STREAM" )
   {
      wxLogError(_("The actual encrypted data part does not have a "
                   "\"application/octet-stream\" type, "
                   "ignoring it."));
      return false;
   }

   String encryptedData = encryptedPart->GetRawContentAsString();

   MCryptoEngineFactory * const factory
      = (MCryptoEngineFactory *)MModule::LoadModule("PGPEngine");
   CHECK( factory, false, "failed to create PGPEngineFactory" );

   MCryptoEngine* pgpEngine = factory->Get();

   MCryptoEngineOutputLog *log = new MCryptoEngineOutputLog(GetWindow());

   String decryptedData;
   MCryptoEngine::Status status =
         pgpEngine->Decrypt(encryptedData, decryptedData, log);

   ClickablePGPInfo *pgpInfo = 0;
   switch ( status )
   {
      case MCryptoEngine::OK:
         {
            pgpInfo = new PGPInfoGoodMsg(this);

            // notice that decryptedData may use just LFs and not CR LF as line
            // separators, e.g. this is often (always?) the case with PGP
            // messages created under Unix systems, but c-client functions used
            // in MimePartVirtual do expect a message in canonical format so we
            // must ensure that we do have CR LFs
            const String dataCRLF = strutil_enforceCRLF(decryptedData);
            MimePartVirtual *mpv = new MimePartVirtual(dataCRLF.utf8_str());
            if ( mpv->IsOk() )
            {

               AddVirtualMimePart(mpv);
               ProcessPart(mpv);
               break;
            }

            // failed to create the virtual MIME part representing it
            delete mpv;
            delete pgpInfo;
         }
         // fall through

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

   ShowTextLine();

   m_viewer->InsertClickable(pgpInfo->GetBitmap(),
                             pgpInfo,
                             pgpInfo->GetColour());

   ShowTextLine();

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
   if ( subtype == "ALTERNATIVE" )
   {
      processed = ProcessAlternativeMultiPart(mimepart, action);
   }
   else if ( subtype == "RELATED" )
   {
      processed = ProcessRelatedMultiPart(mimepart, action);
   }
   else if ( subtype == "SIGNED" )
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
            FAIL_MSG( "unknown MIME part processing action" );
      }
   }
   else if ( subtype == "ENCRYPTED" )
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
            FAIL_MSG( "unknown MIME part processing action" );
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
   CHECK( mimepart, false, "MessageView::ProcessPart: NULL mimepart" );

   const MimeType type = mimepart->GetType();
   switch ( type.GetPrimary() )
   {
      case MimeType::MULTIPART:
         return ProcessMultiPart(mimepart, type.GetSubType(), action);

      case MimeType::MESSAGE:
         if ( m_ProfileValues.inlineEmbedded )
         {
            if ( action == Part_Show )
               ShowEmbeddedMessageStart(*mimepart);

            const bool rc = ProcessAllNestedParts(mimepart, action);

            if ( action == Part_Show )
               ShowEmbeddedMessageEnd(*mimepart);

            return rc;
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
               return (type.GetPrimary() == MimeType::IMAGE &&
                        m_viewer->CanInlineImages()) ||
                           m_viewer->CanProcess(type.GetFull());

            default:
               FAIL_MSG( "unknown MIME part processing action" );
         }
         break;

      default:
         FAIL_MSG( "unknown MIME type" );
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
   MIMEFSHandler::AddFileWithMimeType(cid, data, len, part->GetType().GetFull());

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

   // if the part is explicitly an attachment we show it as attachment anyhow
   // so its real contents don't matter
   if ( !mimepart->IsAttachment() )
   {
      const MimeType type = mimepart->GetType();
      const String subtype = type.GetSubType();
      switch ( type.GetPrimary() )
      {
         case MimeType::MESSAGE:
            if ( !m_ProfileValues.inlineEmbedded )
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

               if ( type.GetSubType() == "RELATED" )
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
            if ( subtype == "PLAIN" )
               contents |= PartContent_Text;
            else if ( subtype == "HTML" )
               contents |= PartContent_HTML;
            break;

         case MimeType::IMAGE:
            contents |= PartContent_Image;
            break;

         default:
            wxFAIL_MSG( "Unknown MIME part type" );
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
         viewerName = "HtmlViewer";
      }
   }

   if ( viewerName.empty() )
   {
      if ( contents & PartContent_Image )
      {
         if ( m_ProfileValues.allowImages )
         {
            // this is the best viewer for showing images currently
            viewerName = "LayoutViewer";
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
   ASSERT_MSG( !viewerName.empty(), "empty viewer name" );

   MessageViewer *viewer = LoadViewer(viewerName);
   if ( !viewer )
   {
      wxLogWarning(_("Viewer \"%s\" couldn't be set."), viewerName);
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
      CHECK_RET( mimepart, "No MIME part to show?" );

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
   wxLogStatus("Message shown in %ldms", timeViewer.Time());
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
         message << NormalizeString(name) << ": ";

         // filenames are case-sensitive, don't modify them
         value = plist_it->value;
         if ( name.CmpNoCase("name") != 0 )
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
         message << NormalizeString(name) << ": ";

         value = plist_it->value;
         if ( name.CmpNoCase("filename") != 0 )
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
   wxString filename = wxFileName::CreateTempFileName("Mtemp");
   if ( !filename.empty() )
   {
      if ( MimeSaveAsMessage(mimepart, filename) )
      {
         wxString name;
         name.Printf(_("Attached message '%s'"),
                     mimepart->GetFilename());

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
      wxLogWarning(_("No command to open file \"%s\"."), filename);
      return;
   }

   // see HandleProcessTermination() for the explanation of "possibly"
   wxString errmsg;
   errmsg.Printf(_("External viewer \"%s\" possibly failed"), command);
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
   if ( wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM") )
   {
      // special handling of "APPLICATION/OCTET-STREAM": this is the default
      // MIME type for all binary attachments and many e-mail clients don't
      // use the correct type (e.g. IMAGE/GIF) always leaving this one as
      // default. Try to guess a better MIME type ourselves from the file
      // extension.
      if ( !filenameOrig.empty() )
      {
         wxString ext = wxFileName(filenameOrig).GetExt();

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
   if ( wxMimeTypesManager::IsOfType(mimetype, "MESSAGE/*") )
   {
      MimeOpenAsMessage(mimepart);

      return;
   }

   String filename = wxFileName::CreateTempFileName("Mtemp");
   if ( filename.empty() )
   {
      wxLogError(_("Failed to open the attachment."));

      delete fileType;
      return;
   }

   wxString ext = wxFileName(filenameOrig).GetExt();

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
   wxFileName::SplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename);
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
        (wxMimeTypesManager::IsOfType(mimetype, "IMAGE/TIFF")
         || wxMimeTypesManager::IsOfType(mimetype, "APPLICATION/OCTET-STREAM")) )
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
         wxLogDebug("Detected image/tiff fax content.");
         // use TIFF2PS command to create a postscript file, open that
         // one with the usual ps viewer
         String filenamePS = filename.BeforeLast('.') + ".ps";
         String command;
         command.Printf(READ_CONFIG_TEXT(profile,MP_TIFF2PS),
                        filename, filenamePS);
         // we ignore the return code, because next viewer will fail
         // or succeed depending on this:
         //system(command);  // this produces a postscript file on  success
         RunProcess(command);
         // We cannot use launch process, as it doesn't wait for the
         // program to finish.
         //wxString msg;
         //msg.Printf(_("Running '%s' to create Postscript file failed."), command);
         //(void)LaunchProcess(command, msg );

         wxRemoveFile(filename);
         filename = filenamePS;
         mimetype = "application/postscript";
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

   String filename = wxFileName::CreateTempFileName("Mtemp");

   wxString ext = wxFileName(filenameOrig).GetExt();
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
   wxFileName::SplitPath(filename, &path, &name, &extOld);
   if ( extOld != ext )
   {
      // Windows creates the temp file even if we didn't use it yet
      if ( !wxRemoveFile(filename) )
      {
         wxLogDebug("Warning: stale temp file '%s' will be left.",
                    filename);
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
      wxFileName::SplitPath(filename, &path, &name, &ext);

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
                        len, filename);
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
         title << " ('" << filename << "')";
      }

      MDialog_ShowText(GetParentFrame(), title, content, "MimeView");
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
   CHECK( GetFolder(), false, "no folder in message view?" );

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
                           "MsgViewFindString") )
            {
               if ( !m_viewer->Find(text) )
               {
                  wxLogStatus(GetParentFrame(),
                              _("'%s' not found"),
                              text);
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

      case WXMENU_VIEW_HEADERS:
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
               "invalid encoding in MessageView::SetEncoding()" );

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
      CHECK_RET( frame, "message view without parent frame?" );
      frame->SetStatusText(wxEmptyString);
   }
}

bool
MessageView::CheckMessageOrPartSize(unsigned long size, SizeCheck check) const
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
   else // big message
   {
      if ( check == Check_NonInteractive )
         return false;
   }

   wxString msg;
   msg.Printf(_("The selected %s is %lu KiB long which is "
                "more than the current threshold of %lu KiB for inline display.\n"
                "\n"
                "Do you still want to download it and show it inline?"),
              check == Check_Part
                  ? _("message part")
                  : _("message"),
              size, maxSize);

   return MDialog_YesNoDialog(msg, GetParentFrame());
}

bool
MessageView::CheckMessagePartSize(const MimePart *mimepart) const
{
   // only check for IMAP here: for POP/NNTP we had already checked it in
   // CheckMessageSize() below and the local folders are fast
   return (m_asyncFolder->GetType() != MF_IMAP) ||
               CheckMessageOrPartSize(mimepart->GetSize(), Check_Part);
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
               CheckMessageOrPartSize(message->GetSize(), Check_Message);
}

void
MessageView::DoShowMessage(Message *mailMessage)
{
   CHECK_RET( mailMessage, "no message to show in MessageView" );
   CHECK_RET( m_asyncFolder, "no folder in MessageView::DoShowMessage()" );

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
      CHECK_RET( mf, "mail message without associated folder?" );

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
   if ( READ_CONFIG(GetProfile(), MP_REPLY_QUOTE_SELECTION) )
   {
      const String text = m_viewer->GetSelection();
      if ( !text.empty() )
         return text;
   }

   // trim trailing empty lines, it is annoying to have to delete them manually
   // when replying
   wxString textBody = m_textBody;
   wxString::reverse_iterator p = textBody.rbegin();
   while ( p != textBody.rend() && (*p == '\r' || *p == '\n') )
      ++p;

   textBody.erase(p.base(), textBody.end());

   return textBody;
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
               command);
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
               command);

   // If we pass a (temporary) file as parameter to the command, we want to
   // monitor the child process termination to be able to remove the file as
   // soon as it's not needed any more. However if we don't use any temporary
   // file, we have no reason to bother with book-keeping and can just launch
   // the process and forget about it.
   wxProcess *process = filename.empty()
                           ? NULL
                           : new wxProcess(GetEventHandlerForProcess());
   int pid = wxExecute(command, wxEXEC_ASYNC, process);
   if ( !pid )
   {
      delete process;

      if ( !errormsg.empty() )
      {
         wxLogError("%s.", errormsg);
      }

      return false;
   }

   if ( process )
   {
      m_processes.Add(new ProcessInfo(process, pid, errormsg, filename));
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

   CHECK_RET( n != procCount, "unknown process terminated!" );

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
                  info->GetErrorMsg(),
                  exitcode);
   }

   m_processes.RemoveAt(n);
   delete info;
}

void MessageView::DetachAllProcesses()
{
   bool removedAll = true;

   // delete all process info objects, we don't need notifications about
   // process termination any more
   const size_t procCount = m_processes.GetCount();
   for ( size_t n = 0; n < procCount; n++ )
   {
      ProcessInfo *info = m_processes[n];

      if ( !info->RemoveTempFile() )
         removedAll = false;

      delete info;
   }

   m_processes.Empty();

   if ( !removedAll )
   {
      wxLogWarning(_("Some of the temporary files couldn't have been removed."));
   }
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

