///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageView.h: declaration of MessageView
// Purpose:     MessageView is a non GUI class which uses a helper object
//              implementing the MessageViewer interface to show a Message
// Author:      Vadim Zeitlin
// Modified by:
// Created:     24.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#ifdef __GNUG__
   #pragma interface "MessageView.h"
#endif

#ifndef USE_PCH
   #include <wx/dynarray.h>
#endif

#include <wx/colour.h>

#include "MEvent.h"

class WXDLLEXPORT wxFrame;
class WXDLLEXPORT wxPoint;
class WXDLLEXPORT wxWindow;

class ASMailFolder;
class ClickableInfo;
class MessageViewer;
class Message;
class MEventData;
class MEventOptionsChangeData;
class MEventASFolderResultData;
class MimePart;
class MsgCmdProc;
class ProcessEvtHandler;
class ProcessInfo;

WX_DEFINE_ARRAY(ProcessInfo *, ArrayProcessInfo);

// the max quoting level for which we have unique colours (after that we reuse
// them)
#define QUOTE_LEVEL_MAX 3

// ----------------------------------------------------------------------------
// MessageView: this class does MIME handling and uses MessageViewer to really
//              show things on screen
// ----------------------------------------------------------------------------

class MessageView : public MEventReceiver
{
   /** @name Ctors and dtor

       MessageView can only be created by Create(), hence ctor is protected
    */
   //@{

protected:
   /// default ctor: we have to use 2 step construction
   MessageView();

   /// takes as argument the parent window for the message viewer one
   void Init(wxWindow *parent);

public:
   /// create a new MessageView
   static MessageView *Create(wxWindow *parent, FolderView *folderView = NULL);

   /// dtor
   virtual ~MessageView();

   //@}

   /** @name Accessors

       Trivial accessor methods
    */
   //@{

   /// get the underlying window
   wxWindow *GetWindow() const;

   /// get the parent frame of the viewer
   wxFrame *GetParentFrame() const;

   /// get the UID of the currently shown message (UID_ILLEGAL if none)
   UIdType GetUId() const { return m_uid; }

   //@}

   /** @name Commands

       Message view handles the commands from "Message" and "Language" menus
       itself normally
    */
   //@{

   /// get the menu command processor we use
   MsgCmdProc *GetCmdProc() const { return m_msgCmdProc; }

   /// handle the command from the menu, return true if processed
   bool DoMenuCommand(int id);

   /// handle a mouse click in MessageViewer (should be only called by it)
   void DoMouseCommand(int id, const ClickableInfo *ci, const wxPoint& pt);

   /// show the message view contents in the given language
   void SetLanguage(int cmdLang);

   /// open the URL (in a new browser window if inNewWindow)
   void OpenURL(const String& url, bool inNewWindow);

   //@}

   /** @name Operations

       The same message view is reused for different messages by calling
       ShowMessage() and Clear() may be called to reset it completely. Before
       calling any of the other methods SetFolder() must be called.
    */
   //@{

   /// clear the message view contents - it won't show anything after this
   void Clear();

   /// tells us to use the given folder
   void SetFolder(ASMailFolder *asmf);

   /// show the messages with this uid (from the folder set previously)
   void ShowMessage(UIdType uid);

   /// print the currently shown message
   bool Print();

   /// show the print preview of the currently shown message
   void PrintPreview();

   /// return the text selected in the viewer, may be empty
   String GetSelection() const;

   //@}

   /** @name Scrolling

       These functions allow to scroll the message view window and to detect
       when it reaches the bottom/top - presumably to pass to the next message
       seemlessly then.
    */
   //@{

   /// scroll line down, return false if already at bottom
   bool LineDown();

   /// scroll line up, return false if already at top
   bool LineUp();

   /// scroll page down, return false if already at bottom
   bool PageDown();

   /// scroll page up, return false if already at top
   bool PageUp();

   //@}

   /** @name MIME attachments operations

        These methods correspond to the actions from the MIME popup menu
    */
   //@{

   /// displays information about the currently selected MIME content
   void MimeInfo(const MimePart *part);

   /// handles the currently selected MIME content
   void MimeHandle(const MimePart *part);

   /// "Opens With..." attachment
   void MimeOpenWith(const MimePart *part);

