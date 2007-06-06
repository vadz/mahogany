/*-*- c++ -*-********************************************************
 * XFace.h -  a class encapsulating XFace handling                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                  *
 *******************************************************************/

#ifndef XFACE_H
#define XFACE_H

#ifndef XFACE_WITH_WXIMAGE
#define    XFACE_WITH_WXIMAGE   1
#endif


#ifdef XFACE_WITH_WXIMAGE
#  include <wx/image.h>
#endif

/**
   XFace class for handling XFaces.
*/
class XFace
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
      Create an XFace from a string containing lines of hex numbers.
      @param xpmdata data buffer containing the numbers.
      @return true on success
   */
   bool	CreateFromData(const char *data);

   /**
      Create an XFace from an XPM.
      @param xpmdata data buffer containing the XPM structure
      @return true on success
   */
   bool	CreateFromXpm(const char *xpmdata);

#if XFACE_WITH_WXIMAGE
   /** Create an XFace from an image file.
       @param filename obvious
       @return true on success
   */
   bool CreateFromFile(const wxChar *filename);

   static class wxImage GetXFaceImg(const String &filename,
                                    bool *hasimg = NULL,
                                    class wxWindow *parent = NULL);
   static String ConvertImgToXFaceData(wxImage &img);
#endif
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

   /**
      Get header line data, including leading X-Face.
      @return header line
   */
   String GetHeaderLine(void) const;
   
   /// initialised == there is a list of paths
   bool	IsInitialised(void) const { return initialised; }

};

#endif
