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

   // methods used by HtmlViewerWindow only
   MessageView *GetMessageView() const { return m_msgView; }
   bool ShouldInlineImage(const String& url) const;

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
// HTML_Handler_META: wxHTML handler for the <meta> tag, see EncodingChanger
// ----------------------------------------------------------------------------

TAG_HANDLER_BEGIN(META, "META" )
    TAG_HANDLER_CONSTR(META) { }

    TAG_HANDLER_PROC(tag)
    {
        if ( tag.GetParam(_T("HTTP-EQUIV")).CmpNoCase(_T("Content-Type")) == 0 )
        {
            // strlen("text/html; charset=")
            static const int CHARSET_STRING_LEN = 19;

            wxString content = tag.GetParam(_T("CONTENT"));
            if ( content.Left(CHARSET_STRING_LEN) == _T("text/html; charset=") )
            {
                wxFontEncoding enc =
                    wxFontMapper::Get()->CharsetToEncoding(
                              content.Mid(CHARSET_STRING_LEN));

                if ( enc == wxFONTENCODING_SYSTEM
#if wxUSE_UNICODE
                   )
#else
                     || enc == m_WParser->GetInputEncoding() )
#endif
                   return FALSE;

#if !wxUSE_UNICODE
                m_WParser->SetInputEncoding(enc);
#endif
                m_WParser->GetContainer()->InsertCell(
                    new wxHtmlFontCell(m_WParser->CreateCurrentFont()));
            }
        }

        return FALSE;
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
                  GetMetaString(_T("iso-8859-1")));
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
      // the order is important:should be the reverse of the order of
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
   virtual void OnCellClicked(wxHtmlCell *cell, wxCoord x, wxCoord y,
                              const wxMouseEvent& event);

   virtual wxHtmlOpeningStatus OnOpeningURL(wxHtmlURLType type,
                                            const wxString& url,
                                            wxString *redirect) const;


   // provide access to some wxHtmlWindow protected methods
   void Copy()
   {
#if wxCHECK_VERSION(2, 5, 1)
      CopySelection();
#endif
   }
   wxString GetSelection()
   {
#if wxCHECK_VERSION(2, 5, 1)
      return SelectionToText();
#else
      return wxEmptyString;
#endif
   }

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
                               wxDefaultPosition, wxDefaultSize,
                               wxHW_SCROLLBAR_AUTO | wxBORDER_SIMPLE)
{
   m_viewer = viewer;

   SetRelatedFrame(GetFrame(parent), _T(""));
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

void HtmlViewerWindow::OnLinkClicked(const wxHtmlLinkInfo& /* link */)
{
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
         ci = new ClickableURL(m_viewer->GetMessageView(), url);
         StoreClickable(ci, url);
      }

      // left click becomes double click as we want to open the URLs on simple
      // click
      m_viewer->GetMessageView()->DoMouseCommand
                                  (
                                    event.LeftDown() ? WXMENU_LAYOUT_DBLCLICK
                                                     : WXMENU_LAYOUT_RCLICK,
                                    ci,
                                    event.GetPosition()
                                  );
   }

   // don't call the base class version, we don't want it to automatically
   // load any URLs which may be in the text!
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
                         _T("(c) 2001 Vadim Zeitlin <vadim@wxwindows.org>"));

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
   m_window->ClearClickables();
   m_window->SetPage(_T(""));

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
   // you need http://www.volny.cz/v.slavik/wx/wx-htmlwin-selectall.patch
   // to enable this
#ifdef wxHAVE_HTML_SELECTALL
   m_window->SelectAll();
#endif // wxHAVE_HTML_SELECTALL
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
   return wxString::Format(_T("%02x%02x%02x"),
                           (int)col.Red(), (int)col.Green(), (int)col.Blue());
}

static wxString MakeHtmlSafe(const wxString& text)
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
            textSafe += _T("&nbsp;");
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
      m_htmlText += wxString::Format(_T(" %s=\"#%s\""),
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
   return wxString::Format(_T("Mhtml%08lx%lu.png"),
                           (unsigned long)this, (unsigned long)n);
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
   m_htmlText = _T("<html><body");

   AddColourAttr(_T("text"), profileValues.FgCol);
   AddColourAttr(_T("bgcolor"), profileValues.BgCol);
   AddColourAttr(_T("link"), profileValues.UrlCol);

   // close <body> tag
   m_htmlText += _T(">");

   wxFont font = profileValues.GetFont();

   int diff = CalculateFontSize(font.GetPointSize() - DEFAULT_FONT_SIZE);
   if ( diff )
   {
      m_htmlText << _T("<font size=") << wxString::Format(_T("%+d"), diff) << _T(">");
      m_htmlEnd.Prepend(_T("</font>"));
   }

   // map the font family into HTML font face name
   //
   // TODO: use <font face="...">
   if ( font.IsFixedWidth() )
   {
      m_htmlText << _T("<tt>");
      m_htmlEnd.Prepend(_T("</tt>"));
   }

   // the next header is going to be the first one
   m_firstheader = true;
}

