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
#  include "Mcommon.h"

#  include "Mdefaults.h"
#  include "gui/wxMApp.h"
#  include "guidef.h"

#  include <wx/image.h>
#endif // USE_PCH

#include "MessageViewer.h"
#include "gui/wxMenuDefs.h"
#include "ClickURL.h"
#include "MTextStyle.h"

#include <wx/fontmap.h>
#include <wx/fs_mem.h>
#include <wx/fs_inet.h>

#include <wx/html/htmlwin.h>    // for wxHtmlWindow
#include <wx/html/htmprint.h>   // for wxHtmlEasyPrinting
#include <wx/html/m_templ.h>    // for TAG_HANDLER_BEGIN

class HtmlViewerWindow;

WX_DEFINE_ARRAY(ClickableInfo *, ArrayClickInfo);

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return RRGGBB HTML colour spec
static wxString Col2Html(const wxColour& col);

// filter out special HTML characters from the text
//
// if dontWrap is true, spaces are replaced by non breaking spaces (&nbsp;),
// otherwise they're left alone
static wxString MakeHtmlSafe(const wxString& text, bool dontWrap = true);

// escape all double quotes in a string by replacing them with HTML entity
//
// this is useful for the HTML attributes values
static wxString EscapeQuotes(const wxString& text);

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

   // methods used by HtmlViewerWindow only
   MessageView *GetMessageView() const { return m_msgView; }
   bool ShouldInlineImage(const String& url) const;

private:
   // called by StartHeaders() first time it's called on a new message
   void StartMessage();

   // HTML helpers
   // ------------

   // add "attr=#colour" attribute to m_htmlText if col is valid
   void AddColourAttr(const wxChar *attr, const wxColour& col);

   // calculate HTML font size for the given font
   int CalculateFontSize(const wxFont& font);

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

#if wxUSE_PRINTING_ARCHITECTURE
   // the object which does all printing for us
   wxHtmlEasyPrinting *m_printHtml;
#endif // wxUSE_PRINTING_ARCHITECTURE

   // do we have text (false) or HTML (true) contents?
   bool m_hasHtmlContents;


   // This is used in GetVirtualFileName() to ensure that the names we return
   // from there are distinct for different objects by incrementing this
   // counter every time a new viewer object is created.
   static unsigned long s_viewerCount;

   DECLARE_MESSAGE_VIEWER()
};

unsigned long HtmlViewer::s_viewerCount = 0;

// ----------------------------------------------------------------------------
// HTML_Handler_META: wxHTML handler for the <meta> tag, see EncodingChanger
// ----------------------------------------------------------------------------

TAG_HANDLER_BEGIN(META, "META" )
    TAG_HANDLER_CONSTR(META) { }

    TAG_HANDLER_PROC(tag)
    {
        if ( tag.GetParam(_T("HTTP-EQUIV")).CmpNoCase(_T("Content-Type")) == 0 )
        {
            wxString content = tag.GetParam(_T("CONTENT")).Lower(),
                     rest;
            if ( content.StartsWith(_T("text/html;"), &rest) )
            {
                // there can be some leading white space
                rest.Trim(false /* from left */);

                wxString charset;
                if ( rest.StartsWith(_T("charset="), &charset) )
                {
                    wxFontEncoding
                        enc = wxFontMapper::Get()->CharsetToEncoding( charset);

                    if ( enc == wxFONTENCODING_SYSTEM
#if !wxUSE_UNICODE
                         || enc == m_WParser->GetInputEncoding()
#endif
                       )
                    {
                       return false;
                    }

#if !wxUSE_UNICODE
                    m_WParser->SetInputEncoding(enc);
#endif
                    m_WParser->GetContainer()->InsertCell(
                        new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
                }
            }
        }

        return false;
    }

TAG_HANDLER_END(META)


TAGS_MODULE_BEGIN(MetaTag)

    TAGS_MODULE_ADD(META)

TAGS_MODULE_END(MetaTag)


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
   AttributeChanger(String& str) : m_str(str) { }

   void DoChange(const String& start, const String& end)
   {
      m_str += start;
      m_end = end;
   }

   ~AttributeChanger() { m_str += m_end; }

private:
   String& m_str;

   // the end tag (or empty if none)
   String m_end;

   DECLARE_NO_COPY_CLASS(AttributeChanger)
};

// ----------------------------------------------------------------------------
// EncodingChanger: change the encoding to the given one during its lifetime
// ----------------------------------------------------------------------------

