///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   gui/wxFolderView.h - wxFolderView and related classes
// Purpose:     wxFolderView is used to show to the user folder contents
// Author:      Karsten Ballüder (Ballueder@gmx.net)
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef WXFOLDERVIEW_H
#define WXFOLDERVIEW_H

#ifndef USE_PCH
#  include "Mdefaults.h"
#  include "gui/wxMFrame.h"
#endif // USE_PCH

#include "FolderView.h"
#include "MEvent.h"
#include "Mpers.h"

#include <wx/persctrl.h>
#include <wx/splitter.h>

class wxFolderViewPanel;
class wxFolderView;
class wxFolderListCtrl;
class wxFolderMsgWindow;
class MailFolder;
class MessageView;
class MsgCmdProc;
class ASMailFolder;
class ASTicketList;
class HeaderInfoList_obj;
class FolderViewAsyncStatus;

enum wxFolderListColumn
{
   WXFLC_STATUS,
   WXFLC_DATE,
   WXFLC_SIZE,
   WXFLC_FROM,
   WXFLC_SUBJECT,
   WXFLC_MSGNO,
   WXFLC_NUMENTRIES,
   WXFLC_NONE = WXFLC_NUMENTRIES // must be equal for GetColumnByIndex()
};

// ----------------------------------------------------------------------------
// wxFolderView: allows the user to view messages in the folder
// ----------------------------------------------------------------------------

class wxFolderView : public FolderView
{
public:
   /** Constructor
       @param parent   the parent window
   */
   static wxFolderView *Create(wxWindow *parent = NULL);

   /// first time constructor
   wxFolderView(wxWindow *parent);

   /// Destructor
   virtual ~wxFolderView();

   virtual bool GoToMessage(MsgnoType msgno);

   virtual bool MoveToNextUnread(bool takeNextIfNoUnread = true);

   /** Set the associated folder.
       @param folder the folder to display or NULL
   */
   virtual void SetFolder(MailFolder *mf);

   /** Open the specified folder
       @param folder the folder to open
       @return true if opened ok, false otherwise
    */
   virtual bool OpenFolder(MFolder *folder, bool readonly = false);

   /** Open some messages.
       @param messages array holding the message numbers
   */
   void OpenMessages(UIdArray const &messages);

   /**
       Expunge messages
    */
   void ExpungeMessages();

   /**
       Mark messages read/unread
       @param read whether message should be marked read (true) or unread
    */
   Ticket MarkRead(const UIdArray& messages, bool read);

   /** For use by the listctrl: get last previewed uid: */
   UIdType GetPreviewUId(void) const;

   /**
      Are we previewing anything?

      Note that this can be called very early, possibly even before
      m_FolderCtrl has been created yet, so check for it.
    */
   bool HasPreview() const
      { return m_FolderCtrl && GetPreviewUId() != UID_ILLEGAL; }

   /** Returns false if no items are selected
    */
   bool HasSelection() const;

   /** Gets an array containing the uids of the selected messages. If there is
       no selection, the array will contain the focused UID (if any)

       @return the array of selections, may be empty
    */
   UIdArray GetSelections() const;

   /** Get either the the currently focused message
   */
   UIdType GetFocus() const;

   /// [de]select all items
   void SelectAll(bool on = true);

   /// select all messages by some status/flag
   void SelectAllByStatus(MailFolder::MessageStatus status, bool isSet = true);

   /// return the parent window
   wxWindow *GetWindow(void) const { return m_SplitterWindow; }

   /// create the "View" menu for the frame
   void CreateViewMenu();

   /// event processing
   virtual bool OnMEvent(MEventData& event)
   {
      if ( event.GetId() == MEventId_OptionsChange )
      {
         OnOptionsChange((MEventOptionsChangeData &)event);

         return TRUE;
      }

      return FolderView::OnMEvent(event);
   }

