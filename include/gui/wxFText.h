/*-*- c++ -*-********************************************************
 * wxFText.h: a set of classes representing formatted text          *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                *
 *******************************************************************/
#ifndef	WXFTEXT_H
#define WXFTEXT_H


#ifdef __GNUG__
#pragma interface "wxFText.h"
#endif

#ifndef USE_PCH

using namespace std;

#	include	<map>
#	include	<list>

#	include	<Profile.h>
#	include	<wxFontManager.h>
#	include	<wxIconManager.h>
#endif

/// describing a FTObject's type
enum FTObjectType { LI_TEXT, LI_COMMAND, LI_NEWLINE, LI_ICON, LI_URL, LI_ILLEGAL };

class wxFTOList;
class FTObject;

/// For scrolling on canvases: number of scroll steps per scroll page.
#define	WXFTEXT_SCROLLSTEPS_PER_PAGE	20

/**
      FTObject holds information about the current line
      */
   
class FTObject
{
   /// type of object
   FTObjectType	type;

   /**@name cursor positions covered by this line */
   //@{
   /// y cursor position
   int	cursorY;
   /// x cursor position
   int	cursorX;
   /// maximum x position covered
   int	cursorX2;
   //@}
   /**@name canvas positions covered by this line */
   //@{
   /// the y position of the line
   coord_t	posY;
   /// the correction for font height adjustments
   coord_t	posYdelta;
   /// the starting x position
   coord_t	posX;
   /// the maximium y position
   coord_t	posY2;
   /// the maximum x position
   coord_t	posX2;
   /// the total height of the line
   coord_t	height;
   /// the total width of the line
   coord_t	width;
   //@}

   /// the contents of this line
   String	text;

   /// if it is an icon, pointer to it
   wxIcon	*icon;

   /// if it is text, the font to use
   wxFont	*font;
   
   /// does the line need to be recalculated?
   Bool	isOk;
   /// does it contain formatting info?
   Bool	needsFormat;

   /**@name delimiters for commands */
   //@{
   /// beginning of command
   char delimiter1;
   /// end of command
   char delimiter2;
   //@}

   friend wxFTOList;
public:
   /// creates an empty object
   FTObject();

   /// creates an empty object
   void Create(void);

   /**
      Creates a FTObject by parsing the string.
       @param str the string describing the object, e.g. the text
       @param cx cursor x coordinate, gets updated
       @param cy cursor y coordinate, gets updated
       @param x x-coordinate for this object, gets set to the next object's coordinate
       @param y y-coordinate for this object, gets set to the next object's coordinate
       @param formatFlag if true, interpret formatting commands
       @param dc pointer to wxDC for calculating size
       @return the object's type
   */
   FTObjectType Create(String & str, int &cx, int &cy, coord_t &x, coord_t
		       &y, bool formatFlag = true, wxDC *dc = NULL);

   /**
      Creates a FTObject by parsing the string.
      For a description, see Create() method.
      @see
   */
   
   FTObject(String & str, int &cx, int &cy, coord_t &x, coord_t &y,
	    bool formatFlag = true, wxDC *dc = NULL);

   /// Destructor.
   ~FTObject();
   
   /**
      Updates the coordinate information of Object.
      @param dc the DC to use for calculating size
      @param x the next object's x coordinate
      @param y the next object's y coordinate
      @param ftc the wxFTOList to process commands
   */
   void Update(wxDC *dc, coord_t &x, coord_t &y, wxFTOList *ftc = NULL);
   
   /**
      Is object fully formatted?
      @return true if ready to print
   */
   Bool	IsOk(void) const { return isOk; }

   /**
      For string objects: set the font to use.
      @param ifont a wxFont to use
   */
   void	SetFont(wxFont *ifont)
      { font = ifont; }

#ifndef NDEBUG
   /// print debugging information
   void	Debug(void) const;
#endif
   
   /**
      Draws the object on a wxFTOList
      @param ftc the wxFTOList
      @param pageing output to printer/PS?
      @param pagecount internal parameter
   */
   void Draw(wxFTOList & ftc, bool pageing = false, int
	     *pagecount = NULL) const;

   /**
      Set delimiters for marking commands.
      @param del1 to mark beginning of command
      @param del2 to mark end of command
   */
   void SetDelimiters(char del1 = '<', char del2 = '>')
      { delimiter1 = del1; delimiter2 = del2; }

   /**
      Return the type of the object.
      @return the object's type
   */
   FTObjectType GetType(void) const { return type; }

