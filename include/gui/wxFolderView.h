/*-*- c++ -*-********************************************************
 * wxFolderView.h: a window displaying a mail folder                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
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

class wxArrayInt;
class wxFolderViewPanel;
class wxFolderView;
class wxFolderListCtrl;
class wxMFrame;
class wxMessageView;
class MailFolder;

enum wxFolderListCtrlFields
{
   WXFLC_STATUS = 0,
   WXFLC_DATE,
   WXFLC_SIZE,
   WXFLC_FROM,
   WXFLC_SUBJECT,
   WXFLC_NUMENTRIES
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
   void  Update(void);

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
   void OpenMessages(wxArrayInt const &messages);

   /** Print messages.
       @param n number of messages to print
       @messages pointer to an array holding the message numbers
   */
   void PrintMessages(wxArrayInt const &messages);

   /** Print-preview messages.
       @param n number of messages to print preview
       @messages pointer to an array holding the message numbers
   */
   void PrintPreviewMessages(wxArrayInt const &messages);

   /** Save messages to a file.
       @param n number of messages
       @messages pointer to an array holding the message numbers
       @return true if all messages got saved
   */
   bool SaveMessagesToFile(wxArrayInt const &messages);

   /** Save messages to a folder.
       @param n number of messages
       @messages pointer to an array holding the message numbers
       @return true if all messages got saved
   */
   bool SaveMessagesToFolder(wxArrayInt const &messages);

   /** Returns false if no items are selected
   */
   bool HasSelection() const;

   /** Gets an array containing the uids of the selected messages.
       The number of selections is returned.
       @param  selections Pass a pointer to an integer array, and do not deallocate the returned array.
       @return number of selections.
   */
   int GetSelections(wxArrayInt &selections);

   /** Show a message in the preview window.
    */
   void PreviewMessage(long messageno);

   void SetSize(const int x, const int y, const int width, int height);

   /// return full folder name
   const String& GetFullName() { return m_folderName; }

   /// return the MWindow pointer:
   MWindow *GetWindow(void) const { return m_SplitterWindow; }
   /// return a profile pointer:
   ProfileBase *GetProfile(void) const { return m_Profile; }
   /// return pointer to folder
   MailFolder * GetFolder(void) const { return m_MailFolder; }

   /// process folder delete event
   virtual void OnFolderDeleteEvent(const String& folderName);
   /// update the folderview
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event);
   /// return profile name for persistent controls
   wxString const &GetFullName(void) const { return m_ProfileName; }
protected:
   /** Save messages to a folder.
       @param n number of messages
       @param file filename
       @messages pointer to an array holding the message numbers
   */
   void SaveMessages(wxArrayInt const &messages, String const &file);

private:
   /// profile name
   wxString m_ProfileName;
   /// first time constructor
   wxFolderView(wxWindow *parent);
   /// are we to deallocate the folder?
   bool ownsFolder;
   /// the mail folder being displayed
   class MailFolder  *m_MailFolder;
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
   /// its parent
   MWindow *m_Parent;
   /// either a listctrl or a treectrl
   wxFolderListCtrl *m_FolderCtrl;
   /// a splitter window
   wxSplitterWindow *m_SplitterWindow;
   /// the preview window
   wxMessageView *m_MessagePreview;
   /// semaphore to avoid duplicate calling of Update
   bool m_UpdateSemaphore;
   /// Profile:
   ProfileBase *m_Profile;
   /// full folder name
   String m_folderName;

   /// allow it to access m_MessagePreview;
   friend class wxFolderListCtrl;
   /// in deletion semaphore, ugly hack to avoid recursion in destructor
   bool m_InDeletion;
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

private:
   void InternalCreate(wxFolderView *fv, wxMFrame *parent = NULL);
   wxFolderViewFrame(String const &name, wxMFrame *parent);
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
#endif // 0

class wxFolderListCtrl : public wxPListCtrl
{
public:
   wxFolderListCtrl(wxWindow *parent, wxFolderView *fv);
   void Clear(void);
   void SetEntry(long index,String const &status, String const &sender, String
                 const &subject, String const &date, String const
                 &size);

   void Select(long index, bool on=true)
      { SetItemState(index,on ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED); }

   int GetSelections(wxArrayInt &selections) const;
   bool IsSelected(long index)
      { return GetItemState(index,wxLIST_STATE_SELECTED) != 0; }

   void OnSelected(wxListEvent& event);
   void OnKey( wxKeyEvent &event);

   void EnableSelectionCallbacks(bool enabledisable = true)
      { m_SelectionCallbacks = enabledisable; }

   void GrabFocus(wxMouseEvent &event)       { SetFocus(); }

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
   /// which entry is in column 0?
   int m_firstColumn;
   /// do we want OnSelect() callbacks?
   bool m_SelectionCallbacks;
};

#endif // WXFOLDERVIEW_H
