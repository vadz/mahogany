/*-*- c++ -*-********************************************************
 * wxComposeView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/



#ifndef WXCOMPOSEVIEW_H
#define WXCOMPOSEVIEW_H

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
#pragma interface "wxComposeView.h"
#endif

#ifndef USE_PCH
#   include   "Message.h"
#   include   "gui/wxMenuDefs.h"
#   include   "gui/wxMFrame.h"
#   include   "gui/wxIconManager.h"
#   include   "Profile.h"
#   include   "kbList.h"
#endif

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------
class wxFTOList;
class wxComposeView;
class wxFTCanvas;
class wxLayoutWindow;

class wxProcess;
class wxProcessEvent;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// just for now, FIXME!
#define WXCOMPOSEVIEW_FTCANVAS_YPOS 80

// ----------------------------------------------------------------------------
// types and classes
// ----------------------------------------------------------------------------

struct wxCVFileMapEntry
{
   unsigned long   id;
   String          filename;
};

KBLIST_DEFINE(wxCVFileMapType,wxCVFileMapEntry);

/** A wxWindows ComposeView class
  */

class wxComposeView : public wxMFrame //FIXME: public ComposeViewBase
{
public:
   // constants

   /// header fields we may show in the composer (not all of them are shown!)
   enum AddressField
   {
      Field_To,
      Field_Subject,
      Field_Cc,
      Field_Bcc,
      Field_Max
   };

   /// Do we want to send mail or post a news article?
   enum Mode
   {
      Mode_SMTP,
      Mode_NNTP
   };

   // a message kind - combined with the mode this determines the template
   // InitText() will use
   enum MessageKind
   {
      Message_New,      // either new mail message or new article
      Message_Reply,    // reply or follow-up
      Message_Forward   // this one is incompatible with Mode_NNTP
   };

   /** Constructor for posting news.
       @param parentProfile parent profile
       @param parent parent window
       @param hide if true, do not show frame
       @return pointer to the new compose view
    */
   static wxComposeView * CreateNewArticle(wxWindow *parent = NULL,
                                            ProfileBase *parentProfile = NULL,
                                            bool hide = false);

   /** Constructor for sending mail.
       @param parent parent window
       @param parentProfile parent profile
       @param hide if true, do not show frame
       @return pointer to the new compose view
    */
   static wxComposeView * CreateNewMessage(wxWindow *parent = NULL,
                                           ProfileBase *parentProfile = NULL,
                                           bool hide = false);


   /** Constructor for sending a reply to a message.

       @param parent parent window
       @param parentProfile parent profile
       @param hide if true, do not show frame
       @return pointer to the new compose view
    */
   static wxComposeView * CreateReplyMessage(wxWindow *parent,
                                             ProfileBase *parentProfile,
                                             bool hide = false);

   /** Constructor for forwarding a message.

       @param parent parent window
       @param parentProfile parent profile
       @param hide if true, do not show frame
       @return pointer to the new compose view
    */
   static wxComposeView * CreateFwdMessage(wxWindow *parent,
                                           ProfileBase *parentProfile,
                                           bool hide = false);

   /** Initializes the composer text: for example, if this is a reply, inserts
       the quoted contents of the message being replied to (except that, in
       fact, it may do whatever the user configured it to do using templates).
       The msg parameter may be NULL only for the new messages, messages
       created with CreateReply/FwdMessage require it to be !NULL.

       @param msg the message we're replying to or forwarding
    */
   void InitText(Message *msg = NULL);

   /// Destructor
   ~wxComposeView();

   /// is it ok to close now?
   virtual bool CanClose() const;

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
   void InsertData(char *data,
                   size_t length,
                   const char *mimetype = NULL,
                   const char *filename = NULL);

   /// move the cursor to the given position
   void MoveCursorTo(int x, int y);

   /** Sets the address fields, To:, CC: and BCC:.
       @param To primary address to send mail to
       @param CC carbon copy addresses
       @param BCC blind carbon copy addresses
   */
   void SetAddresses(const String &To,
                     const String &CC = "",
                     const String & = "");

   /** Set the newsgroups to post to.
       @param groups the list of newsgroups
   */
   void SetNewsgroups(const String &groups);

   /// sets Subject field
   void SetSubject(const String &subj);

   /// get the current value of a header field
   String GetHeaderValue(AddressField which) const
   {
      wxTextCtrl *text = m_txtFields[which];
      return text ? text->GetValue() : "";
   }

   /// inserts a text
   void InsertText(const String &txt);

   /// make a printout of input window
   void Print(void);

   /** Send the message.
       @return true if successful, false otherwise
   */
   bool Send(void);

   /** wxWindows callbacks
   */
   //@{
      /// called when text zone contents changes
   void OnTextChange(wxCommandEvent &event);

      /// called on Menu selection
   void OnMenuCommand(int id);

      /// for button
   void OnExpand(wxCommandEvent &event);

      /// called when external editor terminates
   void OnExtEditorTerm(wxProcessEvent& event);
   //@}

   // for wxAddressTextCtrl usage
   void SetLastAddressEntry(AddressField field) { m_fieldLast = field; }

   // for wxAddressTextCtrl usage:
   ProfileBase *GetProfile(void) const { return m_Profile; }

   /** Adds an extra header line.
       @param entry name of header entry
       @param value value of header entry
   */
   void AddHeaderEntry(const String &entry, const String &value);

   /// reset the "dirty" flag
   void ResetDirty();

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
               ProfileBase *parentProfile = NULL,
               bool hide = false);

   /** Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   wxComposeView(const String &iname, wxWindow *parent = NULL);

   // helpers
   // -------

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

private:
   /// a profile
   ProfileBase * m_Profile;
   /// the name of the class
   String m_name;

   /// the panel
   wxPanel *m_panel;

   /// compose menu
   wxMenu *composeMenu;


   /// the edit/cut menu item
   class wxMenuItem *m_MItemCut;
   /// the edit/copy menu item
   class wxMenuItem *m_MItemCopy;
   /// the edit/paste menu item
   class wxMenuItem *m_MItemPaste;

   /**@name Input fields (arranged into an array) */
   //@{
      /// The text fields
   AddressField m_fieldLast;            // which had the focus last time
   wxTextCtrl  *m_txtFields[Field_Max];

      /// the canvas for displaying the mail
   wxLayoutWindow *m_LayoutWindow;
      /// the alias expand button
   wxButton *aliasButton;
   //@}

   int m_font, m_size;
   wxColour m_fg, m_bg;
   /// the popup menu
   wxMenu *popupMenu;
   /**@name The interface to its canvas. */
   //@{
   /// the ComposeView canvas class
   friend class wxCVCanvas;
   //@}

   /// a list mapping IDs to filenames
   wxCVFileMapType fileMap;
   /// for assigning IDs
   unsigned long nextFileID;
   /// find the filename for an ID
   const char *LookupFileName(unsigned long id);

   /// makes the canvas
   void CreateFTCanvas(void);

   /// enable/disable editing of the message text
   inline void EnableEditing(bool enable);

   /// ids for different processes we may launch
   enum
   {
      HelperProcess_Editor = 1
   };

   /// News or smtp?
   Mode m_mode;
   /// New article, reply/follow-up or forward?
   MessageKind m_kind;

   /// Has message been sent already?
   bool m_sent;

   // external editor support
   // -----------------------

   wxProcess *m_procExtEdit;  // wxWin process notification helper
   String     m_tmpFileName;  // temporary file we passed to the editor
   int        m_pidEditor;    // pid of external editor (0 if none)

   DECLARE_EVENT_TABLE()
   DECLARE_CLASS(wxComposeView)
};

#endif
