/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for editing formatted text		    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.2  1998/03/22 20:41:28  KB
 * included profile setting for fonts etc,
 * made XFaces work, started adding support for highlighted URLs
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXFTCANVAS_H
#define WXFTCANVAS_H

#ifdef __GNUG__
#pragma interface "wxFTCanvas.h"
#endif

class wxFTOList;

/**
   This class provides a high level abstraction to the wxFText
   classes.
   It handles most of the character events with its own callback
   functions, providing an editing ability. All events which cannot be
   handled get passed to the parent window's handlers.
*/
class wxFTCanvas : public wxCanvas
{
   DECLARE_CLASS(wxFTCanvas)
      
   /// the list of FTObject's
   wxFTOList 		* ftoList;
   /// the margin at which to wrap
   int	wrapMargin;
public:
   /** Constructor.
       @param iparent	parent panel to be placed in
       @param ix	x position
       @param iy	y position
       @param iwidth	width
       @param iheight	height
       @param style	style
   */
   wxFTCanvas(wxPanel *iparent, int ix = -1, int iy = -1,
	      int iwidth = -1, int iheight = -1, long style = 0,
	      ProfileBase *profile = NULL);
   /// Destructor.
   ~wxFTCanvas();

   /** Enable/Disable editing.
       @param enable if true, editing is enabled
   */
   void	AllowEditing(bool enable = true);

   /** Extract information from canvas.
       @param ftoType A pointer to an object type variable to hold the
       type of value returned. Set to LI_ILLEGAL when end of list is reached.
       @param restart Has to be set to true to start with first element.
       @return String holding the data of the object.
   */
   String const * GetContent(FTObjectType *ftoType, bool restart = false);
   
   /// Redraw callback.
   void OnPaint(void);
   /// Mouse event handler.
   void OnEvent(wxMouseEvent &event);
   /// Character Event handler.
   void OnChar(wxKeyEvent &event);
   /// Insert text with formatting tags.
   void AddFormattedText(String const &str);
   /// add formatted/non formatted text
   void AddText(String const &txt, bool formatFlag = false)
      { ftoList->AddText(txt, formatFlag); }

   /** Insert Text.
       @param str the text to insert
       @param format if true, interpret text
   */
   void InsertText(String const &str, bool format = false);
   /// Make printout of contents.
   void Print(void);
   /// Move Cursor
   void MoveCursor(int dx, int dy = 0);
   /// Move Cursor to position x,y
   void MoveCursorTo(int x, int y);
   /// Sets wrap margin, set it to -1 to disable auto wrapping
   void SetWrapMargin(int n) { wrapMargin = n; }
   /// add an icon to the iconmanager
   void AddIcon(String const &iconName, IconResourceType data)
      { ftoList->AddIcon(iconName, data); }

};

#endif

