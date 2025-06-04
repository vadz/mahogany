///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   gui/wxComposeView.h - composer GUI frame class
// Purpose:     wxComposeView declaration
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef WXCOMPOSEVIEW_H
#define WXCOMPOSEVIEW_H

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifndef USE_PCH
#  include "gui/wxMFrame.h"
#endif // USE_PCH

#include "Composer.h"         // for Composer

#include "MessageEditor.h"    // for MessageEditor::InsertMode enum

#include "strlist.h"

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class wxRcptMainControl;
class wxRcptExtraControl;
class wxComposeView;
class wxEnhancedPanel;
class SendMessage;
class SendThreadResult;
class MessageEditor;

class IsReplyButton;
class PGPSignButton;

class WXDLLIMPEXP_FWD_CORE wxChoice;
class WXDLLIMPEXP_FWD_CORE wxMenuItem;
class WXDLLIMPEXP_FWD_BASE wxProcess;
class WXDLLIMPEXP_FWD_BASE wxProcessEvent;
class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

WX_DEFINE_ARRAY(wxRcptExtraControl *, ArrayRcptControls);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// when to send the message
enum SendMode
{
   /// either now or later, depending on the options
   SendMode_Default,

   /// send it immediately
   SendMode_Now,

   /// just put it in outbox
   SendMode_Later,

   /// end of enum marker
   SendMode_Max
};

// ----------------------------------------------------------------------------
// wxComposeView: composer frame
// ----------------------------------------------------------------------------

class wxComposeView : public wxMFrame, public Composer
{
public:
   /// Do we want to send mail or post a news article?
   enum Mode
   {
      Mode_Mail,
      Mode_News
   };

   /**
      Message kind:combined with the mode this determines the template
      InitText() will use
    */
   enum MessageKind
   {
      Message_New,      // either new mail message or new article
      Message_Reply,    // reply or follow-up
      Message_Forward   // this one is incompatible with Mode_NNTP
   };

   /**
      Common flags used in some functions.
    */
   enum
   {
      /// Just the absence of Interactive flag
      NonInteractive = 0,

      /// If this flag is specified, we can ask questions to the user
      Interactive = 1,

      /// If this flag is specified, the message is going to be sent right now
      ForSending = 2
   };


   /**
     Set the template to use for this message. Should be called before
     InitText(), if at all.

     @param templ the name of the template
   */
   void SetTemplate(const String& templ)
   {
      m_template = templ;

      // don't forget m_template now, even if our identity changes
      m_customTemplate = true;
   }

   /**
     Set the original message to use when replying or forwarding. Should be
     called only by CreateReplyMessage(), CreateFwdMessage() and CreateFollowUpArticle()
   */
   void SetOriginal(Message *original);

   /**
      Return true if this composer is a reply or followup for the given
      message.

      @param message the message to compare with
    */
   bool IsReplyTo(const Message& original) const;


   // implement Composer pure virtuals
   virtual void InitText(Message *msg = NULL, const MessageView *msgview = NULL);
   virtual void Launch();
   virtual void InsertFile(const wxChar *filename,
                           const wxChar *mimetype = NULL,
                           const wxChar *name     = NULL);

   virtual void InsertData(void *data,
                           size_t length,
                           const wxChar *mimetype = NULL,
                           const wxChar *name     = NULL,
                           const wxChar *filename = NULL);

   virtual void InsertText(const String &txt);

   virtual void InsertMimePart(const MimePart *mimePart);

   virtual void MoveCursorTo(int x, int y);
   virtual void MoveCursorBy(int x, int y);

   /** Set the newsgroups to post to.
       @param groups the list of newsgroups
   */
   void SetNewsgroups(const String &groups);

   virtual void SetFrom(const String& from);

   /// Set the default value for the "From" header (if we have it)
   void SetDefaultFrom();

   /// sets Subject field
   void SetSubject(const String &subj);

   /// adds recipients from addr (Recipient_Max means to reuse the last)
   virtual void AddRecipients(const String& addr,
                              RecipientType rcptType = Recipient_Max,
                              int flags = AddRcpt_Expand);

   /// get from value (empty means default)
   String GetFrom() const;

   /// get (all) addresses of this type
   virtual void GetRecipients(RecipientType type, wxArrayString& list) const;

   /// get addresses of this type formatted into string
   virtual String GetRecipients(RecipientType type) const;

   /**
      Save all the current recipients to the given address book/group

      @param collect the autocollect flag, if it is M_ACTION_NEVER, nothing
                     is done
      @param book the name of the address book to save addresses to
      @param group the name of the group in this book
    */
   void SaveRecipientsAddresses(MAction collect,
                                const String& book,
                                const String& group);

