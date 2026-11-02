///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   modules/WebViewViewer.cpp: implements MessageViewer using
//              wxWebView
// Purpose:     this viewer translates the non-HTML message parts to HTML
//              and shows everything using the native web engine
// Author:      Vadim Zeitlin
// Created:     2026-10-30
// Copyright:   (c) 2026 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "Mpch.h"

#ifdef USE_WEBVIEW

#ifndef USE_PCH
#  include "Mcommon.h"

#  include <wx/bitmap.h>
#  include <wx/image.h>
#endif // USE_PCH

#include "MessageViewer.h"
#include "ClickURL.h"
#include "MTextStyle.h"

#include <wx/base64.h>
#include <wx/mstream.h>

#include <wx/webview.h>

#include <memory>
#include <vector>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the URL scheme used for the links corresponding to our clickable objects
#define CLICKABLE_SCHEME "x-mahogany-clickable:"

// the HTML message parts are shown in iframes which need to be resized to fit
// their contents, as there is no way to do it using just CSS
//
// notice that this script is injected as user script, which is not affected
// by the page CSP, and that it uses capturing listener because load events
// don't bubble
static const char *const FIT_IFRAMES_SCRIPT = R"(
if ( window === window.top ) {
   document.addEventListener('load', function(event) {
      const frame = event.target;
      if ( !(frame instanceof HTMLIFrameElement) )
         return;

      const doc = frame.contentDocument;
      if ( !doc || !doc.documentElement )
         return;

      // set the height to 0 first to make the contents shrink if possible
      // (e.g. if it uses "height: 100%")
      const fit = function() {
         frame.style.height = '0';
         frame.style.height = doc.scrollingElement.scrollHeight + 'px';
      };

      // this also calls fit() initially
      new ResizeObserver(fit).observe(doc.documentElement);
   }, true);
}
)";

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return "#RRGGBB" CSS colour spec
static wxString Col2CSS(const wxColour& col);

// replace HTML special characters with the corresponding entities
static wxString EscapeHtml(const wxString& text);

// return the data: URI containing the given image in PNG format
static wxString ImageToDataURI(const wxImage& image);

// return CSS font-family and font-size properties for the given font
static wxString Font2CSS(const wxFont& font);

// ----------------------------------------------------------------------------
// wxWebView configuration helpers
// ----------------------------------------------------------------------------

// The legacy IE backend under MSW doesn't support Content-Security-Policy,
// which we rely on for blocking scripts and remote content, nor many other
// modern features, so we only use Edge there.
#ifdef __WXMSW__
   #define WEBVIEW_BACKEND wxWebViewBackendEdge
#else
   #define WEBVIEW_BACKEND wxWebViewBackendDefault
#endif

// All our wxWebViews must share the same configuration: with WebKitGTK, each
// configuration has its own web context with its own helper processes, so
// using a new one for every viewer would launch new processes every time the
// viewer is switched. wxWebViewConfiguration only exists since wx 3.3.0, but
// earlier versions used the shared default context anyhow.
#if wxCHECK_VERSION(3, 3, 0)

using WebViewConfig = wxWebViewConfiguration;

static WebViewConfig CreateWebViewConfig()
{
   return wxWebView::NewConfiguration(WEBVIEW_BACKEND);
}

static wxWebView *
CreateWebView(const WebViewConfig& config, wxWindow *parent)
{
   wxWebView * const webview = wxWebView::New(config);
   webview->Create(parent, wxID_ANY, wxWebViewDefaultURLStr,
                   wxDefaultPosition, parent->GetClientSize());
   return webview;
}

#else // wx < 3.3

struct WebViewConfig { };

static WebViewConfig CreateWebViewConfig()
{
   return WebViewConfig();
}

static wxWebView *
CreateWebView(const WebViewConfig& /* config */, wxWindow *parent)
{
   return wxWebView::New(parent, wxID_ANY, wxWebViewDefaultURLStr,
                         wxDefaultPosition, parent->GetClientSize(),
                         WEBVIEW_BACKEND);
}

#endif // wx 3.3+/wx < 3.3

// ----------------------------------------------------------------------------
// WebViewViewer: a wxWebView-based MessageViewer implementation
// ----------------------------------------------------------------------------

