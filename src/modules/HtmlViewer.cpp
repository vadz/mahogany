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

#include <wx/dynarray.h>

#include <wx/fontmap.h>
#include <wx/fs_mem.h>
#include <wx/wxhtml.h>
#include <wx/html/htmprint.h>

class HtmlViewerWindow;

WX_DEFINE_ARRAY(ClickableInfo *, ArrayClickInfo);

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return RRGGBB HTML colour spec
static wxString Col2Html(const wxColour& col);

// filter out special HTML characters from the text
static wxString MakeHtmlSafe(const wxString& text);

// ----------------------------------------------------------------------------
// HtmlViewer: a wxHTML-based MessageViewer implementation
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
   virtual String GetSelection() const;
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
   virtual void InsertImage(const wxImage& image, ClickableInfo *ci);
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

   // calculate font size
   int CalculateFontSize(int diff);

   // get the name of the virtual file for the n-th image we use
   wxString GetVirtualFileName(size_t n) const;

   // create a new image in memory, returns its name
   wxString CreateImageInMemoryFS(const wxImage& image);

   // free all images we created in memory
   void FreeMemoryFS();

   // printing helper: creates m_printHtml object if not done yet
   void InitPrinting();

   // emulate a key press: this is the only way I found to scroll
   // wxScrolledWindow
   void EmulateKeyPress(int keycode);

   // the viewer window
   HtmlViewerWindow *m_window;

   // the HTML string: we accumulate HTML in it and render it in EndBody()
   wxString m_htmlText;

   // the string to put just before </body>: it contains the ends of some tags
   wxString m_htmlEnd;

   // count of images used to generate unique names for them
   size_t m_nImage;

   // current MIME part number
   size_t m_nPart;

   // the XFace if we have any
   wxBitmap m_bmpXFace;

   // did we have some headers already?
   bool m_firstheader;

   // do we have a non default font?
   bool m_hasGlobalFont;

   // the object which does all printing for us
   wxHtmlEasyPrinting *m_printHtml;

   DECLARE_MESSAGE_VIEWER()
};

// ----------------------------------------------------------------------------
// a collection of small classes which add something to the given string in
// its ctor and something else in its dtor, this is useful for HTML tags as
// you can't forget to close them like this
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// AttributeChanger: base class for all changers
// ----------------------------------------------------------------------------

class AttributeChanger
{
public:
   AttributeChanger(String& str) : m_str(str) { m_doChange = false; }

   void DoChange(const String& start, const String& end)
   {
      m_doChange = true;

      m_str += start;
      m_end = end;
   }

   ~AttributeChanger() { if ( m_doChange ) m_str += m_end; }

private:
   String& m_str;

   String m_end;

   bool m_doChange;
};

// ----------------------------------------------------------------------------
// EncodingChanger
// ----------------------------------------------------------------------------

class EncodingChanger : public AttributeChanger
{
public:
   EncodingChanger(wxFontEncoding enc, String& str)
      : AttributeChanger(str)
   {
      if ( enc != wxFONTENCODING_SYSTEM )
      {
         // FIXME: this is a really strange way to change the encoding but
         //        it is the only one which works with wxHTML - so be it
         //        [for now]
         DoChange(GetMetaString(wxFontMapper::GetEncodingName(enc)),
                  GetMetaString("iso-8859-1"));
      }
   }

private:
   static String GetMetaString(const String& charset)
   {
      String s;
      s << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset="
        << charset << "\">";
      return s;
   }
};

// ----------------------------------------------------------------------------
// FontColourChanger
// ----------------------------------------------------------------------------

class FontColourChanger : public AttributeChanger
{
public:
   FontColourChanger(const wxColour& col, String& str)
      : AttributeChanger(str)
   {
      if ( col.Ok() )
      {
         String start;
         start << "<font color=\"#" << Col2Html(col) << "\">";

         DoChange(start, "</font>");
      }
   }
};

// ----------------------------------------------------------------------------
// HtmlViewerWindow: wxLayoutWindow used by HtmlViewer
// ----------------------------------------------------------------------------

class HtmlViewerWindow : public wxHtmlWindow
{
public:
   HtmlViewerWindow(HtmlViewer *viewer, wxWindow *parent);
   virtual ~HtmlViewerWindow();

   // store the clickable info for this URL (we take ownership of it)
   void StoreClickable(ClickableInfo *ci, const String& url);

   // override some base class virtuals
   virtual void OnSetTitle(const wxString& title);
   virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
   virtual void OnCellMouseHover(wxHtmlCell *cell, wxCoord x, wxCoord y);
   virtual void OnCellClicked(wxHtmlCell *cell, wxCoord x, wxCoord y,
                              const wxMouseEvent& event);

private:
   // get the clickable info previousy stored by StoreClickable()
   ClickableInfo *GetClickable(const String& url) const;

