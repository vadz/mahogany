/*-*- c++ -*-********************************************************
 * wxComposeView.h: a window displaying a mail message              *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/
#ifndef WXCOMPOSEVIEW_H
#define WXCOMPOSEVIEW_H

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------
class wxFTOList;
class wxComposeView;
class wxFTCanvas;
class wxLayoutWindow;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// just for now, FIXME!
#define WXCOMPOSEVIEW_FTCANVAS_YPOS 80

// ----------------------------------------------------------------------------
// types and classes
// ----------------------------------------------------------------------------

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
public:
   enum AddressField
   {
      Field_To,
      Field_Subject,
      Field_Cc,
      Field_Bcc,
      Field_Max
   };
   
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
   void InsertFile(const char *filename = NULL,
                   const char *mimetype = NULL);

   /** Insert MIME content data
       @param data pointer to data (we take ownership of it)
       @param len length of data
       @param mimetype mimetype to use
       */
   void InsertData(char *data,
                   size_t length,
                   const char *mimetype = NULL);

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

   /** Send the message.
       @return true if successful, false otherwise
   */
   bool Send(void);

   /** wxWindows callbacks
   */
   //@{
      /// called when text zone contents changes
   void OnTextChange(wxCommandEvent &event);

      /// called when TAB is pressed
   void OnNavigationKey(wxNavigationKeyEvent&);

      /// called on Menu selection
   void OnMenuCommand(int id);

      /// for button
   void OnExpand(wxCommandEvent &event);

      ///
   void OnInsertFile(wxCommandEvent&)
      { OnMenuCommand(WXMENU_COMPOSE_INSERTFILE); }
      ///
   void OnSend(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_SEND); }
      ///
   void OnPrint(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_PRINT); }
      ///
   void OnClear(wxCommandEvent&) { OnMenuCommand(WXMENU_COMPOSE_CLEAR); }

      /// can we close now?
   void OnCloseWindow(wxCloseEvent& event);
   //@}

   // for wxAddressTextCtrl usage
   void SetLastAddressEntry(AddressField field) { m_fieldLast = field; }

private:
   /// a profile
   ProfileBase * m_Profile;

   /// the panel
   wxPanel *m_panel;

   /// compose menu
   wxMenu *composeMenu;
   
   /**@name Input fields (arranged into an array) */
   //@{
      /// The text fields
   AddressField m_fieldLast;            // which had the focus last time
   wxTextCtrl  *m_txtFields[Field_Max];

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
   //@}

   /// a list mapping IDs to filenames
   wxCVFileMapType fileMap;
   /// for assigning IDs
   unsigned long nextFileID;
   /// find the filename for an ID
   const char *LookupFileName(unsigned long id);

   /// makes the canvas
   void CreateFTCanvas(void);

   DECLARE_EVENT_TABLE()
   DECLARE_DYNAMIC_CLASS(wxComposeView)
};

#endif
