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

#include "Mdefaults.h"
#include "wxMFrame.h"

class wxFolderViewPanel;
class wxFolderView;

/** a wxWindows panel for the FolderView class */
class wxFolderViewPanel : public wxPanel
{
   /// the wxFolderView:
   class wxFolderView *folderView;
public:
   /** constructor
       @param parent the parent frame
   */
   wxFolderViewPanel(wxFolderView *parent); 

   /** gets called for events happening in the panel
       @param win the window
   */
#ifdef USE_WXWINDOWS2
#else  // wxWin1
   void OnCommand(wxWindow &win, wxCommandEvent &ev);
#endif // wxWin1/2

   void OnDefaultAction(wxItem *item);
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
class wxFolderView : public FolderViewBase , public wxPanel
{
public:
   /** Constructor
       @param folderName the name of the folder
       @param parent   the parent window
   */
   wxFolderView(String const & folderName,
                wxFrame *parent = NULL);
   /// Destructor
   ~wxFolderView();

   /// update it
   void  Update(void);

   /// return true if initialised
   bool  IsInitialised(void) const { return initialised; }

   /// called on Menu selection
   virtual void OnMenuCommand(int id);


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

   /** Gets an array containing the positions of the selected
       strings. The number of selections is returned. 
       @param  selections Pass a pointer to an integer array, and do not deallocate the returned array.
       @return number of selections.
   */
   int   GetSelections(wxArrayInt &selections);

private:
   /// is initialised?
   bool initialised;
   /// are we to deallocate the folder?
   bool ownsFolder;
   /// the mail folder being displayed
   class MailFolder  *mailFolder;
   /// a panel to fill the frame
   wxFolderViewPanel *panel;
   /// the listbox with the mail messages
   wxListBox   *listBox;
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
   wxWindow *parent;
}; 

class wxFolderViewFrame : public wxMFrame
{
public:
   wxFolderViewFrame(const String &iname, wxFrame *parent = NULL);
   void OnMenuCommand(int id);
#ifdef USE_WXWINDOWS2
      void OnSize( wxSizeEvent &WXUNUSED(event) );
#endif
private:
   wxFolderView *m_FolderView;
   DECLARE_EVENT_TABLE() 
};

#endif
