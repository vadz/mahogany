/*-*- c++ -*-********************************************************
 * wxFolderView.h: a window displaying a mail folder                *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$          *
 *******************************************************************/

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

#include <wx/persctrl.h>
#include <wx/splitter.h>

class wxFolderViewPanel;
class wxFolderView;
class wxFolderListCtrl;
class wxMFrame;
class wxMessageView;
class MailFolder;
class ASMailFolder;

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


/** a wxWindows FolderView class */
class wxFolderView : public FolderView
{
public:
   /** Constructor
       @param parent   the parent window
   */
   static wxFolderView *Create(MWindow *parent = NULL);

   /// Destructor
   ~wxFolderView();

   /// update it
   void  Update(class HeaderInfoList *listing = NULL);

   /** Set the associated folder.
       @param folder the folder to display or NULL
   */
   void SetFolder(MailFolder *mf, bool recreateFolderCtrl = TRUE);

   /** Open folder from profile and display.
       @param profilename the name of the folder profile
       @return pointer to the folder or NULL
   */
   MailFolder * OpenFolder(String const &profilename);

   /// called on Menu selection
   void OnCommandEvent(wxCommandEvent &event);


   /** Open some messages.
       @param n number of messages to open
       @messages pointer to an array holding the message numbers
   */
   void OpenMessages(UIdArray const &messages);

   /** Print messages.
       @param n number of messages to print
       @messages pointer to an array holding the message numbers
   */
   void PrintMessages(UIdArray const &messages);

   /** Print-preview messages.
       @param n number of messages to print preview
       @messages pointer to an array holding the message numbers
   */
   void PrintPreviewMessages(UIdArray const &messages);

   /** For use by the listctrl: get last previewed uid: */
   UIdType GetPreviewUId(void) const { return m_previewUId; }
   /** Save messages to a file.
       @param n number of messages
       @messages pointer to an array holding the message numbers
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

   /** Returns false if no items are selected
   */
   bool HasSelection() const;

   /** Gets an array containing the uids of the selected messages.
       The number of selections is returned.
       @param  selections Pass a pointer to an integer array, and do not deallocate the returned array.
       @return number of selections.
   */
   int GetSelections(UIdArray &selections);

   /** Show a message in the preview window.
    */
   void PreviewMessage(long messageno);

   void SetSize(const int x, const int y, const int width, int height);

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
   /// update the frame title
   virtual void OnFolderStatusEvent(MEventFolderStatusData &event);
   /// update the folderview
   virtual void OnMsgStatusEvent(MEventMsgStatusData &event);
   /// the derived class should react to the result to an asynch operation
   virtual void OnASFolderResultEvent(MEventASFolderResultData &event);
   /// return profile name for persistent controls
   wxString const &GetFullName(void) const { return m_ProfileName; }
   /// for use by the listctrl:
   class ASTicketList *GetTicketList(void) const { return m_TicketList; }
   /// for use by the listctrl only:
   bool GetFocusFollowMode(void) const { return m_FocusFollowMode; }

   /// Update the idea of which messages are selected.
   void UpdateSelectionInfo(void);

   /// called by wxFolderListCtrl to drag abd drop messages
   bool DragAndDropMessages();

protected:
   /** Save messages to a folder.
       @param n number of messages
       @param file filename
       @messages pointer to an array holding the message numbers
   */
   void SaveMessages(UIdArray const &messages, String const &file);

   /// The UIds of the last selected messages.
   UIdArray m_SelectedUIds;
   /// The last focused UId.
   UIdType  m_FocusedUId;
private:
   /// profile name
   wxString m_ProfileName;
   /// first time constructor
   wxFolderView(wxWindow *parent);
   /// are we to deallocate the folder?
   bool ownsFolder;
   /// the number of messages in the folder when last updated
   int m_NumOfMessages;
   /// the number of messages in box
   long  listBoxEntriesCount;
   /// the array to hold the strings for the listbox
   char  **listBoxEntries;
   /// width of window
   int width;
   /// height of window
   int height;
   /// its parent
   MWindow *m_Parent;
   /// either a listctrl or a treectrl
   wxFolderListCtrl *m_FolderCtrl;
   /// a splitter window
   wxSplitterWindow *m_SplitterWindow;
   /// the preview window
   wxMessageView *m_MessagePreview;
   /// UId of last previewed message
   UIdType m_previewUId;
   /// semaphore to avoid duplicate calling of Update
   bool m_UpdateSemaphore;
   /// semaphore to avoid recursion in SetFolder()
   bool m_SetFolderSemaphore;
   /// allow it to access m_MessagePreview;
   friend class wxFolderListCtrl;
   /// in deletion semaphore, ugly hack to avoid recursion in destructor
   bool m_InDeletion;

   /// a list of pending tickets from async operations
   class ASTicketList *m_TicketList;
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

      bool operator==(const AllProfileSettings& other) const
      {
         return dateFormat == other.dateFormat &&
                dateGMT == other.dateGMT &&
                senderOnlyNames == other.senderOnlyNames &&
                replaceFromWithTo == other.replaceFromWithTo &&
                // returnAddresses == other.returnAddresses &&
                memcmp(columns, other.columns, sizeof(columns)) == 0;
      }

      bool operator!=(const AllProfileSettings& other) const
         { return !(*this == other); }

      /// the strftime(3) format for date
      String dateFormat;
      /// TRUE => display time/date in GMT
      bool dateGMT;
      /// Background and foreground colours
      wxColour BgCol, FgCol, NewCol, RecentCol, DeletedCol, UnreadCol;
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
   } m_settings;

   /// read the values from the profile into AllProfileSettings structure
   void ReadProfileSettings(AllProfileSettings *settings);

private:
   void OnOptionsChange(MEventOptionsChangeData& event);

   void SetEntry(HeaderInfoList *hi, size_t idx);

   // MEventManager reg info
   void *m_regOptionsChange;
};


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
   void OnSize(wxSizeEvent& event);
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
   wxFolderViewFrame(void) {ASSERT(0);}
   DECLARE_DYNAMIC_CLASS(wxFolderViewFrame)
private:
   void InternalCreate(wxFolderView *fv, wxMFrame *parent = NULL);
   wxFolderViewFrame(String const &name, wxMFrame *parent);
   wxFolderView *m_FolderView;

   DECLARE_EVENT_TABLE()
};


#endif // WXFOLDERVIEW_H
