///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxBrowseButton.cpp - implementation of browse button
//              classes declared in gui/wxBrowseButton.h.
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     24.12.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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
#  include "guidef.h"
#  include "MApplication.h"
#  include "gui/wxIconManager.h"

#  include <wx/statbmp.h>
#  include <wx/layout.h>
#  include <wx/statbox.h>
#  include <wx/dirdlg.h>
#  include <wx/filedlg.h>
#endif // USE_PCH

#include <wx/colordlg.h>
#include <wx/fontdlg.h>
#include <wx/fontutil.h>
#include <wx/imaglist.h>        // for wxImageList

#include "MFolder.h"

#include "gui/wxDialogLayout.h"
#include "gui/wxBrowseButton.h"

#include "ColourNames.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(wxBitmap *, BitmapArray);

class wxIconView : public wxListCtrl
{
public:
   // the size of icons as we show them
   static const int ms_iconSize;

   wxIconView(wxDialog *parent,
              const BitmapArray& icons,
              int selection);

private:
   DECLARE_NO_COPY_CLASS(wxIconView)
};

class wxIconSelectionDialog : public wxManuallyLaidOutDialog
{
public:
   wxIconSelectionDialog(wxWindow *parent,
                         const wxString& title,
                         const BitmapArray& icons,
                         int selection);

   // accessors
   size_t GetSelection() const { return m_selection; }

   // event handlers
   void OnIconSelected(wxListEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);

private:
   int m_selection;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxIconSelectionDialog)
};

// notify the wxColorBrowseButton about changes to its associated text
// control
class wxColorTextEvtHandler : public wxEvtHandler
{
public:
   wxColorTextEvtHandler(wxColorBrowseButton *btn)
   {
      m_btn = btn;
   }

protected:
   void OnText(wxCommandEvent& event)
   {
      m_btn->UpdateColorFromText();

      event.Skip();
   }

   void OnDestroy(wxWindowDestroyEvent& event)
   {
      event.Skip();

      // delete ourselves as this is the only place where we can do it
      m_btn->OnTextDelete();
   }

private:
   wxColorBrowseButton *m_btn;

   DECLARE_EVENT_TABLE()
   DECLARE_NO_COPY_CLASS(wxColorTextEvtHandler)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event tables
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBrowseButton, wxButton)
   EVT_BUTTON(-1, wxBrowseButton::OnButtonClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxIconSelectionDialog, wxManuallyLaidOutDialog)
   EVT_UPDATE_UI(wxID_OK, wxIconSelectionDialog::OnUpdateUI)
   EVT_LIST_ITEM_SELECTED(-1, wxIconSelectionDialog::OnIconSelected)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxColorTextEvtHandler, wxEvtHandler)
   EVT_TEXT(-1, wxColorTextEvtHandler::OnText)
   EVT_WINDOW_DESTROY(wxColorTextEvtHandler::OnDestroy)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFileBrowseButton
// ----------------------------------------------------------------------------

void wxFileBrowseButton::DoBrowse()
{
   // get the last position
   wxString strLastDir, strLastFile, strLastExt, strPath = GetText();
   wxSplitPath(strPath, &strLastDir, &strLastFile, &strLastExt);

   long style = m_open ? wxOPEN | wxHIDE_READONLY
                       : wxSAVE | wxOVERWRITE_PROMPT;
   if ( m_existingOnly )
      style |= wxFILE_MUST_EXIST;

   wxFileDialog dialog(GetFrame(this), _T(""),
                       strLastDir, strLastFile,
                       wxGetTranslation(wxALL_FILES),
                       style);

   if ( dialog.ShowModal() == wxID_OK )
   {
      SetText(dialog.GetPath());
   }
}

// ----------------------------------------------------------------------------
// wxDirBrowseButton
// ----------------------------------------------------------------------------

void wxDirBrowseButton::DoBrowse()
{
   DoBrowseHelper(this);
}

