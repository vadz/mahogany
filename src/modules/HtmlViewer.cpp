///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/HtmlViewer.cpp: implements MessageViewer using wxHTML
// Purpose:     this viewer translates the non-HTML message parts to HTML
//              and is capable of directly showing the HTML ones
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
   #include "Mcommon.h"

   #include "Profile.h"

   #include "gui/wxMApp.h"
#endif // USE_PCH

#include "Mdefaults.h"

#include "MessageView.h"
#include "MessageViewer.h"

#include <wx/fs_mem.h>
#include <wx/wxhtml.h>

class HtmlViewerWindow;

// ----------------------------------------------------------------------------
// HtmlViewer: a wxTextCtrl-based MessageViewer implementation
// ----------------------------------------------------------------------------

class HtmlViewer : public MessageViewer
{
public:
   // ctor/dtor
   HtmlViewer();
   virtual ~HtmlViewer();

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
   // HTML helpers
   // ------------

   // add "attr=#colour" attribute to m_htmlText if col is valid
   void AddColourAttr(const wxChar *attr, const wxColour& col);

   // add "<font>" tag if necessary, return true if it was added
   bool AddFontTag(const wxColour& col);

   // create a new image in memory, returns its name
   wxString CreateImageInMemoryFS(const wxBitmap& bitmap);

   // free all images we created in memory
   void FreeMemoryFS();

   // emulate a key press: this is the only way I found to scroll
   // wxScrolledWindow
   void EmulateKeyPress(int keycode);

   // the viewer window
   HtmlViewerWindow *m_window;

   // the HTML string: we accumulate HTML in it and render it in EndBody()
   wxString m_htmlText;

   // count of images used to generate unique names for them
   size_t m_nImage;

   // current MIME part number
   size_t m_nPart;

   // the XFace if we have any
   wxBitmap m_bmpXFace;

   DECLARE_MESSAGE_VIEWER()
};

// ----------------------------------------------------------------------------
// HtmlViewerWindow: wxLayoutWindow used by HtmlViewer
// ----------------------------------------------------------------------------

class HtmlViewerWindow : public wxHtmlWindow
{
public:
   HtmlViewerWindow(HtmlViewer *viewer, wxWindow *parent);

   virtual void OnLinkClicked(const wxHtmlLinkInfo& link);

private:
   HtmlViewer *m_viewer;
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

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return RRGGBB HTML colour spec
static wxString Col2Html(const wxColour& col);

// filter out special HTML characters from the text
static wxString MakeHtmlSafe(const wxString& text);

// ============================================================================
// HtmlViewerWindow implementation
// ============================================================================

HtmlViewerWindow::HtmlViewerWindow(HtmlViewer *viewer, wxWindow *parent)
                : wxHtmlWindow(parent)
{
   m_viewer = viewer;
}

void HtmlViewerWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
   // TODO: where should we store the associated ClickableInfo?
   //m_viewer->DoMouseCommand(WXMENU_LAYOUT_DBLCLICK,
   //                         GetClickableInfo(),
   //                         link.GetEvent()->GetPosition());
}

// ============================================================================
// HtmlViewer implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

IMPLEMENT_MESSAGE_VIEWER(HtmlViewer,
                         _("HTML message viewer"),
                         "(c) 2001 Vadim Zeitlin <vadim@wxwindows.org>");

HtmlViewer::HtmlViewer()
{
   m_window = NULL;

   m_nPart =
   m_nImage = 0;

   m_htmlText.reserve(4096);
}

HtmlViewer::~HtmlViewer()
{
   FreeMemoryFS();
}

// ----------------------------------------------------------------------------
// HtmlViewer creation &c
// ----------------------------------------------------------------------------

void HtmlViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;
   m_window = new HtmlViewerWindow(this, parent);
}

void HtmlViewer::Clear()
{
   m_window->SetPage("");

   m_htmlText.clear();

   m_nPart = 0;

   m_bmpXFace = wxNullBitmap;

   FreeMemoryFS();
}

void HtmlViewer::Update()
{
   // nothing to do here
}

void HtmlViewer::UpdateOptions()
{
   // no special options
}

// ----------------------------------------------------------------------------
// HtmlViewer operations
// ----------------------------------------------------------------------------

void HtmlViewer::Find(const String& text)
{
   // TODO
}

void HtmlViewer::FindAgain()
{
   // TODO
}

