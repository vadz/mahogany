/*-*- c++ -*-********************************************************
 * miscutil.h : miscellaneous utility functions                     *
 *                                                                  *
 * (C) 1999-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/


#ifndef MISCUTIL_H
#define MISCUTIL_H

#ifndef  USE_PCH
#endif

class WXDLLEXPORT wxArrayString;
class ASMailFolder;
class UIdArray;

/**@name Miscellaneous utility functions */
//@{

class wxFrame;
class MailFolder;

/// set the title and statusbar to show the number of messages in folder
extern void UpdateTitleAndStatusBars(const String& title,
                                     const String& status,
                                     wxFrame *frame,
                                     const MailFolder *folder);

class wxColour;

/// get the colour by name which may be either a colour or RGB specification
extern bool ParseColourString(const String& name, wxColour* colour = NULL);

/// get the colour name - pass it to ParseColorString to get the same colour
extern String GetColourName(const wxColour& color);

/// get the colour by name and fallback to default (warning the user) if failed
extern void GetColourByName(wxColour *colour,
                            const String& name,
                            const String& defaultName);


//@}

#endif // MISCUTIL_H