void wxDirBrowseButton::DoBrowseHelper(wxTextBrowseButton *browseBtn)
{
   wxDirDialog dialog(browseBtn,
                      _("Please choose a directory"),
                      browseBtn->GetText());

   if ( dialog.ShowModal() == wxID_OK )
   {
      browseBtn->SetText(dialog.GetPath());
   }
}

// ----------------------------------------------------------------------------
// wxFileOrDirBrowseButton
// ----------------------------------------------------------------------------

void wxFileOrDirBrowseButton::DoBrowse()
{
   if ( m_browseForFile )
   {
      wxFileBrowseButton::DoBrowse();
   }
   else
   {
      // unfortuanately we don't derive from wxDirBrowseButton and so we can't
      // write wxDirBrowseButton::DoBrowse()...
      wxDirBrowseButton::DoBrowseHelper(this);
   }
}

void wxFileOrDirBrowseButton::UpdateTooltip()
{
   wxString msg;
   msg.Printf(_("Browse for a %s"), IsBrowsingForFiles() ? _("file")
                                                         : _("directory"));
#if wxUSE_TOOLTIPS
   SetToolTip(msg);
#endif // wxUSE_TOOLTIPS
}

// ----------------------------------------------------------------------------
// wxFolderBrowseButton
// ----------------------------------------------------------------------------

wxFolderBrowseButton::wxFolderBrowseButton(wxTextCtrl *text,
                                           wxWindow *parent,
                                           MFolder *folder)
                    : wxTextBrowseButton(text, parent, _("Browse for folder"))
{
   m_folder = folder;

   SafeIncRef(m_folder);
}

void wxFolderBrowseButton::DoBrowse()
{
   MFolder *folder = MDialog_FolderChoose(GetFrame(this), m_folder);

   if ( folder && folder != m_folder )
   {
      SafeDecRef(m_folder);

      m_folder = folder;
      SetText(m_folder->GetFullName());
   }
   //else: nothing changed or user cancelled the dialog
}

MFolder *wxFolderBrowseButton::GetFolder() const
{
   SafeIncRef(m_folder);

   return m_folder;
}

wxFolderBrowseButton::~wxFolderBrowseButton()
{
   SafeDecRef(m_folder);
}

// ----------------------------------------------------------------------------
// wxColorBrowseButton
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxColorBrowseButton, wxButton)

wxColorBrowseButton::wxColorBrowseButton(wxTextCtrl *text, wxWindow *parent)
                   : wxTextBrowseButton(text, parent, _("Choose colour"))
{
   m_hasText = TRUE;

   m_evtHandlerText = new wxColorTextEvtHandler(this);
   GetTextCtrl()->PushEventHandler(m_evtHandlerText);
}

wxColorBrowseButton::~wxColorBrowseButton()
{
   // the order of control deletion is undetermined, so handle both cases
   if ( m_hasText )
   {
      // we're deleted before the text control
      GetTextCtrl()->PopEventHandler(TRUE /* delete it */);
   }
   else
   {
      // the text control had been already deleted so the links in the event
      // handler chain are broken but m_evtHandlerText still has m_text as next
      // handler - reset it to avoid crashing in ~wxColorTextEvtHandler
      m_evtHandlerText->SetNextHandler(NULL);
      delete m_evtHandlerText;
   }
}

void wxColorBrowseButton::OnTextDelete()
{
   m_hasText = FALSE;
}

void wxColorBrowseButton::DoBrowse()
{
   wxColourData colData;

   wxString colName = GetText();
   if ( !colName.empty() )
   {
      (void)ParseColourString(colName, &m_color);
      colData.SetColour(m_color);
   }

   wxColourDialog dialog(GetFrame(this), &colData);

   if ( dialog.ShowModal() == wxID_OK )
   {
      colData = dialog.GetColourData();
      m_color = colData.GetColour();

      SetText(GetColourName(m_color));

      UpdateColor();
   }
}

