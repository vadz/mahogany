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

#ifndef _M_MESSAGEVIEW_H_
#define _M_MESSAGEVIEW_H_

#ifdef __GNUG__
   #pragma interface "MessageView.h"
#endif

#ifndef USE_PCH
   #include <wx/dynarray.h>
   #include <wx/font.h>
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
class ViewFilter;

WX_DEFINE_ARRAY(ProcessInfo *, ArrayProcessInfo);

// ----------------------------------------------------------------------------
// MessageView: this class does MIME handling and uses ViewFilters (which, in
//              turn, use MessageViewer) to really show things on the screen
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

   /// return the name of the folder which messages we're viewing
   String GetFolderName() const;

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
   bool MimeSave(const MimePart *part, const wxChar *filename = NULL);

   /// view attachment as text
   void MimeViewText(const MimePart *part);

   //@}

   /** @name wxFolderView helpers
    */
   //@{

   /// get the info about all available viewers
   static size_t GetAllAvailableViewers(wxArrayString *names,
                                        wxArrayString *descs);

   /// get the info about all available filters
   static size_t GetAllAvailableFilters(wxArrayString *names,
                                        wxArrayString *labels,
                                        wxArrayInt *states);

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

   /// launch a process, returns FALSE if it failed
   bool LaunchProcess(const String& command,    // cmd to execute
                      const String& errormsg,   // err msg to give on failure
                      const String& tempfile = _T("")); // temp file name if any

   /// create the "View" menu for our parent frame
   virtual void CreateViewMenu();

   /// called when view filter state is toggled
   virtual void OnToggleViewFilter(int id, bool checked) = 0;

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

   /** @name External processes

       We launch external processes to view/open/print attachments and to show
       the URLs
    */
   //@{

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

   /// process a multipart part
   void ProcessMultiPart(const MimePart *part, const String& subtype);

   /// process a multipart/alternative part
   void ProcessAlternativeMultiPart(const MimePart *part);

   /// process a multipart/signed part
   void ProcessSignedMultiPart(const MimePart *part);

   /// process a multipart/encrypted part
   void ProcessEncryptedMultiPart(const MimePart *part);

   /// call ProcessPart() for all subparts of this part
   void ProcessAllNestedParts(const MimePart *part);

   /// show part of any kind
   void ShowPart(const MimePart *part);

   /// show a text part
   void ShowTextPart(const MimePart *part);

   /// show a text
   void ShowText(String textPart, wxFontEncoding textEnc = wxFONTENCODING_SYSTEM);

   /// show an attachment
   void ShowAttachment(const MimePart *part);

   /// show an inline image
   void ShowImage(const MimePart *part);

   /// return the clickable info object (basicly a label) for this part
   ClickableInfo *GetClickableInfo(const MimePart *part) const;

   //@}

   /// @name Access to the view filters for wxMessageView
   //@{

   /**
      Start iterating over all filters in the filter chain.

      @param name the string where the name of the filter is returned
      @param desc the string where the user-readable description of the filter
                  is returned
      @param enabled on return is true if the filter is enabled
      @param cookie opaque cookie to be passed to GetNextViewFilter() later
      @return true if ok, false if there are no filters
    */
   bool GetFirstViewFilter(String *name,
                           String *desc,
                           bool *enabled,
                           void **cookie);

   /**
      Continue iterating over all filters in the filter chain.

      @param name the string where the name of the filter is returned
      @param desc the string where the user-readable description of the filter
                  is returned
      @param enabled on return is true if the filter is enabled
      @param cookie opaque cookie (same as passed to GetFirstViewFilter())
      @return true if ok, false if there are no more filters
    */
   bool GetNextViewFilter(String *name,
                          String *desc,
                          bool *enabled,
                          void **cookie);

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

   /// reset the encoding to the default value
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
               AttCol,           // attachment colour
               HeaderNameCol,
               HeaderValueCol;

      /// the native font description
      String fontDesc;

      /// font family (wxROMAN/wxSWISS/wxTELETYPE/...) used if font is empty
      int fontFamily;

      /// font size (used only if font is empty)
      int fontSize;
      //@}

      /// @name MIME options
      //@{

      /// show all headers?
      bool showHeaders:1;

      /// inline MESSAGE/RFC822 attachments?
      bool inlineRFC822:1;

      /// inline TEXT/PLAIN attachments?
      bool inlinePlainText:1;

      /// max size of inline graphics: 0 if never inline at all, -1 if no limit
      long inlineGFX;

      /// follow the links to external images in the HTML messages?
      bool showExtImages:1;

      /// Show XFaces?
      bool showFaces:1;

      //@}

      /// @name URL viewing
      //@{

      /// highlight the URLs in the message?
      bool highlightURLs;

      /// the colour to use for URL highlighting (if highlightURLs is true)
      wxColour UrlCol;

      //@}

      /// @name Address autocollection
      //@{

      /// Autocollect email addresses?
      int autocollect;

      /// Autocollect only email sender's address?
      int autocollectSenderOnly;

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

   /// @name View filters stuff
   //@{

   /// should be called exactly once to load all filters
   void InitializeViewFilters();

   /// update the state (enabled/disabled) of the filters from profile
   void UpdateViewFiltersState();

   /// linked list of the filters
   class ViewFilterNode *m_filters;

   //@}


   /// the obejct to which we delegate the menu command processing
   MsgCmdProc *m_msgCmdProc;

   friend class ProcessEvtHandler;
   friend class MessageViewer;

   // it calls our PopupMIMEMenu()
   friend class wxMIMETreeDialog;
};

// ----------------------------------------------------------------------------
// global functions (implemented in gui/wxMessageView.cpp)
// ----------------------------------------------------------------------------

/// create and show a new message in a separate frame, return its message view
extern MessageView *ShowMessageViewFrame(wxWindow *parent,
                                         ASMailFolder *asmf,
                                         UIdType uid);

#endif // _M_MESSAGEVIEW_H_