   /**
      Sets the height value.
      @param h new height
      @param setDelta if TRUE, recalculate offset for baseline
   */
   void  SetHeight(coord_t h, Bool setDelta = FALSE);

   /**
      Gets the height value.
      @return new height in DC units
   */
   coord_t GetHeight(void) const { return height; }
   
   /**
      Sets the y position on DC.
      @param y new position on DC
   */
   void  SetYPos(coord_t y) { posY = y; posY2 = y+height; }

   /**
      Sets the adjustment for y position on DC.
      @param delta new adjustment
   */
   void	SetYPosDelta(coord_t delta) { posYdelta = delta; }
   
   /**
      Sets the x position on DC.
      @param x new position on DC
   */
   void  SetXPos(coord_t x) { posX = x; posX2 = x+width; }

   /**
      Gets the width of object on DC.
      @return the width
   */
   coord_t GetWidth(void) const { return width; }

   /** Gets the position of object on DC.
       @return the y position
   */
   coord_t  GetYPos(void) const { return posY; }

   /** Gets the position of object on DC.
       @return the x position
   */
   coord_t  GetXPos(void) const { return posX; }
   coord_t  GetX2Pos(void) const { return posX2; }

   /** Gets the text of the object.
       @return the string
   */
   const String & GetText(void) const { return text; }
   
   const  char *c_str(void) const { return text.c_str(); }
   coord_t  GetNextPosition(void) const { return posY + height; }
   void   SetText(String const &in) { text = in; }

   void	InsertText(String const &str, int offset = 0);
   void	DeleteChar(int offset = 0);

   IMPLEMENT_DUMMY_COMPARE_OPERATORS(FTObject)
};


/**
  * A wxWindows Canvas for printing formatted text
  */

class wxFTOList

{
   /**
      DrawInfo structure holds all the information on how to draw text
   */
   class DrawInfo
   {
   private:
      /// a wxFontManager
      wxFontManager	fontManager;
      /// the default fontsize
      int	fontDefaultSize;
      /// the fontsize
      int	fontSize;
      /// the font family
      int	fontFamily;
      /// the font style
      int	fontStyle;
      /// the font weight
      int	fontWeight;
      /// is it underlined?
      Bool	fontUnderline;
      /// descent of currently selected font
      coord_t	fontDescent;
      /// height of a text line in currently selected font
      coord_t	textHeight;
      
      std::list<int> familyStack;
      std::list<int> styleStack;
      std::list<int> weightStack;
      std::list<Bool> underlineStack;
      std::list<int> sizeStack;

      /// the current font:
      wxFont	*font;

      /// no copy constructor
//      DrawInfo & operator=(DrawInfo const & in) {}
   public:
      /**
	 constructor
	 @param size	pixel size to use
	 @param family	font family
	 @param	style	font style
	 @param	weight	font weight
	 @param underlined TRUE if text is to be underlined
      */	
      DrawInfo(int size = 14, int family =
	       wxROMAN, int style = wxNORMAL, int weight = wxNORMAL,
	       Bool underlined = FALSE);

      /// destructor, freeing resources
      ~DrawInfo();
      
      /// apply settings of this DrawInfo to DC
      wxFont *Apply(wxDC *dc);

      /** Return the current font.
	  @return a wxFont pointer
      */
      wxFont	*GetFont(void) { return font; }
      
      /// set font family
      void	FontFamily(int family, Bool enable = TRUE);
      /// set font style
      void	FontStyle(int style, Bool enable = TRUE);
      /// set font weight
      void	FontWeight(int weight, Bool enable = TRUE);
      /// set font underline
      void	Underline(Bool enable = TRUE);
      /// set font size ion pixels
      void	FontSize(int size, Bool enable = TRUE);
      /// allocate a font with the current settings
      wxFont *	SetFont(wxDC *dc);
      /** change font size in steps, starting from default size
	  @param delta a parameter +/- n
	  @return the new pixel size
      */
      int	ChangeSize(int delta);

      /// return the text height in currently selected font
      coord_t	TextHeight(void) { return textHeight; }
      /// return the descent of currently selected font
      coord_t	Descent(void) { return fontDescent; }
      
