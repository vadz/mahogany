/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:15  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef WXICONMANAGER_H
#define WXICONMANAGER_H

#ifdef __GNUG__
#pragma interface "wxIconManager.h"
#endif

#define	Uses_wxIcon
#include	<wx/wx.h>

#include	<Mcommon.h>


#include	<list>

/**
   IconManager class, this class allocates and deallocates icons for
   the application, freeing the individual gui object from the need to
   allocate or deallocate it.
*/

class wxIconManager
{
   /** A structure holding name and wxIcon pointer.
       This is the element of the list.
   */
   struct	IconData
   {
      String	iconName;
      wxIcon	*iconPtr;
   };
   
   /** A list of all known icons.
       @see IconData
   */
   list<IconData>	*iconList;
   
   /// An Icon to return for unknown lookup strings.
   wxIcon	*unknownIcon;
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
   wxIcon *GetIcon(String const &iconName = String("unknown"));

   /** Add a name/icon pair to the list
       @param iconName the name for the icon
       @param data the xpm data array
   */
   void AddIcon(String const &iconName, char *data[]);
};

#endif