   /// saves the currently selected MIME content
   bool MimeSave(const MimePart *part, const char *filename = NULL);

   /// view attachment as text
   void MimeViewText(const MimePart *part);

   //@}

   /// get the info about all available viewers
   static size_t GetAllAvailableViewers(wxArrayString *names,
                                        wxArrayString *descs);

   /// return a descriptive label for this MIME part
   static String GetLabelFor(const MimePart *mimepart);

protected:
   /** @name Initialization
    */
   //@{

   /// common part of all ctors
   void Init();

   //@}

   /** @name GUI stuff

       We need to interact with the rest of the GUI but we don't do it in this
       class: instead we provide the hooks for the derived one
    */
   //@{

   /// update the "show headers" menu item from m_ProfileValues.showHeaders
   void UpdateShowHeadersInMenu();

   /// show this message in the viewer
   virtual void DoShowMessage(Message *msg);

   /// update GUI to show the new viewer window
   virtual void OnViewerChange(const MessageViewer *viewerOld,
                               const MessageViewer *viewerNew) = 0;

   //@}

   /** @name Accessors

       Some trivial accessors
    */
   //@{

   /// get the profile to read settings from: DO NOT CALL DecRef() ON RESULT
   Profile *GetProfile() const;

   /// get the folder we use: DO NOT CALL DecRef() ON RESULT
   ASMailFolder *GetFolder() const { return m_asyncFolder; }

   //@}

   /** @name External processes

       We launch external processes to view/open/print attachments and to show
       the URLs
    */
   //@{

   /// launch a process, returns FALSE if it failed
   bool LaunchProcess(const String& command,    // cmd to execute
                      const String& errormsg,   // err msg to give on failure
                      const String& tempfile = ""); // temp file name if any

   /**
       synchronous versions of LaunchProcess(): launch a process and wait for
       its termination, returns FALSE it exitcode != 0
    */
   bool RunProcess(const String& command);

   /// called when a process we launched terminates
   void HandleProcessTermination(int pid, int exitcode);

   /// detach from external viewers still running
   void DetachAllProcesses();

   /// get the event handler to use with wxProcess
   ProcessEvtHandler *GetEventHandlerForProcess();

   /// array of process info for all external viewers we have launched
   ArrayProcessInfo m_processes;

   /// the event handler which forwards process events to us
   ProcessEvtHandler *m_evtHandlerProc;

   //@}

   /** @name GUI hooks

       MessageView doesn't know anything about GUI so these methods must be
       implemented in the derived class
    */
   //@{

   /// show the URL popup menu
   virtual void PopupURLMenu(wxWindow *window,
                             const String& url,
                             const wxPoint& pt) = 0;

   /// show the MIME popup menu for this message part
   virtual void PopupMIMEMenu(wxWindow *window,
                              const MimePart *part,
                              const wxPoint& pt) = 0;

   //@}

   /** @name Update

       Update() is called to really show the current message and also when any
       of the options governing its appearance change. The other functions are
       all called from Update() to split the work done there in reasonably
       sized chunks
    */
   //@{

   /// do show the current message
   void Update();

   /// show all configured headers
   void ShowHeaders();

   /// process part and decided what to do with it (call ShowPart or skip)
   void ProcessPart(const MimePart *part);

   /// call ProcessPart() for all subparts of this part
   void ProcessAllNestedParts(const MimePart *part);

   /// show part of any kind
   void ShowPart(const MimePart *part);

   /// show a text part
   void ShowTextPart(const MimePart *part);

   /// show an attachment
   void ShowAttachment(const MimePart *part);

   /// show an inline image
   void ShowImage(const MimePart *part);

   /// return the quoting level of this line, 0 if unquoted
   size_t GetQuotedLevel(const char *text) const;

   /// return the colour to use for the given quoting level
   wxColour GetQuoteColour(size_t qlevel) const;

   /// return the clickable info object (basicly a label) for this part
   ClickableInfo *GetClickableInfo(const MimePart *part) const;

   //@}

private:
   /** @name Preview data */
   //@{
   /// uid of the message being previewed or UID_ILLEGAL
   UIdType m_uid;

   /// the message being previewed or NULL
   Message *m_mailMessage;