      void	Roman(Bool enable = TRUE) 	{ FontFamily(wxROMAN, enable); }
      void	Modern(Bool enable = TRUE) 	{ FontFamily(wxMODERN, enable); }
      void 	Typewriter(Bool enable = TRUE){ FontFamily(wxTELETYPE, enable); }
      void	SansSerif(Bool enable = TRUE) { FontFamily(wxSWISS, enable); }
      void	Script(Bool enable = TRUE) 	{ FontFamily(wxSCRIPT, enable); }
      void	Decorative(Bool enable = TRUE){ FontFamily(wxDECORATIVE, enable); }
      void 	Italics(Bool enable = TRUE)	{ FontStyle(wxITALIC, enable); }
      void	NormalStyle(Bool enable = TRUE) { FontStyle(wxNORMAL, enable); }	
      void	Slanted(Bool enable = TRUE)	{ FontStyle(wxSLANT, enable); }
      void	Bold(Bool enable = TRUE) 	{ FontWeight(wxBOLD, enable); }
      void	Light(Bool enable = TRUE) 	{ FontWeight(wxLIGHT, enable); }
      void	NormalWeight(Bool enable = TRUE) { FontStyle(wxNORMAL, enable); }
   };



   
   //---------------------------- wxFTOList: ------------------------------------




   /// cursor positions
   int		cursorX, cursorY;
   /// maximum allowed cursor position
   int 		cursorMaxX, cursorMaxY;
   
   /// position in DC
   coord_t	dcX, dcY;
   /// the current settings
   DrawInfo	drawInfo;
   /// the DC
   wxDC	*dc;
   
   /**@name A list of all text lines. */
   //@{
   /// The type of the list. 
   typedef std::list<FTObject>	FTOListType;
   /// The list itself:
   FTOListType *listOfLines;
   //@}
   /// a list of all text line lengths, indexed by cursor position
   std::map<int,int>	listOfLengths;
   
   /// a list of all clickable items
   std::list<FTObject const *> *listOfClickables;
   
   /// a wxIconManager
   wxIconManager	iconManager;
      
   /// shall we look for formatting commands
   Bool		formatFlag;

   /// Do we need to split the output into pages?
   bool		pageingFlag;

   /// Is List editable?
   bool		editable;

   /// Do we need to recalculate sizes?
   bool		contentChanged;
   
   /// no copy constructor
//   wxFTOList &operator=(wxFTOList const &in) { }

   /// remember the last line at which redraw started
   std::list<FTObject>::iterator	lastLineFound;

   /// the last drawn cursor
   struct
   {
      coord_t x;
      coord_t y1;
      coord_t y2;
   } lastCursor;
	 
   /// iterator  for content extraction:
   FTOListType::iterator	extractIterator;
   /// string holding the value to be returned in GetContent()
   String	extractContent;
   /**@name Information needed for scrolling on canvases. */
   //@{
   /// If a canvas is given update its scrollbars, otherwise this is NULL.
   wxCanvas	*canvas;
   /// Pixels per scroll step, x direction.
   int	scrollPixelsX;
   /// Pixels per scroll step, y direction.
   int	scrollPixelsY;
   /// Length of scrollbar in steps, x direction.
   int	scrollLengthX;
   /// Length of scrollbar in steps, y direction.
   int	scrollLengthY;
   //@}
   /// Bad, but useful.
   friend FTObject;
public:
   /// Constructor
   wxFTOList(wxDC *dc = NULL, ProfileBase *profile = NULL);
   wxFTOList(wxCanvas *icanvas, ProfileBase *profile = NULL);
   void DrawText(const char *text, int x, int y);

   /**@name Adding strings to the list. */
   //@{
   /** Adds text to the canvas.
       @param line the text to add
       @param formatFlag if true, interpret formatting tags
   */
   void	AddText(String const &line, bool formatFlag = false);
   /** Adds text to the canvas and interpret formatting tags.
       @param line the text to add
   */
   void	AddFormattedText(String const &line) { AddText(line, true); }

   //@}
   /**
      Processes commands by evaluating the command string.
      May change the FTObject's type!
      @param command the text between the delimiters
      @param fto pointer to the FTObject
   */
   void	ProcessCommand(String const &command, FTObject *fto = NULL);

   /** Return the current font from DrawInfo.
       @return a wxFont pointer
   */
   wxFont	*GetFont(void) { return drawInfo.GetFont(); }
   
   /** Sets the DC to use for calculations and drawing.
       @param idc the wxDC to use
       @param prFlag set it to true when printing to a printer/PS
   */
   void	SetDC(wxDC *idc, bool prFlag = false);

   /** Sets the Canvas and DC to use for calculations and drawing.
       The Canvas' scrollbars will be updated.
       @param idc the wxDC to use
   */
   void	SetCanvas(wxCanvas *ic);
   
