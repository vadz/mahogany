/*-*- c++ -*-********************************************************
 * wxFontManager - allocating and deallocating fonts for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxFontManager.h"
#endif

#include	<wxFontManager.h>

/**
   FontManager class, this class allocates and deallocates fonts for
   the application, freeing the individual gui object from the need to
   allocate or deallocate them.
*/


wxFontManager::wxFontManager()
{
   fontList = NEW list<FontData>;
}


wxFontManager::~wxFontManager()
{
   list<FontData>::iterator i;
   for(i = fontList->begin(); i != fontList->end(); i++)
      DELETE (*i).fontPtr;
   DELETE fontList;
}

wxFont *
wxFontManager::GetFont(int size, int family, int style, int weight,
		       Bool underline)
{
   list<FontData>::iterator i;
   
   FontData    	*fd;

   for(i = fontList->begin(); i != fontList->end(); i++)
   {
      fd = &(*i);
      if(fd->size == size &&
	 fd->family == family &&
	 fd->weight == weight &&
	 fd->style == style &&
	 fd->underline == underline)
	 return fd->fontPtr;
   }

   // not found, create:
   FontData	newFont;
   newFont.size = size;
   newFont.family = family;
   newFont.style = style;
   newFont.weight = weight;
   newFont.underline = underline;
   newFont.fontPtr = NEW wxFont(size,family,style,weight,underline);
   fontList->push_front(newFont);
   return newFont.fontPtr;
}

