///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany
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

#ifdef __GNUG__
#pragma interface "wxFolderView.h"
#endif

#include "Mdefaults.h"
#include "wxMFrame.h"
#include "FolderView.h"
#include "wxMessageView.h"
#include "MEvent.h"
#include "Mpers.h"

#include <wx/persctrl.h>
#include <wx/splitter.h>

class wxFolderViewPanel;
class wxFolderView;
class wxFolderListCtrl;
class wxMFrame;
class wxMessageView;
class MailFolder;
class ASMailFolder;
class ASTicketList;

enum wxFolderListCtrlFields
{
   WXFLC_STATUS = 0,
   WXFLC_DATE,
   WXFLC_SIZE,
   WXFLC_FROM,
   WXFLC_SUBJECT,
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
   static wxFolderView *Create(MWindow *parent = NULL);

   /// first time constructor
   wxFolderView(wxWindow *parent);

   /// Destructor
   virtual ~wxFolderView();

   /** Set the associated folder.
       @param folder the folder to display or NULL
   */
   virtual void SetFolder(MailFolder *mf, bool recreateFolderCtrl = TRUE);

   /** Open folder from profile and display.
       @param profilename the name of the folder profile
       @return pointer to the folder or NULL
   */
   MailFolder * OpenFolder(String const &profilename);

   /// called on Menu selection
   void OnCommandEvent(wxCommandEvent &event);


   /** Open some messages.
       @param messages array holding the message numbers
   */
   void OpenMessages(UIdArray const &messages);

   /** Print messages.
       @param messages array holding the message numbers
   */
   void PrintMessages(UIdArray const &messages);

   /** Print-preview messages.
       @param message array holding the message numbers
   */
   void PrintPreviewMessages(UIdArray const &messages);

   /** For use by the listctrl: get last previewed uid: */
   UIdType GetPreviewUId(void) const { return m_uidPreviewed; }

   /** Are we previewing anything? */
   bool HasPreview() const { return GetPreviewUId() != UID_ILLEGAL; }

   /** Save messages to a file.
       @param pointer array holding the message numbers
   */
   void SaveMessagesToFile(UIdArray const &messages);

   /** Save messages to a folder.
       @param n number of messages
       @param messages pointer to an array holding the message numbers
       @param folder is the folder to save to, ask the user if NULL
       @param del if TRUE, delete them when they are saved
       @return the ticket for the save operation
   */
   Ticket SaveMessagesToFolder(UIdArray const &messages,
                               MFolder *folder = NULL,
                               bool del = FALSE);

   /** Save messages to a folder and possibly delete them later.
       This function should be used by drop targets accepting the message
       objects as the folder view must know whether it has to delete the
       messages or not after saving (depending on whether copy or move was
       done)

       @param n number of messages
       @param messages pointer to an array holding the message numbers
       @param folder is the folder to save to, ask the user if NULL
       @param del if TRUE, delete them when they are saved
   */
   void DropMessagesToFolder(UIdArray const &messages, MFolder *folder);

   /** Delete messages
    */
   void DeleteOrTrashMessages(const UIdArray& messages);

   /**
       Expunge messages
    */
   void ExpungeMessages();

   /** Toggle the "flagged" status of the messages.
       @param pointer to an array holding the message numbers
   */
   void ToggleMessages(UIdArray const &messages);

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

   /// Show a message in the preview window.
   void PreviewMessage(long uid);

   /// Show the raw text of the specified message
   void ShowRawText(long uid);

   /// [de]select all items
   void SelectAll(bool on = true);

   /// select all unread messages
   void SelectAllUnread();

   /// called by wxFolderListCtrl to drag abd drop messages
   bool DragAndDropMessages();

   /// return the MWindow pointer:
   MWindow *GetWindow(void) const { return m_SplitterWindow; }

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

   /// Search messages for certain criteria.
   virtual void SearchMessages(void);

   /// process folder delete event
   virtual void OnFolderDeleteEvent(const String& folderName);
   /// update the folderview
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event);
   /// update the folderview
   virtual void OnFolderStatusEvent(MEventFolderStatusData &event);
   /// update the folderview
   virtual void OnFolderExpungeEvent(MEventFolderExpungeData &event);
   /// close the folder
   virtual void OnFolderClosedEvent(MEventFolderClosedData &event);
   /// update the folderview
   virtual void OnMsgStatusEvent(MEventMsgStatusData &event);
   /// the derived class should react to the result to an asynch operation
   virtual void OnASFolderResultEvent(MEventASFolderResultData &event);
   /// return profile name for persistent controls
   wxString const &GetFullName(void) const { return m_ProfileName; }
   /// for use by the listctrl:
   ASTicketList *GetTicketList(void) const { return m_TicketList; }
   /// for use by the listctrl only:
   bool GetFocusFollowMode(void) const { return m_FocusFollowMode; }

   /// called when the focused (== current) item in the listctrl changes
   void OnFocusChange(void);

