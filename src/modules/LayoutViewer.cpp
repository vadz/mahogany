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
#   include "Mcommon.h"
#   include "Mdefaults.h"
#   include "guidef.h"
#endif // USE_PCH

#include "MessageViewer.h"
#include "gui/wxMenuDefs.h"
#include "ClickURL.h"
#include "MTextStyle.h"
#include "gui/wxMDialogs.h"

#include "gui/wxlwindow.h"
#include "gui/wxlparser.h"

class LayoutViewerWindow;

// ----------------------------------------------------------------------------
// options we use
// ----------------------------------------------------------------------------

extern const MOption MP_FOCUS_FOLLOWSMOUSE;
extern const MOption MP_VIEW_AUTOMATIC_WORDWRAP;
extern const MOption MP_VIEW_WRAPMARGIN;

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
   virtual bool Find(const String& text);
   virtual bool FindAgain();
   virtual void SelectAll();
   virtual String GetSelection() const;
   virtual void Copy();
   virtual bool Print();
   virtual void PrintPreview();

   // header showing
   virtual void StartHeaders();
   virtual void ShowRawHeaders(const String& header);
   virtual void ShowHeaderName(const String& name);
   virtual void ShowHeaderValue(const String& value,
                                wxFontEncoding encoding);
   virtual void ShowHeaderURL(const String& text,
                              const String& url);
   virtual void EndHeader();
   virtual void ShowXFace(const wxBitmap& bitmap);
   virtual void EndHeaders();

   // body showing
   virtual void StartBody();
   virtual void StartPart();
   virtual void InsertAttachment(const wxBitmap& icon, ClickableInfo *ci);
   virtual void InsertClickable(const wxBitmap& icon,
                                ClickableInfo *ci,
                                const wxColour& col);
   virtual void InsertImage(const wxImage& image, ClickableInfo *ci);
   virtual void InsertRawContents(const String& data);
   virtual void InsertText(const String& text, const MTextStyle& style);
   virtual void InsertURL(const String& text, const String& url);
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

   virtual bool AcceptsFocusFromKeyboard() const { return FALSE; }

private:
   void OnMouseEvent(wxCommandEvent& event);

   LayoutViewer *m_viewer;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(LayoutViewerWindow)
};

// ----------------------------------------------------------------------------
// LayoutUserData: our wrapper for ClickableInfo which is needed because we
//                 we can't associate it directly with a wxLayoutObject which
//                 wants to have something derived from its UserData
// ----------------------------------------------------------------------------

class LayoutUserData : public wxLayoutObject::UserData
{
public:
   // we take ownership of ClickableInfo object
   LayoutUserData(ClickableInfo *ci)
   {
      m_ci = ci;

      SetLabel(m_ci->GetLabel());
   }

   virtual ~LayoutUserData() { delete m_ci; }

   // get the real data back
   ClickableInfo *GetClickableInfo() const { return m_ci; }

private:
   ClickableInfo *m_ci;
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
   LayoutUserData *data = (LayoutUserData *)obj->GetUserData();
   if ( data )
   {
      int id;
      switch ( event.GetId() )
      {
         case WXLOWIN_MENU_RCLICK:
            id = WXMENU_LAYOUT_RCLICK;
            break;

         default:
            FAIL_MSG(_T("unknown mouse action"));

         case WXLOWIN_MENU_LCLICK:
            id = WXMENU_LAYOUT_LCLICK;
            break;

         case WXLOWIN_MENU_DBLCLICK:
            id = WXMENU_LAYOUT_DBLCLICK;
            break;
      }

      m_viewer->DoMouseCommand(id, data->GetClickableInfo(), GetClickPosition());
   }
}

// ============================================================================
// LayoutViewer implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

IMPLEMENT_MESSAGE_VIEWER(LayoutViewer,
                         _("Rich text message viewer"),
                         "(c) 1997-2001 Mahogany Team");

LayoutViewer::LayoutViewer()
{
   m_window = NULL;
}