   /// process folder delete event
   virtual void OnFolderDeleteEvent(const String& folderName);
   /// update the folderview
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event);
   /// update the folderview
   virtual void OnFolderExpungeEvent(MEventFolderExpungeData &event);
   /// close the folder
   virtual void OnFolderClosedEvent(MEventFolderClosedData &event);
   /// update the folderview
   virtual void OnMsgStatusEvent(MEventMsgStatusData &event);
   /// the derived class should react to the result to an asynch operation
   virtual void OnASFolderResultEvent(MEventASFolderResultData &event);

   /// called when our message viewer changes
   virtual void OnMsgViewerChange(wxWindow *viewerNew);

   /// return profile name for persistent controls
   const wxString& GetFullName(void) const { return m_fullname; }

   /// for use by the listctrl:
   ASTicketList *GetTicketList(void) const { return m_TicketList; }

   /// for use by the listctrl only:
   bool GetFocusFollowMode(void) const { return m_settings.focusOnMouse; }

   /// get the parent frame of the folder view
   wxFrame *GetParentFrame() const { return m_Frame; }

   /**
      @name Event handlers called from elsewhere.
    */
   //@{

   /// General callback, forwards to DoCommandEvent()
   void OnCommandEvent(wxCommandEvent &event);

   /// process a keyboard event in the list control, return true if processed
   bool HandleFolderViewCharEvent(wxKeyEvent& event);

   /// process a keyboard event in the message view part
   bool HandleMsgViewCharEvent(wxKeyEvent& event);

   //@}

protected:
   /// update the view after new messages appeared in the folder
   void Update();

   /// forget the currently shown folder (keep the viewer if necessary though)
   void DoClear(bool keepTheViewer);

   /// call DoClear() but via SetFolder() which allows overriding it
   void Clear() { SetFolder(NULL); }

   /// set the folder to show, can't be NULL (unlike in SetFolder)
   void ShowFolder(MailFolder *mf);

   /// Show a message in the preview window: only called by wxFolderListCtrl!
   void PreviewMessage(long uid);

   /// select the first interesting message in the folder
   void SelectInitialMessage();

   /// select the next unread message, return false if no more
   bool SelectNextUnread(bool takeNextIfNoUnread = true);

   /// called to process messages from the sorting/threading popup menu
   void OnHeaderPopupMenu(int cmd);

   /// called when the focused (== current) item in the listctrl changes
   void OnFocusChange(long item, UIdType uid);

   /// get the number of the messages we show
   inline size_t GetHeadersCount() const;

   /** get the number of deleted messages: always use this, never access
       m_nDeleted directly as it is calculated on demand
    */
   unsigned long GetDeletedCount() const;

   /**
      @name Event handling implementation.

      Most of the events are forwarded to m_msgCmdProc, but some are handled
      here.
    */
   //@{

   /// Called from OnCommandEvent() and HandleFolderViewCharEvent()
   void DoCommandEvent(int cmd);

   /// Move to the next message after processing a command if necessary
   void UpdateFocusAfterCommand(int cmd);

   //@}

