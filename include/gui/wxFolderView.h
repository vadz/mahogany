/*-*- c++ -*-********************************************************
 * wxFolderView.h: a window displaying a mail folder                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 *******************************************************************/
#ifndef  WXFOLDERVIEW_H
#define WXFOLDERVIEW_H

#ifdef __GNUG__
#pragma interface "wxFolderView.h"
#endif

#include   "Mdefaults.h"
#include   "wxMFrame.h"
#include   "MailFolder.h"
#include   "wxMessageView.h"
#include   <wx/listctrl.h>
#include   <wx/dynarray.h>

class wxFolderViewPanel;
class wxFolderView;
class wxFolderListCtrl;
class wxMFrame;
class wxMessageView;


enum wxFolderListCtrlFields
{
   WXFLC_STATUS = 0, WXFLC_DATE, WXFLC_SIZE, WXFLC_FROM,
   WXFLC_SUBJECT, WXFLC_NUMENTRIES
};

/** a timer class for the FolderView */
class wxFVTimer : public wxTimer
{
protected:
   /// the mailfolder to update
   MailFolder  *mf;
public:
   /** constructor
       @param imf the mailfolder to query on timeout
   */
   wxFVTimer(MailFolder *imf)
      {
    mf = imf;
    Start(mf->GetProfile()->readEntry(MP_UPDATEINTERVAL,MP_UPDATEINTERVAL_D)*1000);
      }
   /// get called on timeout and pings the mailfolder
   void Notify(void)
      {
    mf->Ping();
      }
};

/** a wxWindows FolderView class */
class wxFolderView : public FolderViewBase 
{
public:
   /** Constructor
       @param folderName the name of the folder
       @param parent   the parent window
   */
   wxFolderView(String const & folderName,
                MWindow *parent = NULL);
   /// Destructor
   ~wxFolderView();

   /// update it
   void  Update(void);

   /// return true if initialised
   bool  IsInitialised(void) const { return initialised; }

   /// called on Menu selection
   void OnCommandEvent(wxCommandEvent &event);


   /** Open some messages.
       @param n number of messages to open
       @messages pointer to an array holding the message numbers
   */
   void OpenMessages(wxArrayInt const &messages);

   /** Mark messages as deleted.
       @param n number of messages to delete
       @messages pointer to an array holding the message numbers
   */
   void DeleteMessages(wxArrayInt const &messages);

   /** Save messages to a file.
       
       @param n number of messages 
       @messages pointer to an array holding the message numbers
   */
   void SaveMessages(wxArrayInt const &messages);

   /** Reply to selected messages.
       
       @param n number of messages 
       @messages pointer to an array holding the message numbers
   */
   void ReplyMessages(wxArrayInt const &messages);

   /** Forward selected messages.
       
       @param n number of messages 
       @messages pointer to an array holding the message numbers
   */
   void ForwardMessages(wxArrayInt const &messages);

   /** Gets an array containing the positions of the selected
       strings. The number of selections is returned. 
       @param  selections Pass a pointer to an integer array, and do not deallocate the returned array.
       @return number of selections.
   */
   int   GetSelections(wxArrayInt &selections);

   /** Show a message in the preview window.
    */
   void PreviewMessage(long messageno)
      { m_MessagePreview->ShowMessage(mailFolder,messageno+1); }
   void SetSize(const int x, const int y, const int width, int height);
   /// return the MWindow pointer:
   MWindow *GetWindow(void) { return m_SplitterWindow; }
   /// return a profile pointer:
   ProfileBase *GetProfile(void)
      { return mailFolder ? mailFolder->GetProfile() : NULL; }
private:
   /// is initialised?
   bool initialised;
   /// are we to deallocate the folder?
   bool ownsFolder;
   /// the mail folder being displayed
   class MailFolder  *mailFolder;
   /// the number of messages in the folder when last updated
   int m_NumOfMessages;
   /// the number of messages in box
   long  listBoxEntriesCount;
   /// the array to hold the strings for the listbox
   char  **listBoxEntries;
   /// width of window
   int   width;
   /// height of window
   int height;
   /// a timer to update information
   wxFVTimer   *timer;
   /// its parent
   MWindow *parent;
   /// either a listctrl or a treectrl
   wxFolderListCtrl *m_FolderCtrl;
   /// a splitter window
   wxSplitterWindow *m_SplitterWindow;
   /// the preview window
   wxMessageView *m_MessagePreview;
   /// semaphore to avoid duplicate calling of Update
   bool m_UpdateSemaphore;
}; 

class wxFolderViewFrame : public wxMFrame
{
public:
   wxFolderViewFrame(const String &iname, wxFrame *parent = NULL);
   void OnCommandEvent(wxCommandEvent & event);
   void OnSize( wxSizeEvent &WXUNUSED(event) );
private:
   wxFolderView *m_FolderView;
   DECLARE_EVENT_TABLE() 
};

#if 0
// abc to define interface for listctrl/treectrl interaction

class wxFolderBaseCtrl
{
public:
   virtual void Clear(void) = 0;
   virtual void AddEntry(String const &status, String const &sender, String
                         const &subject, String const &date, String
                         const &size) = 0;
   virtual int GetSelections(wxArrayInt &selections) const = 0;
   virtual ~wxFolderBaseCtrl() {}
   virtual wxControl *GetControl(void) const = 0;;
protected:
   /// parent window
   wxWindow *m_Parent;
};
#endif

class wxFolderListCtrl : public wxListCtrl
{
public:
   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   void Clear(void);
   void SetEntry(long index,String const &status, String const &sender, String
                 const &subject, String const &date, String const
                 &size);

   void Select(long index, bool on=true)
      { SetItemState(index,wxLIST_STATE_SELECTED, on ? wxLIST_STATE_SELECTED : 0); }
      
   int GetSelections(wxArrayInt &selections) const;
   bool IsSelected(long index)
      { return GetItemState(index,wxLIST_STATE_SELECTED); }
   void OnSelected(wxListEvent& event);
   void OnSize( wxSizeEvent &event );

   DECLARE_EVENT_TABLE()

protected:
   long m_Style;
   long m_NextIndex;
   /// parent window
   wxWindow *m_Parent;
   /// the folder view
   wxFolderView *m_FolderView;
   /// column numbers
   int m_columns[WXFLC_NUMENTRIES];
   /// column widths
   int m_columnWidths[WXFLC_NUMENTRIES];
   /// which entry is in column 0?
   int m_firstColumn;
};

#endif