class WebViewViewer : public MessageViewer
{
public:
   explicit WebViewViewer(const WebViewConfig& config);

   // creation &c
   void Create(MessageView *msgView, wxWindow *parent) override;
   void Clear() override;
   void Update() override;
   void UpdateOptions() override;
   wxWindow *GetWindow() const override;

   // operations
   bool Find(const String& text) override;
   bool FindAgain() override;
   void SelectAll() override;
   String GetSelection() const override;
   void Copy() override;
   bool Print() override;
   void PrintPreview() override;

   // header showing
   void StartHeaders() override;
   void ShowRawHeaders(const String& header) override;
   void ShowHeaderName(const String& name) override;
   void ShowHeaderValue(const String& value,
                        wxFontEncoding encoding) override;
   void ShowHeaderURL(const String& text,
                      const String& url) override;
   void EndHeader() override;
   void ShowXFace(const wxBitmap& bitmap) override;
   void EndHeaders() override;

   // body showing
   void StartBody() override;
   void StartPart() override;
   void InsertAttachment(const wxBitmap& icon, ClickableInfo *ci) override;
   void InsertClickable(const wxBitmap& icon,
                        ClickableInfo *ci,
                        const wxColour& col) override;
   void InsertImage(const wxImage& image, ClickableInfo *ci) override;
   void InsertRawContents(const String& data) override;
   void InsertText(const String& text, const MTextStyle& style) override;
   void InsertURL(const String& text, const String& url) override;
   void EndPart() override;
   void EndBody() override;

   // scrolling
   bool LineDown() override;
   bool LineUp() override;
   bool PageDown() override;
   bool PageUp() override;

   // capabilities querying
   bool CanInlineImages() const override;
   bool CanProcess(const String& mimetype) const override;

private:
   // called by StartHeaders() first time it's called on a new message
   void StartMessage();

   // return the Content-Security-Policy to use for the current options
   String GetContentSecurityPolicy() const;

   // store the clickable info (taking ownership of it) and return the URL
   // which can be used to refer to it
   String StoreClickable(ClickableInfo *ci);

   // insert the given image as a link to the given clickable
   void InsertClickableImage(const wxImage& image, ClickableInfo *ci);

   // handle a click on the link with the given URL
   void OnLinkClicked(const String& url);

   // scroll by the given (JavaScript) amount in pixels, return true if the
   // scroll position changed
   bool ScrollBy(const char *amount);


   // event handlers
   void OnNavigating(wxWebViewEvent& event);
   void OnNewWindow(wxWebViewEvent& event);


   // the configuration shared by all viewers
   const WebViewConfig m_config;

   // the viewer window
   wxWebView *m_window = nullptr;

   // the HTML string: we accumulate HTML in it and render it in EndBody()
   String m_htmlText;

   // the XFace if we have any
   wxBitmap m_bmpXFace;

   // did we have some headers already?
   bool m_firstheader = true;

   // the clickable objects in the current message, the index in this array
   // is used in their URLs
   std::vector<std::unique_ptr<ClickableInfo>> m_clickables;

   // the string we're searching for
   String m_findText;
};

// ----------------------------------------------------------------------------
// WebViewViewerFactory: creates WebViewViewer objects
// ----------------------------------------------------------------------------

// We don't use IMPLEMENT_MESSAGE_VIEWER() because we need to check if
// wxWebView is available when initializing the module: if it isn't, we fail
// to initialize and MessageView falls back to another viewer.
class WebViewViewerFactory : public MessageViewerFactory
{
public:
   WebViewViewerFactory() : m_config(CreateWebViewConfig()) { }

   MessageViewer *Create() override { return new WebViewViewer(m_config); }

   MMODULE_DEFINE();
   DEFAULT_ENTRY_FUNC;

private:
   // the configuration shared by all viewers we create
   const WebViewConfig m_config;
};

MMODULE_BEGIN_IMPLEMENT(WebViewViewerFactory, "WebViewViewer",
                        MESSAGE_VIEWER_INTERFACE,
                        _("Web message viewer"), "1.00")
   MMODULE_PROP(MMODULE_AUTHOR_PROP, "(c) 2026 Vadim Zeitlin <vadim@zeitlins.org>")
