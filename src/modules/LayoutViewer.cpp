///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/LayoutViewer.cpp: implements default MessageViewer
// Purpose:     this is a wxLayout-based implementation of MessageViewer
//              using bits and pieces of the original Karsten's code but
//              repackaged in a modular way and separated from the rest of
//              defunct wxMessageView
// Author:      Vadim Zeitlin (based on code by Karsten Ballüder)
// Modified by:
// Created:     24.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 1997-2001 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifndef USE_PCH
   #include "Mcommon.h"

   #include "Profile.h"

   #include "gui/wxMApp.h"
#endif // USE_PCH

#include "Mdefaults.h"

#include "MessageView.h"
#include "MessageViewer.h"

#include "gui/wxllist.h"
#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"

class LayoutViewerWindow;

// ----------------------------------------------------------------------------
// LayoutViewer: a wxLayout-based MessageViewer implementation
// ----------------------------------------------------------------------------

class LayoutViewer : public MessageViewer
{
public:
   // default ctor
   LayoutViewer();

   // creation &c
   virtual void Create(MessageView *msgView, wxWindow *parent);
   virtual void Clear();
   virtual void Update();
   virtual void UpdateOptions();
   virtual wxWindow *GetWindow() const;

   // operations
   virtual void Find(const String& text);
   virtual void FindAgain();
   virtual void Copy();
   virtual bool Print();
   virtual void PrintPreview();

   // header showing
   virtual void StartHeaders();
   virtual void ShowRawHeaders(const String& header);
   virtual void ShowHeader(const String& name,
                           const String& value,
                           wxFontEncoding encoding);
   virtual void ShowXFace(const wxBitmap& bitmap);
   virtual void EndHeaders();

   // body showing
   virtual void StartBody();
   virtual void StartPart();
   virtual void InsertAttachment(const wxBitmap& icon, ClickableInfo *ci);
   virtual void InsertImage(const wxBitmap& image, ClickableInfo *ci);
   virtual void InsertRawContents(const String& data);
   virtual void InsertText(const String& text, const TextStyle& style);
   virtual void InsertURL(const String& url);
   virtual void InsertSignature(const String& signature);
   virtual void EndPart();
   virtual void EndBody();

   // scrolling
   virtual bool LineDown();
   virtual bool LineUp();
   virtual bool PageDown();
   virtual bool PageUp();

   // capabilities querying
   virtual bool CanInlineImages() const;
   virtual bool CanProcess(const String& mimetype) const;

   // for m_window only
   void DoMouseCommand(int id, const ClickableInfo *ci, const wxPoint& pt)
   {
      m_msgView->DoMouseCommand(id, ci, pt);
   }

private:
   // set the text colour
   void SetTextColour(const wxColour& col);

   // emulate a key press: this is the only way I found to scroll
   // wxLayoutWindow
   void EmulateKeyPress(int keycode);

   // the viewer window
   LayoutViewerWindow *m_window;

   DECLARE_MESSAGE_VIEWER()
};

// ----------------------------------------------------------------------------
// LayoutViewerWindow: wxLayoutWindow used by LayoutViewer
// ----------------------------------------------------------------------------

class LayoutViewerWindow : public wxLayoutWindow
{
public:
   LayoutViewerWindow(LayoutViewer *viewer, wxWindow *parent);

private:
   void OnMouseEvent(wxCommandEvent& event);

   LayoutViewer *m_viewer;

   DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// LayoutUserData: our wrapper for ClickableInfo which is needed because we
//                 we can't associate it directly with a wxLayoutObject which
//                 wants to have something derived from its UserData
// ----------------------------------------------------------------------------

class LayoutUserData : public wxLayoutObject::UserData
{
public:
   // we tkae ownership of ClickableInfo object
   LayoutUserData(ClickableInfo *ci)
   {
      m_ci = ci;

      SetLabel(m_ci->GetLabel());
   }

private:
   ClickableInfo *m_ci;
};

// ----------------------------------------------------------------------------
// wxMVPreview: tiny helper class used by LayoutViewer::PrintPreview() to
//              restore and set the default zoom level
// ----------------------------------------------------------------------------

class wxMVPreview : public wxPrintPreview
{
public:
   wxMVPreview(Profile *prof,
               wxPrintout *p1, wxPrintout *p2,
               wxPrintDialogData *dd)
      : wxPrintPreview(p1, p2, dd)
      {
         m_profile = prof;
         m_profile->IncRef();
         SetZoom(READ_CONFIG(m_profile, MP_PRINT_PREVIEWZOOM));
      }
   ~wxMVPreview()
      {
         m_profile->writeEntry(MP_PRINT_PREVIEWZOOM, (long) GetZoom());
         m_profile->DecRef();
      }

private:
   Profile *m_profile;
};

// ----------------------------------------------------------------------------
// ScrollPositionChangeChecker: helper class used by {Line/Page}{Up/Down} to
// check if the y-scroll position of the window has changed or not
// ----------------------------------------------------------------------------

class ScrollPositionChangeChecker
{
public:
   ScrollPositionChangeChecker(wxScrolledWindow *window)
   {
      m_window = window;

      m_window->GetViewStart(NULL, &m_y);
   }

