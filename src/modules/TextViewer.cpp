///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/TextViewer.cpp: implements text-only MessageViewer
// Purpose:     this is a wxTextCtrl-based implementation of MessageViewer
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.07.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
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
#   include "guidef.h"                  // for GetFrame
#   include "gui/wxMApp.h"              // for wxMApp

#   include <wx/textctrl.h>
#endif // USE_PCH

#include "gui/wxMenuDefs.h"
#include "MessageViewer.h"
#include "ClickURL.h"
#include "MTextStyle.h"

#include <wx/textbuf.h>

#include <wx/html/htmprint.h>   // for wxHtmlEasyPrinting

#ifdef __WXMSW__
   #include <wx/msw/private.h>
#endif // __WXMSW__

// only Win32 supports URLs in the text control natively so far, define this to
// use this possibility
//
// don't define it any longer because we do it now better than the native
// control
//#define USE_AUTO_URL_DETECTION

class TextViewerWindow;

// ----------------------------------------------------------------------------
// wxTextEasyPrinting: easy way to print text (TODO: move to wxWindows)
// ----------------------------------------------------------------------------

class wxTextEasyPrinting : public wxHtmlEasyPrinting
{
public:
   wxTextEasyPrinting(const wxString& name, wxWindow *parent = NULL)
      : wxHtmlEasyPrinting(name, GetFrame(parent)) { }

   bool Print(wxTextCtrl *text) { return PrintText(ControlToHtml(text)); }
   bool Preview(wxTextCtrl *text) { return PreviewText(ControlToHtml(text)); }

private:
   static wxString ControlToHtml(wxTextCtrl *text);
};

// ----------------------------------------------------------------------------
// TextViewer: a wxTextCtrl-based MessageViewer implementation
// ----------------------------------------------------------------------------

class TextViewer : public MessageViewer
{
public:
   // default ctor
   TextViewer();

   // creation &c
   virtual void Create(MessageView *msgView, wxWindow *parent);
   virtual void Clear();
   virtual void Update();
   virtual void UpdateOptions();
   virtual wxWindow *GetWindow() const;

   // operations
   virtual bool Find(const String& text);
   virtual bool FindAgain();
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
   virtual void EndText();
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
   // create m_printText if necessary
   void InitPrinting();


   // the viewer window
   TextViewerWindow *m_window;

   // the object which does the printing
   wxTextEasyPrinting *m_printText;

   // the position of the last match used by Find() and FindAgain()
   long m_posFind;

   // the text we're searched for the last time
   wxString m_textFind;

   DECLARE_MESSAGE_VIEWER()
};

// ----------------------------------------------------------------------------
// TextViewerClickable: an object we can click on in the text control
// ----------------------------------------------------------------------------

class TextViewerClickable
{
public:
   TextViewerClickable(ClickableInfo *ci, long pos, size_t len)
   {
      m_start = pos;
      m_len = len;
      m_ci = ci;
   }

   ~TextViewerClickable() { delete m_ci; }

   // accessors

   bool Inside(long pos) const
      { return pos >= m_start && pos - m_start < m_len; }

   const ClickableInfo *GetClickableInfo() const { return m_ci; }

private:
   // the range of the text we correspond to
   long m_start,
        m_len;

   ClickableInfo *m_ci;
};

WX_DEFINE_ARRAY(TextViewerClickable *, ArrayClickables);

// ----------------------------------------------------------------------------
// TextViewerWindow: the viewer window used by TextViewer
// ----------------------------------------------------------------------------

class TextViewerWindow : public wxTextCtrl
{
public:
   TextViewerWindow(TextViewer *viewer, wxWindow *parent);
   virtual ~TextViewerWindow();

   void InsertClickable(const wxString& text,
                        ClickableInfo *ci,
                        const wxColour& col = wxNullColour);

   // override some base class virtuals
   virtual void Clear();
   virtual bool AcceptsFocusFromKeyboard() const { return FALSE; }

private:
#ifdef USE_AUTO_URL_DETECTION
   void OnLinkEvent(wxTextUrlEvent& event);
#endif // USE_AUTO_URL_DETECTION

#ifdef __WXMSW__
   // get the text position from the coords
   long GetTextPositionFromCoords(const wxPoint& pt) const;
#endif // __WXMSW__

   // the generic mouse event handler for right/left/double clicks
   void OnMouseEvent(wxMouseEvent& event);

   // process the mouse click at the given text position
   bool ProcessMouseEvent(const wxMouseEvent& event, long pos);

   TextViewer *m_viewer;

