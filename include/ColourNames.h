///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   ColourNames.h
// Purpose:     utility functions for working with colour names
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.03.04 (extracted from defunct miscutil.h)
// CVS-ID:      $Id$
// Copyright:   (c) 1999-2004 M-Team
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_COLOURNAMES_H_
#define _M_COLOURNAMES_H_

class WXDLLIMPEXP_FWD_CORE wxColour;
class MOption;
class Profile;

/// get the colour by name which may be either a colour or RGB specification
extern bool ParseColourString(const String& name, wxColour* colour = NULL);

/// get the colour name - pass it to ParseColorString to get the same colour
extern String GetColourName(const wxColour& color);

/// get the colour by name and fallback to default (then false is returned)
extern bool GetColourByName(wxColour *colour,
                            const String& name,
                            const String& defaultName);
/**
   Read a colour from the profile.

   This function reads the value of the specified option from the given
   profile and tries to convert it to a colour. If it fails, it falls back to
   the default option value.

   @param col the colour to initialize
   @param profile the profile to read from (must not be NULL)
   @param opt the key to read from
 */
extern void ReadColour(wxColour *col, Profile *profile, const MOption& opt);

#endif // _M_COLOURNAMES_H_
