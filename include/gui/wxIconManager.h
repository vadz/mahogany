/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXICONMANAGER_H
#define WXICONMANAGER_H

#ifdef __GNUG__
#pragma interface "wxIconManager.h"
#endif

#ifndef USE_PCH
#  include <wx/icon.h>
#  include <wx/image.h>
#  include <Mcommon.h>
#endif  //USE_PCH

/**
   IconManager class, this class allocates and deallocates icons for
   the application.
*/

/// IconResourceType is XPM under Unix and name of ICO resource under Windows
#ifdef  OS_WIN
  typedef const char *IconResourceType;
#else   //Unix
  typedef char *IconResourceType[];
#endif  //Win/Unix

/** A structure holding name and wxIcon pointer.
   This is the element of the list.
*/
struct  IconData
{
  String  iconName;
  wxIcon  iconRef;
public:
};

KBLIST_DEFINE(IconDataList, IconData);

class wxIconManager
{
   /** A list of all known icons.
       @see IconData
   */
   IconDataList *m_iconList;
   
   /// An Icon to return for unknown lookup strings.
   wxIcon m_unknownIcon;

public:
   /** Constructor
   */
   wxIconManager();

   /** Destructor
       writes back all entries
   */
   ~wxIconManager();

   /** Get an Icon.
       If the iconName is not known, a default icon will be returned.
       @param iconName  the name of the icon
       @return the wxIcon
   */
   wxIcon GetIcon(String const &iconName);

   /** Get the icon for MIME type (first calls GetIcon)
   */
   wxIcon GetIconFromMimeType(const String& type);

   /** Add a name/icon pair to the list
       @param iconName the name for the icon
       @param data the xpm data array (Unix) or the icon resource name (Win)
   */
   void AddIcon(String const &iconName, IconResourceType data);

   /** Get a bitmap: only different from GetIcon under Windows where it looks
       for th bitmap in resources first and then calls GetIcon
       @param bmpName the name of the bitmap
   */
   wxIcon GetBitmap(const String& bmpName);

   /** Load an image file and return it as a wxImage.
       @filename the name of the file
       @success set to true on success
       @return wxImage holding the graphic
   */
   static wxImage & LoadImage(String filename, bool *success);
   
   /** Load an image file and return it as an xpm array.
       @filename the name of the file
   */
   static char **LoadImageXpm(String filename);
   
   /** Load an xpm image file and return it as an xpm array.
       @filename the name of the file
   */
   static char **LoadXpm(String filename);
   /** Free an xpm array returned by LoadImage
       @ptr pointer returned by LoadImage
   */
   static void FreeImage(char **ptr);

private:
   /// have we checked for supported formats?
   static bool m_knowHandlers;
   /// list of supported types, terminated with -1
   static long m_wxBitmapHandlers[];
};

#endif