   bool HasChanged() const
   {
      wxCoord y;
      m_window->GetViewStart(NULL, &y);

      return m_y != y;
   }

private:
   wxScrolledWindow *m_window;

   wxCoord m_y;
};

// ============================================================================
// LayoutViewerWindow implementation
// ============================================================================

BEGIN_EVENT_TABLE(LayoutViewerWindow, wxLayoutWindow)
   EVT_MENU(WXLOWIN_MENU_RCLICK, LayoutViewerWindow::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_LCLICK, LayoutViewerWindow::OnMouseEvent)
   EVT_MENU(WXLOWIN_MENU_DBLCLICK, LayoutViewerWindow::OnMouseEvent)
END_EVENT_TABLE()

LayoutViewerWindow::LayoutViewerWindow(LayoutViewer *viewer, wxWindow *parent)
                  : wxLayoutWindow(parent)
{
   m_viewer = viewer;

   // we want to get the notifications about mouse clicks
   SetMouseTracking();

   // remove unneeded status bar pane
   wxFrame *p = GetFrame(this);
   if ( p )
   {
      wxStatusBar *statusBar = p->GetStatusBar();
      if ( statusBar )
      {
         // we don't edit the message, so the cursor coordinates are useless
         SetStatusBar(statusBar, 0, -1);
      }
   }
}

void LayoutViewerWindow::OnMouseEvent(wxCommandEvent& event)
{
   wxLayoutObject *obj = (wxLayoutObject *)event.GetClientData();
   ClickableInfo *ci = (ClickableInfo *)obj->GetUserData();
   if ( ci )
   {
      int id;
      switch ( event.GetId() )
      {
         case WXLOWIN_MENU_RCLICK:
            id = WXMENU_LAYOUT_RCLICK;
            break;

         default:
            FAIL_MSG("unknown mouse action");

         case WXLOWIN_MENU_LCLICK:
            id = WXMENU_LAYOUT_LCLICK;
            break;

         case WXLOWIN_MENU_DBLCLICK:
            id = WXMENU_LAYOUT_DBLCLICK;
            break;
      }

      m_viewer->DoMouseCommand(id, ci, GetClickPosition());
   }
}

// ============================================================================
// LayoutViewer implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

IMPLEMENT_MESSAGE_VIEWER(LayoutViewer,
                         _("Default message viewer"),
                         "(c) 2001 Vadim Zeitlin <vadimn@wxwindows.org>");

LayoutViewer::LayoutViewer()
{
   m_window = NULL;
}

void LayoutViewer::SetTextColour(const wxColour& col)
{
    // const_cast is harmless but needed because of the broken wxLayoutWindow
    // API
    m_window->GetLayoutList()->SetFontColour((wxColour *)&col);
}

// ----------------------------------------------------------------------------
// LayoutViewer creation &c
// ----------------------------------------------------------------------------

void LayoutViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;
   m_window = new LayoutViewerWindow(this, parent);
}

void LayoutViewer::Clear()
{
   // show cursor on demand
   m_window->SetCursorVisibility(-1);

   // reset visibility params
   const ProfileValues& profileValues = GetOptions();
   m_window->Clear(profileValues.fontFamily,
                   profileValues.fontSize,
                   (int)wxNORMAL,
                   (int)wxNORMAL,
                   0,
                   (wxColour *)&profileValues.FgCol,
                   NULL, //(wxColour *)&profileValues.BgCol,
                   true /* no update */);

   //m_window->SetBackgroundColour( profileValues.BgCol );

   // speeds up insertion of text
   m_window->GetLayoutList()->SetAutoFormatting(FALSE);
}

void LayoutViewer::Update()
{
   m_window->RequestUpdate();
}