   /// the mail folder
   ASMailFolder *m_asyncFolder;
   //@}

   /** @name Event manager stuff

       We process the options change event (update our appearance) and
       ASFolderResult which we use to actually show the message
    */
   //@{

   /// register with MEventManager
   void RegisterForEvents();

   /// unregister
   void UnregisterForEvents();

   /// internal M events processing function
   virtual bool OnMEvent(MEventData& event);

   /// process the result of an async operation (e.g. message retrieval)
   void OnASFolderResultEvent(MEventASFolderResultData& event);

   /// handle options change
   void OnOptionsChange(MEventOptionsChangeData& event);

   /// options change event cookie
   void *m_regCookieOptionsChange;

   /// async result event cookie
   void *m_regCookieASFolderResult;

   //@}

   /** @name Encoding/language

       We normally determine the encoding ourselves but the user may override
       our choice by selecting the language from the menu in which case our
       SetLanguage() is called - which just calls SetEncoding() and the latter
       sets m_encodingUser
    */
   //@{

   /// set m_encodingUser to the given encoding and refresh the view
   void SetEncoding(wxFontEncoding encoding);

   /// reset encoding if not configured to keep it
   void ResetUserEncoding();

   /// the encoding specified by the user or wxFONTENCODING_SYSTEM if none
   wxFontEncoding m_encodingUser;

   /// the auto detected encoding for the current message so far
   wxFontEncoding m_encodingAuto;

   //@}

   /** @name Message size checks

       We have an option to limit the size of the messages we download, but
       checking for it is less trivial as it seems as for some protocols (POP,
       NNTP) we always download the whole message at once while for IMAP we
       only download the parts we need to show inline, so we have 2 methods to
       check for both cases and one method containing their common code (which
       shouldn't be called directly)
    */
   //@{

   /// ask user if it's ok to download this message/part if size > limit
   bool CheckMessageOrPartSize(unsigned long size, bool part) const;

   /// call CheckMessageOrPartSize(true) for IMAP
   bool CheckMessagePartSize(const MimePart *part) const;

   /// call CheckMessageOrPartSize(false) for POP/NNTP
   bool CheckMessageSize(const Message *message) const;

   //@}

   /// printing helper, called by Print() and PrintPreview()
   void PrepareForPrinting();

   /** @name Options

       Message view uses the options from the profile of the folder it
       currently shows.
    */
   //@{

   /// All values read from the profile
   struct AllProfileValues
   {
      /** @name Appearance parameters
       */
      //@{
      /// message viewer (LayoutViewer, TextViewer, ...)
      String msgViewer;

      /// Background and foreground colours, colours for URLs and headers
      wxColour BgCol,
               FgCol,
               UrlCol,           // URL colour (used only if highlightURLs)
               AttCol,
               SigCol,           // signature colour (if highlightSig)
               HeaderNameCol,
               HeaderValueCol;

      /// the native font description
      String fontDesc;

      /// font family (wxROMAN/wxSWISS/wxTELETYPE/...) used if font is empty
      int fontFamily;

      /// font size (used only if font is empty)
      int fontSize;
      //@}

      /// @name URL highlighting and text quoting colourizing data
      //@{

      /// the colours for quoted text (only used if quotedColourize)
      wxColour QuotedCol[QUOTE_LEVEL_MAX];

      /// max number of whitespaces before >
      int quotedMaxWhitespace;

      /// max number of A-Z before >
      int quotedMaxAlpha;

      /// process quoted text colourizing?
      bool quotedColourize:1;

      /// if there is > QUOTE_LEVEL_MAX levels of quoting, cycle colours?
      bool quotedCycleColours:1;

      /// highlight URLs?
      bool highlightURLs:1;

      /// highlight the signature?
      bool highlightSig:1;

      //@}

      /// @name MIME options
      //@{

      /// show all headers?
      bool showHeaders;

      /// inline MESSAGE/RFC822 attachments?
      bool inlineRFC822;

      /// inline TEXT/PLAIN attachments?
      bool inlinePlainText;

      /// max size of inline graphics: 0 if never inline at all, -1 if no limit
      long inlineGFX;

      /// follow the links to external images in the HTML messages?
      bool showExtImages;

      /// Show XFaces?
      bool showFaces;

