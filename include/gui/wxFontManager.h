/*-*- c++ -*-********************************************************
 * wxFontManager - allocating and deallocating fonts for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/05/01 14:02:41  KB
 * Ran sources through conversion script to enforce #if/#ifdef and space/TAB
 * conventions.
 *
 * Revision 1.2  1998/03/26 23:05:38  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdefndef WXFONTMANAGER_H
#define WXFONTMANAGER_H

#ifdefdef __GNUG__
#pragma interface "wxFontManager.h"
#endif

#ifdef         !USE_PCH
  #define	Uses_wxFont
  #include	<wx/wx.h>

  #include	<Mcommon.h>

  #include	<list>
#endif

#define	WXFM_DEFAULT_SIZE	12
#define	WXFM_DEFAULT_FAMILY	wxROMAN
#define	WXFM_DEFAULT_STYLE	wxNORMAL
#define	WXFM_DEFAULT_WEIGHT	wxNORMAL
#define	WXFM_DEFAULT_UNDERLINE	FALSE

/**
   FontManager class, this class allocates and deallocates fonts for
   the application, freeing the individual gui object from the need to
   allocate or deallocate them.
*/

/** A structure holding information about the fonts.
   This is the element of the list.
*/
struct	FontData
{
  int	size;
  int	family;
  int	style;
  int	weight;
  Bool	underline;
  wxFont	*fontPtr;

  IMPLEMENT_DUMMY_COMPARE_OPERATORS(FontData)
};

/** A list of all known fonts.
   @see FontData
*/
typedef std::list<FontData> FontDataList;

class wxFontManager
{
private:
   /** A list of all known fonts.
      @see FontData
   */
   FontDataList *fontList;
   
   /// An Font to return for unknown lookup strings.
   wxFont	*unknownFont;
public:
   /** Constructor
   */
   wxFontManager();

   /** Destructor
       writes back all entries
   */
   ~wxFontManager();

   /** Get an Font.
       If the fontName is not known, a default font will be returned.
       All parameters are like in wxFont() constructor.
       @return the wxFont
   */
   wxFont *GetFont(int size = WXFM_DEFAULT_SIZE,
		   int family = WXFM_DEFAULT_FAMILY,
		   int style = WXFM_DEFAULT_STYLE,
		   int weight = WXFM_DEFAULT_WEIGHT,
		   Bool underline = WXFM_DEFAULT_UNDERLINE);
};

#endif