void HtmlViewer::Copy()
{
   // TODO
}

// ----------------------------------------------------------------------------
// HtmlViewer printing
// ----------------------------------------------------------------------------

bool HtmlViewer::Print()
{
   // TODO

   return false;
}

void HtmlViewer::PrintPreview()
{
   // TODO
}

wxWindow *HtmlViewer::GetWindow() const
{
   return m_window;
}

// ----------------------------------------------------------------------------
// HTML helpers
// ----------------------------------------------------------------------------

static wxString Col2Html(const wxColour& col)
{
   return wxString::Format("%02x%02x%02x",
                           (int)col.Red(), (int)col.Green(), (int)col.Blue());
}

static wxString MakeHtmlSafe(const wxString& text)
{
   wxString textSafe;
   textSafe.reserve(text.length());

   for ( const char *p = text.c_str(); *p; p++ )
   {
      switch ( *p )
      {
         case '<':
            textSafe += "&lt;";
            break;

         case '>':
            textSafe += "&gt;";
            break;

         case '&':
            textSafe += "&amp;";
            break;

         case '"':
            textSafe += "&quot;";
            break;

         case '\r':
            // ignore, we process '\n' below
            break;

         case '\n':
            // don't put <br> at the end as we're inside <pre> anyhow, so
            // there will a line break anyhow
            if ( p[1] )
            {
               textSafe += "<br>";
            }
            break;

         default:
            textSafe += *p;
      }
   }

   return textSafe;
}

bool HtmlViewer::AddFontTag(const wxColour& col)
{
   if ( !col.Ok() || col == GetOptions().FgCol )
      return false;

   m_htmlText += "<font";
   AddColourAttr("color", col);
   m_htmlText += '>';

   return true;
}

void HtmlViewer::AddColourAttr(const wxChar *attr, const wxColour& col)
{
   if ( col.Ok() )
   {
      m_htmlText += wxString::Format(" %s=\"#%s\"",
                                     attr, Col2Html(col).c_str());
   }
}

wxString HtmlViewer::CreateImageInMemoryFS(const wxBitmap& bitmap)
{
   wxString filename = wxString::Format("image%d.png", m_nImage++);
   wxMemoryFSHandler::AddFile(filename, bitmap, wxBITMAP_TYPE_PNG);
   return filename;
}

void HtmlViewer::FreeMemoryFS()
{
   for ( size_t n = 0; n < m_nImage; n++ )
   {
      wxMemoryFSHandler::RemoveFile(wxString::Format("image%d.png", n));
   }

   m_nImage = 0;
}

// ----------------------------------------------------------------------------
// header showing
// ----------------------------------------------------------------------------

void HtmlViewer::StartHeaders()
{
   // set the default attributes
   const ProfileValues& profileValues = GetOptions();

   m_htmlText = "<html><body";

   AddColourAttr("text", profileValues.FgCol);
   AddColourAttr("bgcolor", profileValues.BgCol);
   AddColourAttr("link", profileValues.UrlCol);

   // close <body> tag and start the table containing the headers
   m_htmlText += ">"
                 "<table cellspacing=1 cellpadding=1 border=0>";
}

void HtmlViewer::ShowRawHeaders(const String& header)
{
   m_htmlText << "<pre>" << header << "</pre>";
}

void HtmlViewer::ShowHeader(const String& headerName,
                            const String& headerValue,
                            wxFontEncoding encHeader)
{
   // don't show empty headers at all
   if ( headerValue.empty() )
      return;

   const ProfileValues& profileValues = GetOptions();

   // first row: header names
   m_htmlText += "<tr>"
                 // FIXME: wxHTML doesn't support width="20%" construct
                 "<td align=\"right\" width=120>"
                 ;

   bool hasFont = AddFontTag(profileValues.HeaderNameCol);

   m_htmlText << "<tt>" << headerName << ":&nbsp;</tt>";

   if ( hasFont )
      m_htmlText += "</font>";

   // second row: header values
   m_htmlText += "</td><td>";

   hasFont = AddFontTag(profileValues.HeaderValueCol);

   m_htmlText << "<pre>" << MakeHtmlSafe(headerValue) << "</pre>";

   if ( hasFont )
      m_htmlText += "</font>";

   m_htmlText += "</td></tr>";

   // TODO: handle encHeader!!
}

