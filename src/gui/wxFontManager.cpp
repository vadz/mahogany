/*-*- c++ -*-********************************************************
 * wxFontManager - allocating and deallocating fonts for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxFontManager.h"
#endif

#include  "Mpch.h"
#include	"Mcommon.h"

#include	"gui/wxFontManager.h"

/**
   FontManager class, this class allocates and deallocates fonts for
   the application, freeing the individual gui object from the need to
   allocate or deallocate them.
*/


wxFontManager::wxFontManager()
{
   fontList = GLOBAL_NEW std::list<FontData>;
}


wxFontManager::~wxFontManager()
{
   std::list<FontData>::iterator i;
   for(i = fontList->begin(); i != fontList->end(); i++)
      GLOBAL_DELETE (*i).fontPtr;
   GLOBAL_DELETE fontList;
}

wxFont *
wxFontManager::GetFont(int size, int family, int style, int weight,
		       Bool underline)
{
   std::list<FontData>::iterator i;
   
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
   newFont.fontPtr = GLOBAL_NEW wxFont(size,family,style,weight,underline != 0);
   fontList->push_front(newFont);
   return newFont.fontPtr;
}

