/*-*- c++ -*-********************************************************
 * FolderView.h : a window which shows a MailFolder                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:10  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

/**
   FolderView class, a window displaying a MailFolder.

*/

class FolderViewBase
{   
 public:
   /// update the user interface
   virtual void Update(void) = 0;
};

#endif