   /// get the currently entered subject
   virtual String GetSubject() const;

   /// set the focus to the editor window itself
   void SetFocusToComposer();

   /// make a printout of input window
   void Print(void);

   /**
     Save the message in the drafts folder.

     @return true if message was saved ok
   */
   bool SaveAsDraft() const;

   /**
     Delete the draft of this message from the drafts folder, if any (i.e. it
     is safe to call this function unconditionally)

     @return true if the draft message was deleted, false otherwise
   */
   bool DeleteDraft();

   /**
     Save a snapshot of the composer contents into a file to be able to restore
     it later if the app crashes.
   */
   bool AutoSave();

   /**
     Start sending the message.

     This function assembles the message and passes it to a background thread
     for sending. The thread will later post an event which will result in
     OnSendResult() being called. Until then the window will be disabled to
     prevent the user from editing a message which is in process of being sent.

     @param mode when to send it
   */
   void Send(SendMode mode = SendMode_Default);

   /** wxWindows callbacks
   */
   //@{
      /// is it ok to close now?
   virtual bool CanClose() const;

      /// called when text zone contents changes
   void OnTextChange(wxCommandEvent &event);

      /// called on Menu selection
   void OnMenuCommand(int id);

      /// for button
   void OnExpand(wxCommandEvent &event);

      /// for identity combo
   void OnIdentChange(wxCommandEvent& event);

      /// called when external editor terminates
   void OnExtEditorTerm(wxProcessEvent& event);

      /// called when composer window gets focus for the 1st time
   virtual bool OnFirstTimeFocus();

      /// called just before text in composer is modified for the 1st time
   virtual void OnFirstTimeModify();

      /// called when rcpt type is changed
   void OnRcptTypeChange(RecipientType type);

      /// called to remove the recipient with this index
   void OnRemoveRcpt(size_t index);

      /// used to layout recepients area only once
   void OnIdle(wxIdleEvent& event);

   //@}

   /// is the control with this index enabled?
   bool IsRecipientEnabled(size_t index) const;

   /// get the profile to use for options
   Profile *GetProfile(void) const { return m_Profile; }

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   void AddHeaderEntry(const String &entry, const String &value);

   // set/reset the "dirty" flag
   virtual void ResetDirty();
   virtual void SetDirty();

   // implement base class virtual
   virtual wxComposeView *GetComposeView() { return this; }
   virtual wxFrame *GetFrame() { return this; }

   /// set the colours and fonts for a (freshly created) text control
   void SetTextAppearance(wxTextCtrl *text);

   /**
      Return true if we're configured to cryptographically sign the message.
    */
  bool IsPGPSigningEnabled() const;

   /**
      Return true if we're a reply to another message.

      This doesn't simply check whether we have an original message but checks
      if we have an In-Reply-To header.
    */
   bool IsInReplyTo() const;

   /**
      Toggles the value returned by IsPGPSigningEnabled().
    */
   void TogglePGPSigning();

   /**
      Configures whether this message is a reply to another one.

      This is used for handling the menu command "Set if this is a reply" and
      presents user with a dialog allowing to change this.

      @return true if anything changed, false if nothing happened
    */
   bool ConfigureInReplyTo();


   /**
      Update the composer frame title.

      This should be called whenever the subject (shown in the title bar) or
      the external editor status (shown too) changes and is only used
      internally.
    */
   void UpdateTitle();

protected:
   /** quasi-Constructor
       @param parent parent window
       @param parentProfile parent profile
   */
   void Create(wxWindow *parent = NULL, Profile *parentProfile = NULL);

   /** Constructor
       @param name  name of windowclass
       @param mode can be either news or email
       @param kind is new message/article, reply/followup or forward
       @param parent parent window
   */
   wxComposeView(const String& name,
                 Mode mode,
                 MessageKind kind,
                 wxWindow *parent = NULL);

   // helpers
   // -------

   /// clear the window
   void DoClear();

   /**
      InitText() helper called when we can evaluate the template.

      This is needed because if the template refers to the headers values we
      want to let the user to fill in the headers first and to evaluate the
      template later, however this should happen compeltely transparently for
      the code outside of wxComposeView so we check whether we can evaluate the
      template or not inside the public InitText() and either call DoInitText()
      from there immediately or postpone it until later.

      The parameter is used only if it is not NULL (which only happens when
      we're called directly from InitText()), otherwise the previously
      remembered (by InitText() itself) m_textToQuote is used.
    */
   void DoInitText(Message *msgOrig = NULL);

