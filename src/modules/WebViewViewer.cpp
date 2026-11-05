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
#  include <wx/frame.h>
#  include <wx/image.h>
#endif // USE_PCH

#include "MessageViewer.h"
#include "ClickURL.h"
#include "MTextStyle.h"

#include <wx/base64.h>
#include <wx/filesys.h>
#include <wx/mstream.h>
#include <wx/uri.h>

#include <wx/webview.h>

#include <memory>
#include <vector>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the URL scheme used for the links corresponding to our clickable objects
#define CLICKABLE_SCHEME "x-mahogany-clickable:"

// the name of the script message handler used by VIEWER_SCRIPT
#define SCRIPT_MSG_HANDLER "mahogany"

// this script is injected as user script into all pages, which is not affected
// by the page CSP, and sends messages of the form "type:generation:data" to us,
// where the generation is that of the page, see StartMessage()
//
// it handles the hovering over and right clicking the links, which we want to
// handle ourselves, as well as the HTML message parts, which are shown in
// iframes which need to be resized to fit their contents, as there is no way
// to do it using just CSS, and whose text needs to be sent to us, as it is
// only available once they're rendered
//
// notice that the scripts in the iframes are blocked by the CSP inherited by
// them, but we can still access their contents and add our event listeners to
// them from here
static const char *const VIEWER_SCRIPT = R"(
if ( window === window.top ) {
   const post = function(type, data) {
      window.)" SCRIPT_MSG_HANDLER R"(.postMessage(type + ':' +
         document.documentElement.dataset.generation + ':' + data);
   };

   // return the URL of the link containing the given element or empty string
   // (notice that SVG links don't have string href and are just ignored)
   const linkAt = function(elem) {
      const link = elem.closest('a[href]');
      return link && typeof link.href === 'string' ? link.href : '';
   };

   // the menu to show when the right mouse button is released, see below
   let pendingMenu = null;

   let hovered = '';
   const setHovered = function(url) {
      if ( url !== hovered ) {
         hovered = url;
         post('hover', url);
      }
   };

   // handle the links in the given document, which is either our own one or
   // that of the given iframe
   const handleLinks = function(doc, frame) {
      doc.addEventListener('mouseover', function(event) {
         setHovered(linkAt(event.target));
      });

      doc.addEventListener('mouseout', function(event) {
         // this happens when leaving the window or the iframe, in the latter
         // case we'll get mouseover for the element containing the mouse now
         if ( !event.relatedTarget )
            setHovered('');
      });

      doc.addEventListener('contextmenu', function(event) {
         const url = linkAt(event.target);
         if ( !url )
            return;

         event.preventDefault();

         let x = event.clientX,
             y = event.clientY;
         if ( frame ) {
            const rect = frame.getBoundingClientRect();
            x += rect.left;
            y += rect.top;
         }

         const menu = Math.round(x) + ',' + Math.round(y) + ':' + url;

         // under some platforms (e.g. GTK) this event is generated when the
         // button is pressed and the menu shown at this moment would be
         // dismissed when it is released, so wait until then to show it
         if ( event.buttons & 2 )
            pendingMenu = menu;
         else
            post('menu', menu);
      });

      doc.addEventListener('mouseup', function(event) {
         // releasing any other button cancels the pending menu, in case we
         // somehow didn't get the right button release event
         if ( event.button === 2 && pendingMenu )
            post('menu', pendingMenu);

         pendingMenu = null;
      });
   };

   handleLinks(document, null);

   // use capturing listener because load events don't bubble
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

      handleLinks(doc, frame);

      // send the text of the part, prefixed with its index, to be used for
      // quoting it in the replies
      const root = doc.body || doc.documentElement;
      post('text', frame.dataset.part + ':' + root.innerText);
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

// return the data: URI with the given contents
static wxString DataToDataURI(const wxString& mimetype,
                              const void *data,
                              size_t len);

// return the data: URI containing the given image in PNG format
static wxString ImageToDataURI(const wxImage& image);

// return the data: URI with the contents of the MIME part with the given CID
// stored by MessageView or an empty string if there is no such part
static wxString CIDToDataURI(const wxString& cid);

// replace all "cid:" URLs referring to the stored MIME parts in the given HTML
// with data: URIs
static wxString ReplaceCIDs(const wxString& html);

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

   // return the clickable corresponding to the given URL, creating a new one
   // for a normal URL, or nullptr if the URL is an invalid clickable one
   ClickableInfo *GetClickable(const String& url);

   // handle a click on the link with the given URL
   void OnLinkClicked(const String& url);

   // scroll by the given (JavaScript) amount in pixels, return true if the
   // scroll position changed
   bool ScrollBy(const char *amount);


   // handlers for the different VIEWER_SCRIPT messages
   void OnPartText(const String& data);
   void OnLinkHover(const String& url);
   void OnLinkMenu(const String& data);


   // event handlers
   void OnNavigating(wxWebViewEvent& event);
   void OnNewWindow(wxWebViewEvent& event);
   void OnScriptMessage(wxWebViewEvent& event);


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

   // the text of the HTML parts is only available after they're rendered, so
   // we remember the position in the message body text where it must be
   // inserted and the length of the text inserted so far (0 if none yet)
   struct HtmlPartText
   {
      size_t pos = 0;
      size_t len = 0;
   };

   std::vector<HtmlPartText> m_htmlParts;

   // incremented every time we're cleared to ignore the messages from the
   // previously shown message which could still arrive after it
   unsigned m_generation = 0;

   // true if we show the label of the link under mouse in the status bar
   bool m_showsLinkLabel = false;
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

static wxString DataToDataURI(const wxString& mimetype,
                              const void *data,
                              size_t len)
{
   return "data:" + mimetype.Lower() + ";base64," + wxBase64Encode(data, len);
}

static wxString ImageToDataURI(const wxImage& image)
{
   wxMemoryOutputStream stream;
   if ( !image.SaveFile(stream, wxBITMAP_TYPE_PNG) )
      return wxString();

   const wxStreamBuffer * const buf = stream.GetOutputStreamBuffer();

   return DataToDataURI("image/png",
                        buf->GetBufferStart(), buf->GetBufferSize());
}

static wxString CIDToDataURI(const wxString& cid)
{
   // MessageView stores the parts using the wxFileSystem handler for "cid:"
   // protocol
   wxFileSystem fs;
   std::unique_ptr<wxFSFile> file(fs.OpenFile("cid:" + cid));
   if ( !file )
   {
      // the URLs are supposed to be URL-encoded (RFC 2392), while the
      // Content-ID header values are not, so try decoding it too
      const wxString cidUnescaped = wxURI::Unescape(cid);
      if ( cidUnescaped != cid )
         file.reset(fs.OpenFile("cid:" + cidUnescaped));

      if ( !file )
         return wxString();
   }

   wxMemoryOutputStream stream;
   file->GetStream()->Read(stream);

   const wxStreamBuffer * const buf = stream.GetOutputStreamBuffer();

   return DataToDataURI(file->GetMimeType(),
                        buf->GetBufferStart(), buf->GetBufferSize());
}

static wxString ReplaceCIDs(const wxString& html)
{
   // the URLs can occur in many different attributes and in CSS too, so,
   // rather than parsing HTML, just look for all occurrences of "cid:"
   // preceded by a character which can occur before an URL: a quote, an equal
   // sign (for unquoted attribute values) or an opening parenthesis (for CSS
   // url()), as this is good enough in practice
   const wxString htmlLower = html.Lower();

   // the result is at least as long as the original HTML, as data: URIs are
   // longer than the "cid:" URLs they replace
   wxString result;
   result.reserve(html.length());

   size_t posCopied = 0;
   for ( size_t pos = 0; ; )
   {
      pos = htmlLower.find("cid:", pos);
      if ( pos == wxString::npos )
         break;

      const size_t posCID = pos + 4; // strlen("cid:")
      if ( pos == 0 || wxString("\"'=(").find(html[pos - 1]) == wxString::npos )
      {
         pos = posCID;
         continue;
      }

      size_t posEnd = html.find_first_of("\"'()<> \t\r\n", posCID);
      if ( posEnd == wxString::npos )
         posEnd = html.length();

      const wxString cid = html.substr(posCID, posEnd - posCID);
      const wxString uri = CIDToDataURI(cid);
      if ( !uri.empty() )
      {
         result.append(html, posCopied, pos - posCopied);
         result += uri;
         posCopied = posEnd;
      }
      else if ( cid.find('&') != wxString::npos )
      {
         // we don't decode HTML entities, so such CIDs are not found, but
         // they're not supposed to occur in practice, so just mention it
         wxLogDebug("CID \"%s\" containing HTML entities not supported.", cid);
      }

      pos = posEnd;
   }

   result.append(html, posCopied, wxString::npos);

   return result;
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
   // which make no sense for us, we show our own menu for the links instead
   m_window->EnableContextMenu(false);

   m_window->AddUserScript(VIEWER_SCRIPT);

   // notice that under GTK this runs a nested event loop to wait until the
   // handler is registered in the current page
   m_window->AddScriptMessageHandler(SCRIPT_MSG_HANDLER);

   m_window->Bind(wxEVT_WEBVIEW_NAVIGATING,
                  [this](wxWebViewEvent& event) { OnNavigating(event); });
   m_window->Bind(wxEVT_WEBVIEW_NEWWINDOW,
                  [this](wxWebViewEvent& event) { OnNewWindow(event); });
   m_window->Bind(wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED,
                  [this](wxWebViewEvent& event) { OnScriptMessage(event); });
}

void WebViewViewer::Clear()
{
   m_window->SetPage("<html><body></body></html>", wxString());

   m_htmlText.clear();
   m_bmpXFace = wxNullBitmap;
   m_clickables.clear();
   m_htmlParts.clear();
   m_generation++;

   // the link under mouse, if any, is gone now
   OnLinkHover(String());
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

   // the generation is used by VIEWER_SCRIPT to identify this page
   m_htmlText = wxString::Format("<!DOCTYPE html>\n"
                                 R"(<html data-generation="%u"><head>)",
                                 m_generation);
   m_htmlText << R"(<meta charset="utf-8">)"
                 R"(<meta http-equiv="Content-Security-Policy" content=")"
              << GetContentSecurityPolicy() << R"(">)";

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
   //
   // as nothing can be loaded from outside the iframe, embed the images
   // and other parts referenced by it directly into it
   String html = ReplaceCIDs(data);

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

   // the sandbox restricts what the iframe contents can do, but we need to
   // allow scripts in it as otherwise even our own event listeners added to
   // it by VIEWER_SCRIPT are not called: the scripts from the message itself
   // are still blocked by our CSP, which is inherited by the iframe
   //
   // the text of this part will be inserted at the current position in the
   // body text once it's rendered and VIEWER_SCRIPT sends it to us
   m_htmlText << wxString::Format(R"(<iframe data-part="%zu" )",
                                  m_htmlParts.size());
   m_htmlParts.push_back({m_msgView->GetBodyTextPos()});

   m_htmlText << R"(class="html" scrolling="no" )"
                 R"(sandbox="allow-scripts allow-same-origin allow-popups )"
                 R"(allow-top-navigation-by-user-activation" )"
                 R"(srcdoc=")" << EscapeHtml(html) << R"("></iframe>)";
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

ClickableInfo *WebViewViewer::GetClickable(const String& url)
{
   String index;
   if ( url.StartsWith(CLICKABLE_SCHEME, &index) )
   {
      unsigned long n;
      if ( !index.ToULong(&n) || n >= m_clickables.size() )
      {
         wxLogDebug("Invalid clickable URL \"%s\".", url);
         return nullptr;
      }

      return m_clickables[n].get();
   }

   // a normal URL
   ClickableInfo * const ci = new ClickableURL(m_msgView, url);
   StoreClickable(ci);
   return ci;
}

void WebViewViewer::OnLinkClicked(const String& url)
{
   ClickableInfo * const ci = GetClickable(url);
   if ( ci )
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

void WebViewViewer::OnScriptMessage(wxWebViewEvent& event)
{
   // the message is "type:generation:data", see VIEWER_SCRIPT
   const String msg = event.GetString();

   unsigned long generation;
   const size_t posSep1 = msg.find(':');
   const size_t posSep2 = msg.find(':', posSep1 + 1);
   if ( posSep2 == String::npos ||
         !msg.substr(posSep1 + 1, posSep2 - posSep1 - 1).ToULong(&generation) )
   {
      wxLogDebug("Unexpected script message \"%s\".", msg);
      return;
   }

   // ignore the messages from the previously shown message, it's harmless
   if ( generation != m_generation )
      return;

   const String type = msg.substr(0, posSep1);
   const String data = msg.substr(posSep2 + 1);
   if ( type == "text" )
      OnPartText(data);
   else if ( type == "hover" )
      OnLinkHover(data);
   else if ( type == "menu" )
      OnLinkMenu(data);
   else
      wxLogDebug("Unknown script message type \"%s\".", type);
}

void WebViewViewer::OnPartText(const String& data)
{
   // the data is "index:text"
   unsigned long index;
   const size_t posSep = data.find(':');
   if ( posSep == String::npos || !data.substr(0, posSep).ToULong(&index) )
   {
      wxLogDebug("Unexpected HTML part text message \"%s\".", data);
      return;
   }

   if ( index >= m_htmlParts.size() || m_htmlParts[index].len )
   {
      wxLogDebug("Unexpected text for HTML part %lu.", index);
      return;
   }

   String text = data.substr(posSep + 1);
   if ( text.empty() )
      return;

   // separate it from the text of the next part, if any
   if ( text.Last() != '\n' )
      text += '\n';

   // the text of the previous parts may have been already inserted before
   // the position of this one, account for it
   size_t pos = m_htmlParts[index].pos;
   for ( size_t n = 0; n < index; n++ )
      pos += m_htmlParts[n].len;

   m_msgView->InsertBodyText(pos, text);

   m_htmlParts[index].len = text.length();
}

void WebViewViewer::OnLinkHover(const String& url)
{
   // don't overwrite the status bar text set by somebody else if we hadn't
   // shown anything there
   if ( url.empty() && !m_showsLinkLabel )
      return;

   wxFrame * const frame = m_msgView->GetParentFrame();
   if ( !frame || !frame->GetStatusBar() )
      return;

   String label;
   String index;
   if ( url.StartsWith(CLICKABLE_SCHEME, &index) )
   {
      unsigned long n;
      if ( index.ToULong(&n) && n < m_clickables.size() )
         label = m_clickables[n]->GetLabel();
   }
   else // a normal URL
   {
      label = url;
   }

   frame->SetStatusText(label);

   m_showsLinkLabel = !label.empty();
}

void WebViewViewer::OnLinkMenu(const String& data)
{
   // the data is "x,y:url" with the coordinates in CSS pixels
   long x, y;
   const size_t posComma = data.find(',');
   const size_t posSep = data.find(':', posComma);
   if ( posSep == String::npos ||
         !data.substr(0, posComma).ToLong(&x) ||
         !data.substr(posComma + 1, posSep - posComma - 1).ToLong(&y) )
   {
      wxLogDebug("Unexpected link menu message \"%s\".", data);
      return;
   }

   ClickableInfo * const ci = GetClickable(data.substr(posSep + 1));
   if ( !ci )
      return;

   // CSS pixels are DIPs, at least as long as the page is not zoomed
   ci->OnRightClick(m_window->FromDIP(wxPoint(x, y)));
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
