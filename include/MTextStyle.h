///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MTextStyle.h: declaration of MTextStyle utility class
// Purpose:     MTextStyle is used by MessageViewer to describe the text style
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.11.02 (extracted from MessageViewer.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MTEXTSTYLE_H_
#define _MTEXTSTYLE_H_

#ifndef   USE_PCH
#  include <wx/textctrl.h>
#endif // USE_PCH

// use the standard wxWin class: even if it is not really intended for this, it
// just what we need here as it combines text colours and font info
class MTextStyle : public wxTextAttr
{
public:
    MTextStyle() { }
    MTextStyle(const wxColour& colText,
               const wxColour& colBack = wxNullColour,
               const wxFont& font = wxNullFont)
        : wxTextAttr(colText, colBack, font) { }
};

#endif // _MTEXTSTYLE_H_

