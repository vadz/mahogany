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
#   include   "Mcommon.h"
#   include   "MApplication.h"
#   include   "Profile.h"
#   include   "guidef.h"
#endif

#include "MFolder.h"
#include "MFolderDialogs.h"

#include "gui/wxBrowseButton.h"

// ============================================================================
// implementation
// ============================================================================

// -----------------------------------------------------------------------------
// event tables
// -----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxBrowseButton, wxButton)
   EVT_BUTTON(-1, wxBrowseButton::OnButtonClick)
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
// wxFolderBrowseButton
// ----------------------------------------------------------------------------

void wxFolderBrowseButton::DoBrowse()
{
   MFolder *folder = ShowFolderSelectionDialog(m_folder, this);

   if ( folder )
   {
      m_folder = folder;
      SetText(m_folder->GetFullName());
   }
   //else: nothing changed, user cancelled the dialog
}