   /// InsertData() and InsertFile() helper
   void DoInsertAttachment(EditorContentPart *mc, const wxChar *mimetype);

   /// ask the user for the files to attach and attach them
   void LetUserAddAttachment();

   /// set encoding to use and pass it on to composer (by default same one)
   void SetEncoding(wxFontEncoding encoding,
                    wxFontEncoding encodingEditor = wxFONTENCODING_SYSTEM);

   /// set the message encoding to be equal to the encoding of this part and
   /// its subparts, returns true if encoding was set, false if no text part
   /// with non default encoding found
   bool SetEncodingToSameAs(const MimePart *part);

   /// set the message encoding to be equal to the encoding of this msg
   void SetEncodingToSameAs(const Message *msg);

   /// set the draft message we were started with
   void SetDraft(Message *msg);

   /// check if the user forgot to add an attachment and return false in this
   /// case
   bool CheckForForgottenAttachments() const;

   /// verify that the message can be sent
   bool IsReadyToSend() const;

   /// has the message been modified since last save?
   bool IsModified() const;

   /// do we have anything at all in the editor?
   bool IsEmpty() const;

   /// insert a text file at the current cursor position
   bool InsertFileAsText(const String& filename,
                         MessageEditor::InsertMode insMode);

   /// save the first text part of the message to the given file
   bool SaveMsgTextToFile(const String& filename) const;

   /// Launch the external editor
   bool StartExternalEditor();

   /// Return true if the external editor is currently running
   bool IsExternalEditorRunning() const { return m_procExtEdit != NULL; }

   /**
     Return a SendMessage object filled with all data we have. It must be
     deleted by the caller (presumably after calling its Send() or
     WriteToString()).

     @param flags by default contains Interactive flag
     @return SendMessage object to be deleted by the caller
   */
   SendMessage *BuildMessage(int flags = Interactive | ForSending) const;

   /**
     Return the message to be sent as a draft: it simply adds a few additional
     headers which we put in our draft messages and use in EditMessage() later.

     @param flags by default contains Interactive flag
     @return SendMessage object to be deleted by the caller
   */
   SendMessage *BuildDraftMessage(int flags = Interactive) const;

   // determines the encoding to use for representing the given Unicode text as
   // char string and returns it appropriately encoded; also updating the
   // encoding if we couldn't have used it
   //
   // may return NULL buffer only if the user cancelled the required
   // transcoding
   //
   // flags have the same meaning as for BuildMessage but can only be 0 or
   // Interactive in this function
   wxCharBuffer EncodeText(const wxString& text,
                           wxFontEncoding *encoding,
                           int flags) const;


   /// Destructor
   ~wxComposeView();

   /// A list of all extra headers to add to header.
   StringList m_extraHeadersNames;

   /// A list of values corresponding to the names above.
   StringList m_extraHeadersValues;

private:
   // implement base class pure virtual methods
   virtual void DoCreateToolBar();
   virtual void DoCreateStatusBar();

   /// initialize the menubar
   void CreateMenu();

   /// create and initialize the editor
   void CreateEditor();

   /// create the PGP-related controls, returns the window containing them all
   wxWindow *CreatePGPControls();

   /// init our various appearance parameters
   void InitAppearance();

   /// create the header fields
   wxSizer *CreateHeaderFields();

   // which kind of text ctrl to create in the functions below
   enum TextField
   {
      TextField_Normal,
      TextField_Address
   };

   // add a place holder to the recipients sizer
   void CreatePlaceHolder();

   // delete the controls created by CreatePlaceHolder()
   void DeletePlaceHolder();

   // create the new controls for another recipient
   void AddRecipientControls(const String& value, RecipientType rt);

   // adds one recipient, helper of AddRecipients()
   void AddRecipient(const String& addr, RecipientType rcptType);

   /// enable/disable editing of the message text
   void EnableEditing(bool enable);

   /// get the options (for MessageEditor)
   const Options& GetOptions() const { return m_options; }

   /**
       Called with the result of sending the message.

       This method may be called directly from our own Send() or from
       OnSendThreadDone() event handler when it receives a notification from
       the background sending thread.

       @param res The object containing information about whether the message
         sending succeeded or failed.
    */
   //@{
   void OnSendResult(const SendThreadResult& res);
   //@}

   /**
       Event handler for background sending thread.

       Simply forwards to OnSendResult().
    */
   void OnSendThreadDone(wxThreadEvent& evt);


   /// the profile (never NULL)
   Profile *m_Profile;

   /// the initial from/reply-to address
   String m_from;

   /// the main splitter
   wxSplitterWindow *m_splitter;

   /// the panel: all header controls are its children
   wxPanel *m_panel;