   wxSortedArrayString m_urls;
   ArrayClickInfo m_clickables;

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

// ============================================================================
// HtmlViewerWindow implementation
// ============================================================================

HtmlViewerWindow::HtmlViewerWindow(HtmlViewer *viewer, wxWindow *parent)
                : wxHtmlWindow(parent)
{
   m_viewer = viewer;

   SetRelatedFrame(GetFrame(parent), "");
   SetRelatedStatusBar(0);
}

HtmlViewerWindow::~HtmlViewerWindow()
{
   WX_CLEAR_ARRAY(m_clickables);
}

void HtmlViewerWindow::StoreClickable(ClickableInfo *ci, const String& url)
{
   m_clickables.Insert(ci, m_urls.Add(url));
}

ClickableInfo *HtmlViewerWindow::GetClickable(const String& url) const
{
   int index = m_urls.Index(url);

   return index == wxNOT_FOUND ? NULL : m_clickables[(size_t)index];
}

void HtmlViewerWindow::OnSetTitle(const wxString& title)
{
   // don't do anything, we don't want to show the title at all
}

void HtmlViewerWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
}

void HtmlViewerWindow::OnCellMouseHover(wxHtmlCell *cell, wxCoord x, wxCoord y)
{
   wxHtmlLinkInfo *link = cell->GetLink();
   wxFrame *frame = GetFrame(this);
   CHECK_RET( frame, "no parent frame?" );

   String statText;
   if ( link )
   {
      ClickableInfo *ci = GetClickable(link->GetHref());
      if ( !ci || ci->GetType() == ClickableInfo::CI_URL )
      {
         // let the base class process it
         return;
      }

      statText = ci->GetLabel();
   }

   frame->SetStatusText(statText);
}

void HtmlViewerWindow::OnCellClicked(wxHtmlCell *cell, wxCoord x, wxCoord y,
                                     const wxMouseEvent& event)
{
   wxHtmlLinkInfo *link = cell->GetLink(x, y);
   if ( link )
   {
      String url = link->GetHref();
      ClickableInfo *ci = GetClickable(url);

      if ( !ci )
      {
         ci = new ClickableInfo(url);
         StoreClickable(ci, url);
      }

      // left click becomes double click as we want to open the URLs on simple
      // click
      m_viewer->DoMouseCommand(event.LeftDown()
                                 ? WXMENU_LAYOUT_DBLCLICK
                                 : WXMENU_LAYOUT_RCLICK,
                               ci,
                               event.GetPosition());
   }

   // don't call the base class version, we don't want it to automatically
   // load any URLs which may be in the text!
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

   m_printHtml = NULL;

   m_htmlText.reserve(4096);
}

HtmlViewer::~HtmlViewer()
{
   FreeMemoryFS();

   delete m_printHtml;
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

String HtmlViewer::GetSelection() const
{
   // TODO
   return "";
}

// ----------------------------------------------------------------------------
// HtmlViewer printing
// ----------------------------------------------------------------------------

void HtmlViewer::InitPrinting()
{
   if ( !m_printHtml )
   {
      m_printHtml = new wxHtmlEasyPrinting(_("Mahogany Printing"),
                                           GetFrame(m_window));
   }

   wxMApp *app = (wxMApp *)mApplication;

   *m_printHtml->GetPrintData() = *app->GetPrintData();
   *m_printHtml->GetPageSetupData() = *app->GetPageSetupData();
}

bool HtmlViewer::Print()
{
   InitPrinting();

   return m_printHtml->PrintText(m_htmlText);
}

void HtmlViewer::PrintPreview()
{
   InitPrinting();

   (void)m_printHtml->PreviewText(m_htmlText);
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

         case ' ':
            textSafe += "&nbsp;";
            break;

         case '\t':
            // we hardcode the tab width to 8
            textSafe += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
            break;

         case '\r':
            // ignore, we process '\n' below
            break;

         case '\n':
            textSafe += "<br>";
            break;

         default:
            textSafe += *p;
      }
   }

   return textSafe;
}

void HtmlViewer::AddColourAttr(const wxChar *attr, const wxColour& col)
{
   if ( col.Ok() )
   {
      m_htmlText += wxString::Format(" %s=\"#%s\"",
                                     attr, Col2Html(col).c_str());
   }
}

int HtmlViewer::CalculateFontSize(int diff)
{
   // map the point size into the HTML font size so that if the standard font
   // size is 12pt, 6pt is very small and 24pt is very big
   //
   // this is not very rigorous, of course...
   if ( diff > 0 )
      diff /= 4;
   else
      diff /= 2;
   return diff;
}

wxString HtmlViewer::GetVirtualFileName(size_t n) const
{
   // the image file names must be globally unique, so concatenate the address
   // of this object together with counter to obtain a really unique name
   return wxString::Format("Mhtml%08x%d.png", this, n);
}

wxString HtmlViewer::CreateImageInMemoryFS(const wxImage& image)
{
   wxString filename = GetVirtualFileName(m_nImage++);
   wxMemoryFSHandler::AddFile(filename, image, wxBITMAP_TYPE_PNG);
   return filename;
}