      //@}

      /// @name URL viewing
      //@{
      /// URL viewer
      String browser;

#ifdef OS_UNIX
      /// Is URL viewer of the netscape variety?
      bool browserIsNS;
#endif // Unix

      /// open netscape in new window?
      bool browserInNewWindow;
      //@}

      /// @name Address autocollection
      //@{
      /// Autocollect email addresses?
      int autocollect;

      /// Autocollect only email addresses with complete name?
      int autocollectNamed;

      /// Name of the ADB book to use for autocollect.
      String autocollectBookName;
      //@}

      AllProfileValues();

      bool operator==(const AllProfileValues& other) const;
      bool operator!=(const AllProfileValues& other) const
         { return !(*this == other); }

      /// get the font corresponding to the current options
      wxFont GetFont(wxFontEncoding enc = wxFONTENCODING_DEFAULT) const;
   } m_ProfileValues;

   /// read all our options from the current profile (returned by GetProfile())
   void ReadAllSettings(AllProfileValues *settings);

   /// reread the entries from our profile
   void UpdateProfileValues();

   /// get the profile values - mainly for MessageViewer
   const AllProfileValues& GetProfileValues() const { return m_ProfileValues; }

   //@}

   /** @name Viewer related stuff
    */
   //@{

   /// create the viewer we'll use
   void CreateViewer(wxWindow *parent);

   /// set the viewer (may be NULL, then CreateDefaultViewer() is used)
   void SetViewer(MessageViewer *viewer, wxWindow *parent);

   /**
      Default viewer creation: we must always have at least this viewer
      available even if no others can be found (they live in modules). This is
      also the viewer we use when there is no folder selected
   */
   virtual MessageViewer *CreateDefaultViewer() const = 0;

   /// returns true if we have a non default viewer
   bool HasRealViewer() const { return m_viewer && !m_usingDefViewer; }

   /// the viewer we use
   MessageViewer *m_viewer;

   /// is it the default one?
   bool m_usingDefViewer;
   //@}

   MsgCmdProc *m_msgCmdProc;

   friend class ProcessEvtHandler;
   friend class MessageViewer;

   // it calls our PopupMIMEMenu()
   friend class wxMIMETreeDialog;
};

// ----------------------------------------------------------------------------
// ClickableInfo: data associated with the clickable objects in MessageView
// ----------------------------------------------------------------------------

class ClickableInfo
{
public:
   enum Type
   {
      CI_ICON,
      CI_URL
   };

   /** @name Ctors
    */
   //@{

   /// ctor for URL
   ClickableInfo(const String& url)
      : m_label(url)
      {
         m_type = CI_URL;
      }

   /// ctor for all the rest
   ClickableInfo(const MimePart *part, const String& label)
      : m_label(label)
      {
         m_part = part;
         m_type = CI_ICON;
      }

   //@}

   /** @name Accessors
    */
   //@{

   /// is it an URL or an attachment?
   Type GetType() const { return m_type; }

   /// get the URL text
   const String& GetUrl() const
   {
      ASSERT_MSG( m_type == CI_URL, "no URL for this ClickableInfo!" );

      return m_label;
   }

   /// get the MIME part (not for URLs)
   const MimePart *GetPart() const
   {
      ASSERT_MSG( m_type != CI_URL, "no part number for this ClickableInfo!" );

      return m_part;
   }

   /// get the label
   const String& GetLabel() const
   {
      ASSERT_MSG( m_type != CI_URL, "Use GetUrl() for URLs!" );

      return m_label;
   }

   //@}

private:
   Type   m_type;
   String m_label;
   const MimePart *m_part;
};

// ----------------------------------------------------------------------------
// global functions (implemented in gui/wxMessageView.cpp)
// ----------------------------------------------------------------------------

/// create and show a new message in a separate frame, return its message view
extern MessageView *ShowMessageViewFrame(wxWindow *parent,
                                         ASMailFolder *asmf,
                                         UIdType uid);

/// try to convert text in UTF-8 or 7 to the current encoding in place
/// returns wxLocale::GetSystemEncoding()
extern wxFontEncoding ConvertUnicodeToSystem(wxString *strUtf,
                                             bool isUTF7 = false);

#endif // MESSAGEVIEW_H
