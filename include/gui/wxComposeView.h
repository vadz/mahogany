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

#ifdef __GNUG__
#  pragma interface "wxComposeView.h"
#endif

#ifndef USE_PCH
#   include   "Message.h"
#   include   "gui/wxMenuDefs.h"
#   include   "gui/wxMFrame.h"
#   include   "gui/wxIconManager.h"
#   include   "Profile.h"
#   include   "kbList.h"
#endif // USE_PCH

#include "Composer.h"   // the base class

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

class wxNewAddressTextCtrl;
class wxRcptControls;
class wxComposeView;
class wxEnhancedPanel;

class WXDLLEXPORT wxChoice;
class WXDLLEXPORT wxLayoutWindow;
class WXDLLEXPORT wxProcess;
class WXDLLEXPORT wxProcessEvent;
class WXDLLEXPORT wxSplitterWindow;
class WXDLLEXPORT wxTextCtrl;

#include <wx/dynarray.h>

WX_DEFINE_ARRAY(wxRcptControls *, ArrayRcptControls);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

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

   // a message kind - combined with the mode this determines the template
   // InitText() will use
   enum MessageKind
   {
      Message_New,      // either new mail message or new article
      Message_Reply,    // reply or follow-up
      Message_Forward   // this one is incompatible with Mode_NNTP
   };

   /** Initializes the composer text: for example, if this is a reply, inserts
       the quoted contents of the message being replied to (except that, in
       fact, it may do whatever the user configured it to do using templates).
       The msg parameter may be NULL only for the new messages, messages
       created with CreateReply/FwdMessage require it to be !NULL.

       @param msg the message we're replying to or forwarding
    */
   void InitText(Message *msg = NULL, MessageView *msgview = NULL);

   /** insert a file into buffer
       @param filename file to insert
       @param mimetype mimetype to use
       @param num_mimetype numeric mimetype
       */
   void InsertFile(const char *filename = NULL,
                   const char *mimetype = NULL);

   /** Insert MIME content data
       @param data pointer to data (we take ownership of it)
       @param len length of data
       @param mimetype mimetype to use
       @param filename optional filename to add to list of parameters
       */
   void InsertData(void *data,
                   size_t length,
                   const char *mimetype = NULL,
                   const char *filename = NULL);

   /// inserts a text
   void InsertText(const String &txt);

   /// move the cursor to the given position
   void MoveCursorTo(int x, int y);

   /** Set the newsgroups to post to.
       @param groups the list of newsgroups
   */
   void SetNewsgroups(const String &groups);

   /// Set the default value for the "From" header (if we have it)
   void SetDefaultFrom();

   /// sets Subject field
   void SetSubject(const String &subj);

   /// adds recepients from addr (Recepient_Max means to reuse the last)
   void AddRecipients(const String& addr,
                      RecipientType rcptType = Recipient_Max);

   /// get the subject value
   String GetSubject() const;

   /// get from value (empty means default)
   String GetFrom() const;

   /// get (all) addresses of this type
   virtual String GetRecipients(RecipientType type) const;

   /// make a printout of input window
   void Print(void);

   /** Send the message.
       @param schedule if TRUE, call calendar module to schedule sending
       @return true if successful, false otherwise
   */
   bool Send(bool schedule = FALSE);

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
   bool OnFirstTimeFocus();

      /// called just before text in composer is modified for the 1st time
   void OnFirstTimeModify();

      /// called when rcpt type is changed
   void OnRcptTypeChange(RecipientType type);

      /// called to remove the recipient with this index
   void OnRemoveRcpt(size_t index);
   //@}

   // for wxAddressTextCtrl usage: remember last focused field
   void SetLastAddressEntry(int field) { m_indexLast = field; }

   // get the profile to use for options
   Profile *GetProfile(void) const { return m_Profile; }

   // is the control with this index enabled?
   bool IsRecipientEnabled(size_t index) const;

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   void AddHeaderEntry(const String &entry, const String &value);

   /// reset the "dirty" flag
   void ResetDirty();

   // implement base class virtual
   virtual wxComposeView *GetComposeView() { return this; }
   virtual wxFrame *GetFrame() { return this; }

