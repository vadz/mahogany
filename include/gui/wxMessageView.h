/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef EXPERIMENTAL_karsten
#   include "wxMessageViewNew.h"
#else

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
                      // public MessageViewBase,
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
                 wxWindow *parent = NULL,
                 bool show = TRUE);

   /** Constructor
       @param folder the mailfolder
       @param num    sequence number of message (0 based)
       @param parent parent window
       @param show if FALSE, don't show it
   */
   wxMessageView(ASMailFolder *folder,
                 long num,
                 wxFolderView *fv,
                 wxWindow  *parent = NULL,
                 bool show = TRUE);

   /// Tell it a new parent profile - in case folder changed.
   void SetParentProfile(Profile *profile);

   /// Destructor
   ~wxMessageView();

   /** show message
       @param mailfolder the folder
       @param num the message uid
   */
   void ShowMessage(ASMailFolder *folder, UIdType uid);
   void ShowMessage(class Message *msg);

   /// update it
   void Update(void);

   /// set the user-specified encoding and update
   void SetEncoding(wxFontEncoding enc);

   /** Prints the currently displayed message.
       @param interactive if TRUE, ask for user input
       return TRUE on success
   */
   bool Print(bool interactive = TRUE);

   /// print-previews the currently displayed message
   void PrintPreview(void);

   /// set the language to use for message display
   void SetLanguage(int id);

   // callbacks
   // ---------

#ifndef EXPERIMENTAL_karsten
   /// called on Menu selection
   void OnMenuCommand(wxCommandEvent& event) { (void) DoMenuCommand(event.GetId()); }
#endif // EXPERIMENTAL_karsten

   /// returns true if it handled the command
   bool DoMenuCommand(int command);

   /// called on mouse click
   void OnMouseEvent(wxCommandEvent & event);

   /// called when a process we launched terminates
   void OnProcessTermination(wxProcessEvent& event);

   // internal M events processing function
   virtual bool OnMEvent(MEventData& event);

   // for use by wxMessageViewFrame, to be removed after OnCommandEvent() is
   // cleaned up:
   wxFolderView *GetFolderView(void) { return m_FolderView; }

   /// Clear the window.
   void Clear(void);

   /// returns the mail folder
   ASMailFolder *GetFolder(void);

   UIdType GetUId(void) const { return m_uid; }
   /// the derived class should react to the result to an asynch operation
   void OnASFolderResultEvent(MEventASFolderResultData &event);

   /// scroll down one line:
   void LineDown(void);
   /// scroll down one page:
   void PageDown(void);
   /// scroll up one page:
   void PageUp(void);

   /// intercept character events
   void OnChar(wxKeyEvent& event);

private:
   /// register with MEventManager
   void RegisterForEvents();

   /// reread the entries from our profile
   void UpdateProfileValues();

   /// just clear the window without forgetting m_mailMessage
   void ClearWithoutReset();

   /// call mkey processing code in wxScrolledWindow, used by PageDown/Up
   void EmulateKeyPress(int keycode);

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
   // -- unused? int      mimeDisplayPart;
   /// this can hold an xface
   XFace   *xface;
   /// and the xpm for it
   char **xfaceXpm;
   /// string used for last search in message
   wxString m_FindString;
   /// Profile
   Profile *m_Profile;
   /// the MIME popup menu
   wxMenu *m_MimePopup;

   /// event registration handle
   void *m_eventReg;
   void *m_regCookieASFolderResult;

protected:
   friend class MimePopup;
   friend class UrlPopup;

   /// update the "show headers" menu item from m_ProfileValues.showHeaders
   void UpdateShowHeadersInMenu();

   /// displays information about the currently selected MIME content
   void MimeInfo(int num);

   /// handles the currently selected MIME content
   void MimeHandle(int num);

   /// "Opens With..." attachment
   void MimeOpenWith(int num);

   /// saves the currently selected MIME content
   bool MimeSave(int num, const char *filename = NULL);

   /// view attachment as text
   void MimeViewText(int num);

   /// launch a process, returns FALSE if it failed
   bool LaunchProcess(const String& command,    // cmd to execute
                      const String& errormsg,   // err msg to give on failure
                      const String& tempfile = ""); // temp file nameif any

   /** launch a process and wait for its termination, returns FALSE it
       exitcode != 0 */
   bool RunProcess(const String& command);

   /// show the URL popup menu
   void PopupURLMenu(const String& url, const wxPoint& pt);

   /// open the URL
   void OpenURL(const String& url, bool inNewWindow);

   /// All values read from the profile
   struct AllProfileValues
   {
      /// Background and foreground colours, colours for URLs and headers
      wxColour BgCol, FgCol, UrlCol, HeaderNameCol, HeaderValueCol;
      wxColour QuotedCol[3];
      /// process quoted text colourizing?
      bool quotedColourize;
      /// if there is >3 levels of quoting, cycle colours?
      bool quotedCycleColours;
      /// max number of whitespaces before >
      int quotedMaxWhitespace;
      /// max number of A-Z before >
      int quotedMaxAlpha;
      /// font attributes
      int font, size;
      /// show headers?
      bool showHeaders;
      /// inline MESSAGE/RFC822 attachments?
      bool inlineRFC822;
      /// inline TEXT/PLAIN attachments?
      bool inlinePlainText;
      /// highlight URLs?
      bool highlightURLs;
      /// max size of graphics to inline: 0 if none, -1 if not inline at all
      long inlineGFX;
      /// URL viewer
      String browser;
#ifdef OS_UNIX
      /// Is URL viewer of the netscape variety?
      bool browserIsNS;
#endif // Unix
      /// open netscape in new window?
      bool browserInNewWindow;
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

   /// the encoding specified by the user or wxFONTENCODING_SYSTEM if none
   wxFontEncoding m_encodingUser;

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
   wxMessageView *GetMessageView() { return m_MessageView; }

   DECLARE_DYNAMIC_CLASS(wxMessageViewFrame)
private:
   wxMessageView *m_MessageView;

   DECLARE_EVENT_TABLE()
};

#endif // WXMESSAGEVIEW_H

#endif // EXPERIMENTAL_karsten