   /** Extract information from canvas.
       @param ftoType A pointer to an object type variable to hold the
       type of value returned. Set to LI_ILLEGAL when end of list is reached.
       @param restart Has to be set to true to start with first element.
       @return String holding the data of the object.
   */
   String const *GetContent(FTObjectType *ftoType, bool restart = false);
   
   /// Destroys the list, leaves an empty one.
   void Clear(void);

   /** Draws/redraws the list of strings on dc. */
   void Draw(void);

   /** Adds an object to the list of clickables
       @param obj the object
   */
   void AddClickable(FTObject const * obj);
   /** Finds a clickable object.
       @param x the x coordinate of mouse click
       @param y the y coordinate of mouse click
       @return a pointer to the object or NULL if not found
   */
   FTObject const *FindClickable(coord_t x, coord_t y) const;
   
#ifndef NDEBUG
   void	Debug(void) const;
#endif
   
   /** Recalculate the dimensions of all objects. Needs to be called
       once before redrawing, after all text has been added.
       @param maxWidth if not NULL, the maximum width gets stored here
       @param maxHeight if not NULL, the maximum height gets stored here
   */
   void ReCalculateLines(coord_t *maxWidth = NULL, coord_t *maxHeight = NULL);

   /** Toggle editable status.
       @param enable true to activate editing
   */
   void SetEditable(bool enable = true);

   /** Return editable status.
       @return true if editing is enabled.
   */
   inline bool IsEditable(void) { return editable; }

   /** Insert a string.
       @param str the string to insert
       @param format if true, interpret formatting info
   */
   void InsertText(String const &str, bool format = false);

   /** Moves the cursor. Should only be called with either deltaX OR
       deltaY != 0.
       @param deltaX how far to move in columns
       @param deltaY how far to move in rows
       @return false if cursor could not be moved this far
   */
   bool MoveCursor(int deltaX, int deltaY = 0);
      /// Move Cursor to position x,y
   void MoveCursorTo(int x, int y);
   /// Moves the cursor to the end of the line.
   void	MoveCursorEndOfLine(void);
   /// Moves the cursor to the begin of the line.
   void MoveCursorBeginOfLine(void);
   /** Deletes one character to the right of the cursor.
       @param toEndOfLine if true, deletes to end of line
   */
   void Delete(bool toEndOfLine = false);
   /// Draws the cursor at the new position and erases the old one.
   void DrawCursor(void);
   /// If drawing on a canvas, makes sure that cursor is visible.
   void	ScrollToCursor(void);
   /// Check whether we need to wrap the text.
   void	Wrap(int margin);
   
   /**@name Methods for manipulating the list of objects. */
   //@{
   /** Find the object belonging to cursor position.
       @param xoffset Pointer to an integer variable. This will be set
       to the cursor position's offset relative to the beginning of a
       text object.
       @return An iterator to the object found, or listOfLines->end().
   */
   FTOListType::iterator FindCurrentObject(int *xoffset = NULL);

   /** Find the object belonging to the beginning of the line with the
       current cursor position.
       @param i0 An iterator from where to search, or end() to search
       from cursor position.
       @return An iterator to the first object in this line,
       or listOfLines->begin().
   */
   FTOListType::iterator FindBeginOfLine(FTOListType::iterator i0);

   /** Find the last object in the line with the currrent cursor position.
       @param xoffset Pointer to an integer variable. This will be set
       to the cursor position's offset relative to the beginning of a
       text object.
       @return An iterator to the object found, or listOfLines->end().
   */
   FTOListType::iterator FindEndOfLine(void);

   /** Simplify the line by merging text objects.
       @param i Iterator of any object within this line.
       @param toEnd If true, go on simplifying until end of list.
       @return Iterator to the first object of the next line, or listOfLines->end().
   */
   FTOListType::iterator SimplifyLine(FTOListType::iterator i, bool toEnd =
				false);
   
   /** Recalculate the line by merging text objects.
       @param i Iterator of any object within this line.
       @param toEnd If true, go on simplifying until end of list.
       @return Iterator to the first object of the next line, or listOfLines->end().
   */
   FTOListType::iterator ReCalculateLine(FTOListType::iterator i, bool toEnde = false);

   /// add an icon to the iconmanager
   void AddIcon(String const &iconName, IconResourceType data)
      { iconManager.AddIcon(iconName, data); }

//@}
/// Destructor
   ~wxFTOList();
   
};

#endif