MMODULE_END_IMPLEMENT(WebViewViewerFactory)

MModule *WebViewViewerFactory::Init(int /* version_major */,
                                    int /* version_minor */,
                                    int /* version_release */,
                                    MInterface * /* minterface */,
                                    int * /* errorCode */)
{
   if ( !wxWebView::IsBackendAvailable(WEBVIEW_BACKEND) )
      return nullptr;

   return new WebViewViewerFactory();
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// HTML helpers
// ----------------------------------------------------------------------------

static wxString Col2CSS(const wxColour& col)
{
   return col.GetAsString(wxC2S_HTML_SYNTAX);
}

static wxString EscapeHtml(const wxString& text)
{
   wxString textSafe;
   textSafe.reserve(text.length());

   for ( const auto ch : text )
   {
      switch ( ch.GetValue() )
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
            // ignore, we only keep '\n'
            break;

         default:
            textSafe += ch;
      }
   }

   return textSafe;
}

static wxString ImageToDataURI(const wxImage& image)
{
   wxMemoryOutputStream stream;
   if ( !image.SaveFile(stream, wxBITMAP_TYPE_PNG) )
      return wxString();

   const wxStreamBuffer * const buf = stream.GetOutputStreamBuffer();

   return "data:image/png;base64," +
            wxBase64Encode(buf->GetBufferStart(), buf->GetBufferSize());
}

static wxString Font2CSS(const wxFont& font)
{
   return wxString::Format(R"(font-family: "%s", %s; font-size: %gpt;)",
                           font.GetFaceName(),
                           font.IsFixedWidth() ? "monospace" : "sans-serif",
                           font.GetFractionalPointSize());
}

// ----------------------------------------------------------------------------
// WebViewViewer creation &c
// ----------------------------------------------------------------------------

WebViewViewer::WebViewViewer(const WebViewConfig& config)
             : m_config(config)
{
   m_htmlText.reserve(4096);
}

void WebViewViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;

   m_window = CreateWebView(m_config, parent);

   // the default context menu contains items such as "Reload" or "Back"
   // which make no sense for us
   //
   // TODO: show our own context menu, especially for the clickables
   m_window->EnableContextMenu(false);

   m_window->AddUserScript(FIT_IFRAMES_SCRIPT);

   m_window->Bind(wxEVT_WEBVIEW_NAVIGATING,
                  [this](wxWebViewEvent& event) { OnNavigating(event); });
   m_window->Bind(wxEVT_WEBVIEW_NEWWINDOW,
                  [this](wxWebViewEvent& event) { OnNewWindow(event); });
}

void WebViewViewer::Clear()
{
   m_window->SetPage("<html><body></body></html>", wxString());

   m_htmlText.clear();
   m_bmpXFace = wxNullBitmap;
   m_clickables.clear();
}

void WebViewViewer::Update()
{
   // nothing to do here
}

void WebViewViewer::UpdateOptions()
{
   // no special options
}

wxWindow *WebViewViewer::GetWindow() const
{
   return m_window;
}

// ----------------------------------------------------------------------------
// WebViewViewer operations
// ----------------------------------------------------------------------------

bool WebViewViewer::Find(const String& text)
{
   m_findText = text;

   // the first call to wxWebView::Find() with the new text just counts the
   // matches, so call FindAgain() to actually select the first of them
   const long count = m_window->Find(m_findText, wxWEBVIEW_FIND_WRAP);
   if ( count == wxNOT_FOUND || count == 0 )
      return false;

   return FindAgain();
}

bool WebViewViewer::FindAgain()
{
   return m_window->Find(m_findText, wxWEBVIEW_FIND_WRAP) != wxNOT_FOUND;
}

void WebViewViewer::SelectAll()
{
   m_window->SelectAll();
}

String WebViewViewer::GetSelection() const
{
   // don't use wxWebView::GetSelectedText() as it depends on the web
   // extension being available under GTK, which is not necessarily the case
   //
   // also check for the selection inside the focused iframe, if any
   String text;
   m_window->RunScript
   (
      "(function() {"
      "  const frame = document.activeElement;"
      "  const win = frame instanceof HTMLIFrameElement ? frame.contentWindow"
      "                                                 : window;"
      "  return win.getSelection().toString();"
      "})()",
      &text
   );
   return text;
}

