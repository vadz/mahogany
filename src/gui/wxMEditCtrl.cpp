/*-*- c++ -*-********************************************************
 * wxMEditCtrl : a wxLayout based implementation of the MEditCtrl   *
 *                                                                  *
 * (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifdef EXPERIMENTAL_karsten

#ifdef __GNUG__
#   pragma implementation "MEditCtrl.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
#  include "Mcommon.h"
#  include "guidef.h"
#  include "Profile.h"
#  include "MApplication.h"
#  include "gui/wxMApp.h"
#  include "gui/wxIconManager.h"
#endif //USE_PCH

#include "Mdefaults.h"
#include "MEditCtrl.h"
#include "XFace.h"
#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"
#include <wx/file.h>
#include <wx/mimetype.h>

#include "miscutil.h"
// ----------------------------------------------------------------------------
// wxMEditCtrl
// ----------------------------------------------------------------------------

#define NUM_FONTS 7
static int wxFonts[NUM_FONTS] =
{
  wxDEFAULT,
  wxDECORATIVE,
  wxROMAN,
  wxSCRIPT,
  wxSWISS,
  wxMODERN,
  wxTELETYPE
};



/* Helper class to avoid double inheritance in wxMEditCtrl. This is 
   the actual wxLayoutWindow object. Eventually it should contain no
   extra functionality apart from linking
   wxLayoutWindow<->wxMEditCtrl. */
class wxMEditCtrlLWindow : public wxLayoutWindow
{
public:
   wxMEditCtrlLWindow(class wxMEditCtrl *ec, wxWindow *parent)
      : wxLayoutWindow(parent)
      {
         m_EC = ec;

         // we're always interested in mouse events
         SetMouseTracking();
      }
   ~wxMEditCtrlLWindow();
   
   /// intercept character events
   void OnChar(wxKeyEvent& event);

   // forward mouse events to wxMEditCtrl
   void OnMouseEvent(wxCommandEvent &event);

private:
   class wxMEditCtrl *m_EC;

   DECLARE_EVENT_TABLE()
};

   

/* Implementation of MEditCtrl using wxLayoutWindow */
class wxMEditCtrl : public MEditCtrl
{
public:
   wxMEditCtrl(MEditCtrlCbHandler *cbh,wxWindow *parent, Profile *p)
      {
         m_CbHandler = cbh;
         // we do not clean this up as it will clean us up from GUI
         // deletion 
         m_LWin = new wxMEditCtrlLWindow(this,parent);
         ReadProfile(p);
      }
   ~wxMEditCtrl()
      {
         delete m_CbHandler;
      }

   /// Tell it to read the configuration settings
   virtual void ReadProfile(Profile *prof);

   /// clear the control
   virtual void Clear(void)
      {
         wxColour fg, bg;
         GetColourByName(&fg,m_FontStyle.m_ColourFG,"black");
         GetColourByName(&bg,m_FontStyle.m_ColourBG,"white");
         m_LWin->Clear(m_FontStyle.m_Family,
                       m_FontStyle.m_Size,
                       m_FontStyle.m_Style,
                       m_FontStyle.m_Weight,
                       m_FontStyle.m_Underline,
                       &fg, &bg);
         SetFontStyle(
            FontStyle(m_ProfileValues.font, m_ProfileValues.size,
                      (int)wxNORMAL, (int)wxNORMAL, 0,
                      m_ProfileValues.FgCol,
                      m_ProfileValues.BgCol)
      );

      }
   /** Redraws the window. */
   virtual void RequestUpdate(void)
      {
         m_LWin->GetLayoutList()->ForceTotalLayout();
         m_LWin->RequestUpdate();
      }
   /** Enable redrawing while we add content */
   virtual void EnableAutoUpdate(bool enable = TRUE)
      {
         m_LWin->GetLayoutList()->SetAutoFormatting(TRUE);
      }