   // all "active" objects
   ArrayClickables m_clickables;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(TextViewerWindow)
};

// ============================================================================
// wxTextEasyPrinting implementation
// ============================================================================

/* static */
wxString wxTextEasyPrinting::ControlToHtml(wxTextCtrl *text)
{
   wxCHECK_MSG( text, wxEmptyString, _T("NULL control in wxTextEasyPrinting") );

   const long posEnd = text->GetLastPosition();

   wxString s;
   s.reserve(posEnd + 100);

   s << _T("<html><body><tt>");

   wxString ch;
   wxTextAttr attr, attrNew;
   for ( long pos = 0; pos < posEnd; pos++ )
   {
      ch = text->GetRange(pos, pos + 1);

      // this is not implemented in wxGTK and doesn't want to work under MSW
      // for some reason (TODO: debug it)
#if 0
      if ( text->GetStyle(pos, attrNew) )
      {
         bool changed = false;
         if ( attrNew.GetTextColour() != attr.GetTextColour() )
         {
            if ( attrNew.HasTextColour() )
            {
               s << _T("<font color=\"#")
                 << Col2Html(attrNew.GetTextColour())
                 << _T("\">");
            }
            else
            {
               s << _T("</font>");
            }

            changed = true;
         }

         if ( changed )
         {
            attr = attrNew;
         }
      }
#endif // 0

      switch ( ch[0u] )
      {
         case '<':
            s += "&lt;";
            break;

         case '>':
            s += "&gt;";
            break;

         case '&':
            s += "&amp;";
            break;

         case '"':
            s += "&quot;";
            break;

         case '\t':
            // we hardcode the tab width to 8 spaces
            s += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
            break;

         case '\r':
            // ignore, we process '\n' below
            break;

         case '\n':
            s += "<br>\n";
            break;

         case ' ':
            s += "&nbsp;";
            break;

         default:
            s += ch[0u];
      }
   }

   s << _T("</tt></body></html>");

   return s;
}

// ============================================================================
// TextViewerWindow implementation
// ============================================================================

BEGIN_EVENT_TABLE(TextViewerWindow, wxTextCtrl)
#ifdef USE_AUTO_URL_DETECTION
   EVT_TEXT_URL(-1, TextViewerWindow::OnLinkEvent)
#endif // USE_AUTO_URL_DETECTION

   EVT_RIGHT_UP(TextViewerWindow::OnMouseEvent)
   EVT_LEFT_UP(TextViewerWindow::OnMouseEvent)
   EVT_LEFT_DCLICK(TextViewerWindow::OnMouseEvent)
END_EVENT_TABLE()

TextViewerWindow::TextViewerWindow(TextViewer *viewer, wxWindow *parent)
                : wxTextCtrl(parent, -1, "",
                             wxDefaultPosition, wxDefaultSize,
                             wxTE_RICH2 |
#ifdef USE_AUTO_URL_DETECTION
                             wxTE_AUTO_URL |
#endif // USE_AUTO_URL_DETECTION
                             wxTE_MULTILINE)
{
   m_viewer = viewer;

   SetEditable(false);
}

TextViewerWindow::~TextViewerWindow()
{
   WX_CLEAR_ARRAY(m_clickables);
}

void TextViewerWindow::InsertClickable(const wxString& text,
                                       ClickableInfo *ci,
                                       const wxColour& col)
{
   if ( col.Ok() )
   {
      SetDefaultStyle(wxTextAttr(col));
   }

   TextViewerClickable *clickable =
      new TextViewerClickable(ci, GetLastPosition(), text.length());
   m_clickables.Add(clickable);

   AppendText(text);

   if ( col.Ok() )
   {
      // reset the style back to the previous one
      SetDefaultStyle(wxTextAttr());
   }
}

void TextViewerWindow::Clear()
{
   wxTextCtrl::Clear();

   // reset the default style because it could have font with an encoding which
   // we shouldn't use for the next message we'll show
   SetDefaultStyle(wxTextAttr());

   WX_CLEAR_ARRAY(m_clickables);
}

#ifdef USE_AUTO_URL_DETECTION

void TextViewerWindow::OnLinkEvent(wxTextUrlEvent& event)
{
   wxMouseEvent eventMouse = event.GetMouseEvent();
   wxEventType type = eventMouse.GetEventType();
   if ( type == wxEVT_RIGHT_UP ||
        type == wxEVT_LEFT_UP ||
        type == wxEVT_LEFT_DCLICK )
   {
      if ( ProcessMouseEvent(eventMouse, event.GetURLStart()) )
      {
         // skip event.Skip() below
         return;
      }
   }

   event.Skip();
}

