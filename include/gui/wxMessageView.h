/*-*- c++ -*-********************************************************
 * wxMessageView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/22 20:41:28  KB
 * included profile setting for fonts etc,
 * made XFaces work, started adding support for highlighted URLs
 *
 * Revision 1.1  1998/03/14 12:21:16  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXMESSAGEVIEW_H
#define WXMESSAGEVIEW_H

#ifdef __GNUG__
#pragma interface "wxMessageView.h"
#endif

#include	<Message.h>
#include	<MessageView.h>
#include	<gui/wxMenuDefs.h>
#include	<gui/wxMFrame.h>
#include	<XFace.h>

//#include	<gui/wxFTCanvas.h>
class wxFTOList;

class wxMessageViewPanel;
class wxMessageView;
class wxFTCanvas;

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

private:
   /// is initialised?
   bool initialised;
   /// a panel to fill the frame
   wxPanel	*panel;

   /// the current message
   Message	*mailMessage;
   /// the mail folder
   MailFolder	*folder;
   /// the menu with message related commands
   wxMenu	*messageMenu;
   /// the canvas for displaying the mail
   wxCanvas	*ftCanvas;
   /// the list of items for display
   wxFTOList	*ftoList;
   /// the popup menu
   wxMenu	*popupMenu;
   /// the message part selected for MIME display
   int		mimeDisplayPart;

   /// this can hold an xface
   XFace	*xface;
   /// and the xpm for it
   char **xfaceXpm;
   
   /**@name The interface to its canvas. */
   //@{
   /// the MessageView canvas class
   friend class wxMVCanvas;
   /// Process a Mouse Event.
   void	ProcessMouse(wxMouseEvent &event);
   //@}

   /// displays information about the currently selected MIME content
   void MimeInfo(void);
   /// handles the currently selected MIME content
   void MimeHandle(void);
   /// saves the currently selected MIME content
   void MimeSave(const char *filename = NULL);

public:
   /** quasi-Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   void Create(const String &iname = String("wxMessageView"),
	  wxFrame *parent = NULL);

   /** Constructor
       @param iname  name of windowclass
       @param parent parent window
   */
   wxMessageView(const String &iname = String("wxMessageView"),
	  wxFrame *parent = NULL);
   
   /** Constructor
       @param folder the mailfolder
       @param num    number of message (0 based)
       @param iname  name of windowclass
       @param parent parent window
   */
   wxMessageView(MailFolder *folder,
		 long num,
		 const String &iname = String("wxMessageView"),
		 wxFrame *parent = NULL);
   /// Destructor
   ~wxMessageView();

   /** show message
       @param mailfolder the folder
       @param num the message number
   */
   void ShowMessage(MailFolder *folder, long num);
   
   /// update it
   void	Update(void);

   /// return true if initialised
   bool	IsInitialised(void) const { return initialised; }

   /// called on Menu selection
   void OnMenuCommand(int id);

   /// prints the currently displayed message
   void Print(void);

   /// convert string in cptr to one in which URLs are highlighted
   void HighLightURLs(const char *cptr, String &out);
};

#endif