void WebViewViewer::Copy()
{
   m_window->Copy();
}

bool WebViewViewer::Print()
{
   m_window->Print();

   return true;
}

void WebViewViewer::PrintPreview()
{
   // wxWebView doesn't support print preview, but the native print dialog
   // typically does, so just show it
   m_window->Print();
}

// ----------------------------------------------------------------------------
// HTML generation helpers
// ----------------------------------------------------------------------------

String WebViewViewer::GetContentSecurityPolicy() const
{
   const ProfileValues& profileValues = GetOptions();

   // don't allow loading anything from outside by default (this prevents
   // e.g. frames and plugins from being used too)
   String csp = "default-src 'none'; base-uri 'none'; form-action 'none'; ";

   // never allow running scripts from the message, but we need to allow eval
   // as this is what wxWebView::RunScript() uses: it's harmless as, without
   // inline scripts, nothing in the page itself can call it
   csp += "script-src 'unsafe-eval'; ";

   // the external resources are only allowed if we're configured to show the
   // external images
   const bool allowExt = profileValues.inlineGFX && profileValues.showExtImages;
   const char * const ext = allowExt ? " *" : "";

   // data: URIs are used for our own images and are always allowed
   csp << "img-src data:" << ext << "; "
       << "style-src 'unsafe-inline'" << ext << "; "
       << "font-src data:" << ext << ";";

   return csp;
}

void WebViewViewer::StartMessage()
{
   const ProfileValues& profileValues = GetOptions();

   m_htmlText = "<!DOCTYPE html>\n"
                "<html><head>"
                R"(<meta charset="utf-8">)"
                R"(<meta http-equiv="Content-Security-Policy" content=")";
   m_htmlText << GetContentSecurityPolicy() << R"(">)";

   m_htmlText << "<style>\n"
                 "body {";

   if ( profileValues.FgCol.IsOk() )
      m_htmlText << " color: " << Col2CSS(profileValues.FgCol) << ";";
   if ( profileValues.BgCol.IsOk() )
      m_htmlText << " background-color: " << Col2CSS(profileValues.BgCol) << ";";

   const wxFont font(profileValues.GetFont());
   if ( font.IsOk() )
      m_htmlText << " " << Font2CSS(font);

   m_htmlText << " }\n";

   if ( profileValues.UrlCol.IsOk() )
      m_htmlText << "a { color: " << Col2CSS(profileValues.UrlCol) << "; }\n";

   if ( profileValues.HeaderNameCol.IsOk() )
   {
      m_htmlText << "td.hdr-name { color: "
                 << Col2CSS(profileValues.HeaderNameCol) << "; }\n";
   }

   if ( profileValues.HeaderValueCol.IsOk() )
   {
      m_htmlText << "td.hdr-value { color: "
                 << Col2CSS(profileValues.HeaderValueCol) << "; }\n";
   }

   m_htmlText << "div.headers { display: flex; align-items: flex-start; }\n"
                 "img.xface { margin-left: auto; }\n"
                 "td.hdr-name { text-align: right; vertical-align: top;"
                 " font-family: monospace; white-space: nowrap; }\n"
                 "div.part { margin: 1em 0; }\n"
                 "span.text { white-space: pre-wrap; }\n"
                 "iframe.html { display: block; width: 100%; border: none;"
                 " background-color: white; }\n"
                 "</style>"
                 "</head><body>";
}

String WebViewViewer::StoreClickable(ClickableInfo *ci)
{
   m_clickables.emplace_back(ci);

   return wxString::Format(CLICKABLE_SCHEME "%zu", m_clickables.size() - 1);
}

void WebViewViewer::InsertClickableImage(const wxImage& image,
                                         ClickableInfo *ci)
{
   const String label = EscapeHtml(ci->GetLabel());

   m_htmlText << R"(<a href=")" << StoreClickable(ci) << R"(" )"
                 R"(title=")" << label << R"(">)"
                 R"(<img alt=")" << label << R"(" )"
                 R"(src=")" << ImageToDataURI(image) << R"("></a>)";
}