void LayoutViewer::SetTextColour(const wxColour& colToSet)
{
   wxColour col = colToSet.Ok() ? colToSet : GetOptions().FgCol;

   // const_cast is harmless but needed because of the broken
   // wxLayoutWindow API
   m_window->GetLayoutList()->SetFontColour((wxColour *)&col);
}

// ----------------------------------------------------------------------------
// LayoutViewer creation &c
// ----------------------------------------------------------------------------

void LayoutViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;
   m_window = new LayoutViewerWindow(this, parent);
#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   m_window ->
      SetFocusFollowMode(READ_CONFIG_BOOL(GetProfile(), MP_FOCUS_FOLLOWSMOUSE));
#endif
}

void LayoutViewer::Clear()
{
   // show cursor on demand
   m_window->SetCursorVisibility(-1);

   // reset visibility params
   const ProfileValues& profileValues = GetOptions();
   m_window->Clear(profileValues.GetFont(),
                   (wxColour *)&profileValues.FgCol,
                   (wxColour *)&profileValues.BgCol,
                   true /* no update */);

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

#if defined(__WXGTK__) || defined(EXPERIMENTAL_FOCUS_FOLLOWS)
   m_window->SetFocusFollowMode(READ_CONFIG_BOOL(profile, MP_FOCUS_FOLLOWSMOUSE));
#endif

   m_window->SetWrapMargin(READ_CONFIG(profile, MP_VIEW_WRAPMARGIN));
}

// ----------------------------------------------------------------------------
// LayoutViewer operations
// ----------------------------------------------------------------------------

bool LayoutViewer::Find(const String& text)
{
   return m_window->Find(text);
}

bool LayoutViewer::FindAgain()
{
   return m_window->FindAgain();
}

void LayoutViewer::Copy()
{
   m_window->Copy();
}

void LayoutViewer::SelectAll()
{
   wxLayoutList *llist = m_window->GetLayoutList();
   llist->StartSelection(wxPoint(0, 0));
   llist->EndSelection(wxPoint(1000, 1000));
   m_window->Refresh();
}

String LayoutViewer::GetSelection() const
{
   String sel;

   wxLayoutList *llist = m_window->GetLayoutList();
   if ( llist->HasSelection() )
   {
      wxLayoutList *llistSel = llist->GetSelection(NULL, false);

      wxLayoutExportStatus status(llistSel);
      wxLayoutExportObject *exp;
      while( (exp = wxLayoutExport(&status)) != NULL )
      {
         switch ( exp->type )
         {
            case WXLO_EXPORT_TEXT:
               sel += *exp->content.text;
               break;

            case WXLO_EXPORT_EMPTYLINE:
               sel += _T("\n");
               break;

            default:
               FAIL_MSG( _T("unexpected wxLayoutExport result") );
         }
      }

      delete llistSel;
   }

   return sel;
}

// ----------------------------------------------------------------------------
// LayoutViewer printing
// ----------------------------------------------------------------------------

bool LayoutViewer::Print()
{
#if wxUSE_PRINTING_ARCHITECTURE
   return wxLayoutPrintout::Print(m_window, m_window->GetLayoutList());
#else // !wxUSE_PRINTING_ARCHITECTURE
   return false;
#endif // wxUSE_PRINTING_ARCHITECTURE/!wxUSE_PRINTING_ARCHITECTURE
}

