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

#ifndef USE_PCH
#   include   <Message.h>
#   include   <MessageView.h>
#   include   <gui/wxMenuDefs.h>
#   include   <gui/wxMFrame.h>
#   include   <XFace.h>
#endif


class wxMessageViewPanel;
class wxMessageView;
class wxLayoutWindow;

/** a wxWindows panel for the MessageView class */
class wxMessageViewPanel : public wxPanel
{
   DECLARE_CLASS(wxMessageViewPanel)

   /// the message view
   wxMessageView *messageView;
public:
   /** constructor
       @param parent the parent frame
   */
   wxMessageViewPanel(wxMessageView *parent);

   /** gets called for events happening in the panel
       @param win the window
   */
   void OnCommand(wxWindow &win, wxCommandEvent &ev);
};

/** A wxWindows MessageView class
  */

class wxMessageView : public MessageViewBase , public wxMFrame
{
   DECLARE_CLASS(wxMessageView)
public:
   /** quasi-Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   void Create(const String &iname = String("wxMessageView"),
     wxWindow *parent = NULL);

   /** Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   wxMessageView(const String &iname = String("wxMessageView"),
     wxWindow *parent = NULL);
   
   /** Constructor
       @param folder the mailfolder
       @param num    number of message (0 based)
       @param iname  name of windowclass
       @param parent parent window
   */
   wxMessageView(MailFolder *folder,
       long num,
       const String &iname = String("wxMessageView"),
       wxWindow  *parent = NULL);
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
   DECLARE_EVENT_TABLE()

private:
   /// is initialised?
   bool initialised;
   /// a panel to fill the frame
   wxPanel   *panel;

   /// the current message
   Message   *mailMessage;
   /// the mail folder
   MailFolder   *folder;
   /// the menu with message related commands
   wxMenu   *messageMenu;
   /// the canvas for displaying the mail
   //wxCanvas   *ftCanvas;
   wxLayoutWindow *m_LWindow;
   /// the popup menu
   wxMenu   *popupMenu;
   /// the message part selected for MIME display
   int      mimeDisplayPart;

   /// this can hold an xface
   XFace   *xface;
   /// and the xpm for it
   char **xfaceXpm;

   /// Profile
   Profile *m_Profile;
   /// the MIME popup
   wxDialog *m_MimePopup;
   
protected:
   friend class MimeDialog;
   /// displays information about the currently selected MIME content
   void MimeInfo(int num);
   /// handles the currently selected MIME content
   void MimeHandle(int num);
   /// saves the currently selected MIME content
   void MimeSave(int num, const char *filename = NULL);
};

#endif