void LayoutViewer::UpdateOptions()
{
   Profile *profile = GetProfile();

#ifndef OS_WIN
   m_window->SetFocusFollowMode(READ_CONFIG(profile, MP_FOCUS_FOLLOWSMOUSE) != 0);
#endif

   m_window->SetWrapMargin(READ_CONFIG(profile, MP_VIEW_WRAPMARGIN));
}

// ----------------------------------------------------------------------------
// LayoutViewer operations
// ----------------------------------------------------------------------------

void LayoutViewer::Find(const String& text)
{
   m_window->Find(text);
}

void LayoutViewer::FindAgain()
{
   m_window->FindAgain();
}

void LayoutViewer::Copy()
{
   m_window->Copy();
}

// ----------------------------------------------------------------------------
// LayoutViewer printing
// ----------------------------------------------------------------------------

bool LayoutViewer::Print()
{
   const ProfileValues& profileValues = GetOptions();

#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   bool found;
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);

   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(profileValues.afmpath, false);
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
   wxLayoutPrintout printout(m_window->GetLayoutList(), _("Mahogany: Printout"));

   if ( !printer.Print(m_window, &printout, TRUE)
        && printer.GetLastError() != wxPRINTER_CANCELLED )
   {
      wxMessageBox(_("There was a problem with printing the message:\n"
                     "perhaps your current printer is not set up correctly?"),
                   _("Printing"), wxOK);
      return FALSE;
   }
   else
   {
      (*((wxMApp *)mApplication)->GetPrintData())
         = printer.GetPrintDialogData().GetPrintData();
      return TRUE;
   }
}