   // currently unused
#if 0
   /// the edit/cut menu item
   wxMenuItem *m_MItemCut;

   /// the edit/copy menu item
   wxMenuItem *m_MItemCopy;

   /// the edit/paste menu item
   wxMenuItem *m_MItemPaste;
#endif // 0

   /**@name Child controls */
   //@{

      /// the from address - may be NULL if not shown
   wxTextCtrl *m_txtFrom;

      /// the subject (never NULL)
   wxTextCtrl *m_txtSubject;

      /// the main recipient field
   wxRcptMainControl *m_rcptMain;

      /// the sizer containing all recipients
   wxSizer *m_sizerRcpts;

      /// the panel containing already entered addresses
   wxEnhancedPanel *m_panelRecipients;

      /// the additional recipients: array of corresponding controls
   ArrayRcptControls m_rcptExtra;

      /// the editor where the message is really edited
   MessageEditor *m_editor;

      /// button toggling In-Reply-To header
   IsReplyButton *m_btnIsReply;

      /// button enabling cryptographically signing the message
   PGPSignButton *m_btnPGPSign;

   //@}

   /// the type of the last recipient
   RecipientType m_rcptTypeLast;

   /// the options
   Options m_options;

   /// News or smtp?
   Mode m_mode;

   /// New article, reply/follow-up or forward?
   MessageKind m_kind;

   /// Non-NULL if we're in process of sending the message.
   SendMessage *m_msgBeingSent;

   /// Have we been modified since the last save?
   bool m_isModified;

   /// Number of recently pending recepients which we have to layout
   size_t m_numNewRcpts;

   /// Is it ok to send this message in an encoding different from original?
   mutable bool m_okToConvertOnSend;

   /// If replying, this is the original message (may be NULL)
   Message *m_OriginalMessage;

   /// if we're continuing to edit a draft, the original draft message
   Message *m_DraftMessage;

   /// the template to use or an empty string
   String m_template;

   /// True if m_template was specified by the user (and not read from profile)
   bool m_customTemplate;

   /// the text to be quoted in a reply/followup (may be empty)
   String m_textToQuote;

   /// the name of the file we autosaved ourselves to (may be empty)
   String m_filenameAutoSave;

   /// the (main) encoding (== charset) to use for the message
   wxFontEncoding m_encoding;

   /**
     @name external editor support
    */
   //@{

   /// compute the text hash to see if it was changed by external editor
   unsigned long ComputeTextHash() const;

   /// ids for different processes we may launch
   enum
   {
      HelperProcess_Editor = 1
   };

   /// wxWin process notification helper
   wxProcess *m_procExtEdit;

   /// temporary file we passed to the editor
   String     m_tmpFileName;

   /// pid of external editor (0 if none)
   int        m_pidEditor;

   /// the hash of text computed before it was passed to the ext editor
   unsigned long m_oldTextHash;

   /// true if the ext editor had been already invoked before
   bool       m_alreadyExtEdited;

   /// true if the user asked to close this window (do not launch the composer)
   bool       m_closing;

   /// true if we should close the window after successfully sending message
   bool       m_closeAfterSending;

   //@}

private:
   // it creates us
   friend wxComposeView *CreateComposeView(Profile *profile,
                                           const MailFolder::Params& params,
                                           wxComposeView::Mode mode,
                                           wxComposeView::MessageKind kind);

   // it uses our m_DraftMessage
   friend Composer *Composer::EditMessage(Profile *profile, Message *msg);

   // wxWindows macros
   DECLARE_EVENT_TABLE()
   DECLARE_DYNAMIC_CLASS_NO_COPY(wxComposeView)
};

// ----------------------------------------------------------------------------
// other functions
// ----------------------------------------------------------------------------

/**
  Returns true if the template contains the references to the headers of the
  message itself (in this case it can't be evaluated until the headers are
  set)
*/
extern bool TemplateNeedsHeaders(const String& templateValue);

/**
  Expand the given template using the profile and message (which may be NULL if
  it is not a reply/follow up) and insert the result into the composer

  @return true if ok, false if template contained errors
*/
extern bool ExpandTemplate(Composer& cv,
                           Profile *profile,
                           const String& templateValue,
                           Message *msgOriginal,
                           const String& textToQuote);

/**
   Return the string containing quoted (i.e. prefixed with reply marker) text.

   @param text the text to quote
   @param profile to get the options from
   @param msg the message we're replying to (for attribution)
   @return the quoted text
 */
extern String QuoteText(const String& text,
                        Profile *profile,
                        Message *msgOriginal);

#endif // WXCOMPOSEVIEW_H