   /** Set dirty flag */
   virtual void SetDirty(bool dirty = TRUE)
      { if(dirty) m_LWin->SetDirty(); else m_LWin->ResetDirty(); }
   virtual bool GetDirty(void) const
      { return m_LWin->IsDirty(); }
   
   /** Insert some content.
    */
   virtual void Append(const MimeContent &mc);
   
   virtual void Append(const String &txt)
      {
         char *buf = strutil_strdup(txt);
         wxLayoutImportText(m_LWin->GetLayoutList(),buf);
         delete [] buf;
      }

   virtual bool Find(const wxString &needle,
                     wxPoint * fromWhere = NULL,
                     const wxString &configPath = "MsgViewFindString")
      { return m_LWin->Find(needle, fromWhere, configPath); }

   /**@name Clipboard interaction */
   //@{
   /// Pastes text from clipboard.
   virtual void Paste(bool privateFormat = FALSE,
                      bool usePrimarySelection = FALSE)
      {
         m_LWin->Paste(privateFormat, usePrimarySelection);
      }
   /** Copies selection to clipboard.
       @param invalidate used internally, see wxllist.h for details
   */
   virtual bool Copy(bool invalidate = true,
                     bool privateFormat = FALSE, bool primary = FALSE)
      { return m_LWin->Copy(invalidate, privateFormat, primary); }
   /// Copies selection to clipboard and deletes it.
   virtual bool Cut(bool privateFormat = FALSE,
                    bool usePrimary = FALSE)
      { return m_LWin->Cut(privateFormat, usePrimary); }
   //@}


   /** Prints the currently displayed message.
       @param interactive if TRUE, ask for user input
       return TRUE on success
   */
   virtual bool Print(bool interactive = TRUE);
   /// print-previews the currently displayed message
   virtual void PrintPreview(void);

   void SetSize(int x, int y)
      {
         m_LWin->SetSize(x,y);
      }
   void SetFontStyle(const FontStyle &fs)
      {
         m_FontStyle = fs;
      }
   void SetFocusFollowMode(bool enable)
      { m_LWin->SetFocusFollowMode(enable); }
   
   /** Sets the wrap margin.
       @param margin set this to 0 to disable it
   */
   void SetWrapMargin(CoordType margin)
      { m_LWin->SetWrapMargin(margin); }
   /** Tell window to update a wxStatusBar with UserData labels and
       cursor positions.
       @param bar wxStatusBar pointer
       @param labelfield field to use in statusbar for URLs/userdata labels, or -1 to disable
       @param cursorfield field to use for cursor position, or -1 to disable
   */
   void SetStatusBar(class wxStatusBar *bar,
                       int labelfield = -1,
                       int cursorfield = -1)
      {
         m_LWin->SetStatusBar(bar,labelfield,cursorfield);
      }

   ///    Enable or disable editing, i.e. processing of keystrokes.
   void SetEditable(bool toggle)
      { m_LWin->SetEditable(toggle); SetCursorVisibility(toggle); }
   /** Sets cursor visibility, visible=1, invisible=0,
       visible-on-demand=-1, to hide it until moved.
       @param visibility -1,0 or 1
       @return the old visibility
   */
   inline void SetCursorVisibility(int visibility = -1)
      { m_LWin->SetCursorVisibility(visibility); }
   /// moves the cursor to the new position
   virtual void MoveCursorTo(int x, int y) const
      { m_LWin->GetLayoutList()->MoveCursorTo(wxPoint(x,y)); }

   virtual void PageUp(void) const
      {
         m_LWin->GetLayoutList()->MoveCursorVertically(20); // FIXME: ugly hard-coded line count
         m_LWin->ScrollToCursor();
      }
   virtual void PageDown(void) const
      {
         m_LWin->GetLayoutList()->MoveCursorVertically(-20); // FIXME: ugly hard-coded line count
         m_LWin->ScrollToCursor();
      }
   