void LayoutViewer::PrintPreview()
{
#if wxUSE_PRINTING_ARCHITECTURE
   (void)wxLayoutPrintout::PrintPreview(m_window->GetLayoutList());
#endif // wxUSE_PRINTING_ARCHITECTURE
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

void LayoutViewer::ShowHeaderName(const String& name)
{
   wxLayoutList *llist = m_window->GetLayoutList();

   // insert the header name
   llist->SetFontWeight(wxBOLD);
   SetTextColour(GetOptions().HeaderNameCol);

   // always terminate the header names with ": " - configurability
   // cannot be endless neither
   llist->Insert(name + _T(": "));

   llist->SetFontWeight(wxNORMAL);
}

void LayoutViewer::ShowHeaderValue(const String& value,
                                   wxFontEncoding encoding)
{
   SetTextColour(GetOptions().HeaderValueCol);

   wxLayoutImportText(m_window->GetLayoutList(), value, encoding);
}

void LayoutViewer::ShowHeaderURL(const String& text,
                                 const String& url)
{
   InsertURL(text, url);
}

void LayoutViewer::EndHeader()
{
   wxLayoutList *llist = m_window->GetLayoutList();

   llist->LineBreak();

   // restore the default encoding after possibly changing it in
   // ShowHeaderValue()
   llist->SetFontEncoding(wxFONTENCODING_DEFAULT);
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
   LayoutUserData* data = new LayoutUserData(ci);
   obj->SetUserData(data);
   // SetUserData has incremented the refCount, which is now 2
   // (it was already 1 right after creation)
   data->DecRef();

   // multiple images look better alligned vertically rather than
   // horizontally
   llist->Insert(obj);

   // add extra whitespace so lines with multiple icons can
   // be broken:
   llist->Insert(_T(" "));
}

void LayoutViewer::InsertClickable(const wxBitmap& icon,
                                   ClickableInfo *ci,
                                   const wxColour& /* col */)
{
   InsertAttachment(icon, ci);
}

void LayoutViewer::InsertImage(const wxImage& image, ClickableInfo *ci)
{
   InsertAttachment(wxBitmap(image), ci);
}

void LayoutViewer::InsertRawContents(const String& /* data */)
{
   // as we return false from our CanProcess(), MessageView is not supposed to
   // ask us to process any raw data
   FAIL_MSG( _T("LayoutViewer::InsertRawContents() shouldn't be called") );
}

void LayoutViewer::InsertText(const String& text, const MTextStyle& style)
{
   wxLayoutList *llist = m_window->GetLayoutList();

   bool hasFont = style.HasFont();
   if ( hasFont )
     llist->SetFont(style.GetFont());

   wxColour colFg, colBg;
   if ( style.HasTextColour() )
      colFg = style.GetTextColour();
   else
      colFg = GetOptions().FgCol;
   if ( style.HasBackgroundColour() )
      colBg = style.GetBackgroundColour();
   else
      colBg = GetOptions().BgCol;

   llist->SetFontColour(colFg.Ok() ? &colFg : NULL,
                        colBg.Ok() ? &colBg : NULL);

   wxFontEncoding enc = hasFont ? style.GetFont().GetEncoding()
                                : wxFONTENCODING_SYSTEM;

   wxLayoutImportText(llist, text, enc);
}

void LayoutViewer::InsertURL(const String& textOrig, const String& url)
{
   wxLayoutList *llist = m_window->GetLayoutList();

   LayoutUserData* data = new LayoutUserData(new ClickableURL(m_msgView, url));
   SetTextColour(GetOptions().UrlCol);

   // the text can contain newlines (when we have a wrapped URL) but a single
   // wxLayoutObject can't span several lines
   String text = textOrig,
          textRest;
   do
   {
      const wxChar *p0 = text;
      const wxChar *p = wxStrchr(p0, _T('\n'));
      if ( p )
      {
         textRest = text.substr(p - p0 + 1);

         if ( p > p0 && *(p - 1) == '\r' )
         {
            p--;
         }

         text.erase(p - p0);
      }
      else // no newline
      {
         textRest.clear();
      }

      wxLayoutObject *obj = new wxLayoutObjectText(text);
      obj->SetUserData(data);
      llist->Insert(obj);

      if ( p )
         llist->LineBreak();

      text = textRest;
   }
   while ( !text.empty() );

   // SetUserData has incremented the refCount, which is now 2
   // (it was already 1 right after creation)
   data->DecRef();
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

bool LayoutViewer::CanProcess(const String& /* mimetype */) const
{
   // we don't have any special processing for any MIME types
   return false;
}

