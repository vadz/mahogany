/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef WXICONMANAGER_H
#define WXICONMANAGER_H

#ifdef __GNUG__
#pragma interface "wxIconManager.h"
#endif

#ifndef USE_PCH
#  define   Uses_wxIcon
#  include  <wx/wx.h>

#  include  <Mcommon.h>
#endif  //USE_PCH

/**
   IconManager class, this class allocates and deallocates icons for
   the application, freeing the individual gui object from the need to
   allocate or deallocate it.
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

   /** Add a name/icon pair to the list
       @param iconName the name for the icon
       @param data the xpm data array (Unix) or the icon resource name (Win)
   */
   void AddIcon(String const &iconName, IconResourceType data);

   /** Get a bitmap: only different from GetIcon under Windows where it looks
       for th bitmap in resources first and then calls GetIcon
       @param bmpName the name of the bitmap
   */
   wxBitmap GetBitmap(const String& bmpName);
};

#endif
