/*-*- c++ -*-********************************************************
 * miscutil.cpp : miscellaneous utility functions                   *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *                                                                  *
 *******************************************************************/

#include "Mpch.h"
#include "Mcommon.h"

#ifndef   USE_PCH
#   include "sysutil.h"
#   include "strutil.h"
#   include "guidef.h"
#   include "gui/wxMFrame.h"
#   include "MApplication.h"
#   include "Message.h"
#endif

#include <wx/colour.h>
#include <wx/bitmap.h>
#include <wx/image.h>

#include "gui/wxIconManager.h"

#include "Mdefaults.h"
#include "MailFolder.h"    // UpdateTitleAndStatusBars uses it
#include "ASMailFolder.h"
#include "miscutil.h"

#include <ctype.h>

// helper function to parse a string for foldr status messages
static String ParseStatusFormat(const String& format,
                                unsigned long total,
                                unsigned long recent,
                                unsigned long newmsgs,
                                const String& name)
{
   String result;
   const char *start = format.c_str();
   for ( const char *p = start; *p; p++ )
   {
      if ( *p == '%' )
      {
         switch ( *++p )
         {
            case '\0':
               wxLogWarning(_("Unexpected end of string in the status format "
                              "string '%s'."), start);
               p--; // avoid going beyond the end of string
               break;

            case 'f':
               result += name;
               break;

            case 't':
               result += wxString::Format("%lu", total);
               break;

            case 'r':
               result += wxString::Format("%lu", recent);
               break;

            case 'n':
               result += wxString::Format("%lu", newmsgs);
               break;

            case '%':
               result += '%';
               break;

            default:
               wxLogWarning(_("Unknown macro '%c%c' in the status format "
                              "string '%s'."), *(p-1), *p, start);
         }
      }
      else // not a format spec
      {
         result += *p;
      }
   }

   return result;
}

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

   // we could probably optimize this somewhat by only counting the messages if
   // we need them, but is it worth it?
   String folderName = mailFolder->GetName();
   unsigned long total = mailFolder->CountMessages(),
      recent =  mailFolder->CountMessages(MailFolder::MSG_STAT_RECENT |
                                          MailFolder::MSG_STAT_SEEN,
                                          MailFolder::MSG_STAT_RECENT |
                                          MailFolder::MSG_STAT_SEEN),
      // recent & !seen --> new
      newmsgs = mailFolder->CountMessages(MailFolder::MSG_STAT_RECENT |
                                          MailFolder::MSG_STAT_SEEN,
                                          MailFolder::MSG_STAT_RECENT);

   // contruct the messages
   wxString titleMsg(title), statusMsg(status);
   statusMsg += ParseStatusFormat(READ_APPCONFIG(MP_FOLDERSTATUS_STATBAR),
                                  total, recent, newmsgs, folderName);
   titleMsg += ParseStatusFormat(READ_APPCONFIG(MP_FOLDERSTATUS_TITLEBAR),
                                 total, recent, newmsgs, folderName);

   // show them
   frame->SetStatusText(statusMsg);
   frame->SetTitle(titleMsg);

   if ( newmsgs > 0 )
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON("MainFrameNewMail") : ICON("MFrameNewMail"));
   else
      frame->SetIcon( frame == mApplication->TopLevelFrame() ?
                      ICON("MainFrame") : ICON("MFrame"));
}

// ---------------------------------------------------------------------------
// colour to string conversion
// ---------------------------------------------------------------------------

static const char *rgbSpecificationString = gettext_noop("RGB(%d, %d, %d)");

bool ParseColourString(const String& name, wxColour* colour)
{
   wxString customColourString(wxGetTranslation(rgbSpecificationString));

   // first check if it's a RGB specification
   int red, green, blue;
   if ( sscanf(name, customColourString, &red, &green, &blue) == 3 )
   {
      // it's a custom colour
      if ( colour )
         colour->Set(red, green, blue);
   }
   else // a colour name
   {
      wxColour *col = wxTheColourDatabase->FindColour(name);
      if ( !col )
         return FALSE;

      if ( colour )
         *colour = *col;
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



String GetSequenceString(const UIdArray *array)
{
   String sequence;
   for(size_t i = 0; i < array->Count(); i++)
   {
      sequence += strutil_ultoa((*array)[i]);
      sequence += ',';
   }
   size_t len = sequence.length();
   if ( len > 0 )
      sequence = sequence.substr(0, len-1); //strip off comma

   return sequence;
}

UIdArray *GetAllMessagesSequence(ASMailFolder *mf)
{
   ASSERT(mf);
   UIdArray *sequence = new UIdArray;
   HeaderInfoList *hil = mf->GetHeaders();
   if(hil)
   {
      for(size_t i = 0; i < hil->Count(); i++)
         sequence->Add((*hil)[i]->GetUId());
      hil->DecRef();
   }
   return sequence;
}




/// Get the user's From mail address from a profile
extern String miscutil_GetFromAddress(Profile *p,
                                      String *pers,
                                      String *email)
{
   ASSERT(p);
   String personal = READ_CONFIG(p, MP_PERSONALNAME);
   String mbox = READ_CONFIG(p, MP_FROM_ADDRESS);

   strutil_delwhitespace(mbox);

   if(pers) *pers = personal;
   if(email) *email = mbox;
   return strutil_makeMailAddress(personal,mbox);
}

/// Get the user's reply address from a profile
extern String miscutil_GetReplyAddress(Profile *p)
                                       //String *pers,
                                       //String *email)
{
   ASSERT(p);
   String replyTo = READ_CONFIG(p, MP_REPLY_ADDRESS);
   strutil_delwhitespace(replyTo);
   return replyTo;
}

extern String miscutil_GetDefaultHost(Profile *p)
{
   if( READ_CONFIG(p,   MP_ADD_DEFAULT_HOSTNAME ) == 0)
      return "";
   String hostname = READ_CONFIG(p, MP_HOSTNAME);
   if(hostname.Length() == 0)
   {
      // get it from (reply-to) From address:
      (void) miscutil_GetFromAddress(p, NULL, &hostname);
      hostname = hostname.AfterLast('@');
   }
   return hostname;
}

/// Expand a local (user-only) mail address
extern String miscutil_ExpandLocalAddress(Profile *p, const String &mbox)
{
   if( READ_CONFIG(p,   MP_ADD_DEFAULT_HOSTNAME ) == 0)
      return mbox; // nothing to do

   String email = Message::GetEMailFromAddress(mbox);
   if(mbox.Find('@') != wxNOT_FOUND)
      return mbox; // contains a host name

   String mbox2 = mbox;

   String personal = Message::GetNameFromAddress(mbox2);


   mbox2 << '@' << miscutil_GetDefaultHost(p);
   return strutil_makeMailAddress(personal, mbox2);
}
