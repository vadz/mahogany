/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$          *
 *******************************************************************/
#ifndef	WXMESSAGEVIEW_H
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

class wxMessageView : public MessageViewBase, public wxLayoutWindow
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

   /// called on Menu selection
   void OnMenuCommand(int id);

   /// prints the currently displayed message
   void Print(void);

   /// convert string in cptr to one in which URLs are highlighted
   void HighLightURLs(const char *cptr, String &out);

   /// wxWin2 event system
   void OnCommandEvent(wxCommandEvent & event);

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
   MailFolder   *folder;
   /// the folder view, which handles some actions for us
   wxFolderView *m_FolderView;
   /// the message part selected for MIME display
   int      mimeDisplayPart;

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
};


class wxMessageViewFrame : public wxMFrame
{
public:
   wxMessageViewFrame(MailFolder *folder,
                      long num, 
                      wxFolderView *folderview,
                      wxWindow  *parent = NULL,
                      const String &iname = String("MessageViewFrame"));

   /// wxWin2 event system
   void OnCommandEvent(wxCommandEvent & event);
#ifdef USE_WXWINDOWS2
      void OnSize( wxSizeEvent &WXUNUSED(event) );
#endif
private:
   wxMessageView *m_MessageView;
   DECLARE_EVENT_TABLE() 
};

#endif