#endif // USE_AUTO_URL_DETECTION

#ifdef __WXMSW__

long TextViewerWindow::GetTextPositionFromCoords(const wxPoint& pt) const
{
   POINTL ptl = { pt.x, pt.y };

   // can't use SendMessage because it's a class name!
   return ::SendMessageA(GetHwnd(), EM_CHARFROMPOS, 0, (LPARAM)&ptl);
}

#endif // __WXMSW__

void TextViewerWindow::OnMouseEvent(wxMouseEvent& event)
{
#ifdef __WXMSW__
   long pos = GetTextPositionFromCoords(event.GetPosition());
#else
   long pos = GetInsertionPoint();
#endif // __WXMSW__/!__WXMSW__

   if ( !ProcessMouseEvent(event, pos) )
   {
      event.Skip();
   }
}

bool TextViewerWindow::ProcessMouseEvent(const wxMouseEvent& event, long pos)
{
   size_t count = m_clickables.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      TextViewerClickable *clickable = m_clickables[n];
      if ( clickable->Inside(pos) )
      {
         int id;
         if ( event.RightUp() )
         {
            id = WXMENU_LAYOUT_RCLICK;
         }
         else if ( event.LeftUp() )
         {
            // the mouse cursor is captured by the text control when we're here
            // and this results in very strange behaviour if we open a window
            // without releasing mouse capture first
            ReleaseMouse();

            id = WXMENU_LAYOUT_LCLICK;
         }
         else // must be double click, what else?
         {
            ASSERT_MSG( event.LeftDClick(), _T("unexpected mouse event") );

            id = WXMENU_LAYOUT_DBLCLICK;
         }

         m_viewer->DoMouseCommand(id, clickable->GetClickableInfo(),
                                  event.GetPosition());

         return true;
      }
   }

   return false;
}

// ============================================================================
// TextViewer implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

IMPLEMENT_MESSAGE_VIEWER(TextViewer,
                         _("Text only message viewer"),
                         "(c) 2001 Vadim Zeitlin <vadim@wxwindows.org>");

TextViewer::TextViewer()
{
   m_window = NULL;
   m_printText = NULL;
   m_posFind = -1;
}

// ----------------------------------------------------------------------------
// TextViewer creation &c
// ----------------------------------------------------------------------------

void TextViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;
   m_window = new TextViewerWindow(this, parent);
}

void TextViewer::Clear()
{
   m_window->Clear();

   m_window->Freeze();

   const ProfileValues& profileValues = GetOptions();

   m_window->SetFont(profileValues.GetFont());
   m_window->SetForegroundColour(profileValues.FgCol);
   m_window->SetBackgroundColour(profileValues.BgCol);
}

void TextViewer::Update()
{
   m_window->Thaw();
}

void TextViewer::UpdateOptions()
{
   // no special options

   // TODO: support for line wrap
}

// ----------------------------------------------------------------------------
// TextViewer operations
// ----------------------------------------------------------------------------

bool TextViewer::Find(const String& text)
{
   m_posFind = -1;
   m_textFind = text;

   return FindAgain();
}

bool TextViewer::FindAgain()
{
   const char *pStart = m_window->GetValue();

   const char *p = pStart;
   if ( m_posFind != -1 )
   {
      // start looking at the next position after the last match
      p += m_posFind + 1;
   }

   p = *p != '\0' ? strstr(p, m_textFind) : NULL;

   if ( p )
   {
      m_posFind = p - pStart;

      m_window->SetSelection(m_posFind, m_posFind + m_textFind.length());
   }
   else // not found
   {
      m_window->SetSelection(0, 0);
   }

   return p != NULL;
}

void TextViewer::Copy()
{
   m_window->Copy();
}

String TextViewer::GetSelection() const
{
   return m_window->GetStringSelection();
}

// ----------------------------------------------------------------------------
// TextViewer printing
// ----------------------------------------------------------------------------

void TextViewer::InitPrinting()
{
   if ( !m_printText )
   {
      m_printText = new wxTextEasyPrinting(_("Mahogany Printing"),
                                           GetFrame(m_window));
   }

   wxMApp *app = (wxMApp *)mApplication;

   *m_printText->GetPrintData() = *app->GetPrintData();
   *m_printText->GetPageSetupData() = *app->GetPageSetupData();
}

bool TextViewer::Print()
{
   InitPrinting();

   return m_printText->Print(m_window);
}

