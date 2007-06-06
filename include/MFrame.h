/*-*- c++ -*-********************************************************
 * MFrame.h : GUI independent frame/window representation           *
 *                                                                  *
 * (C) 1997, 1998 by Karsten Ballüder (Ballueder@usa.net)           *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MFRAME_H
#define MFRAME_H

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
   const wxChar *GetName() const { return name.c_str(); }

   // VZ: this could lead to an ambiguity as wxFrame (from which wxMFrame
   //     derives as well) has this (virtual) method too
#if 0
   /// used to set the title of the window
   virtual void SetTitle(String const & name) = 0;
#endif

   /// add a menu to the bar
   virtual void AddFileMenu(void) = 0;
   virtual void AddHelpMenu(void) = 0;
   virtual bool ProcessModulesMenu(int id) = 0;

   /// make it visible or invisible
   virtual bool Show(bool visible = true) = 0;

   /// is it ok to close the frame?
   virtual bool CanClose() const { return true; }

   /// virtual destructor
   virtual ~MFrameBase() { }
};

#endif