void LayoutViewer::PrintPreview()
{
   const ProfileValues& profileValues = GetOptions();

#ifdef OS_WIN
   wxGetApp().SetPrintMode(wxPRINT_WINDOWS);
#else // Unix
   wxGetApp().SetPrintMode(wxPRINT_POSTSCRIPT);
   // set AFM path
   PathFinder pf(mApplication->GetGlobalDir()+"/afm", false);
   pf.AddPaths(profileValues.afmpath, false);
   pf.AddPaths(mApplication->GetLocalDir(), true);
   bool found;
   String afmpath = pf.FindDirFile("Cour.afm", &found);
   if(found)
      wxSetAFMPath(afmpath);
#endif // in/Unix

   // Pass two printout objects: for preview, and possible printing.
   wxPrintDialogData pdd(*((wxMApp *)mApplication)->GetPrintData());
   wxPrintPreview *preview = new wxMVPreview(
      GetProfile(),
      new wxLayoutPrintout(m_window->GetLayoutList()),
      new wxLayoutPrintout(m_window->GetLayoutList()),
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

wxWindow *LayoutViewer::GetWindow() const
{
   return m_window;
}

// ----------------------------------------------------------------------------
// header showing
// ----------------------------------------------------------------------------

void LayoutViewer::StartHeaders()
{
}

void LayoutViewer::ShowRawHeaders(const String& header)
{
   wxLayoutList *llist = m_window->GetLayoutList();
   wxLayoutImportText(llist, header);
   llist->LineBreak();
}

void LayoutViewer::ShowHeader(const String& headerName,
                              const String& headerValue,
                              wxFontEncoding encHeader)
{
   wxLayoutList *llist = m_window->GetLayoutList();
   const ProfileValues& profileValues = GetOptions();

   // don't show the header at all if there is no value
   if ( !headerValue.empty() )
   {
      // insert the header name
      llist->SetFontWeight(wxBOLD);
      SetTextColour(profileValues.HeaderNameCol);

      // always terminate the header names with ": " - configurability
      // cannot be endless neither
      llist->Insert(headerName + ": ");

      // insert the header value
      llist->SetFontWeight(wxNORMAL);
      SetTextColour(profileValues.HeaderValueCol);

      wxLayoutImportText(llist, headerValue, encHeader);
      llist->LineBreak();

      if ( encHeader != wxFONTENCODING_SYSTEM )
      {
         // restore the default encoding
         llist->SetFontEncoding(wxFONTENCODING_DEFAULT);
      }
   }
}

void LayoutViewer::ShowXFace(const wxBitmap& bitmap)
{
   wxLayoutList *llist = m_window->GetLayoutList();
   llist->Insert(new wxLayoutObjectIcon(new wxBitmap(bitmap)));
   llist->LineBreak();
}

void LayoutViewer::EndHeaders()
{
}

// ----------------------------------------------------------------------------
// body showing
// ----------------------------------------------------------------------------

void LayoutViewer::StartBody()
{
   // restore the normal colour as we changed it in ShowHeader() which was
   // called before
   SetTextColour(GetOptions().FgCol);
}

void LayoutViewer::StartPart()
{
   // put a blank line before each part start - including the very first one to
   // separate it from the headers
   m_window->GetLayoutList()->LineBreak();
}

void LayoutViewer::InsertAttachment(const wxBitmap& icon, ClickableInfo *ci)
{
   wxLayoutList *llist = m_window->GetLayoutList();

   wxLayoutObject *obj = new wxLayoutObjectIcon(icon);
   obj->SetUserData(new LayoutUserData(ci));

   // multiple images look better alligned vertically rather than
   // horizontally
   llist->Insert(obj);

   // add extra whitespace so lines with multiple icons can
   // be broken:
   llist->Insert(" ");
}

void LayoutViewer::InsertImage(const wxBitmap& image, ClickableInfo *ci)
{
   InsertAttachment(image, ci);
}

void LayoutViewer::InsertRawContents(const String& data)
{
   // as we return false from our CanProcess(), MessageView is not supposed to
   // ask us to process any raw data
   FAIL_MSG( "LayoutViewer::InsertRawContents() shouldn't be called" );
}

void LayoutViewer::InsertText(const String& text, const TextStyle& style)
{
   wxLayoutList *llist = m_window->GetLayoutList();

   wxColour colFg, colBg;
   if ( style.HasTextColour() )
      colFg = style.GetTextColour();
   if ( style.HasBackgroundColour() )
      colBg = style.GetBackgroundColour();

   llist->SetFontColour(&colFg, &colBg);

   wxFontEncoding enc = style.HasFont() ? style.GetFont().GetEncoding()
                                        : wxFONTENCODING_SYSTEM;

   wxLayoutImportText(llist, text, enc);
}

void LayoutViewer::InsertURL(const String& url)
{
   wxLayoutObject *obj = new wxLayoutObjectText(url);
   obj->SetUserData(new LayoutUserData(new ClickableInfo(url)));

   SetTextColour(GetOptions().UrlCol);
   m_window->GetLayoutList()->Insert(obj);
}

void LayoutViewer::InsertSignature(const String& signature)
{
   // this is not called by MessageView yet, but should be implemented when it
   // starts recognizing signatures in the messages
}

void LayoutViewer::EndPart()
{
   // this function intentionally left (almost) blank
}

void LayoutViewer::EndBody()
{
   wxLayoutList *llist = m_window->GetLayoutList();

   llist->LineBreak();
   llist->MoveCursorTo(wxPoint(0,0));

   // we have modified the list directly, so we need to mark the
   // window as dirty:
   m_window->SetDirty();

   // re-enable auto-formatting, seems safer for selection
   // highlighting, not sure if needed, though
   llist->SetAutoFormatting(TRUE);

   // setup the line wrap
   CoordType wrapMargin = READ_CONFIG(GetProfile(), MP_VIEW_WRAPMARGIN);
   m_window->SetWrapMargin(wrapMargin);
   if( wrapMargin > 0 && READ_CONFIG(GetProfile(), MP_VIEW_AUTOMATIC_WORDWRAP) )
      llist->WrapAll(wrapMargin);

   // yes, we allow the user to edit the buffer, in case he wants to
   // modify it for pasting or wrap lines manually:
   m_window->SetEditable(FALSE);
   m_window->SetCursorVisibility(-1);
   llist->ForceTotalLayout();

   // for safety, this is required to set scrollbars
   m_window->ScrollToCursor();

   Update();
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void LayoutViewer::EmulateKeyPress(int keycode)
{
   wxKeyEvent event;
   event.m_keyCode = keycode;

#ifdef __WXGTK__
   m_window->OnChar(event);
#else
   m_window->HandleOnChar(event);
#endif
}

/// scroll down one line:
bool
LayoutViewer::LineDown()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_DOWN);

   return check.HasChanged();
}

/// scroll up one line:
bool
LayoutViewer::LineUp()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_UP);

   return check.HasChanged();
}

/// scroll down one page:
bool
LayoutViewer::PageDown()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_PAGEDOWN);

   return check.HasChanged();
}

/// scroll up one page:
bool
LayoutViewer::PageUp()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_PAGEUP);

   return check.HasChanged();
}

// ----------------------------------------------------------------------------
// capabilities querying
// ----------------------------------------------------------------------------

bool LayoutViewer::CanInlineImages() const
{
   // we can show inline images
   return true;
}

bool LayoutViewer::CanProcess(const String& mimetype) const
{
   // we don't have any special processing for any MIME types
   return false;
}

