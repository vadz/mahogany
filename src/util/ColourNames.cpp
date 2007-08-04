///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   util/ColourNames.cpp
// Purpose:     implementation of functions from ColourNames.h
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.03.04 (extracted from defunct miscutil.cpp)
// CVS-ID:      $Id$
// Copyright:   (c) 1999-2004 M-Team
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#include "Mpch.h"

#ifndef   USE_PCH
#  include "Mcommon.h"
#  include "Mdefaults.h"

#  include <wx/wxchar.h>               // for wxPrintf/Scanf
#  include "wx/colour.h"
#  include "wx/gdicmn.h"
#endif // USE_PCH

#include "ColourNames.h"

// ---------------------------------------------------------------------------
// colour to string and vice versa conversions
// ---------------------------------------------------------------------------

static const wxChar *rgbSpecificationString = gettext_noop("RGB(%d, %d, %d)");

bool ParseColourString(const String& name, wxColour* colour)
{
   if ( name.empty() )
      return false;

   wxString customColourString(wxGetTranslation(rgbSpecificationString));

   // first check if it's a RGB specification
   int red, green, blue;
   if ( wxSscanf(name.wx_str(), customColourString, &red, &green, &blue) == 3 )
   {
      // it's a custom colour
      if ( colour )
         colour->Set(red, green, blue);
   }
   else // a colour name
   {
      wxColour col = wxTheColourDatabase->Find(name);
      if ( !col.Ok() )
         return false;

      if ( colour )
         *colour = col;
   }

   return true;
}

// TODO: use wxTo/FromString(wxColour) now that we have it
String GetColourName(const wxColour& colour)
{
   wxString colName(wxTheColourDatabase->FindName(colour));
   if ( !colName )
   {
      // no name for this colour
      colName.Printf(wxGetTranslation(rgbSpecificationString),
                     (int)colour.Red(),
                     (int)colour.Green(),
                     (int)colour.Blue());
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
bool GetColourByName(wxColour *colour,
                     const String& name,
                     const String& def)
{
   if ( !ParseColourString(name, colour) )
   {
      *colour = def;
      return false;
   }

   return true;
}

void ReadColour(wxColour *col, Profile *profile, const MOption& opt)
{
   CHECK_RET( profile, _T("NULL profile in ReadColour()") );

   const String value = GetOptionValue(profile, opt).GetTextValue();

   if ( !GetColourByName(col, value, GetStringDefault(opt)) )
   {
      wxLogError(_("Cannot find a colour named \"%s\", using default instead "
                   "(please check the value of option \"%s\")"),
                 value.c_str(),
                 GetOptionName(opt));
   }
}
