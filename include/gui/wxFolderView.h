/*-*- c++ -*-********************************************************
 * wxFolderView.h: a window displaying a mail folder                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/06/05 16:56:56  VZ
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.3  1998/05/11 20:29:40  VZ
 * compiles under Windows again + option USE_WXCONFIG added
 *
 * Revision 1.2  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef  WXFOLDERVIEW_H
#define WXFOLDERVIEW_H

#ifdef __GNUG__
#pragma interface "wxFolderView.h"
#endif

#include "Mdefaults.h"

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
   void OnCommand(wxWindow &win, wxCommandEvent &ev);

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
class wxFolderView : public FolderViewBase , public wxMFrame
{
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
public:
   /** Constructor
       @param iMailFolder the MailFolder to display
       @param iname    the frame class name
       @param parent   the parent window
       @param ownsFolder  if true, wxFolderView will deallocate folder
   */
   wxFolderView(MailFolder *iMailFolder,
      const String &iname =
      String("wxFolderView"),
      wxFrame *parent = NULL,
      bool ownsFolder = true);
   /// Destructor
   ~wxFolderView();

   /// update it
   void  Update(void);

   /** called from OnSize(), rebuilds listbox
       @param x new width
       @param y new height
       */
   void  Build(int x, int y);
   
   /// return true if initialised
   bool  IsInitialised(void) const { return initialised; }

   /// called on Menu selection
   void OnMenuCommand(int id);

   /** Open some messages.
       @param n number of messages to open
       @messages pointer to an array holding the message numbers
   */
   void OpenMessages(int n, int *messages);

   /** Mark messages as deleted.
       @param n number of messages to delete
       @messages pointer to an array holding the message numbers
   */
   void DeleteMessages(int n, int *messages);

   /** Save messages to a file.
       
       @param n number of messages 
       @messages pointer to an array holding the message numbers
   */
   void SaveMessages(int n, int *messages);

   /** Reply to selected messages.
       
       @param n number of messages 
       @messages pointer to an array holding the message numbers
   */
   void ReplyMessages(int n, int *messages);

   /** Gets an array containing the positions of the selected
       strings. The number of selections is returned. 
       @param  selections Pass a pointer to an integer array, and do not deallocate the returned array.
       @return number of selections.
   */
   int   GetSelections(int **selections);


}; 

#endif