// ----------------------------------------------------------------------------
// header showing
// ----------------------------------------------------------------------------

void WebViewViewer::StartHeaders()
{
   // this can be called multiple times for display of the embedded message
   // headers, only start the message once
   if ( m_htmlText.empty() )
      StartMessage();

   // the headers table is put inside this div together with the X-Face, if
   // any
   m_htmlText += R"(<div class="headers">)";

   // the next header will be the first one
   m_firstheader = true;
}

void WebViewViewer::ShowRawHeaders(const String& header)
{
   m_htmlText << "<pre>" << EscapeHtml(header) << "</pre>";
}

void WebViewViewer::ShowHeaderName(const String& name)
{
   if ( m_firstheader )
   {
      m_htmlText += "<table>";

      m_firstheader = false;
   }

   m_htmlText << R"(<tr><td class="hdr-name">)" << EscapeHtml(name) << ":</td>"
                 R"(<td class="hdr-value">)";
}

void WebViewViewer::ShowHeaderValue(const String& value,
                                    wxFontEncoding /* encoding */)
{
   // the value is already in Unicode, so we don't need the encoding
   m_htmlText += EscapeHtml(value);
}

void WebViewViewer::ShowHeaderURL(const String& text,
                                  const String& url)
{
   InsertURL(text, url);
}

void WebViewViewer::EndHeader()
{
   m_htmlText += "</td></tr>";
}

void WebViewViewer::ShowXFace(const wxBitmap& bitmap)
{
   // we show it in EndHeaders(), after the headers table
   m_bmpXFace = bitmap;
}

void WebViewViewer::EndHeaders()
{
   if ( !m_firstheader )
   {
      m_htmlText += "</table>";

      m_firstheader = true;
   }
   //else: we had no headers at all

   if ( m_bmpXFace.IsOk() )
   {
      m_htmlText << R"(<img class="xface" src=")"
                 << ImageToDataURI(m_bmpXFace.ConvertToImage()) << R"(">)";

      m_bmpXFace = wxNullBitmap;
   }

   m_htmlText += "</div>";
}

// ----------------------------------------------------------------------------
// body showing
// ----------------------------------------------------------------------------

void WebViewViewer::StartBody()
{
}

void WebViewViewer::StartPart()
{
   // don't use <p> as the HTML parts contents can't be put inside it
   m_htmlText += R"(<div class="part">)";
}

void WebViewViewer::InsertAttachment(const wxBitmap& icon, ClickableInfo *ci)
{
   InsertClickableImage(icon.ConvertToImage(), ci);
}

void WebViewViewer::InsertClickable(const wxBitmap& icon,
                                    ClickableInfo *ci,
                                    const wxColour& /* col */)
{
   InsertAttachment(icon, ci);
}

void WebViewViewer::InsertImage(const wxImage& image, ClickableInfo *ci)
{
   InsertClickableImage(image, ci);
}

void WebViewViewer::InsertRawContents(const String& data)
{
   // put the HTML contents into an iframe to keep its styles separate from
   // ours and vice versa
   String html = data;

   // HTML messages are meant to be shown using their own colours, which we
   // don't change, but use our font for them if they don't specify any: this
   // style uses zero specificity so that anything in the message overrides it
   // and, as it's appended at the end of the document, the browser will move
   // it inside the body, which is fine
   const wxFont font(GetOptions().GetFont());
   if ( font.IsOk() )
      html << "<style>:where(html) { " << Font2CSS(font) << " }</style>";

   // the links navigating the iframe itself are blocked by our CSP, which
   // doesn't allow loading anything in the frames, so make them navigate the
   // top window instead, which allows us to handle them in OnNavigating(),
   // unless the message has its own <base target>, as the first one is used
   html << R"(<base target="_top">)";

   // the sandbox prevents any scripts from running inside the iframe, even
   // independently of our CSP, which is inherited by it, but allows us to
   // access its contents and to handle the links inside it
   m_htmlText << R"(<iframe class="html" scrolling="no" )"
                 R"(sandbox="allow-same-origin allow-popups )"
                 R"(allow-top-navigation-by-user-activation" )"
                 R"(srcdoc=")" << EscapeHtml(html) << R"("></iframe>)";

   // TODO: unlike HtmlViewer, we don't call m_msgView->OnBodyText() here, so
   //       quoting HTML-only messages in replies doesn't work yet
}