class EncodingChanger : private AttributeChanger
{
public:
   // change the encoding to the given one during this object lifetime
   //
   // does nothing if enc == wxFONTENCODING_SYSTEM
   EncodingChanger(wxFontEncoding enc, String& str)
      : AttributeChanger(str)
   {
      if ( enc != wxFONTENCODING_SYSTEM )
      {
         // this is a really strange way to change the encoding but it is the
         // only one which works with wxHTML
         //
         // update: now that meta tag handling was removed from wxHTML itself
         // its handler moved in this file (see TAG_HANDLER macros) and so we
         // could change it either to our own custom tag or to something
         // completely different, but for now leave it as is...
         DoChange(GetMetaString(wxFontMapper::GetEncodingName(enc)),
                  GetDefaultMeta());
      }
   }

   // another version using the charset name, does nothing if it's empty
   EncodingChanger(const String& charset, String& str)
      : AttributeChanger(str)
   {
      if ( !charset.empty() )
      {
         DoChange(GetMetaString(charset), GetDefaultMeta());
      }
   }

private:
   static String GetMetaString(const String& charset)
   {
      String s;
      s << _T("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=")
        << charset << _T("\">");
      return s;
   }

   static String GetDefaultMeta() { return GetMetaString(_T("iso-8859-1")); }

   DECLARE_NO_COPY_CLASS(EncodingChanger)
};

// ----------------------------------------------------------------------------
// FontColourChanger: change the font colour during its lifetime
// ----------------------------------------------------------------------------

class FontColourChanger : private AttributeChanger
{
public:
   FontColourChanger(const wxColour& col, String& str)
      : AttributeChanger(str)
   {
      if ( col.Ok() )
      {
         String start;
         start << _T("<font color=\"#") << Col2Html(col) << _T("\">");

         DoChange(start, _T("</font>"));
      }
   }

private:
   DECLARE_NO_COPY_CLASS(FontColourChanger)
};

// ----------------------------------------------------------------------------
// FontStyleChanger: changes bold/italic attributes during its lifetime
// ----------------------------------------------------------------------------

class FontStyleChanger
{
public:
   FontStyleChanger(const wxFont& font, String& str)
      : m_changerWeight(str),
        m_changerSlant(str)
   {
      if ( font.IsOk() )
      {
         // the order is important: should be the reverse of the order of
         // destruction of the subobjects
         if ( font.GetStyle() == wxFONTSTYLE_ITALIC )
         {
            m_changerSlant.DoChange(_T("<i>"), _T("</i>"));
         }

         if ( font.GetWeight() == wxFONTWEIGHT_BOLD )
         {
            m_changerWeight.DoChange(_T("<b>"), _T("</b>"));
         }
      }
   }

private:
   AttributeChanger m_changerWeight,
                    m_changerSlant;

   DECLARE_NO_COPY_CLASS(FontStyleChanger)
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

   // remove all clickable infos
   void ClearClickables();

   // override some base class virtuals
   virtual void OnSetTitle(const wxString& title);
   virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
   virtual void OnCellMouseHover(wxHtmlCell *cell, wxCoord x, wxCoord y);

   virtual wxHtmlOpeningStatus OnOpeningURL(wxHtmlURLType type,
                                            const wxString& url,
                                            wxString *redirect) const;


   // provide access to some wxHtmlWindow protected methods
   void Copy() { CopySelection(); }
   wxString GetSelection() { return SelectionToText(); }

private:
   // get the clickable info previousy stored by StoreClickable()
   ClickableInfo *GetClickable(const String& url) const;

   wxSortedArrayString m_urls;
   ArrayClickInfo m_clickables;

   HtmlViewer *m_viewer;