void TextViewer::PrintPreview()
{
   InitPrinting();

   (void)m_printText->Preview(m_window);
}

wxWindow *TextViewer::GetWindow() const
{
   return m_window;
}

// ----------------------------------------------------------------------------
// header showing
// ----------------------------------------------------------------------------

void TextViewer::StartHeaders()
{
}

void TextViewer::ShowRawHeaders(const String& header)
{
   m_window->AppendText(header);
}

void TextViewer::ShowHeaderName(const String& name)
{
   const ProfileValues& profileValues = GetOptions();

   MTextStyle attr(profileValues.HeaderNameCol);
   wxFont font = m_window->GetFont();
   font.SetWeight(wxFONTWEIGHT_BOLD);
   attr.SetFont(font);

   InsertText(name + ": ", attr);

   // restore the non bold font
   attr.SetFont(m_window->GetFont());
   m_window->SetDefaultStyle(attr);
}

void TextViewer::ShowHeaderValue(const String& value,
                                 wxFontEncoding encoding)
{
   const ProfileValues& profileValues = GetOptions();

   wxColour col = profileValues.HeaderValueCol;
   if ( !col.Ok() )
      col = profileValues.FgCol;

   MTextStyle attr(col);
   if ( encoding != wxFONTENCODING_SYSTEM )
   {
      wxFont font = profileValues.GetFont(encoding);
      attr.SetFont(font);
   }

   InsertText(value, attr);
}

void TextViewer::ShowHeaderURL(const String& text,
                               const String& url)
{
   InsertURL(text, url);
}

void TextViewer::EndHeader()
{
   InsertText("\n", MTextStyle());
}

void TextViewer::ShowXFace(const wxBitmap& /* bitmap */)
{
   // we don't show XFaces
}

void TextViewer::EndHeaders()
{
}

// ----------------------------------------------------------------------------
// body showing
// ----------------------------------------------------------------------------

void TextViewer::StartBody()
{
}

void TextViewer::StartPart()
{
   // put a blank line before each part start - including the very first one to
   // separate it from the headers
   m_window->AppendText("\n");
}

void TextViewer::InsertAttachment(const wxBitmap& /* icon */, ClickableInfo *ci)
{
   String str;
   str << _("[Attachment: ") << ci->GetLabel() << _T(']');

   m_window->InsertClickable(str, ci, GetOptions().AttCol);
}

void TextViewer::InsertClickable(const wxBitmap& /* icon */,
                                 ClickableInfo *ci,
                                 const wxColour& col)
{
   String str;
   str << _T('[') << ci->GetLabel() << _T(']');

   m_window->InsertClickable(str, ci, col);
}

void
TextViewer::InsertImage(const wxImage& /* image */, ClickableInfo * /* ci */)
{
   // as we return false from CanInlineImages() this is not supposed to be
   // called
   FAIL_MSG( _T("unexpected call to TextViewer::InsertImage") );
}

void TextViewer::InsertRawContents(const String& /* data */)
{
   // as we return false from our CanProcess(), MessageView is not supposed to
   // ask us to process any raw data
   FAIL_MSG( _T("unexpected call to TextViewer::InsertRawContents()") );
}

void TextViewer::InsertText(const String& text, const MTextStyle& style)
{
   m_window->SetDefaultStyle(style);

#ifdef OS_WIN
   m_window->AppendText(text);
#else
   m_window->AppendText(wxTextBuffer::Translate(text, wxTextFileType_Unix));
#endif
}

void TextViewer::InsertURL(const String& text, const String& url)
{
   m_window->InsertClickable(text,
                             new ClickableURL(m_msgView, url),
                             GetOptions().UrlCol);
}

void TextViewer::EndText()
{
   // we don't need to do anything special here
}

void TextViewer::EndPart()
{
   // this function intentionally left (almost) blank
}

void TextViewer::EndBody()
{
   m_window->SetInsertionPoint(0);

   m_window->Thaw();
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

bool TextViewer::LineDown()
{
   return m_window->LineDown();
}

bool TextViewer::LineUp()
{
   return m_window->LineUp();
}

bool TextViewer::PageDown()
{
   return m_window->PageDown();
}

bool TextViewer::PageUp()
{
   return m_window->PageUp();
}

// ----------------------------------------------------------------------------
// capabilities querying
// ----------------------------------------------------------------------------

bool TextViewer::CanInlineImages() const
{
   // we can't show any images
   return false;
}

bool TextViewer::CanProcess(const String& /* mimetype */) const
{
   // we don't have any special processing for any MIME types
   return false;
}