void WebViewViewer::InsertText(const String& text, const MTextStyle& style)
{
   m_htmlText += R"(<span class="text")";

   String css;
   const wxColour& col = style.GetTextColour();
   if ( col.IsOk() )
      css << "color: " << Col2CSS(col) << ";";

   if ( style.HasFont() )
   {
      const wxFont& font = style.GetFont();
      if ( font.GetWeight() == wxFONTWEIGHT_BOLD )
         css += "font-weight: bold;";
      if ( font.GetStyle() == wxFONTSTYLE_ITALIC )
         css += "font-style: italic;";
   }

   if ( !css.empty() )
      m_htmlText << R"( style=")" << css << R"(")";

   m_htmlText << ">" << EscapeHtml(text) << "</span>";
}

void WebViewViewer::InsertURL(const String& text, const String& url)
{
   m_htmlText << R"(<a href=")" << EscapeHtml(url) << R"(">)"
              << EscapeHtml(text)
              << "</a>";
}

void WebViewViewer::EndPart()
{
   m_htmlText += "</div>";
}

void WebViewViewer::EndBody()
{
   m_htmlText += "</body></html>";

   wxLogTrace("webview", "Generated HTML output:\n%s\n", m_htmlText);

   m_window->SetPage(m_htmlText, wxString());
}

// ----------------------------------------------------------------------------
// links handling
// ----------------------------------------------------------------------------

void WebViewViewer::OnLinkClicked(const String& url)
{
   ClickableInfo *ci = nullptr;

   String index;
   if ( url.StartsWith(CLICKABLE_SCHEME, &index) )
   {
      unsigned long n;
      if ( !index.ToULong(&n) || n >= m_clickables.size() )
      {
         wxLogDebug("Invalid clickable URL \"%s\".", url);
         return;
      }

      ci = m_clickables[n].get();
   }
   else // a normal URL
   {
      ci = new ClickableURL(m_msgView, url);
      StoreClickable(ci);
   }

   ci->OnLeftClick();
}

void WebViewViewer::OnNavigating(wxWebViewEvent& event)
{
   // we must allow loading our own contents set by SetPage(), which uses
   // "about:blank" URL, but nothing else
   const String url = event.GetURL();
   if ( url.StartsWith("about:") )
      return;

   event.Veto();

   // don't handle navigation not initiated by the user, it could be e.g. a
   // <meta> refresh tag in the message
   if ( !(event.GetNavigationAction() & wxWEBVIEW_NAV_ACTION_USER) )
      return;

   OnLinkClicked(url);
}

void WebViewViewer::OnNewWindow(wxWebViewEvent& event)
{
   // this is generated for the links with target="_blank" which are very
   // common in HTML messages, handle them as normal links
   OnLinkClicked(event.GetURL());
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

bool WebViewViewer::ScrollBy(const char *amount)
{
   const String script = wxString::Format
   (
      "(function() {"
      "  var line = parseFloat(getComputedStyle(document.body).fontSize) * 1.25;"
      "  var page = window.innerHeight - 2*line;"
      "  var y = window.scrollY;"
      "  window.scrollBy({top: %s, behavior: 'instant'});"
      "  return window.scrollY != y;"
      "})()",
      amount
   );

   String result;
   if ( !m_window->RunScript(script, &result) )
      return false;

   return result == "true";
}

bool WebViewViewer::LineDown()
{
   return ScrollBy("line");
}

bool WebViewViewer::LineUp()
{
   return ScrollBy("-line");
}

bool WebViewViewer::PageDown()
{
   return ScrollBy("page");
}

bool WebViewViewer::PageUp()
{
   return ScrollBy("-page");
}

// ----------------------------------------------------------------------------
// capabilities querying
// ----------------------------------------------------------------------------

bool WebViewViewer::CanInlineImages() const
{
   return true;
}

bool WebViewViewer::CanProcess(const String& mimetype) const
{
   return mimetype == "TEXT/HTML";
}

#endif // USE_WEBVIEW