void HtmlViewer::ShowXFace(const wxBitmap& bitmap)
{
   // make a new table inside the header table for the headers
   m_htmlText += "<td><table cellspacing=1 cellpadding=1 border=0>";

   m_bmpXFace = bitmap;
}

void HtmlViewer::EndHeaders()
{
   // close the headers table
   m_htmlText += "</table>";

   if ( m_bmpXFace.Ok() )
   {
      wxString filename = CreateImageInMemoryFS(m_bmpXFace);
      m_htmlText << "</td><td width="
                 << wxString::Format("%d", m_bmpXFace.GetWidth()) << ">"
                    "<img src=\"memory:" << filename << "\">"
                    "</td>"
                    "</table>";
   }

   // skip a line before the body
   m_htmlText += "<br>";
}

// ----------------------------------------------------------------------------
// body showing
// ----------------------------------------------------------------------------

void HtmlViewer::StartBody()
{
   m_htmlText += "<br>";
}

void HtmlViewer::StartPart()
{
   // no, looks ugly: we should only do this if the last part was a text one
   // and this one is too, but how to know?
#if 0
   if ( m_nPart++ )
   {
      // separate subsequent parts from each other but don't insert the
      // separator between the headers and the first one
      m_htmlText += "<hr>";
   }
#endif // 0

   m_htmlText += "<p>";
}

void HtmlViewer::InsertAttachment(const wxBitmap& icon, ClickableInfo *ci)
{
   wxString filename = CreateImageInMemoryFS(icon);

   m_htmlText << "<img alt=\"" << ci->GetLabel() << "\" src=\"memory:"
              << filename << "\">";

   // TODO: remember ClickableInfo
}

void HtmlViewer::InsertImage(const wxBitmap& image, ClickableInfo *ci)
{
   wxString filename = CreateImageInMemoryFS(image);

   m_htmlText << "<p><img alt=\"" << ci->GetLabel() << "\" src=\"memory:"
              << filename << "\">";

   // TODO: remember ClickableInfo
}

void HtmlViewer::InsertRawContents(const String& data)
{
   m_htmlText += data;
}

void HtmlViewer::InsertText(const String& text, const TextStyle& style)
{
   if ( text == "\r\n" )
   {
      m_htmlText += "<br>";

      return;
   }

   // TODO: encoding support!!

   bool hasFont = AddFontTag(style.GetTextColour());

   // FIXME: avoid having unnecessary "</pre><pre>" in the text
   m_htmlText << "<pre>" << MakeHtmlSafe(text) << "</pre>";

   if ( hasFont )
      m_htmlText += "</font>";
}

void HtmlViewer::InsertURL(const String& url)
{
   m_htmlText << "<a href=\"" << url << "\">" << url << "</a>";
}

void HtmlViewer::InsertSignature(const String& signature)
{
   // this is not called by MessageView yet, but should be implemented when it
   // starts recognizing signatures in the messages
   FAIL_MSG( "not implemented" );
}

void HtmlViewer::EndPart()
{
   m_htmlText += "<br>";
}

void HtmlViewer::EndBody()
{
   m_htmlText += "</body></html>";

   // makes cut-&-pasting into Netscape easier
   //printf("Generated HTML output:\n%s\n", m_htmlText.c_str());

   m_window->SetPage(m_htmlText);
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void HtmlViewer::EmulateKeyPress(int keycode)
{
   wxKeyEvent event;
   event.m_keyCode = keycode;

#ifdef __WXGTK__
   m_window->OnChar(event);
#else
   m_window->HandleOnChar(event);
#endif
}

bool
HtmlViewer::LineDown()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_DOWN);

   return check.HasChanged();
}

bool
HtmlViewer::LineUp()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_UP);

   return check.HasChanged();
}

bool
HtmlViewer::PageDown()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_PAGEDOWN);

   return check.HasChanged();
}

bool
HtmlViewer::PageUp()
{
   ScrollPositionChangeChecker check(m_window);

   EmulateKeyPress(WXK_PAGEUP);

   return check.HasChanged();
}

// ----------------------------------------------------------------------------
// capabilities querying
// ----------------------------------------------------------------------------

bool HtmlViewer::CanInlineImages() const
{
   // we can show images inline
   return true;
}

bool HtmlViewer::CanProcess(const String& mimetype) const
{
   // we understand HTML directly
   return mimetype == "text/html";
}

