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

typedef MessageView::AllProfileValues ProfileValues;

// ----------------------------------------------------------------------------
// LayoutViewer: a wxLayout-based MessageViewer implementation
// ----------------------------------------------------------------------------

class LayoutViewer : public MessageViewer
{
public:
   // default ctor
   LayoutViewer();

   // operations
   virtual void Create(MessageView *msgView, wxWindow *parent);
   virtual void Clear();
   virtual void Update();
   virtual bool Print();
   virtual void PrintPreview();
   virtual wxWindow *GetWindow() const;

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

   // capabilities querying
   virtual bool CanInlineImages() const;
   virtual bool CanProcess(const String& mimetype) const;

private:
   // set the text colour
   void SetTextColour(const wxColour& col)
      { m_window->GetLayoutList()->SetFontColour((wxColour *)&col); }

   // the viewer window
   wxLayoutWindow *m_window;

   DECLARE_MESSAGE_VIEWER()
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

// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------

void LayoutViewer::Create(MessageView *msgView, wxWindow *parent)
{
   m_msgView = msgView;
   m_window = new wxLayoutWindow(parent);
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
                   (wxColour *)&profileValues.BgCol,
                   true /* no update */);

   m_window->SetBackgroundColour( profileValues.BgCol );

   // speeds up insertion of text
   m_window->GetLayoutList()->SetAutoFormatting(FALSE);
}

void LayoutViewer::Update()
{
   m_window->RequestUpdate();
}

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