   DECLARE_NO_COPY_CLASS(HtmlViewerWindow)
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
                : wxHtmlWindow(parent, -1,
                               wxDefaultPosition,
                               parent->GetClientSize(),
                               wxHW_SCROLLBAR_AUTO | wxBORDER_SIMPLE)
{
   m_viewer = viewer;

   SetRelatedFrame(GetFrame(parent), wxEmptyString);
   SetRelatedStatusBar(0);
}

HtmlViewerWindow::~HtmlViewerWindow()
{
   ClearClickables();
}

void HtmlViewerWindow::StoreClickable(ClickableInfo *ci, const String& url)
{
   m_clickables.Insert(ci, m_urls.Add(url));
}

void HtmlViewerWindow::ClearClickables()
{
   WX_CLEAR_ARRAY(m_clickables);

   m_urls.Empty();
}

ClickableInfo *HtmlViewerWindow::GetClickable(const String& url) const
{
   int index = m_urls.Index(url);

   return index == wxNOT_FOUND ? NULL : m_clickables[(size_t)index];
}

void HtmlViewerWindow::OnSetTitle(const wxString& /* title */)
{
   // don't do anything, we don't want to show the title at all
}

void HtmlViewerWindow::OnCellMouseHover(wxHtmlCell *cell,
                                        wxCoord /* x */, wxCoord /* y */)
{
   wxHtmlLinkInfo *link = cell->GetLink();
   wxFrame *frame = GetFrame(this);
   CHECK_RET( frame, _T("no parent frame?") );

   String statText;
   if ( link )
   {
      ClickableInfo *ci = GetClickable(link->GetHref());
      if ( !ci )
      {
         // let the base class process it
         return;
      }

      statText = ci->GetLabel();
   }

   frame->SetStatusText(statText);
}

void HtmlViewerWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
   String url = link.GetHref();
   ClickableInfo *ci = GetClickable(url);

   if ( !ci )
   {
      ci = new ClickableURL(m_viewer->GetMessageView(), url);
      StoreClickable(ci, url);
   }

   // left click becomes double click as we want to open the URLs on simple
   // click
   const wxMouseEvent& event = *link.GetEvent();
   if ( event.GetEventType() == wxEVT_LEFT_UP )
      ci->OnLeftClick();
   else
      ci->OnRightClick(event.GetPosition());
}

wxHtmlOpeningStatus
HtmlViewerWindow::OnOpeningURL(wxHtmlURLType type,
                               const wxString& url,
                               wxString *redirect) const
{
   if ( type == wxHTML_URL_IMAGE )
   {
      if ( !m_viewer->ShouldInlineImage(url) )
      {
         return wxHTML_BLOCK;
      }
   }

   return wxHtmlWindow::OnOpeningURL(type, url, redirect);
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
   s_viewerCount++;

   wxFileSystem::AddHandler(new wxInternetFSHandler);

   m_window = NULL;

   m_nPart =
   m_nImage = 0;

#if wxUSE_PRINTING_ARCHITECTURE
   m_printHtml = NULL;
#endif // wxUSE_PRINTING_ARCHITECTURE

   m_hasHtmlContents = false;

   m_htmlText.reserve(4096);
}

