/*-*- c++ -*-********************************************************
 * MGuiApp class: all GUI specific application stuff, pure virtual  *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:11  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MGUIAPP_H
#define MGUIAPP_H

#include	<Mcommon.h>

/**
   GUI Application class, defining the interface to the real GUI
   application class
*/

class MGuiApp
{   
public:
   /// virtual destructor
   virtual ~MGuiApp() {};
};
#endif