   /// set font family
   inline void SetFontFamily(int family) { m_LWin->GetLayoutList()->SetFont(family); }
   /// set font size
   inline void SetFontSize(int size) { m_LWin->GetLayoutList()->SetFont(-1,size); }
   /// set font style
   inline void SetFontStyle(int style) { m_LWin->GetLayoutList()->SetFont(-1,-1,style); }
   /// set font weight
   inline void SetFontWeight(int weight) { m_LWin->GetLayoutList()->SetFont(-1,-1,-1,weight); }
   /// toggle underline flag
   inline void SetFontUnderline(bool ul) { m_LWin->GetLayoutList()->SetFont(-1,-1,-1,-1,(int)ul); }
   
   /// set font colours by colour
   inline void SetFontColour(wxString fg, wxString bg)
      {
         wxColour fgc, bgc;
         GetColourByName(&fgc, fg, m_FontStyle.m_ColourFG);
         GetColourByName(&bgc, bg, m_FontStyle.m_ColourBG);
         m_LWin->GetLayoutList()->SetFont(-1,-1,-1,-1,-1,&fgc,&bgc);
      }
   //@}

   virtual void NewLine(void) { m_LWin->GetLayoutList()->LineBreak(); }
   virtual wxPoint GetClickPosition(void) const
      {
         return m_LWin->GetClickPosition();
      }

   // called by wxMEditCtrlLWindow
   void OnMouseEvent(wxCommandEvent &event);

protected:
   void AppendXFace(const wxString &face);

private:
   wxLayoutWindow     * m_LWin;
   MEditCtrlCbHandler * m_CbHandler;
   FontStyle            m_FontStyle;

   /// All values read from the profile
   struct AllProfileValues
   {
      /// Background and foreground colours, colours for URLs and headers
      wxString BgCol, FgCol, UrlCol, HeaderNameCol, HeaderValueCol;
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
      /// Show XFaces?
      bool showFaces;
      bool operator==(const AllProfileValues& other);
   } m_ProfileValues;
};