void wxColorBrowseButton::SetValue(const wxString& text)
{
   // we might be given "RGB(r,g,b)" string but if it corresponds to a known
   // colour, we want to show the colour name to the user, not RGB values
   wxString nameCol;

   if ( !text.empty() )
   {
      if ( !ParseColourString(text, &m_color) )
      {
         nameCol = text;
      }
   }
   else // no valid colour, use default one
   {
      m_color = GetParent()->GetBackgroundColour();
   }

   UpdateColor();

   if ( nameCol.empty() && m_color.Ok() )
   {
      nameCol = GetColourName(m_color);
   }

   SetText(nameCol);
}

void wxColorBrowseButton::UpdateColorFromText()
{
   if ( ParseColourString(GetText(), &m_color) )
   {
      UpdateColor();
   }
}

void wxColorBrowseButton::UpdateColor()
{
   if ( !m_color.Ok() )
      return;

   // some combinations of the fg/bg colours may be unreadable, so change the
   // fg colour to be visible
   wxColour colFg;
   if ( m_color.Red() < 127 && m_color.Blue() < 127 && m_color.Green() < 127 )
   {
      colFg = *wxWHITE;
   }
   else
   {
      colFg = *wxBLACK;
   }

   SetForegroundColour(colFg);
   SetBackgroundColour(m_color);
}

// ----------------------------------------------------------------------------
// wxFontBrowseButton
// ----------------------------------------------------------------------------

wxFontBrowseButton::wxFontBrowseButton(wxTextCtrl *text, wxWindow *parent)
                  : wxTextBrowseButton(text, parent, _("Choose font"))
{
}

// FIXME: these methods rely on internals of wxNativeFontInfo because they
//        know that it prepends the format version number (currently 0) to
//        the real font desc string - they shouldn't but we should add methods
//        to wxNativeFontInfo to do this conversion instead!

String wxFontBrowseButton::FontDescToUser(const String& desc)
{
   String user = desc;
   if ( user.length() > 2 && user[0u] == '0' && user[1u] == ';' )
   {
      user.erase(0, 2);
   }

   return user;
}

String wxFontBrowseButton::FontDescFromUser(const String& user)
{
   String desc;
   if ( !user.empty() )
   {
      desc = _T("0;");
   }

   desc += user;

   return desc;
}

void wxFontBrowseButton::DoBrowse()
{
   wxFont font;
   wxNativeFontInfo fontInfo;
   wxString desc = GetText();
   if ( !desc.empty() )
   {
      if ( fontInfo.FromString(FontDescFromUser(desc)) )
      {
         font.SetNativeFontInfo(fontInfo);
      }
   }

   wxFontData data;
   data.SetInitialFont(font);

   wxFontDialog dialog(GetFrame(this), &data);
   if ( dialog.ShowModal() == wxID_OK )
   {
      font = dialog.GetFontData().GetChosenFont();

      SetText(FontDescToUser(font.GetNativeFontInfoDesc()));
   }
}

// ----------------------------------------------------------------------------
// wxIconBrowseButton
// ----------------------------------------------------------------------------

wxBitmap wxIconBrowseButton::GetBitmap(size_t nIcon) const
{
   wxIconManager *iconManager = mApplication->GetIconManager();

   return iconManager->GetBitmap(m_iconNames[nIcon]);
}

void wxIconBrowseButton::SetIcon(size_t nIcon)
{
   CHECK_RET( nIcon < m_iconNames.GetCount(), _T("invalid icon index") );

   if ( m_nIcon == (int)nIcon )
      return;

   m_nIcon = nIcon;
   if ( m_staticBitmap )
   {
      wxBitmap bmp = GetBitmap(m_nIcon);

      // scale the icon if necessary
      int w1, h1; // size of the icon on the screen
      m_staticBitmap->GetSize(&w1, &h1);

      if ( w1 && h1 )
      {
         // size of the icon
         int w2 = bmp.GetWidth(),
             h2 = bmp.GetHeight();

         if ( (w1 != w2) || (h1 != h2) )
         {
            bmp = wxBitmap(bmp.ConvertToImage().Rescale(w1, h1));
         }
         //else: the size is already correct
      }
      //else: don't rescale the image to invalid size (happens under MSW
      //      during the initial layout phase)

      m_staticBitmap->SetBitmap(bmp);
   }
}