void HtmlViewer::ShowRawHeaders(const String& header)
{
   const ProfileValues& profileValues = GetOptions();
   wxFont font = profileValues.GetFont();

   int diff = CalculateFontSize(font.GetPointSize() - DEFAULT_FONT_SIZE);
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
   m_htmlText += _T("<tr>"
                 "<td align=\"right\" width=\"1\">");

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
   }
   //else: we had no headers at all

   if ( m_bmpXFace.Ok() )
   {
      wxString filename = CreateImageInMemoryFS(m_bmpXFace.ConvertToImage());
      m_htmlText << _T("</td><td width=")
                 << wxString::Format(_T("%d"), m_bmpXFace.GetWidth()) << _T(">")
                    _T("<img src=\"memory:") << filename << _T("\">"
                    "</td>"
                    "</table>");
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

   m_htmlText << _T("<a href=\"") << url << _T("\">"
                 "<img alt=\"") << ci->GetLabel() << _T("\" src=\"") << url
              << _T("\"></a>");

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

   m_htmlText << _T("<a href=\"") << url << _T("\">"
                 "<p><img alt=\"") << ci->GetLabel() << _T("\" "
                 "src=\"") << url << _T("\"></a>");

   m_window->StoreClickable(ci, url);
}

void HtmlViewer::InsertRawContents(const String& data)
{
   // collect all meta tags in the HTML document in this string
   String metaTags;

   // we're already inside body so we can't insert the entire HTML document
   // here, instead we're going to try to insert our existing contents in the
   // beginning of the given document
   const wxChar *pHtml = data.c_str();
   const wxChar *pBody = pHtml;
   while ( pBody )
   {
      pBody = wxStrchr(pBody, _T('<'));
      if ( pBody )
      {
         // we also check for "<meta charset>" tag or, as it's simpler, for any
         // "<meta>" tags at all, as otherwise we could have a wrong charset
         if ( (pBody[1] == _T('m') || pBody[1] == _T('M')) &&
              (pBody[2] == _T('e') || pBody[2] == _T('E')) &&
              (pBody[3] == _T('t') || pBody[3] == _T('T')) &&
              (pBody[4] == _T('a') || pBody[4] == _T('A')) )
         {
            const wxChar *pEnd = wxStrchr(pBody + 4, _T('>'));
            if ( pEnd )
            {
               metaTags += String(pBody, pEnd + 1);
            }
         }

         if ( (pBody[1] == _T('b') || pBody[1] == _T('B')) &&
              (pBody[2] == _T('o') || pBody[2] == _T('O')) &&
              (pBody[3] == _T('d') || pBody[3] == _T('D')) &&
              (pBody[4] == _T('y') || pBody[4] == _T('Y')) )
         {
            pBody = wxStrchr(pBody + 4, _T('>'));
            if ( pBody )
            {
               // skip '>'
               pBody++;
            }

            break;
         }

         pBody++;
      }
   }

   if ( pBody )
   {
      m_htmlText = String(pHtml, pBody) + m_htmlText + metaTags + pBody;
   }
   else // HTML fragment only?
   {
      m_htmlText += pHtml;
   }
}

void HtmlViewer::InsertText(const String& text, const MTextStyle& style)
{
   EncodingChanger encChanger(style.HasFont() ? style.GetFont().GetEncoding()
                                              : wxFONTENCODING_SYSTEM,
                              m_htmlText);

   FontColourChanger colChanger(style.GetTextColour(), m_htmlText);

   FontStyleChanger styleChanger(style.GetFont(), m_htmlText);

   m_htmlText += MakeHtmlSafe(text);
}

void HtmlViewer::InsertURL(const String& text, const String& url)
{
   // URLs may contain special characters which must be replaced by HTML
   // entities (i.e. '&' -> "&amp;") so use MakeHtmlSafe() to filter them
   m_htmlText << _T("<a href=\"") << MakeHtmlSafe(url) << _T("\">")
              << MakeHtmlSafe(text)
              << _T("</a>");
}

void HtmlViewer::EndText()
{
   // nothing to do here
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
   //wxLogTrace(_T("html"), _T("Generated HTML output:\n%s\n"), m_htmlText.c_str());

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

// this function is used by HtmlViewerWindow to decide if we should inline this
// concrete image or not
bool HtmlViewer::ShouldInlineImage(const String& url) const
{
   const ProfileValues& profileValues = GetOptions();

   return profileValues.inlineGFX &&
            (url.StartsWith(_T("memory:")) || profileValues.showExtImages);
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

