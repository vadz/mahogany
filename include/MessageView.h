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
   #include <wx/dynarray.h>     // for WX_DEFINE_ARRAY
   #include "Mdefaults.h"       // for MAction enum
#endif // USE_PCH

#include <wx/font.h>         // for wxFont
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
class FolderView;

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

   /// same arguments as in ctor(s)
   void Init(wxWindow *parent, Profile *profile = NULL);

public:
   /**
      Create a new MessageView as part of a FolderView.

      @param parent the parent window for the GUI control
      @param folderView the associated folder view, must not be NULL
      @return new MessageView object to be deleted by the caller or NULL
    */
   static MessageView *Create(wxWindow *parent, FolderView *folderView);

   /**
      Create a new standalone MessageView.

      @param parent the parent window for the GUI control
      @param profile our own (temp) profile, must not be NULL; note that we
                     do not take ownership of this pointer
      @return new MessageView object to be deleted by the caller or NULL
    */
   static MessageView *CreateStandalone(wxWindow *parent, Profile *profile);


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

   /// get the message being previewed, do not DecRef() (may be NULL)
   Message *GetMessage() const { return m_mailMessage; }

   /// return true if we're showing a message
   bool HasMessage() const { return m_mailMessage != NULL; }

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

   /// return the text we show (to be quoted in replies)
   String GetText() const;

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

   /// opens a MIME part as MESSAGE/RFC822
   void MimeOpenAsMessage(const MimePart *part);

   /// helper of MimeOpen() and MimeOpenWith()
   void MimeDoOpen(const String& command, const String& filename);

   /// another helper (of MimeOpenAsMessage() and MimeSave())
   bool MimeSaveAsMessage(const MimePart *part, const wxChar *filename);

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

   /**
      Get the profile to read settings from.

      If we don't have any profile, this returns a pointer to the global one.
      In any case, it must never return NULL pointer.

      @return profile pointer, never NULL. DO NOT CALL DecRef() ON RESULT
    */
   Profile *GetProfile() const;

   /**
      Get the folder we use

      Note that this may return NULL.

      @return async mail folder or NULL. DO NOT CALL DecRef() ON RESULT
    */
   ASMailFolder *GetFolder() const { return m_asyncFolder; }

   //@}

   /// launch a process, returns FALSE if it failed
   bool LaunchProcess(const String& command,    // cmd to execute
                      const String& errormsg,   // err msg to give on failure
                      const String& tempfile = wxEmptyString); // temp file name if any

   /// create the "View" menu for our parent frame
   virtual void CreateViewMenu();

   /**
      Change the viewer to the one with the specified name or the default one.

      Note that the viewer is changed only for the current message and will
      revert back to the default one when the user switches to another message.
      To change the viewer permanently the appropriate value must be written
      into Profile.

      @param viewer the name of the viewer to use
      @return true if the viewer was set successfully or false on error
    */
   bool ChangeViewer(const String& viewer);

   /**
      Called when view filter state is toggled.

      @param id the corresponding menu id, starting from
                WXMENU_VIEW_FILTERS_BEGIN + 1
    */
   virtual void OnToggleViewFilter(int id) = 0;

   /**
      Called to change the viewer to be used.

      @param id the corresponding menu id, starting from
                WXMENU_VIEW_VIEWERS_BEGIN + 1
    */
   virtual void OnSelectViewer(int id) = 0;

   /**
      Called by view filter code only to notify that text is being added to the
      displayed text.
    */
   void OnBodyText(const String& text) { m_textBody += text; }

   /**
      Give us ownership of a virtual mime part created on the fly.

      This is a hack: sometimes we create virtual, i.e. not existing in the
      original message, mime parts which don't exist in the original Message.
      This creates a problem of disposing them: normally we don't free MimePart
      objects at all because they belong to the Message, but the virtual ones
      don't, so someone else has to delete them. This someone else is us.
    */
   void AddVirtualMimePart(MimePart *mimepart);