protected:
   /** quasi-Constructor
       @param parent parent window
       @param parentProfile parent profile
       @param to default value for To field
       @param cc default value for Cc field
       @param bcc default value for Bcc field
       @param hide if true, do not show frame
   */
   void Create(wxWindow *parent = NULL,
               Profile *parentProfile = NULL,
               bool hide = false);

   /** Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   wxComposeView(const String &iname, wxWindow *parent = NULL);

   // helpers
   // -------

   /// clear the window
   void DoClear();

   /// InitText() helper
   void DoInitText(Message *msg, MessageView *msgview = NULL);

   /// set the message encoding to be equal to the encoding of this msg
   void SetEncodingToSameAs(Message *msg);

   /// set encoding to use
   void SetEncoding(wxFontEncoding encoding);

   /// verify that the message can be sent
   bool IsReadyToSend() const;

   /// insert a text file at the current cursor position
   bool InsertFileAsText(const String& filename,
                         bool replaceFirstTextPart = false);

   /// save the first text part of the message to the given file
   bool SaveMsgTextToFile(const String& filename,
                          bool errorIfNoText = true) const;

   /// A list of all extra headerslines to add to header.
   kbStringList m_ExtraHeaderLinesNames;
   /// A list of all extra headerslines to add to header.
   kbStringList m_ExtraHeaderLinesValues;

   /// Launch the external editor
   bool StartExternalEditor();

   /// Destructor
   ~wxComposeView();

private:
   /// initialize the menubar
   void CreateMenu();

   /// initialize the toolbar and statusbar
   void CreateToolAndStatusBars();

   /// makes the canvas
   void CreateFTCanvas(void);

   /// create the header fields
   wxSizer *CreateHeaderFields();

   // which kind of text ctrl to create in the functions below
   enum TextField
   {
      TextField_Normal,
      TextField_Address
   };

   // create a horz sizer containing the given control and the text ctrl
   // (pointer to which will be saved in the provided variable if not NULL)
   // with the specified id
   wxSizer *CreateSizerWithText(wxControl *control,
                                wxTextCtrl **ppText = NULL,
                                TextField tf = TextField_Normal,
                                wxWindow *parent = NULL);

   // create a sizer containing a label and a text ctrl
   wxSizer *CreateSizerWithTextAndLabel(const wxString& label,
                                        wxTextCtrl **ppText = NULL,
                                        TextField tf = TextField_Normal);

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

   /// the profile (never NULL)
   Profile *m_Profile;

   /// the name of the class
   String m_name;

   /// the initial from/reply-to address
   String m_from;

   /// the main splitter
   wxSplitterWindow *m_splitter;

   /// the panel: all header controls are its children
   wxPanel *m_panel;

   /// the edit/cut menu item
   class wxMenuItem *m_MItemCut;
   /// the edit/copy menu item
   class wxMenuItem *m_MItemCopy;
   /// the edit/paste menu item
   class wxMenuItem *m_MItemPaste;

   /**@name Child controls */
   //@{
      /// the from address - may be NULL if not shown
   wxTextCtrl *m_txtFrom;

      /// the subject (never NULL)
   wxTextCtrl *m_txtSubject;

      /// the address field
   wxNewAddressTextCtrl *m_txtRecipient;

      /// the sizer containing all recipients
   wxSizer *m_sizerRcpts;

      /// the panel containing already entered addresses
   wxEnhancedPanel *m_panelRecipients;

      /// the additional recipients: array of corresponding controls
   ArrayRcptControls m_rcptControls;

      /// the canvas for displaying the mail
   wxLayoutWindow *m_LayoutWindow;
   //@}

   /// the last focused address field: index of -1 means m_txtRecipient itself
   int m_indexLast;

   /// the index of the next text control to create (-1 initially)
   int m_indexRcpt;

   /// the type of the last recipient
   RecipientType m_rcptTypeLast;

   /// composer font
   int m_font, m_size;

   /// composer colours
   wxColour m_fg, m_bg;

   /// News or smtp?
   Mode m_mode;

   /// New article, reply/follow-up or forward?
   MessageKind m_kind;

   /// Are we sending the message?
   bool m_sending;

   /// If replying, this is the original message
   Message *m_OriginalMessage;

   /// the template to use or an empty string
   String m_template;

   /// the (main) encoding (== charset) to use for the message
   wxFontEncoding m_encoding;

   // external editor support
   // -----------------------

   /// ids for different processes we may launch
   enum
   {
      HelperProcess_Editor = 1
   };

   wxProcess *m_procExtEdit;  // wxWin process notification helper
   String     m_tmpFileName;  // temporary file we passed to the editor
   int        m_pidEditor;    // pid of external editor (0 if none)

private:
   // it creates us
   friend class Composer;

   // wxWindows macros
   DECLARE_EVENT_TABLE()
   DECLARE_CLASS(wxComposeView)
};

// ----------------------------------------------------------------------------
// other functions
// ----------------------------------------------------------------------------

// expand the given template using the profile and message (which may be NULL
// if it is not a reply/follow up) and insert the result into the composer
//
// return true if ok, false if template contained errors
extern bool ExpandTemplate(Composer& cv,
                           Profile *profile,
                           const String& templateValue,
                           Message *msgOriginal,
                           MessageView *msgview = NULL);

#endif // WXCOMPOSEVIEW_H