void HtmlViewer::FreeMemoryFS()
{
   for ( size_t n = 0; n < m_nImage; n++ )
   {
      wxMemoryFSHandler::RemoveFile(GetVirtualFileName(n));
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

   m_htmlEnd.clear();
   m_htmlText = "<html><body";

   AddColourAttr("text", profileValues.FgCol);
   AddColourAttr("bgcolor", profileValues.BgCol);
   AddColourAttr("link", profileValues.UrlCol);

   // close <body> tag
   m_htmlText += ">";

   int diff = CalculateFontSize(profileValues.fontSize - DEFAULT_FONT_SIZE);
   if ( diff )
   {
      m_htmlText << "<font size=" << wxString::Format("%+d", diff) << ">";
      m_htmlEnd.Prepend("</font>");
   }

   // map the font family into HTML font face name
   //
   // FIXME: use <font face="...">
   if ( profileValues.fontFamily == wxTELETYPE )
   {
      m_htmlText << "<tt>";
      m_htmlEnd.Prepend("</tt>");
   }

   // the next header is going to be the first one
   m_firstheader = true;
}

void HtmlViewer::ShowRawHeaders(const String& header)
{
   const ProfileValues& profileValues = GetOptions();
   int diff = CalculateFontSize(profileValues.fontSize - DEFAULT_FONT_SIZE);
   m_htmlText << "<pre>" << "<font size=" << wxString::Format("%+d", diff) << ">"
              << MakeHtmlSafe(header) << "</font>" << "</pre>";
}

void HtmlViewer::ShowHeader(const String& headerName,
                            const String& headerValue,
                            wxFontEncoding encHeader)
{
   // don't show empty headers at all
   if ( headerValue.empty() )
      return;

   if ( m_firstheader )
   {
      // start the table containing the headers
      m_htmlText += "<table cellspacing=1 cellpadding=1 border=0>";

      m_firstheader = false;
   }

   const ProfileValues& profileValues = GetOptions();

   // first column: header names (width=1 means minimal width)
   m_htmlText += "<tr>"
                 "<td align=\"right\" width=\"1\">";

   {
      FontColourChanger colChanger(profileValues.HeaderNameCol, m_htmlText);

      m_htmlText << "<tt>" << headerName << ":&nbsp;</tt>";
   }

   // second column: header values
   m_htmlText += "</td><td>";

   if ( encHeader == wxFONTENCODING_UTF8 )
   {
      // convert from UTF-8 to environment's default encoding
      // FIXME it won't be needed when full Unicode support is available
      m_htmlText = wxString(m_htmlText.wc_str(wxConvUTF8), wxConvLocal);
      encHeader = wxLocale::GetSystemEncoding();
   }

   {
      FontColourChanger colChanger(profileValues.HeaderValueCol, m_htmlText);

      EncodingChanger encChanger(encHeader, m_htmlText);

      m_htmlText += MakeHtmlSafe(headerValue);
   }

   m_htmlText += "</td></tr>";
}

void HtmlViewer::ShowXFace(const wxBitmap& bitmap)
{
   // make a new table inside the header table for the headers
   m_htmlText += "<td><table cellspacing=1 cellpadding=1 border=0>";

   m_bmpXFace = bitmap;
}

void HtmlViewer::EndHeaders()
{
   if ( !m_firstheader )
   {
      // close the headers table
      m_htmlText += "</table>";
   }
   //else: we had no headers at all

   if ( m_bmpXFace.Ok() )
   {
      wxString filename = CreateImageInMemoryFS(wxImage(m_bmpXFace));
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
   wxString url;
   url << "memory:" << CreateImageInMemoryFS(wxImage(icon));

   m_htmlText << "<a href=\"" << url << "\">"
                 "<img alt=\"" << ci->GetLabel() << "\" src=\"" << url
              << "\"></a>";

   m_window->StoreClickable(ci, url);
}

void HtmlViewer::InsertImage(const wxImage& image, ClickableInfo *ci)
{
   wxString url;
   url << "memory:" << CreateImageInMemoryFS(wxImage(image));

   m_htmlText << "<a href=\"" << url << "\">"
                 "<p><img alt=\"" << ci->GetLabel() << "\" "
                 "src=\"" << url << "\"></a>";

   m_window->StoreClickable(ci, url);
}

void HtmlViewer::InsertRawContents(const String& data)
{
   m_htmlText += data;
}

void HtmlViewer::InsertText(const String& text, const TextStyle& style)
{
   EncodingChanger encChanger(style.HasFont() ? style.GetFont().GetEncoding()
                                              : wxFONTENCODING_SYSTEM,
                              m_htmlText);

   FontColourChanger colChanger(style.GetTextColour(), m_htmlText);

   m_htmlText += MakeHtmlSafe(text);
}

void HtmlViewer::InsertURL(const String& url)
{
   // URLs may contain special characters which must be replaced by HTML
   // entities (i.e. '&' -> "&amp;")
   String htmlizedUrl = MakeHtmlSafe(url);

   m_htmlText << "<a href=\"" << htmlizedUrl << "\">" << htmlizedUrl << "</a>";
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
   m_htmlText += m_htmlEnd;

   m_htmlText += "</body></html>";

   // makes cut-&-pasting into Netscape easier
   //wxLogTrace("html", "Generated HTML output:\n%s\n", m_htmlText.c_str());

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
   return mimetype == "TEXT/HTML";
}

