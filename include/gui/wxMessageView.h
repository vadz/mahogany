/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXMESSAGEVIEW_H
#define WXMESSAGEVIEW_H

#ifdef __GNUG__
#   pragma interface "wxMessageView.h"
#endif

#include "FolderType.h"
#include "MessageView.h"

#include "gui/wxlwindow.h"

#ifndef USE_PCH
#  include "gui/wxMFrame.h"

#  include <wx/dynarray.h>
#endif

#include <wx/process.h>

class ProcessInfo;
WX_DEFINE_ARRAY(ProcessInfo *, ArrayProcessInfo);

class wxMessageViewPanel;
class wxMessageView;
class wxFolderView;
class XFace;
class Message;
class ASMailFolder;

/** A wxWindows MessageView class
  */

class wxMessageView : public wxLayoutWindow,
                      public MessageViewBase,
                      public MEventReceiver
{
public:
   /** quasi-Constructor
       @param fv a folder view which handles some actions for us
       @param parent parent window
   */
   void Create(wxFolderView *fv,
               wxWindow *parent = NULL);

   /** Constructor
       @param fv a folder view which handles some actions for us
       @param parent parent window
   */
   wxMessageView(wxFolderView *fv,
                 wxWindow *parent = NULL);

   /** Constructor
       @param folder the mailfolder
       @param num    sequence number of message (0 based)
       @param parent parent window
   */
   wxMessageView(ASMailFolder *folder,
                 long num,
                 wxFolderView *fv,
                 wxWindow  *parent = NULL);

   /// Tell it a new parent profile - in case folder changed.
   void SetParentProfile(ProfileBase *profile);

   /// Destructor
   ~wxMessageView();

   /** show message
       @param mailfolder the folder
       @param num the message uid
   */
   void ShowMessage(ASMailFolder *folder, UIdType uid);
   void ShowMessage(class Message *msg);

   /// update it
   void   Update(void);

   /// prints the currently displayed message
   void Print(void);

   /// print-previews the currently displayed message
   void PrintPreview(void);

   /// convert string in cptr to one in which URLs are highlighted
   String HighLightURLs(const char *cptr);

   // callbacks
   // ---------

   /// called on Menu selection
   void OnMenuCommand(wxCommandEvent& event) { (void) DoMenuCommand(event.GetId()); }
   /// returns true if it handled the command
   bool DoMenuCommand(int command);

   /// called on mouse click
   void OnMouseEvent(wxCommandEvent & event);

   /// called when a process we launched terminates
   void OnProcessTermination(wxProcessEvent& event);

   // internal M events processing function
   virtual bool OnMEvent(MEventData& event);

   /// call to show the raw text of the current message (modal dialog)
   bool ShowRawText(void);

   /// for use by wxMessageViewFrame, to be removed after
   /// OnCommandEvent() is cleaned up:
   wxFolderView *GetFolderView(void) { return m_FolderView; }

   /// Clear the window.
   void Clear(void);

   /// returns the mail folder
   ASMailFolder *GetFolder(void);

   UIdType GetUId(void) const { return m_uid; }
   /// the derived class should react to the result to an asynch operation
   void OnASFolderResultEvent(MEventASFolderResultData &event);
private:
   /// register with MEventManager
   void RegisterForEvents();

   /// reread the entries from our profile
   void UpdateProfileValues();

   /// the parent window
   wxWindow   *m_Parent;
   /// uid of the message
   UIdType m_uid;
   /// the current message
   Message   *m_mailMessage;
   /// the mail folder (only used if m_FolderView is NULL)
   ASMailFolder   *m_folder;
   /// the folder view, which handles some actions for us
   wxFolderView *m_FolderView;
   /// the message part selected for MIME display
   int      mimeDisplayPart;
   /// this can hold an xface
   XFace   *xface;
   /// and the xpm for it
   char **xfaceXpm;
   /// string used for last search in message
   wxString m_FindString;
   /// Profile
   ProfileBase *m_Profile;
   /// the MIME popup menu
   wxMenu *m_MimePopup;

   /// event registration handle
   void *m_eventReg;
   void *m_regCookieASFolderResult;

protected:
   friend class MimePopup;

   /// displays information about the currently selected MIME content
   void MimeInfo(int num);
   /// handles the currently selected MIME content
   void MimeHandle(int num);
   /// saves the currently selected MIME content
   bool MimeSave(int num, const char *filename = NULL);

   /// launch a process, returns FALSE if it failed
   bool LaunchProcess(const String& command,    // cmd to execute
                      const String& errormsg,   // err msg to give on failure
                      const String& tempfile = ""); // temp file nameif any

   /// launch a process and wait for its termination, returns FALSE it
   /// exitcode != 0
   bool RunProcess(const String& command);

   /// All values read from the profile
   struct AllProfileValues
   {
      /// Background and foreground colours, colours for URLs and headers
      wxColour BgCol, FgCol, UrlCol, HeaderNameCol, HeaderValueCol;
      /// font attributes
      int font, size;
      /// show headers?
      bool showHeaders;
      /// show forwarded messages as text?
      bool rfc822isText;
      /// highlight URLs?
      bool highlightURLs;
      /// inline graphics?
      bool inlineGFX;
      /// URL viewer
      String browser;
      /// Is URL viewer of the netscape variety?
      bool browserIsNS;
      /// Autocollect email addresses?
      int autocollect;
      /// Autocollect only email addresses with complete name?
      int autocollectNamed;
      /// Name of the ADB book to use for autocollect.
      String autocollectBookName;
#ifdef OS_UNIX
      /// Where to find AFM files.
      String afmpath;
#endif // Unix
      /// Show XFaces?
      bool showFaces;

      bool operator==(const AllProfileValues& other);
   } m_ProfileValues;

   void ReadAllSettings(AllProfileValues *settings);

private:
   /// array of process info for all external viewers we have launched
   ArrayProcessInfo m_processes;

   DECLARE_EVENT_TABLE()
};


class wxMessageViewFrame : public wxMFrame
{
public:
   wxMessageViewFrame(ASMailFolder *folder,
                      long num,
                      wxFolderView *folderview,
                      wxWindow  *parent = NULL,
                      const String &iname = String("MessageViewFrame"));

   /// wxWin2 event system callbacks
   void OnCommandEvent(wxCommandEvent & event);
   void OnSize(wxSizeEvent & event);
   void ShowMessage(Message *msg)
      { m_MessageView->ShowMessage(msg); }
   /// don't even think of using this!
   wxMessageViewFrame(void) {ASSERT(0);}
   DECLARE_DYNAMIC_CLASS(wxMessageViewFrame)
private:
   wxMessageView *m_MessageView;

   DECLARE_EVENT_TABLE()
};

#endif
