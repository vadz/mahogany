/*-*- c++ -*-********************************************************
 * wxComposeView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef WXCOMPOSEVIEW_H
#define WXCOMPOSEVIEW_H

#ifdef __GNUG__
#pragma interface "wxComposeView.h"
#endif

#ifndef USE_PCH
#   include   "Message.h"
#   include   "gui/wxMenuDefs.h"
#   include   "gui/wxMFrame.h"
#   include   "gui/wxIconManager.h"
#   include   "Profile.h"
#   include   "kbList.h"
#endif

class wxFTOList;
class wxComposeView;
class wxFTCanvas;
class wxLayoutWindow;



/// just for now, FIXME!
#define WXCOMPOSEVIEW_FTCANVAS_YPOS 80

struct wxCVFileMapEntry
{
   unsigned long   id;
   String          filename;
};

KBLIST_DEFINE(wxCVFileMapType,wxCVFileMapEntry);

/** A wxWindows ComposeView class
  */

class wxComposeView : public wxMFrame //FIXME: public ComposeViewBase
{
   DECLARE_DYNAMIC_CLASS(wxComposeView)
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
        wxWindow *parent = NULL,
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
   wxWindow *parent = NULL,
   ProfileBase *parentProfile = NULL,
   bool hide = false);

   /// Destructor
   ~wxComposeView();

   /** insert a file into buffer
       @param filename file to insert
       @param mimetype mimetype to use
       @param num_mimetype numeric mimetype
       */
   void InsertFile(const char *filename = NULL, const char *mimetype =
   NULL, int num_mimetype = 0);

   /** Insert MIME content data
       @param data pointer to data
       @param len length of data
       @param mimetype mimetype to use
       @param num_mimetype numeric mimetype
       */
   void InsertData(const char *data,
                   size_t length,
                   const char *mimetype = NULL,
                   int num_mimetype = 0);

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

   /// for button
   void OnExpand(wxCommandEvent &event);

#ifdef  USE_WXWINDOWS2
   //@{ Menu callbacks
      ///
   void OnInsertFile(wxCommandEvent&)
      { OnMenuCommand(WXMENU_COMPOSE_INSERTFILE); }
      ///
   void OnSend(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_SEND); }
      ///
   void OnPrint(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_PRINT); }
      ///
   void OnClear(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_CLEAR); }

   DECLARE_EVENT_TABLE()
#else //wxWin1
   /// resize callback
   void OnSize(int w, int h);

   /// for button
   void OnCommand(wxWindow &win, wxCommandEvent &event);
#endif //wxWin1/2
private:
   /// a profile
   Profile * m_Profile;

   /// the panel
   wxPanel *m_panel;

   /// compose menu
   wxMenu *composeMenu;
   
   /**@name Input fields (arranged into an array) */
   //@{
      /// last length of To field (for expansion)
   int  txtToLastLength;
      /// The text fields
   enum
   {
      Field_To,
      Field_Subject,
      Field_Cc,
      Field_Bcc,
      Field_Max
   };
   wxText *m_txtFields[Field_Max];
      /// the canvas for displaying the mail
   wxLayoutWindow *m_LayoutWindow;
      /// the alias expand button
   wxButton *aliasButton;
   //@}

   /// the popup menu
   wxMenu *popupMenu;
   /**@name The interface to its canvas. */
   //@{
   /// the ComposeView canvas class
   friend class wxCVCanvas;
   /// Process a Mouse Event.
   void ProcessMouse(wxMouseEvent &event);
   //@}

   /// a list mapping IDs to filenames
   wxCVFileMapType fileMap;
   /// for assigning IDs
   unsigned long nextFileID;
   /// find the filename for an ID
   const char *LookupFileName(unsigned long id);

   /// makes the canvas
   void CreateFTCanvas(void);
};

#endif