private:
   /// show the next or previous message matching the search criteria
   void MoveToNextSearchMatch(bool forward);

   /// the full name of the folder opened in this folder view
   wxString m_fullname;

   /// number of deleted messages in the folder
   unsigned long m_nDeleted;

   /// the parent window
   wxWindow *m_Parent;

   /// and the parent frame
   wxFrame *m_Frame;

   /// a splitter window: it contains m_FolderCtrl and m_MessageWindow
   wxSplitterWindow *m_SplitterWindow;

   /// the control showing the message headers
   wxFolderListCtrl *m_FolderCtrl;

   /// container window for the message viewer (it changes, we don't)
   wxFolderMsgWindow *m_MessageWindow;

   /// the preview window
   MessageView *m_MessagePreview;

   /// the command processor object
   MsgCmdProc *m_msgCmdProc;

   /// a list of pending tickets from async operations
   ASTicketList *m_TicketList;

   /// the data we store in the profile
   struct AllProfileSettings
   {
      AllProfileSettings();

      // default copy ctor is ok for now, add one if needed later!

      bool operator==(const AllProfileSettings& other) const;
      bool operator!=(const AllProfileSettings& other) const
         { return !(*this == other); }

      /// the strftime(3) format for date
      String dateFormat;
      /// TRUE => display time/date in GMT
      bool dateGMT;

      /// the folder view control colours
      wxColour BgCol,         // background (same for all messages)
               FgCol,         // normal text colour
               NewCol,        // text colour for new messages
               FlaggedCol,    //                 flagged
               RecentCol,     //                 recent
               DeletedCol,    //                 deleted
               UnreadCol;     //                 unseen

      /// font description
      String font;
      /// font family and size if font is empty
      int fontFamily,
          fontSize;

      /// do we want to preview messages when activated?
      bool previewOnSingleClick;

      /// delay between selecting a message and previewing it
      unsigned long previewDelay;

      /// strip e-mail address from sender and display only name?
      bool senderOnlyNames;

      /// replace "From" with "To" for messages sent from oneself?
      bool replaceFromWithTo;

      /// all the addresses corresponding to "oneself"
      wxArrayString returnAddresses;

      /// the order of columns
      int columns[WXFLC_NUMENTRIES];

      /// how to show the size
      long /* MessageSizeShow */ showSize;

      /// do we have focus-follow enabled?
      bool focusOnMouse;

      /// do we automatically go to the next unread message?
      bool autoNextUnread;

      /// do we use trash or delete in place?
      bool usingTrash;

      /// do we show the details of the current message in the status bar?
      bool updateStatus;

      /// the format of the status messages (only if updateStatus)
      String statusFormat;

      /// is the folder view split vertically (or horizontally)?
      bool splitVert;

      /// is the folder view on top/to the lft of msg view (or bottom/right)?
      bool folderOnTop;
   } m_settings;

   /// the last message we the user has jumped to
   long m_nLastJump;

   /// the search parameters
   struct SearchData
   {
      /// what we searched for
      String str;

      /// the UIDs of the messages we found during last search
      UIdArray uids;

      /// the index of the next matching message to show (used by
      /// MoveToNextSearchMatch())
      size_t idx;

      /// true if we are doing fwd sesrch ('/'), false if backwards ('?')
      bool forward;

      /// true if the new search has just been started, reset by first call to
      /// MoveToNextSearchMatch()
      bool justStarted;

      /// should be called to initialize with the search results data
      void Init(const UIdArray& uidsSearched)
      {
         uids = uidsSearched;
         idx = forward ? 0 : uidsSearched.GetCount() - 1;
         justStarted = true;
      }
   } m_searchData;


   /// read the values from the profile into AllProfileSettings structure
   void ReadProfileSettings(AllProfileSettings *settings);

   /// tell the list ctrl to use the new options
   void ApplyOptions();

   /// handler of options change event, refreshes the view if needed
   void OnOptionsChange(MEventOptionsChangeData& event);

   /// MEventManager reg info
   void *m_regOptionsChange;

   // allow it to access m_MessagePreview;
   friend class wxFolderListCtrl;

   // allow it to call Update()
   friend class FolderViewMessagesDropWhere;
};

// ----------------------------------------------------------------------------
// wxFolderViewFrame: a frame containing just a folder view
// ----------------------------------------------------------------------------

class wxFolderViewFrame : public wxMFrame
{
public:
   /* Opens a FolderView for a mail folder defined by a profile entry.
      @param folder folder to open in the folder view
      @parent parent window (use top level frame if NULL)
      @return pointer to FolderViewFrame or NULL
   */
   static wxFolderViewFrame *Create(MFolder *folder,
                                    wxMFrame *parent = NULL,
                                    MailFolder::OpenMode openmode =
                                       MailFolder::Normal);

   /// dtor
   ~wxFolderViewFrame();

   /**
      This virtual method returns a pointer to the profile of the mailfolder
      being displayed, for those wxMFrames which have a folder displayed or the
      global application profile for the other ones. Used to make the compose
      view inherit the current folder's settings.

      @return profile pointer, the caller must DecRef() it
   */
   virtual Profile *GetFolderProfile(void) const;

protected:
   // event processing
   void OnCommandEvent(wxCommandEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   // implement base class pure virtual methods
   virtual void DoCreateToolBar();
   virtual void DoCreateStatusBar();

   void InternalCreate(wxFolderView *fv, wxMFrame *parent = NULL);

   /// ctor
   wxFolderViewFrame(String const &name, wxMFrame *parent);

   /// the associated folder view
   wxFolderView *m_FolderView;

   DECLARE_ABSTRACT_CLASS(wxFolderViewFrame)
   DECLARE_NO_COPY_CLASS(wxFolderViewFrame)
   DECLARE_EVENT_TABLE()
};


#endif // WXFOLDERVIEW_H