HtmlViewer::~HtmlViewer()
{
   FreeMemoryFS();

#if wxUSE_PRINTING_ARCHITECTURE
   delete m_printHtml;
#endif // wxUSE_PRINTING_ARCHITECTURE
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
   m_window->ClearClickables();
   m_window->SetPage(wxEmptyString);

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

bool HtmlViewer::Find(const String& /* text */)
{
   return FindAgain();
}

bool HtmlViewer::FindAgain()
{
   // TODO
   wxLogError(_("Sorry, searching is not implemented in the HTML viewer yet"));

   return false;
}

void HtmlViewer::Copy()
{
   m_window->Copy();
}

void HtmlViewer::SelectAll()
{
   m_window->SelectAll();
}

String HtmlViewer::GetSelection() const
{
   return m_window->GetSelection();
}

// ----------------------------------------------------------------------------
// HtmlViewer printing
// ----------------------------------------------------------------------------

void HtmlViewer::InitPrinting()
{
#if wxUSE_PRINTING_ARCHITECTURE
   if ( !m_printHtml )
   {
      m_printHtml = new wxHtmlEasyPrinting(_("Mahogany Printing"),
                                           GetFrame(m_window));
   }

   wxMApp *app = (wxMApp *)mApplication;

   *m_printHtml->GetPrintData() = *app->GetPrintData();
   *m_printHtml->GetPageSetupData() = *app->GetPageSetupData();
#endif // wxUSE_PRINTING_ARCHITECTURE
}

bool HtmlViewer::Print()
{
#if wxUSE_PRINTING_ARCHITECTURE
   InitPrinting();

   return m_printHtml->PrintText(m_htmlText);
#else // !wxUSE_PRINTING_ARCHITECTURE
   return false;
#endif // wxUSE_PRINTING_ARCHITECTURE/!wxUSE_PRINTING_ARCHITECTURE
}

void HtmlViewer::PrintPreview()
{
#if wxUSE_PRINTING_ARCHITECTURE
   InitPrinting();

   (void)m_printHtml->PreviewText(m_htmlText);
#endif // wxUSE_PRINTING_ARCHITECTURE
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
   return wxString::Format(_T("%02x%02x%02x"),
                           (int)col.Red(), (int)col.Green(), (int)col.Blue());
}

static wxString MakeHtmlSafe(const wxString& text, bool dontWrap)
{
   wxString textSafe;
   textSafe.reserve(text.length());

   for ( const wxChar *p = text.c_str(); *p; p++ )
   {
      switch ( *p )
      {
         case '<':
            textSafe += _T("&lt;");
            break;

         case '>':
            textSafe += _T("&gt;");
            break;

         case '&':
            textSafe += _T("&amp;");
            break;

         case '"':
            textSafe += _T("&quot;");
            break;

         case '\t':
            // we hardcode the tab width to 8 spaces
            textSafe += _T("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
            break;

         case '\r':
            // ignore, we process '\n' below
            break;

         case '\n':
            textSafe += _T("<br>");
            break;

         case ' ':
            if ( dontWrap )
            {
               textSafe += _T("&nbsp;");
               break;
            }
            //else: fall through

         default:
            textSafe += *p;
      }
   }

   return textSafe;
}

static wxString EscapeQuotes(const wxString& text)
{
   wxString textSafe;
   textSafe.reserve(text.length());

   for ( const wxChar *p = text.c_str(); *p; p++ )
   {
      if ( *p == '"' )
         textSafe += _T("&quot;");
      else
         textSafe += *p;
   }

   return textSafe;
}

#ifdef USE_STRIP_TAGS

static wxString StripHtmlTags(const wxString& html)
{
   wxString text;
   text.reserve(html.length());

   bool inTag = false,
        inQuote = false;
   for ( const wxChar *p = html.c_str(); *p; p++ )
   {
      if ( inTag )
      {
         if ( inQuote )
         {
            if ( *p == _T('"') )
               inQuote = false;
         }
         else // not inside quoted text
         {
            if ( *p == _T('>') )
               inTag = false;
            else if ( *p == _T('"') )
               inQuote = true;
         }
      }
      else // not inside a tag, copy
      {
         if ( *p == _T('<') )
            inTag = true;
         else
            text += *p;
      }
   }


   // finally, replace all entities with their values
   wxHtmlEntitiesParser entities;
   return entities.Parse(text);
}

#endif // 0

void HtmlViewer::AddColourAttr(const wxChar *attr, const wxColour& col)
{
   if ( col.Ok() )
   {
      m_htmlText += wxString::Format(_T(" %s=\"#%s\""),
                                     attr, Col2Html(col));
   }
}

int HtmlViewer::CalculateFontSize(const wxFont& font)
{
   if ( !font.Ok() )
   {
      // use default size
      return 0;
   }

   // map the point size into the HTML font size so that if the standard font
   // size is 12pt, 6pt is very small and 24pt is very big
   //
   // this is not very rigorous, of course...
   int diff = font.GetPointSize() - wxNORMAL_FONT->GetPointSize();
   if ( diff > 0 )
      diff /= 4;
   else
      diff /= 2;
   return diff;
}

wxString HtmlViewer::GetVirtualFileName(size_t n) const
{
   return wxString::Format(_T("Mhtml%08lx%zd.png"), s_viewerCount, n);
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

void HtmlViewer::StartMessage()
{
   // set the default attributes
   const ProfileValues& profileValues = GetOptions();

   m_htmlEnd.clear();
   m_htmlText = _T("<html><body");

   AddColourAttr(_T("text"), profileValues.FgCol);
   AddColourAttr(_T("bgcolor"), profileValues.BgCol);
   AddColourAttr(_T("link"), profileValues.UrlCol);

   // close <body> tag
   m_htmlText += _T(">");

   wxFont font(profileValues.GetFont());
   int diff = CalculateFontSize(font);
   if ( diff )
   {
      m_htmlText << _T("<font size=") << wxString::Format(_T("%+d"), diff) << _T(">");
      m_htmlEnd.Prepend(_T("</font>"));
   }

   // map the font family into HTML font face name
   //
   // TODO: use <font face="...">
   if ( font.Ok() && font.IsFixedWidth() )
   {
      m_htmlText << _T("<tt>");
      m_htmlEnd.Prepend(_T("</tt>"));
   }
}

// ----------------------------------------------------------------------------
// header showing
// ----------------------------------------------------------------------------

void HtmlViewer::StartHeaders()
{
   // this can be called multiple times for display of the embedded message
   // headers, only start the message once
   if ( m_htmlText.empty() )
      StartMessage();

   // the next header will be the first one
   m_firstheader = true;
}

void HtmlViewer::ShowRawHeaders(const String& header)
{
   int diff = CalculateFontSize(GetOptions().GetFont());
   m_htmlText << _T("<pre>") << _T("<font size=") << wxString::Format(_T("%+d"), diff) << _T(">")
              << MakeHtmlSafe(header) << _T("</font>") << _T("</pre>");
}

void HtmlViewer::ShowHeaderName(const String& name)
{
   if ( m_firstheader )
   {
      // start the table containing the headers
      m_htmlText += _T("<table cellspacing=1 cellpadding=1 border=0>");

      m_firstheader = false;
   }

   // create the first column: header names (width=1 means minimal width)
   m_htmlText += _T("<tr>")
                 _T("<td align=\"right\" valign=\"top\" width=\"1\">");

   FontColourChanger colChanger(GetOptions().HeaderNameCol, m_htmlText);

   // second column will be for the header values
   m_htmlText << _T("<tt>") << name << _T(":&nbsp;</tt></td><td>");
}

void HtmlViewer::ShowHeaderValue(const String& value,
                                 wxFontEncoding encoding)
{
   FontColourChanger colChanger(GetOptions().HeaderValueCol, m_htmlText);

   EncodingChanger encChanger(encoding, m_htmlText);

   m_htmlText += MakeHtmlSafe(value);
}

void HtmlViewer::ShowHeaderURL(const String& text,
                               const String& url)
{
   InsertURL(text, url);
}

void HtmlViewer::EndHeader()
{
   // terminate the header row
   m_htmlText += _T("</td></tr>");
}

void HtmlViewer::ShowXFace(const wxBitmap& bitmap)
{
   // make a new table inside the header table for the headers
   m_htmlText += _T("<table cellspacing=1 cellpadding=1 border=0><td>");

   m_bmpXFace = bitmap;
}

void HtmlViewer::EndHeaders()
{
   if ( !m_firstheader )
   {
      // close the headers table
      m_htmlText += _T("</table>");

      m_firstheader = true;
   }
   //else: we had no headers at all

   if ( m_bmpXFace.Ok() )
   {
      wxString filename = CreateImageInMemoryFS(m_bmpXFace.ConvertToImage());
      m_htmlText << _T("</td><td width=")
                 << wxString::Format(_T("%d"), m_bmpXFace.GetWidth()) << _T(">")
                    _T("<img src=\"memory:") << EscapeQuotes(filename) << _T("\">")
                    _T("</td>")
                    _T("</table>");
   }

   // skip a line before the body
   m_htmlText += _T("<br>");
}

// ----------------------------------------------------------------------------
// body showing
// ----------------------------------------------------------------------------

void HtmlViewer::StartBody()
{
   m_htmlText += _T("<br>");
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
      m_htmlText += _T("<hr>");
   }
#endif // 0

   m_htmlText += _T("<p>");
}

void HtmlViewer::InsertAttachment(const wxBitmap& icon, ClickableInfo *ci)
{
   wxString url;
   url << _T("memory:") << CreateImageInMemoryFS(icon.ConvertToImage());

   m_htmlText << _T("<a href=\"") << url << _T("\">")
                 _T("<img alt=\"") << EscapeQuotes(ci->GetLabel())
              << _T("\" src=\"") << url << _T("\"></a>");

   m_window->StoreClickable(ci, url);
}

void HtmlViewer::InsertClickable(const wxBitmap& icon,
                                 ClickableInfo *ci,
                                 const wxColour& /* col */)
{
   InsertAttachment(icon, ci);
}

void HtmlViewer::InsertImage(const wxImage& image, ClickableInfo *ci)
{
   wxString url;
   url << _T("memory:") << CreateImageInMemoryFS(wxImage(image));

   m_htmlText << _T("<a href=\"") << url << _T("\">")
                 _T("<p><img alt=\"") << EscapeQuotes(ci->GetLabel())
              << _T("\" src=\"") << url << _T("\"></a>");

   m_window->StoreClickable(ci, url);
}

void HtmlViewer::InsertRawContents(const String& data)
{
   // we're already inside body so we can't insert the entire HTML document
   // here, so extract just its body and also take its charset
   EncodingChanger
      encChanger(wxHtmlParser::ExtractCharsetInformation(data), m_htmlText);

   // tag handler used to extract the <body> tag contents
   //
   // note that we have to use string indices with wx 2.8 but string iterators
   // with 2.9 to avoid deprecation warnings (as index versions are much less
   // efficient with wx 2.9)
   class BodyTagHandler : public wxHtmlTagHandler
   {
   public:
      BodyTagHandler(const String& text)
      {
         // by default, take all
         m_begin = text.begin();
         m_end = text.end();
      }

      virtual wxString GetSupportedTags() { return "BODY"; }
      virtual bool HandleTag(const wxHtmlTag& tag)
      {
         // "end 1" is the position just before the closing tag while "end 2"
         // is after the tag, as we don't need the tag itself, use the former
         m_begin = tag.GetBeginIter();
         m_end = tag.GetEndIter1();

         return false;
      }

      typedef wxString::const_iterator IterType;

      IterType GetBegin() const { return m_begin; }
      IterType GetEnd() const { return m_end; }

   private:
      IterType m_begin,
               m_end;
   };

   class BodyParser : public wxHtmlParser
   {
   public:
      BodyParser() { }

      // provide stubs for base class pure virtual methods which we don't use
      virtual wxObject* GetProduct() { return NULL; }
      virtual void AddText(const wxString& WXUNUSED(txt)) { }
   };

   BodyTagHandler *handler = new BodyTagHandler(data);
   BodyParser parser;
   parser.AddTagHandler(handler);
   parser.Parse(data);

   m_htmlText += wxString(handler->GetBegin(), handler->GetEnd());

   // set the flag for EndBody
   m_hasHtmlContents = true;
}

void HtmlViewer::InsertText(const String& text, const MTextStyle& style)
{
   EncodingChanger encChanger(style.HasFont() ? style.GetFont().GetEncoding()
                                              : wxFONTENCODING_SYSTEM,
                              m_htmlText);

   FontColourChanger colChanger(style.GetTextColour(), m_htmlText);

   FontStyleChanger styleChanger(style.GetFont(), m_htmlText);

   m_htmlText += MakeHtmlSafe(text, false /* allow wrapping */);
}

void HtmlViewer::InsertURL(const String& text, const String& url)
{
   // URLs may contain special characters which must be replaced by HTML
   // entities (i.e. '&' -> "&amp;") so use MakeHtmlSafe() to filter them
   m_htmlText << _T("<a href=\"") << MakeHtmlSafe(url) << _T("\">")
              << MakeHtmlSafe(text)
              << _T("</a>");
}

void HtmlViewer::EndPart()
{
   m_htmlText += _T("<br>");
}

void HtmlViewer::EndBody()
{
   m_htmlText += m_htmlEnd;

   m_htmlText += _T("</body></html>");

   // makes cut-&-pasting into Netscape easier
   //wxLogTrace(_T("html"), _T("Generated HTML output:\n%s\n"), m_htmlText);

   m_window->SetPage(m_htmlText);

   // if we display HTML text, we need to let the msg view know about the text
   // we have so that it could be quoted later -- normally this is done by
   // TransparentFilter which intercepts all InsertText() calls, but it can't
   // do this for InsertRawContents()
   if ( m_hasHtmlContents )
   {
      String text(m_window->ToText());
      size_t posEndHeaders = text.find("\n\n");
      if ( posEndHeaders != String::npos )
         text.erase(0, posEndHeaders + 2);
      m_msgView->OnBodyText(text);

      m_hasHtmlContents = false;
   }
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void HtmlViewer::EmulateKeyPress(int keycode)
{
   wxKeyEvent event;
   event.m_keyCode = keycode;
   m_window->HandleOnChar(event);
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

// this function is used by HtmlViewerWindow to decide if we should inline this
// concrete image or not
bool HtmlViewer::ShouldInlineImage(const String& url) const
{
   const ProfileValues& profileValues = GetOptions();

   return profileValues.inlineGFX &&
            (url.StartsWith(_T("memory:")) ||
               url.StartsWith(_T("cid:")) ||
                  profileValues.showExtImages);
}

// this is called by MessageView to ask us if, in principle, we can inline
// images
bool HtmlViewer::CanInlineImages() const
{
   // yes, we can
   return true;
}

bool HtmlViewer::CanProcess(const String& mimetype) const
{
   // we understand HTML directly
   return mimetype == _T("TEXT/HTML");
}

