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
public:
   /** @name Ctors and dtor

       MessageView can be created normally, there is no need to have a factory
       function for it.
    */
   //@{

   /// ctor takes as argument the parent window for the message viewer one
   MessageView(wxWindow *parent);

   /// dtor
   virtual ~MessageView();

   //@}

   /** @name Accessors

       Trivial accessor methods
    */
   //@{

   /// get the underlying window
   wxWindow *GetWindow() const;

   /// get the UID of the currently shown message (UID_ILLEGAL if none)
   UIdType GetUId() const { return m_uid; }

   //@}

   /** @name Commands

       Message view handles the commands from "Message" and "Language" menus
       itself normally
    */
   //@{

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
   void MimeInfo(int numPart);

   /// handles the currently selected MIME content
   void MimeHandle(int numPart);

   /// "Opens With..." attachment
   void MimeOpenWith(int numPart);

   /// saves the currently selected MIME content
   bool MimeSave(int numPart, const char *filename = NULL);

   /// view attachment as text
   void MimeViewText(int numPart);

   //@}

protected:
   /** @name Initialization
    */
   //@{

   /// common part of all ctors
   void Init();

   //@}

   /// update the "show headers" menu item from m_ProfileValues.showHeaders
   void UpdateShowHeadersInMenu();

   /// show this message in the viewer
   virtual void DoShowMessage(Message *msg);

   /** @name Accessors

       Some trivial accessors
    */
   //@{

   /// get the profile to read settings from: DO NOT CALL DecRef() ON RESULT
   Profile *GetProfile() const;

   /// get the folder we use: DO NOT CALL DecRef() ON RESULT
   ASMailFolder *GetFolder() const { return m_asyncFolder; }

   /// get the parent frame of the viewer
   wxFrame *GetParentFrame() const;

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

   /** @name Popup menus

       MessageView doesn't know anything about GUI so these methods must be
       implemented in the derived class
    */
   //@{

   /// show the URL popup menu
   virtual void PopupURLMenu(const String& url, const wxPoint& pt) = 0;

   /// show the MIME popup menu for this message part
   virtual void PopupMIMEMenu(size_t nPart, const wxPoint& pt) = 0;

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

   /// show all configured headers, returns "main" header encoding
   wxFontEncoding ShowHeaders();

   /// show a text part
   void ShowTextPart(wxFontEncoding encBody, size_t nPart);

   /// show an attachment
   void ShowAttachment(size_t nPart, const String& mimeType, size_t partSize);

   /// show an inline image
   void ShowImage(size_t nPart, const String& mimeType, size_t partSize);

   /// return the quoting level of this line, 0 if unquoted
   size_t GetQuotedLevel(const char *text) const;

   /// return the colour to use for the given quoting level
   wxColour GetQuoteColour(size_t qlevel) const;

   /// return the clickable info object (basicly a label) for this part
   ClickableInfo *GetClickableInfo(size_t nPart,
                                   const String& mimeType,
                                   const String& mimeFileName,
                                   size_t partSize) const;

   //@}

private:
   /** @name Preview data */
   //@{
   /// uid of the message being previewed or UID_ILLEGAL
   UIdType m_uid;

   /// the message being previewed or NULL
   Message *m_mailMessage;

   /// the mail folder (only used if m_FolderView is NULL)
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
      /// Background and foreground colours, colours for URLs and headers
      wxColour BgCol,
               FgCol,
               UrlCol,
               HeaderNameCol,
               HeaderValueCol;

      /// font family (wxROMAN/wxSWISS/wxTELETYPE/...)
      int fontFamily;

      /// font size
      int fontSize;
      //@}

      /// @name Quoted text colourizing data
      //@{
      /// the colours for quoted text (only used if quotedColourize)
      wxColour QuotedCol[QUOTE_LEVEL_MAX];

      /// process quoted text colourizing?
      bool quotedColourize;

      /// if there is > QUOTE_LEVEL_MAX levels of quoting, cycle colours?
      bool quotedCycleColours;

      /// max number of whitespaces before >
      int quotedMaxWhitespace;

      /// max number of A-Z before >
      int quotedMaxAlpha;
      //@}

      /// @name MIME options
      //@{
      /// show all headers?
      bool showHeaders;

      /// inline MESSAGE/RFC822 attachments?
      bool inlineRFC822;

      /// inline TEXT/PLAIN attachments?
      bool inlinePlainText;

      /// highlight URLs?
      bool highlightURLs;

      /// max size of graphics to inline: 0 if none, -1 if not inline at all
      long inlineGFX;

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

#ifdef OS_UNIX
      /// Where to find AFM files.
      String afmpath;
#endif // Unix

      AllProfileValues();

      bool operator==(const AllProfileValues& other);
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

   /// the viewer we use
   MessageViewer *m_viewer;

   //@}

   friend class ProcessEvtHandler;
   friend class MessageViewer;
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
   ClickableInfo(int part, const String &label)
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

   /// get the part number (not for URLs)
   int GetPart() const
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
   int    m_part;
   String m_label;
};

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

/// create and show a new message view frame, return its message view
extern MessageView *ShowMessageViewFrame(wxWindow *parent);

#endif // MESSAGEVIEW_H

