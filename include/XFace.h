/*-*- c++ -*-********************************************************
 * XFace.h -  a class encapsulating XFace handling                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/03/26 23:05:37  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.2  1998/03/22 20:41:16  KB
 * included profile setting for fonts etc,
 * made XFaces work, started adding support for highlighted URLs
 *
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef XFACE_H
#define XFACE_H

#ifdef __GNUG__
#pragma interface "XFace.h"
#endif

/**
   XFace class for handling XFaces.
*/
class XFace : public CommonBase
{
   bool
      initialised;
   char
      * xface,
      * data;
 public:
   /**
      Constructor.
      @param initial pathlist, separated by either colons (unix) or semicolons (dos)
      @param recursive if true, add all subdirectories
   */
   XFace();

   /**
      Destructor.
   */
   ~XFace();

   /**
      Create an XFace from an XPM.
      @param xpmdata data buffer containing the XPM structure
      @return true on success
   */
   bool	CreateFromXpm(const char *xpmdata);

   /**
      Create an XFace from an XFace line.
      @param xfacedata buffer containing the xface line
      @return true on success
   */
   bool	CreateFromXFace(const char *xfacedata);

   /**
      Create an Xpm from an XFace, including all the declaration
      stuff, just like for writing to a file.
      @param xpm String for the xpm data
      @return true on success
   */
   bool	CreateXpm(String &xpm);

   /**
      Create an Xpm from an XFace, creates only the contents of the
      character array.
      @param xpm String for the xpm data
      @return true on success
   */
   bool	CreateXpm(char ***xpm);

   /// initialised == there is a list of paths
   bool	IsInitialised(void) const { return initialised; }

   CB_DECLARE_CLASS(XFace, CommonBase);
};

//@}

#endif