void
wxMEditCtrl::ReadProfile(Profile *p)
{
#define GET_COLOUR_FROM_PROFILE(which, name) \
      m_ProfileValues.which = READ_CONFIG(p, MP_MVIEW_##name)

   if(p == NULL) p = mApplication->GetProfile();
   
   GET_COLOUR_FROM_PROFILE(FgCol, FGCOLOUR);
   GET_COLOUR_FROM_PROFILE(BgCol, BGCOLOUR);
   GET_COLOUR_FROM_PROFILE(UrlCol, URLCOLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderNameCol, HEADER_NAMES_COLOUR);
   GET_COLOUR_FROM_PROFILE(HeaderValueCol, HEADER_VALUES_COLOUR);

#undef GET_COLOUR_FROM_PROFILE

   m_ProfileValues.font = READ_CONFIG(p,MP_MVIEW_FONT);
   ASSERT(m_ProfileValues.font >= 0 && m_ProfileValues.font <= NUM_FONTS);
   m_ProfileValues.font = wxFonts[m_ProfileValues.font];
   m_ProfileValues.size = READ_CONFIG(p,MP_MVIEW_FONT_SIZE);
   m_ProfileValues.showHeaders = READ_CONFIG(p,MP_SHOWHEADERS) != 0;
   m_ProfileValues.rfc822isText = READ_CONFIG(p,MP_RFC822_IS_TEXT) != 0;
   m_ProfileValues.highlightURLs = READ_CONFIG(p,MP_HIGHLIGHT_URLS) != 0;
   m_ProfileValues.inlineGFX = READ_CONFIG(p, MP_INLINE_GFX) != 0;
   m_ProfileValues.browser = READ_CONFIG(p, MP_BROWSER);
   m_ProfileValues.browserIsNS = READ_CONFIG(p, MP_BROWSER_ISNS) != 0;
   m_ProfileValues.showFaces = READ_CONFIG(p, MP_SHOW_XFACES) != 0;
#ifndef OS_WIN
   SetFocusFollowMode(READ_CONFIG(p,MP_FOCUS_FOLLOWSMOUSE) != 0);
#endif
   SetWrapMargin(READ_CONFIG(p, MP_WRAPMARGIN));

}

// data associated with the clickable objects
class ClickableInfo : public wxLayoutObject::UserData
{
public:
   enum Type
   {
      CI_ICON,
      CI_URL
   };

   ClickableInfo(const String& url)
      : m_url(url)
      { m_type = CI_URL; SetLabel(url); }
   ClickableInfo(int id, const String &label)
      {
         m_id = id;
         m_type = CI_ICON;
         SetLabel(label);
      }

   // accessors
   Type          GetType() const { return m_type; }
   const String& GetUrl()  const { return m_url;  }
   int           GetPart() const { return m_id;   }

private:
   ~ClickableInfo() {}
   Type   m_type;

   int    m_id;
   String m_url;

   GCC_DTOR_WARN_OFF
};




void
wxMEditCtrl::AppendXFace(const wxString &face)
{
#ifdef HAVE_XFACES
   // need real XPM support in windows
   char **xfaceXpm = NULL;
   XFace *xface = new XFace();
   xface->CreateFromXFace(face.c_str());
   if(xface->CreateXpm(&xfaceXpm))
   {
      m_LWin->GetLayoutList()->Insert(new wxLayoutObjectIcon(new wxBitmap(xfaceXpm)));
      NewLine();
   }
   if(xfaceXpm) wxIconManager::FreeImage(xfaceXpm);
#endif // HAVE_XFACES
}

void
wxMEditCtrl::Append(const MimeContent &mc)
{
   MOcheck();

   /* First, check for our special mime types xface and URL */
   if(mc.GetType() == "application/x-m-mc-xface")
   {
      AppendXFace(mc.GetText());
      return;
   }

   wxLayoutObject *obj = NULL;
   wxString mimeType = mc.GetType();
   // try to guess the correct type:
   if(mimeType.BeforeFirst('/') == "application")
   {
      wxString ext = mc.GetFileName().AfterLast('.');
      wxMimeTypesManager& mimeManager = mApplication->GetMimeManager();
      wxFileType *ft = mimeManager.GetFileTypeFromExtension(ext);
      if(ft)
      {
         wxString mt;
         ft->GetMimeType(&mt);
         delete ft;
         if(wxMimeTypesManager::IsOfType(mt,"image/*")
            || wxMimeTypesManager::IsOfType(mt,"audio/*")
            || wxMimeTypesManager::IsOfType(mt,"video/*"))
            mimeType = mt;
      }
   }
   
   if(wxMimeTypesManager::IsOfType(mimeType, "text/*")
      &&
      /* Insert text:
         - if it is text/plain and not "attachment" or with a filename
         - if it is rfc822 and it is configured to be displayed
         - HTML is for now displayed as normal text
      */

      /* FIXME: the correct insertion of message text/rfc822 in either 
         text/plain or text/rfc822 isn't done yet
                        || (t == Message::MSG_TYPEMESSAGE
                   && (m_ProfileValues.rfc822isText != 0)))
      */
      // INLINE TEXT:
      (mc.GetFileName().Length() == 0 && mc.GetDisposition() != "attachment")
      )
      {
         NewLine();
         if( m_ProfileValues.highlightURLs )
         {
            String tmp = mc.GetText();
            String url;
            String before;
            do
            {
               before =  strutil_findurl(tmp,url);
               wxLayoutImportText(m_LWin->GetLayoutList(),before);
               if(!strutil_isempty(url))
               {
                  ClickableInfo *ci = new ClickableInfo(url);
                  obj = new wxLayoutObjectText(url);
                  obj->SetUserData(ci);
                  ci->DecRef();
                  m_LWin->GetLayoutList()->SetFontColour(m_ProfileValues.UrlCol);
                  m_LWin->GetLayoutList()->Insert(obj);
                  m_LWin->GetLayoutList()->SetFontColour(m_ProfileValues.FgCol);
               }
            }
            while( !strutil_isempty(tmp) );
         }
      }
   else
      /* This block captures all non-inlined message parts. They get
         represented by an icon.
         In case of image content, we check whether it might be a
         Fax message. */
   {
      wxString mimeFileName = mc.GetFileName();
      if(wxMimeTypesManager::IsOfType(mimeType, "image/*")
         /*FIXME && m_ProfileValues.inlineGFX*/
         )
      {
         wxLayoutObject *obj = NULL;
         wxString filename = wxGetTempFileName("Mtemp");
         if(mimeFileName.Length() == 0)
         {
            wxFile out(filename, wxFile::write);
            if ( out.IsOpened() )
            {
               size_t written = out.Write(mc.GetData(), mc.GetSize());
               if ( written != mc.GetSize() )
                  return; //FIXME
            }
            else
               return ; //FIXME
         }
         bool ok;
         wxImage img =  wxIconManager::LoadImage(filename, &ok, true);
         wxRemoveFile(filename);
         if(ok)
            obj = new wxLayoutObjectIcon(img.ConvertToBitmap());
         else
            obj = new wxLayoutObjectIcon(
               mApplication->GetIconManager()->
               GetIconFromMimeType(mc.GetType(),
                                   mimeFileName.AfterLast('.'))
               );
      }
      else
      {
         obj = new wxLayoutObjectIcon(
            mApplication->GetIconManager()->
            GetIconFromMimeType(mc.GetType(),
                                mimeFileName.AfterLast('.'))
            );
      }

      ASSERT(obj);
      {
         String label;
         label = mimeFileName;
         if(label.Length()) label << " : ";
         label << mc.GetType() << ", "
               << mc.GetSize() << _(" bytes");
         ClickableInfo *ci = new ClickableInfo(mc.GetPartNumber(), label);
         obj->SetUserData(ci); // gets freed by list
         ci->DecRef();
         m_LWin->GetLayoutList()->Insert(obj);
         // add extra whitespace so lines with multiple icons can
         // be broken:
         m_LWin->GetLayoutList()->Insert(" ");
      }
   }
   NewLine();
}

void
wxMEditCtrl::OnMouseEvent(wxCommandEvent &event)
{
   MOcheck();
   ClickableInfo *ci;
   wxLayoutObject *obj = (wxLayoutObject *)event.GetClientData();
   ci = (ClickableInfo *)obj->GetUserData();
   if(ci)
   {
      switch( ci->GetType() )
      {
         case ClickableInfo::CI_ICON:
         {
            switch ( event.GetId() )
            {
               case WXLOWIN_MENU_RCLICK:
                  m_CbHandler->OnMouseRight(ci->GetPart());
                  break;

               case WXLOWIN_MENU_LCLICK:
                  // for now, do the same thing as double click: perhaps the
                  // left button behaviour should be configurable?

               case WXLOWIN_MENU_DBLCLICK:
                  m_CbHandler->OnMouseLeft(ci->GetPart());
                  break;

               default:
                  FAIL_MSG("unknown mouse action");
            }
         }
         break;

         case ClickableInfo::CI_URL:
            m_CbHandler->OnUrl(ci->GetUrl());
            break;
         default:
            FAIL_MSG("unknown embedded object type");
      }
   ci->DecRef();
   }
}

BEGIN_EVENT_TABLE(wxMEditCtrlLWindow, wxLayoutWindow)
   EVT_CHAR(wxMEditCtrlLWindow::OnChar)

   // mouse click processing
   EVT_MENU(WXLOWIN_MENU_RCLICK, wxMEditCtrlLWindow::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_LCLICK, wxMEditCtrlLWindow::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_DBLCLICK, wxMEditCtrlLWindow::OnMouseEvent)
END_EVENT_TABLE()

void
wxMEditCtrlLWindow::OnMouseEvent(wxCommandEvent &event)
{
   m_EC->OnMouseEvent(event);
}

void
wxMEditCtrlLWindow::OnChar(wxKeyEvent& event)
{
   // FIXME: this should be more intelligent, i.e. use the
   // wxlayoutwindow key bindings:
   if(m_WrapMargin > 0 &&
      event.KeyCode() == 'q' || event.KeyCode() == 'Q')
   {
      // temporarily allow editing to enable manual word wrap:
      SetEditable(TRUE);
      event.Skip();
      SetEditable(FALSE);
      SetCursorVisibility(1);
   }
   else
      event.Skip();
}

wxMEditCtrlLWindow::~wxMEditCtrlLWindow()
{
   delete m_EC;
}
   
bool
wxMEditCtrl::Print(bool interactive)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   bool found;
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(READ_APPCONFIG(MP_AFMPATH), false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
   {
      ((wxMApp *)mApplication)->GetPrintData()->SetFontMetricPath(afmpath);
      wxThePrintSetupData->SetAFMPath(afmpath);
   }
#endif // Win/Unix
   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrinter printer(& pdd);
#ifndef OS_WIN
   wxThePrintSetupData->SetAFMPath(afmpath);
#endif
   wxLayoutPrintout printout(m_LWin->GetLayoutList(), _("Mahogany: Printout"));
   if ( !printer.Print(m_LWin, &printout, interactive)
      && ! printer.GetAbort() )
   {
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
      return FALSE;
   }
   else
   {
      (* ((wxMApp *)mApplication)->GetPrintData())
         = printer.GetPrintDialogData().GetPrintData();
      return TRUE;
   }
}



// tiny class to restore and set the default zoom level
class wxMVPreview : public wxPrintPreview
{
public:
   wxMVPreview(Profile *prof,
               wxPrintout *p1, wxPrintout *p2,
               wxPrintDialogData *dd)
      : wxPrintPreview(p1, p2, dd)
      {
         ASSERT(prof);
         m_Profile = prof;
         m_Profile->IncRef();
         SetZoom(READ_CONFIG(m_Profile, MP_PRINT_PREVIEWZOOM));
      }
   ~wxMVPreview()
      {
         m_Profile->writeEntry(MP_PRINT_PREVIEWZOOM, (long) GetZoom());
         m_Profile->DecRef();
      }
private:
   Profile *m_Profile;
};



void
wxMEditCtrl::PrintPreview(void)
{
#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(READ_APPCONFIG(MP_AFMPATH), false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   bool found;
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
      wxSetAFMPath(afmpath);
#endif // in/Unix

   // Pass two printout objects: for preview, and possible printing.
   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrintPreview *preview = new wxMVPreview(
      mApplication->GetProfile(),
      new wxLayoutPrintout(m_LWin->GetLayoutList()),
      new wxLayoutPrintout(m_LWin->GetLayoutList()),
      &pdd
      );
   if( !preview->Ok() )
   {
      wxMessageBox(_("There was a problem with showing the preview:\n"
                     "perhaps your current printer is not set correctly?"),
                   _("Previewing"), wxOK);
      return;
   }

   (* ((wxMApp *)mApplication)->GetPrintData())
      = preview->GetPrintDialogData().GetPrintData();

   wxPreviewFrame *frame = new wxPreviewFrame(preview, NULL, //GetFrame(m_Parent),
                                              _("Print Preview"),
                                              wxPoint(100, 100), wxSize(600, 650));
   frame->Centre(wxBOTH);
   frame->Initialize();
   frame->Show(TRUE);
}


// --------------------------- MEditCtrl ----------------------------

/* static */
MEditCtrl *
MEditCtrl::Create(int type,
                  MEditCtrlCbHandler *cbh, 
                  wxWindow *parent,
                  Profile *p)
{
   CHECK(type == MEC_WXLAYOUT, NULL, "unsupported MEditCtrl type");
   return new wxMEditCtrl(cbh, parent, p);
}

#endif
