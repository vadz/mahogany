/*-*- c++ -*-********************************************************
 * MFrame.h : GUI independent frame/window representation           *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MFRAME_H
#define MFRAME_H

//#ifdef __GNUG__
//#pragma interface "MFrame.h"
//#endif

/**
   MFrameBase virtual base class, defining the interface for a window.
   Every window should have a unique name associated with it for use
   in the configuration file. E.g. "FolderView" or "ComposeWindow".
*/

class MFrameBase
{
private:
   /// each frame has a unique name used to identify it
   String name;

public:
   /// ctor takes the name of the frame class
   MFrameBase(const String& str) : name(str) { }

   /// retrieve the name of the window class
   const char *GetName() const { return name.c_str(); }

   /// used to set the title of the window
   virtual void SetTitle(String const & name) = 0;

   /// make it visible or invisible
   virtual SHOW_TYPE Show(bool visible = true) = 0;

   /// is it ok to close the frame?
   virtual bool CanClose() const { return true; }

   /// virtual destructor
   virtual ~MFrameBase() {};
};

#endif
