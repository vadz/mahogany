/*-*- c++ -*-********************************************************
 * wxComposeView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.4  1998/05/01 14:02:41  KB
 * Ran sources through conversion script to enforce #if/#ifdef and space/TAB
 * conventions.
 *
 * Revision 1.3  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.2  1998/03/16 18:22:42  karsten
 * started integration of python, fixed bug in wxFText/word wrapping
 *
 * Revision 1.1  1998/03/14 12:21:14  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifdefndef	WXCOMPOSEVIEW_H
#define WXCOMPOSEVIEW_H

#ifdefdef __GNUG__
#pragma interface "wxComposeView.h"
#endif

#ifdef !USE_PCH
  #include	<map>

  #include	<Message.h>
  #include	<wxMenuDefs.h>
  #include	<wxMFrame.h>
  #include	<Profile.h>

  using namespace std;
#endif

class wxFTOList;
class wxComposeView;
class wxFTCanvas;


/// just for now, FIXME!
#define	WXCOMPOSEVIEW_FTCANVAS_YPOS	80

/** A wxWindows ComposeView class
  */

class wxComposeView : public wxMFrame //FIXME: public ComposeViewBase
{
   DECLARE_DYNAMIC_CLASS(wxComposeView)
private:
   /// is initialised?
   bool initialised;

   /// a profile
   Profile	* profile;

   /// the panel
   wxPanel	* panel;

   /// compose menu
   wxMenu	*composeMenu;
   
   /**@name Input fields. */
   //@{
   /// The To: field
   wxText	*txtTo;
   wxMessage	*txtToLabel;
   // last length of To field (for expansion)
   int		txtToLastLength;
   /// The CC: field
   wxText	*txtCC;
   wxMessage	*txtCCLabel;
   /// The BCC: field
   wxText	*txtBCC;
   wxMessage	*txtBCCLabel;
   /// The Subject: field
   wxText	*txtSubject;
   wxMessage	*txtSubjectLabel;
   /// the canvas for displaying the mail
   wxFTCanvas	*ftCanvas;
   /// the alias expand button
   wxButton	*aliasButton;
   //@}

   /// the popup menu
   wxMenu	*popupMenu;
   /**@name The interface to its canvas. */
   //@{
   /// the ComposeView canvas class
   friend class wxCVCanvas;
   /// Process a Mouse Event.
   void	ProcessMouse(wxMouseEvent &event);
   //@}

   typedef std::map<unsigned long, String> MapType;
   MapType	fileMap;
   unsigned long	nextFileID;

   /// makes the canvas
   void CreateFTCanvas(void);
public:
   /** quasi-Constructor
       @param iname  name of windowclass
       @param parent parent window
       @param parentProfile parent profile
       @param to default value for To field
       @param cc default value for Cc field
       @param bcc default value for Bcc field
       @param hide if true, do not show frame
   */
   void Create(const String &iname = String("wxComposeView"),
	       wxFrame *parent = NULL,
	       ProfileBase *parentProfile = NULL,
	       String const &to = "",
	       String const &cc = "",
	       String const &bcc = "",
	       bool hide = false);

   /** Constructor
       @param iname  name of windowclass
       @param parentProfile parent profile
       @param parent parent window
       @param hide if true, do not show frame
   */
   wxComposeView(const String &iname = String("wxComposeView"),
		 wxFrame *parent = NULL,
		 ProfileBase *parentProfile = NULL,
		 bool hide = false);
   
   /// Destructor
   ~wxComposeView();

   /// return true if initialised
   inline bool	IsInitialised(void) const { return initialised; }

   /// insert a file into buffer
   void InsertFile(void);
   
   /// sets To field
   void SetTo(const String &to);
   
   /// sets CC field
   void SetCC(const String &cc);

   /// sets Subject field
   void SetSubject(const String &subj);

   /// inserts a text
   void InsertText(const String &txt);
   
   /// make a printout of input window
   void Print(void);

   /// send the message
   void Send(void);
   
   /// called on Menu selection
   void OnMenuCommand(int id);

   /// resize callback
   void OnSize(int w, int h);

   /// for button
   void OnCommand(wxWindow &win, wxCommandEvent &event);
};

#endif
