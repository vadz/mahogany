/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXICONMANAGER_H
#define WXICONMANAGER_H

#ifndef USE_PCH
#  include <wx/icon.h>
#endif  //USE_PCH

#include <wx/image.h>

#include <list>

// under Windows we can use resources as "normal" windows programs do or we can
// use XPMs as under Unix
#if defined(OS_WIN)
   #define USE_ICONS_FROM_RESOURCES
#endif

/**
   IconManager class, this class allocates and deallocates icons for
   the application.
*/

/// IconResourceType is XPM under Unix and name of ICO resource under Windows
#ifdef  USE_ICONS_FROM_RESOURCES
  typedef const char *IconResourceType;
#else   //Unix
  typedef const char *IconResourceType[];
#endif  //Win/Unix

/** A structure holding name and wxIcon pointer.
   This is the element of the list.
*/
struct  IconData
{
  String  iconName;
  wxIcon  iconRef;
};

typedef std::list<IconData> IconDataList;

class wxIconManager
{
public:
   /** Constructor
       @param sub_dir if not empty a favourite subdirectory to use for the icons
    */
   wxIconManager(wxString sub_dir = wxEmptyString);

   /** Tells the IconManager to use a different sub-directory.
       @param sub_dir if not empty a favourite subdirectory to use for the icons
    */
   void SetSubDirectory(wxString sub_dir);

   /** Get an Icon.
       If the iconName is not known, a default icon will be returned.
       @param iconName  the name of the icon
       @return the wxIcon
   */
   wxIcon GetIcon(String const &iconName);

   /** Get a bitmap: only different from GetIcon under Windows where it looks
       for the bitmap in resources first and then calls GetIcon
       @param bmpName the name of the bitmap
   */
   wxBitmap GetBitmap(const String& bmpName);

   /** Get the icon for MIME type (first calls GetIcon)
       @param type mime type
       @param extension filename extension if known
   */
   wxIcon GetIconFromMimeType(const String& type, const String &
                              extension = wxEmptyString);

   /** Add a name/icon pair to the list
       @param iconName the name for the icon
       @param data the xpm data array (Unix) or the icon resource name (Win)
   */
   void AddIcon(String const &iconName, IconResourceType data);

   /** Load an image file and return it as a wxImage.
       @param filename the name of the file
       @param success set to true on success
       @parem showDlg if true, show abort/progress dialog
       @return wxImage holding the graphic
   */
   static wxImage & LoadImage(String filename,
                              bool *success,
                              bool showDlg = false);

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

protected:
   /// look up an icon in the cache
   bool FindInCache(const String& iconName, wxIcon *icon) const;

private:
   /** A list of all known icons.
       @see IconData
   */
   IconDataList m_iconList;

   /// An Icon to return for unknown lookup strings.
   wxIcon m_unknownIcon;

   /// have we checked for supported formats?
   static bool m_knowHandlers;
   /// list of supported types, terminated with -1
   static wxBitmapType m_wxBitmapHandlers[];
   /// number of handlers
   static int ms_NumOfHandlers;
   /// check this path first
   static wxString ms_IconPath;
   /// the preferred sub-directory to use for the icons
   wxString m_SubDir;
   /// the global icon directory:
   wxString m_GlobalDir;
   /// the local icon directory:
   wxString m_LocalDir;
};

#endif
