/*-*- c++ -*-********************************************************
 * FolderView.h : a window which shows a MailFolder                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

/**
   FolderView class, a window displaying a MailFolder.
*/
class FolderView
{   
public:
   /// update the user interface
   virtual void Update(void) = 0;
   /// virtual destructor
   virtual ~FolderView() {}
};
#endif
