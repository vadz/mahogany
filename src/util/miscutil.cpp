/*-*- c++ -*-********************************************************
 * miscutil.cpp : miscellaneous utility functions                   *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"

#ifndef   USE_PCH
#   include "Mcommon.h"
#   include "MApplication.h"
#   include "gui/wxIconManager.h"
#   include "Mdefaults.h"
#   include "gui/wxMFrame.h"

#   include <wx/frame.h>                // for wxFrame
#endif // USE_PCH

#include "MFStatus.h"
#include "MailFolder.h"

#include "miscutil.h"

class MOption;

extern const MOption MP_FOLDERSTATUS_STATBAR;
extern const MOption MP_FOLDERSTATUS_TITLEBAR;

// show the number of new/unread/total messages in the title and status bars
void UpdateTitleAndStatusBars(const String& title,
                              const String& status,
                              wxFrame *frame,
                              const MailFolder *mailFolder)
{
#ifdef OS_WIN
   // the MP_FOLDERSTATUS_STATBAR/TOOLBAR entries contain '%' which shouldn't
   // be handled as var expansions under Windows
   ProfileEnvVarSave disableExpansion(mApplication->GetProfile());
#endif // Win

   MailFolderStatus mfStatus;
   String folderName = mailFolder->GetName();

   // contruct the messages
   wxString statusMsg(status);
   statusMsg += FormatFolderStatusString
                (
                 READ_APPCONFIG(MP_FOLDERSTATUS_STATBAR),
                 folderName,
                 &mfStatus,
                 mailFolder
                );

   wxString titleMsg(title);
   titleMsg += FormatFolderStatusString
               (
                READ_APPCONFIG(MP_FOLDERSTATUS_TITLEBAR),
                folderName,
                &mfStatus,
                mailFolder
               );

   // show them
   frame->SetStatusText(statusMsg);
   frame->SetTitle(titleMsg);

   // TODO: customize this to show "unread mail" icon!
   if ( mfStatus.newmsgs > 0 )
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON(_T("MainFrameNewMail")) : ICON(_T("MFrameNewMail")));
   else
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON(_T("MainFrame")) : ICON(_T("MFrame")));
}

// ---------------------------------------------------------------------------
// colour to string conversion
// ---------------------------------------------------------------------------

static const wxChar *rgbSpecificationString = gettext_noop("RGB(%d, %d, %d)");

bool ParseColourString(const String& name, wxColour* colour)
{
   if ( name.empty() )
      return FALSE;

   wxString customColourString(wxGetTranslation(rgbSpecificationString));

   // first check if it's a RGB specification
   int red, green, blue;
   if ( wxSscanf(name, customColourString, &red, &green, &blue) == 3 )
   {
      // it's a custom colour
      if ( colour )
         colour->Set(red, green, blue);
   }
   else // a colour name
   {
      wxColour col = wxTheColourDatabase->Find(name);
      if ( !col.Ok() )
         return FALSE;

      if ( colour )
         *colour = col;
   }

   return TRUE;
}

String GetColourName(const wxColour& colour)
{
   wxString colName(wxTheColourDatabase->FindName(colour));
   if ( !colName )
   {
      // no name for this colour
      colName.Printf(wxGetTranslation(rgbSpecificationString),
                     colour.Red(), colour.Green(), colour.Blue());
   }
   else
   {
      // at least under X the string returned is always capitalized,
      // convert it to lower case (it doesn't really matter, but capitals
      // look ugly)
      colName.MakeLower();
   }

   return colName;
}

// get the colour by name and warn the user if it failed
void GetColourByName(wxColour *colour,
                     const String& name,
                     const String& def)
{
   if ( !ParseColourString(name, colour) )
   {
      wxLogError(_("Cannot find a colour named '%s', using default instead.\n"
                   "(please check the settings)"), name.c_str());
      *colour = def;
   }
}
