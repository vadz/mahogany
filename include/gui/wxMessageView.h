/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$     *
 *******************************************************************/

#ifndef WXMESSAGEVIEW_H
#define WXMESSAGEVIEW_H

#ifdef __GNUG__
#pragma interface "wxMessageView.h"
#endif

#include "MessageView.h"

#include   "gui/wxlwindow.h"

#ifndef USE_PCH
#   include   "gui/wxMFrame.h"
#endif


class wxMessageViewPanel;
class wxMessageView;
class wxFolderView;
class XFace;
class Message;
class MailFolder;

/** A wxWindows MessageView class
  */

class wxMessageView : public wxLayoutWindow, public MessageViewBase
{
public:
   /** quasi-Constructor
       @param fv a folder view which handles some actions for us
       @param parent parent window
       @param iname  name of windowclass
   */
   void Create(wxFolderView *fv,
               wxWindow *parent = NULL,
               const String &iname = wxString("MessageView"));

   /** Constructor
       @param fv a folder view which handles some actions for us
       @param parent parent window
       @param iname  name of windowclass
   */
   wxMessageView(wxFolderView *fv,
                 wxWindow *parent = NULL,
                 const String &iname = String("MessageView")
                 );

   /** Constructor
       @param folder the mailfolder
       @param num    number of message (0 based)
       @param iname  name of windowclass
       @param parent parent window
   */
   wxMessageView(MailFolder *folder,
                 long num,
                 wxFolderView *fv,
                 wxWindow  *parent = NULL,
                 const String &iname = String("MessageView")
      );
   /// Destructor
   ~wxMessageView();

   /** show message
       @param mailfolder the folder
       @param num the message number
   */
   void ShowMessage(MailFolder *folder, long num);

   /// update it
   void   Update(void);

   /// return true if initialised
   bool   IsInitialised(void) const { return initialised; }

   /// prints the currently displayed message
   void Print(void);

   /// print-previews the currently displayed message
   void PrintPreview(void);

   /// convert string in cptr to one in which URLs are highlighted
   String HighLightURLs(const char *cptr);

   // callbacks
      /// called on Menu selection
   void OnMenuCommand(wxCommandEvent& event) { (void) DoMenuCommand(event.GetId()); }
   /// returns true if it handled the command
   bool DoMenuCommand(int command);

   /// called on mouse click
   void OnMouseEvent(wxCommandEvent & event);

   /// call to show the raw text of the current message (modal dialog)
   bool ShowRawText(MailFolder *folder = NULL);

   /// for use by wxMessageViewFrame, to be removed after
   /// OnCommandEvent() is cleaned up:
   wxFolderView *GetFolderView(void) { return m_FolderView; }
private:
   /// is initialised?
   bool initialised;
   /// the parent window
   wxWindow   *m_Parent;
   /// number of the message
   long m_uid;
   /// the current message
   Message   *mailMessage;
   /// the mail folder
   MailFolder   *m_folder;
   /// the folder view, which handles some actions for us
   wxFolderView *m_FolderView;
   /// the message part selected for MIME display
   int      mimeDisplayPart;
   /// do we want to display all header lines?
   bool m_showHeaders;
   /// this can hold an xface
   XFace   *xface;
   /// and the xpm for it
   char **xfaceXpm;

   /// Profile
   ProfileBase *m_Profile;
   /// the MIME popup menu
   wxMenu *m_MimePopup;

protected:
   friend class MimePopup;

   /// displays information about the currently selected MIME content
   void MimeInfo(int num);
   /// handles the currently selected MIME content
   void MimeHandle(int num);
   /// saves the currently selected MIME content
   void MimeSave(int num, const char *filename = NULL);

   DECLARE_EVENT_TABLE()
};


class wxMessageViewFrame : public wxMFrame
{
public:
   wxMessageViewFrame(MailFolder *folder,
                      long num,
                      wxFolderView *folderview,
                      wxWindow  *parent = NULL,
                      const String &iname = String("MessageViewFrame"));

   /// wxWin2 event system callbacks
   void OnCommandEvent(wxCommandEvent & event);
   void OnSize(wxSizeEvent & event);

private:
   wxMessageView *m_MessageView;

   DECLARE_EVENT_TABLE()
};

#endif
