///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   guidef.h
// Purpose:     miscellaneous GUI helpers
// Author:      Karsten Ballüder, Vadim Zeitlin
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2006 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef M_GUIDEF_H_
#define M_GUIDEF_H_

#ifndef USE_PCH
#  include <wx/frame.h>
#endif // USE_PCH

class WXDLLEXPORT wxClassInfo;

/// how much space to leave in frame around other items
#define   WXFRAME_WIDTH_DELTA   16
/// how much space to leave in frame around other items
#define   WXFRAME_HEIGHT_DELTA   64

// minimal space to leave between dialog/panel items: tune them if you want,
// but use these constants everywhere instead of raw numbers to make the look
// of all dialogs consistent
#define LAYOUT_MARGIN       5
#define LAYOUT_X_MARGIN       LAYOUT_MARGIN
#define LAYOUT_Y_MARGIN       LAYOUT_MARGIN

// this function is obsolete
inline long AdjustCharHeight(long h)
{
   return h;
}

// calculate the "optimal" text height for given label height
#define TEXT_HEIGHT_FROM_LABEL(h)      (23*(h)/13)

// to get nice looking buttons width should be proportional to the height with
// this coefficient (it is not my personal taste, but rather the coefficient
// is the same as for the standard buttons under Windows)
#define BUTTON_WIDTH_FROM_HEIGHT(w)    (75*(w)/23)

// this functions go up the window inheritance chain until it finds a window of
// the given class or a window without parent (in which case the function will
// return NULL). This function is mainly for the use with the macro below.
extern wxWindow *GetParentOfClass(const wxWindow *win, wxClassInfo *classinfo);

// get the pointer to the parent window of given type (may be NULL), very
// useful to find, for example, the parent frame for any control in the frame.
#define GET_PARENT_OF_CLASS(win, classname)  \
   ((classname *)(GetParentOfClass(win, CLASSINFO(classname))))

// get the pointer to the frame we belong to (returns NULL if none)
inline wxFrame *GetFrame(const wxWindow *win)
{
   return  GET_PARENT_OF_CLASS(win, wxFrame);
}

#if !wxUSE_UNICODE

/**
  Check if the given font encoding is available on this system. If not, try to
  find a replacement encoding - if this succeeds, the text is translated into
  this encoding and the encoding parameter is modified in place.

  @param encoding the encoding to check, may be modified
  @param text the text we want to show in this encoding, may be translated
  @return true if this or equivalent encoding is available, false otherwise
 */
extern bool EnsureAvailableTextEncoding(wxFontEncoding *encoding,
                                        wxString *text = NULL,
                                        bool mayAskUser = false);

#endif // !wxUSE_UNICODE

/**
   Create a font from the given native font description or font family and
   size.

   Notice that if neither of the parameters is specified (description is empty
   and the other ones are -1), an invalid font is returned and the caller
   should check for it and avoid using it in this case to avoid overriding the
   default system font if the user didn't set any specific font to use.

   @param fontDesc opaque string returned by wxFont::GetNativeFontInfoDesc()
   @param fontSize font size, -1 if not specified
   @param fontFamily font family, wxFONTFAMILY_DEFAULT if not specified
   @return font which may be invalid, to be checked by the caller
 */
extern wxFont
CreateFontFromDesc(const String& fontDesc, int fontSize, int fontFamily);


// Prevent MEvent dispatch inside wxYield
extern int g_busyCursorYield;

extern void MBeginBusyCursor();
extern void MEndBusyCursor();

class MBusyCursor
{
public:
   MBusyCursor() { MBeginBusyCursor(); }
   ~MBusyCursor() { MEndBusyCursor(); }
};

#endif // M_GUIDEF_H_