void wxIconBrowseButton::DoBrowse()
{
   size_t n, nIcons = m_iconNames.GetCount();

   BitmapArray icons;
   icons.Alloc(nIcons);

   for ( n = 0; n < nIcons; n++ )
   {
      wxBitmap bmp = GetBitmap(n);

      // save some typing
      static const int size = wxIconView::ms_iconSize;
      if ( bmp.GetWidth() != size || bmp.GetHeight() != size )
      {
         // must resize the icon
         wxImage image(bmp.ConvertToImage());
         image.Rescale(size, size);
         bmp = wxBitmap(image);
      }

      icons.Add(new wxBitmap(bmp));
   }

   wxIconSelectionDialog dlg(GetFrame(this), _("Choose icon"), icons, m_nIcon);

   if ( dlg.ShowModal() == wxID_OK )
   {
      size_t icon = dlg.GetSelection();
      if ( (int)icon != m_nIcon )
      {
         SetIcon(icon);

         OnIconChange();
      }
   }

   for ( n = 0; n < nIcons; n++ )
   {
      delete icons[n];
   }
}

// ----------------------------------------------------------------------------
// wxFolderIconBrowseButton
// ----------------------------------------------------------------------------

wxFolderIconBrowseButton::wxFolderIconBrowseButton(wxWindow *parent,
                                                   const wxString& tooltip)
                        : wxIconBrowseButton(parent, tooltip)
{
   size_t nIcons = GetNumberOfFolderIcons();
   wxArrayString icons;
   for ( size_t n = 0; n < nIcons; n++ )
   {
      icons.Add(GetFolderIconName(n));
   }

   SetIcons(icons);
}

// ----------------------------------------------------------------------------
// wxIconView - the canvas which shows all icons
// ----------------------------------------------------------------------------

#ifdef __WXGTK__
const int wxIconView::ms_iconSize = 16;
#else
const int wxIconView::ms_iconSize = 32;
#endif

wxIconView::wxIconView(wxDialog *parent,
                       const BitmapArray& icons,
                       int selection)
          : wxListCtrl(parent, -1, wxDefaultPosition, wxDefaultSize,
                       wxLC_ICON |
                       wxLC_SINGLE_SEL |
                       wxLC_AUTOARRANGE |
                       wxLC_ALIGN_LEFT)
{
   size_t n, count = icons.GetCount();
   wxImageList *imageList = new wxImageList(ms_iconSize, ms_iconSize,
                                            TRUE, count);
   for ( n = 0; n < count; n++ )
   {
      imageList->Add(*icons[n]);
   }

   SetImageList(imageList, wxIMAGE_LIST_NORMAL);

   for ( n = 0; n < count; n++ )
   {
      InsertItem(n, n);
   }

   if ( selection != -1 )
   {
      SetItemState(selection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
   }
}

// ----------------------------------------------------------------------------
// wxIconSelectionDialog - dialog which allows the user to select an icon
// ----------------------------------------------------------------------------

wxIconSelectionDialog::wxIconSelectionDialog(wxWindow *parent,
                                             const wxString& title,
                                             const BitmapArray& icons,
                                             int selection)
                     : wxManuallyLaidOutDialog(parent, title, _T("IconSelect"))
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Current icon"));

   wxIconView *iconView = new wxIconView(this, icons, selection);

   c = new wxLayoutConstraints;
   c->centreY.SameAs(box, wxCentreY);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.Absolute(wxIconView::ms_iconSize + 9*LAYOUT_X_MARGIN);
   iconView->SetConstraints(c);

   SetDefaultSize(8*wBtn, wxIconView::ms_iconSize + 6*hBtn);
}

void wxIconSelectionDialog::OnIconSelected(wxListEvent& event)
{
   m_selection = event.GetIndex();
}

void wxIconSelectionDialog::OnUpdateUI(wxUpdateUIEvent& event)
{
   event.Enable(m_selection != -1);
}