protected:
   /** Save messages to a folder.
       @param messages array holding the message numbers
       @param file filename
   */
   void SaveMessages(UIdArray const &messages, String const &file);

   /// update the view after new messages appeared in the folder
   void Update();

   /// set the currently previewed UID
   void SetPreviewUID(UIdType uid);

   /// invalidate the last previewed UID
   void InvalidatePreviewUID() { SetPreviewUID(UID_ILLEGAL); }

private:
   /// profile name
   wxString m_ProfileName;

   /// the number of messages in the folder when last updated
   unsigned long m_NumOfMessages;
   /// number of deleted messages in the folder
   unsigned long m_nDeleted;

   /// its parent
   MWindow *m_Parent;
   /// and the parent frame
   MFrame *m_Frame;

   /// either a listctrl or a treectrl
   wxFolderListCtrl *m_FolderCtrl;
   /// a splitter window
   wxSplitterWindow *m_SplitterWindow;
   /// the preview window
   wxMessageView *m_MessagePreview;

   /// UId of last previewed message (may be UID_ILLEGAL)
   UIdType m_uidPreviewed;

   /// UId of the focused message, may be different from m_uidPreviewed!
   UIdType m_uidFocused;

   /// semaphore to avoid duplicate calling of Update
   bool m_UpdateSemaphore;
   /// semaphore to avoid recursion in SetFolder()
   bool m_SetFolderSemaphore;
   /// in deletion semaphore, ugly hack to avoid recursion in destructor
   bool m_InDeletion;

   /// a list of pending tickets from async operations
   ASTicketList *m_TicketList;
   /// a list of tickets we should delete if copy operation succeeded
   ASTicketList *m_TicketsToDeleteList;

   // drag and dropping messages is complicated because all operations
   // (message saving, deletion *and* DoDragDrop() call) are async, so
   // everything may happen in any order, yet we should never delete the
   // messages which couldn't be copied successfully. To deal with this we
   // maintain the following lists:
   //  * m_TicketsDroppedList which contains all tickets created by
   //    DropMessagesToFolder()
   //  * m_UIdsCopiedOk which contains messages from tickets of
   //    m_TicketsDroppedList which have been already saved successfully.

   /// a list of tickets which we _might_ have to delete
   ASTicketList *m_TicketsDroppedList;

   /// a list of UIDs we might have to delete
   UIdArray m_UIdsCopiedOk;

   /// do we have focus-follow enabled?
   bool m_FocusFollowMode;

   /// the data we store in the profile
   struct AllProfileSettings
   {
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
      /// font attributes
      int font, size;
      /// do we want to preview messages when activated?
      bool previewOnSingleClick;
      /// strip e-mail address from sender and display only name?
      bool senderOnlyNames;
      /// replace "From" with "To" for messages sent from oneself?
      bool replaceFromWithTo;
      /// all the addresses corresponding to "oneself"
      wxArrayString returnAddresses;
      /// the order of columns
      int columns[WXFLC_NUMENTRIES];
      /// how to show the size
      MessageSizeShow showSize;
   } m_settings;

   /// read the values from the profile into AllProfileSettings structure
   void ReadProfileSettings(AllProfileSettings *settings);

   /// get the full key to use in persistent message boxes
   String GetFullPersistentKey(MPersMsgBox key);

private:
   /// handler of options change event, refreshes the view if needed
   void OnOptionsChange(MEventOptionsChangeData& event);

   /// adds the entry to the list control
   void AddEntry(const HeaderInfo *hi);

   /// set the entry colour to correspond to its status
   void SetEntryColour(size_t index, const HeaderInfo *hi);

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
      @param profileName name of the profile
      @parent parent window
      @return pointer to FolderViewFrame or NULL
   */
   static wxFolderViewFrame * Create(const String &profileName, wxMFrame *parent = NULL);

   /* Opens a FolderView for an already opened mail folder.
      @param mf the mail folder to display
      @parent parent window
      @return pointer to FolderViewFrame or NULL
   */
   static wxFolderViewFrame * Create(MailFolder *mf, wxMFrame *parent = NULL);
   ~wxFolderViewFrame();

   // callbacks
   void OnCommandEvent(wxCommandEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);

   /** This virtual method returns either NULL or a (not incref'd)
       pointer to the profile of the mailfolder being displayed, for
       those wxMFrames which have a folder displayed. Used to make the
       compose view inherit the current folder's settings.
   */
   virtual Profile *GetFolderProfile(void)
      {
         return m_FolderView ?
            m_FolderView->GetProfile() : NULL;
      }

   /// don't even think of using this!
   wxFolderViewFrame(void) { wxFAIL_MSG("unreachable"); }

private:
   void InternalCreate(wxFolderView *fv, wxMFrame *parent = NULL);
   wxFolderViewFrame(String const &name, wxMFrame *parent);
   wxFolderView *m_FolderView;

   DECLARE_DYNAMIC_CLASS(wxFolderViewFrame)
   DECLARE_EVENT_TABLE()
};


#endif // WXFOLDERVIEW_H
