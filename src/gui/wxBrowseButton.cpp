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
#  include "MApplication.h"
#  include "Profile.h"
#  include "guidef.h"

#  include <wx/cmndata.h>
#   include <wx/statbmp.h>
#   include <wx/dcmemory.h>
#   include <wx/layout.h>
#   include <wx/statbox.h>
#endif

#include <wx/colordlg.h>

#include "MFolder.h"
#include "MFolderDialogs.h"

#include "gui/wxIconManager.h"
#include "gui/wxDialogLayout.h"

#include "gui/wxBrowseButton.h"

#include "miscutil.h" // for ParseColorString and GetColorName

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

WX_DEFINE_ARRAY(wxBitmap *, wxIconArray);

class wxIconView : public wxScrolledWindow
{
public:
   // the size of icons as we show them
   static const int ms_iconSize;

   wxIconView(wxDialog *parent,
              const wxIconArray& icons,
              int selection);

   // accessors
   size_t GetSelection() const
   {
      ASSERT_MSG( m_selection != -1, "no selection" );

      return (size_t)m_selection;
   }

protected:
   void OnPaint(wxPaintEvent& event);
   void OnClick(wxMouseEvent& event);
   void OnUpdateUI(wxUpdateUIEvent& event);
   void OnSize(wxSizeEvent& event);

   void DoDrawIcon(wxDC& dc, size_t n);

private:
   wxIconArray m_icons;
   int         m_selection;
   int         m_y;

   DECLARE_EVENT_TABLE()
};

class wxIconSelectionDialog : public wxManuallyLaidOutDialog
{
public:
   wxIconSelectionDialog(wxWindow *parent,
                         const wxString& title,
                         const wxIconArray& icons,
                         int selection);