protected:
   /** @name Initialization
    */
   //@{

   /// common part of all ctors
   void Init();

   //@}

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

      /// choose the viewer automatically?
      bool autoViewer:1;

      /// prefer HTML to plain text?
      bool preferHTML:1;

      /// allow HTML as last resort?
      bool allowHTML:1;

      /// switch viewer to be able to show images inline?
      bool allowImages:1;

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
      MAction autocollect;

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

   /// reread the entries from our profile, return true if anything changed
   bool UpdateProfileValues();

   /// get the profile values (mainly for MessageViewer)
   const AllProfileValues& GetProfileValues() const { return m_ProfileValues; }

   //@}

   /** @name GUI stuff

       We need to interact with the rest of the GUI but we don't do it in this
       class: instead we provide the hooks for the derived one
    */
   //@{

   /// show this message in the viewer
   virtual void DoShowMessage(Message *msg);

   /// update GUI to show the new viewer window
   virtual void OnViewerChange(const MessageViewer *viewerOld,
                               const MessageViewer *viewerNew,
                               const String& nameViewer) = 0;

   /// update GUI to indicate the new "show headers" options state
   virtual void OnShowHeadersChange() = 0;

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

   /// temporary files we have created for the view processes
   wxArrayString m_tempFiles;

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

   /// show a single header in the viewer
   void ShowHeader(const String& name, const String& value, wxFontEncoding enc);

   /// show all configured headers
   void ShowHeaders();

   /**
      Possible values for the second parameter of ProcessPart.
    */
   enum MimePartAction
   {
      /// Show the part in the viewer
      Part_Show,

      /// Just test if the viewer can show this part
      Part_Test,
   };

   /**
      Process part and take the given action.

      @param part the MIME part we work with
      @param action specifies whether we want to really show this part or just
                    to test whether we can do it
      @return true if we can process this part
    */
   bool ProcessPart(const MimePart *part, MimePartAction action = Part_Show);

   /// process a multipart part
   bool ProcessMultiPart(const MimePart *part,
                         const String& subtype,
                         MimePartAction action = Part_Show);

   /// process a multipart/alternative part
   bool ProcessAlternativeMultiPart(const MimePart *part,
                                    MimePartAction action = Part_Show);

   /// process a multipart/signed part
   bool ProcessSignedMultiPart(const MimePart *part);

   /// process a multipart/encrypted part
   bool ProcessEncryptedMultiPart(const MimePart *part);

   /// call ProcessPart() for all subparts of this part
   bool ProcessAllNestedParts(const MimePart *part,
                              MimePartAction action = Part_Show);

   /**
      Return true if the current viewer can process the given part.
    */
   bool CanViewerProcessPart(const MimePart *part)
   {
      return ProcessPart(part, Part_Test);
   }

public:
   /// show part of any kind
   void ShowPart(const MimePart *part);

protected:
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

   /// the obejct to which we delegate the menu command processing
   MsgCmdProc *m_msgCmdProc;

   /// our profile object (use GetProfile() instead of accessing directly!)
   Profile *m_profile;

   /// the full text shown to the user
   String m_textBody;

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

   /** @name Viewer related stuff
    */
   //@{

   /// create the viewer we'll use
   void CreateViewer();

   /**
      Set the viewer.

      Notice that this method automatically deletes the old viewer, use
      DoSetViewer() if this is undesirable.

      @param viewer the viewer to use, may be NULL, then CreateDefaultViewer()
                    is used; this viewer will be deleted automatically when
                    this function is called the next time
      @param viewerName the name of the viewer
      @param parent the parent window for the viewer, we may find it ourselves
                    once we're fully created so it can be left NULL except for
                    the initial call
    */
   void SetViewer(MessageViewer *viewer,
                  const String& viewerName,
                  wxWindow *parent = NULL);

   /**
      Initializes and sets the given non-NULL viewer.

      Unlike SetViewer(), this method has no side effects (except for calling
      OnViewerChange()).

      @param viewer non-NULL viewer
      @param viewerName its name
      @param parent the parent window or NULL if we already have it
    */
   void DoSetViewer(MessageViewer *viewer,
                    const String& viewerName,
                    wxWindow *parent = NULL);

   /**
      Reset the viewer to the default one.

      @param parent the parent window, may be NULL
    */
   void ResetViewer(wxWindow *parent = NULL)
   {
      SetViewer(NULL, String(), parent);
   }

   /**
      Creates the viewer for the given name and sets it as the current one
      remembering the original viewer.

      This method doesn't update the display, use ChangeViewer() for this.

      It rememebers the current viewer and its name in m_viewerOld and
      m_viewerNameOld so that they can restored by RestoreOldViewer() later.

      @param viewer the name of the viewer to use
      @return true if the viewer was set successfully or false on error
    */
   bool ChangeViewerWithoutUpdate(const String& viewer);

   /**
      Default viewer creation: we must always have at least this viewer
      available even if no others can be found (they live in modules). This is
      also the viewer we use when there is no folder selected
   */
   virtual MessageViewer *CreateDefaultViewer() const = 0;

   /// returns true if we have a non default viewer
   bool HasRealViewer() const { return m_viewer && !m_usingDefViewer; }

   /**
      Display the message in the current viewer.

      This function does just this and so shouldn't be called directly, it's
      used by Update() (which does the rest) and ChangeViewer().
    */
   void DisplayMessageInViewer();

   /// this is a private function used to study what kind of contents we have
   int DeterminePartContent(const MimePart *mimepart);

   /**
      Adjusts the viewer to correspond to the given MIME structure.

      We may want to choose a specific viewer for this message, e.g. we
      currently use HTML viewer for the HTML messages without any plain text
      part (of course, this is subject to user-defined options). When this
      function changes the viewer, it saves the old one as m_viewerOld and when
      it's called the next time, it restores it.
    */
   void AutoAdjustViewer(const MimePart *mimepart);

   /// Restore the old viewer if we had changed it
   void RestoreOldViewer();


   /// the viewer we use
   MessageViewer *m_viewer;

   /// the last viewer we used, before temporarily switching to the above one
   MessageViewer *m_viewerOld;

   /// the names of the current and old viewer (may be empty)
   String m_viewerName,
          m_viewerNameOld;

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


   /// list of all virtual MIME parts, created on demand
   class VirtualMimePartsList *m_virtualMimeParts;


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
