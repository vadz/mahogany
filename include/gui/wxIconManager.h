/*-*- c++ -*-********************************************************
 * wxIconManager - allocating and deallocating icons for drawing    *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.5  1998/06/05 16:56:57  VZ
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.4  1998/05/02 15:21:33  KB
 * Fixed the #if/#ifndef etc mess - previous sources were not compilable.
 *
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

#ifndef WXICONMANAGER_H
#define WXICONMANAGER_H

#ifdef __GNUG__
#pragma interface "wxIconManager.h"
#endif

#ifndef USE_PCH
#  define   Uses_wxIcon
#  include	<wx/wx.h>

#  include	<Mcommon.h>

#  undef T
#  include	<list>
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
struct	IconData
{
  String	iconName;
  wxIcon	*iconPtr;

  IMPLEMENT_DUMMY_COMPARE_OPERATORS(IconData);
};

class wxIconManager
{
   /** A list of all known icons.
       @see IconData
   */
   std::list<IconData>	*iconList;
   
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
       @param data the xpm data array (Unix) or the icon resource name (Win)
   */
   void AddIcon(String const &iconName, IconResourceType data);
};

#endif