   size_t GetSelection() const { return m_iconView->GetSelection(); }

private:
   wxIconView *m_iconView;
};

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBrowseButton, wxButton)
   EVT_BUTTON(-1, wxBrowseButton::OnButtonClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(wxIconView, wxScrolledWindow)
   EVT_PAINT(wxIconView::OnPaint)
   EVT_SIZE(wxIconView::OnSize)
   EVT_UPDATE_UI(wxID_OK, wxIconView::OnUpdateUI)
   EVT_LEFT_DOWN(wxIconView::OnClick)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxFileBrowseButton
// ----------------------------------------------------------------------------

void wxFileBrowseButton::DoBrowse()
{
   // get the last position
   wxString strLastDir, strLastFile, strLastExt, strPath = GetText();
   wxSplitPath(strPath, &strLastDir, &strLastFile, &strLastExt);

   wxFileDialog dialog(this, "",
                       strLastDir, strLastFile,
                       _("All files (*.*)|*.*"),
                       wxHIDE_READONLY | wxFILE_MUST_EXIST);

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
   SetToolTip(msg);
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
   MFolder *folder = ShowFolderSelectionDialog(m_folder, this);

   if ( folder )
   {
      SafeDecRef(m_folder);

      m_folder = folder;
      SetText(m_folder->GetFullName());
   }
   //else: nothing changed, user cancelled the dialog
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

void wxColorBrowseButton::DoBrowse()
{
   (void)ParseColourString(GetText(), &m_color);

   wxColourData colData;
   colData.SetColour(m_color);

   wxColourDialog dialog(this, &colData);

   if ( dialog.ShowModal() == wxID_OK )
   {
      colData = dialog.GetColourData();
      m_color = colData.GetColour();

      SetText(GetColourName(m_color));
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
   CHECK_RET( nIcon < m_iconNames.GetCount(), "invalid icon index" );

   if ( m_nIcon == (int)nIcon )
      return;

   m_nIcon = nIcon;
   if ( m_staticBitmap )
   {
      wxBitmap bmp = GetBitmap(m_nIcon);

      // scale the icon if necessary
      int w1, h1; // size of the icon on the screen
      m_staticBitmap->GetSize(&w1, &h1);

      // size of the icon
      int w2 = bmp.GetWidth(),
          h2 = bmp.GetHeight();

      if ( (w1 != w2) || (h1 != h2) )
      {
         wxImage image(bmp);
         image.Rescale(w1, h1);
         bmp = image.ConvertToBitmap();
      }
      //else: the size is already correct

      m_staticBitmap->SetBitmap(bmp);
   }
}

void wxIconBrowseButton::DoBrowse()
{
   size_t n, nIcons = m_iconNames.GetCount();

   wxIconArray icons;
   icons.Alloc(nIcons);

   for ( n = 0; n < nIcons; n++ )
   {
      wxBitmap bmp = GetBitmap(n);

      // save some typing
      static const int size = wxIconView::ms_iconSize;

      if ( bmp.GetWidth() != size || bmp.GetHeight() != size )
      {
         // must resize the icon
         wxImage image(bmp);
         image.Rescale(size, size);
         bmp = image.ConvertToBitmap();
      }

      icons.Add(new wxBitmap(bmp));
   }

   wxIconSelectionDialog dlg(this, _("Choose icon"), icons, m_nIcon);

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

const int wxIconView::ms_iconSize = 64;

wxIconView::wxIconView(wxDialog *parent,
                       const wxIconArray& icons,
                       int selection)
          : wxScrolledWindow(parent, -1,
                             wxDefaultPosition, wxDefaultSize,
                             wxHSCROLL | wxBORDER)
{
   m_icons = icons;
   m_selection = selection;

   SetBackgroundColour(*wxWHITE);

   SetScrollbars(ms_iconSize, 0, m_icons.GetCount(), 1,
                 m_selection, 0);

   EnableScrolling(TRUE, FALSE);
}

void wxIconView::OnPaint(wxPaintEvent& WXUNUSED(event))
{
   wxPaintDC dc(this);
   PrepareDC(dc);

#ifdef __WXGTK__
   Clear();
#endif // GTK

   int x0, y0;
   ViewStart(&x0, &y0);

   x0 *= ms_iconSize;

   int nIconFirst = x0 / ms_iconSize;
   int nIconLast = (x0 + GetClientSize().x) / ms_iconSize;
   if ( nIconLast >= (int)m_icons.GetCount() )
   {
      nIconLast = m_icons.GetCount() - 1;
   }

   for ( int n = nIconFirst; n <= nIconLast; n++ )
   {
      DoDrawIcon(dc, (size_t)n);
   }
}

void wxIconView::OnClick(wxMouseEvent& event)
{
   int x;
   CalcUnscrolledPosition(event.GetX(), 0, &x, NULL);

   int selection = (int) (x / ms_iconSize);
   if ( selection != m_selection )
   {
      m_selection = selection;

      Refresh();
   }
}

void wxIconView::OnUpdateUI(wxUpdateUIEvent& event)
{
   event.Enable(m_selection != -1);
}

void wxIconView::OnSize(wxSizeEvent& event)
{
   // always centre the icons vertically
   m_y = (event.GetSize().y - ms_iconSize) / 2;

   event.Skip();
}

void wxIconView::DoDrawIcon(wxDC& dc, size_t n)
{
   wxMemoryDC dcMem(&dc);
   wxBitmap bmp(ms_iconSize, ms_iconSize);
   dcMem.SelectObject(bmp);
   dcMem.DrawBitmap(*m_icons[n], 0, 0);

   // draw the selected icon inverted
   int rop;
   if ( (int)n == m_selection )
   {
      rop = wxSRC_INVERT;
   }
   else
   {
      rop = wxCOPY;
   }

   dc.Blit(n*ms_iconSize, m_y, ms_iconSize, ms_iconSize, &dcMem, 0, 0, rop);

   dcMem.SelectObject(wxNullBitmap);
}

// ----------------------------------------------------------------------------
// wxIconSelectionDialog - dialog which allows the user to select an icon
// ----------------------------------------------------------------------------

wxIconSelectionDialog::wxIconSelectionDialog(wxWindow *parent,
                                             const wxString& title,
                                             const wxIconArray& icons,
                                             int selection)
                     : wxManuallyLaidOutDialog(parent, title, "IconSelect")
{
   // layout the controls
   // -------------------
   wxLayoutConstraints *c;

   // Ok and Cancel buttons and a static box around everything else
   wxStaticBox *box = CreateStdButtonsAndBox(_("&Current icon"));

   m_iconView = new wxIconView(this, icons, selection);

   c = new wxLayoutConstraints;
   c->centreY.SameAs(box, wxCentreY);
   c->left.SameAs(box, wxLeft, 2*LAYOUT_X_MARGIN);
   c->right.SameAs(box, wxRight, 2*LAYOUT_X_MARGIN);
   c->height.Absolute(wxIconView::ms_iconSize + 4*LAYOUT_X_MARGIN);
   m_iconView->SetConstraints(c);

   SetDefaultSize(4*wBtn, wxIconView::ms_iconSize + 4*hBtn);
}
