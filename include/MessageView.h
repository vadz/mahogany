/*-*- c++ -*-********************************************************
 * MessageView.h : a window which shows a Message                   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:12  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

// don't use interface for virtual base class as there is no .cc file
// for it !
//#ifdef __GNUG__
//#pragma interface "MessageView.h"
//#endif

#include	<Message.h>

/**
   MessageView class, a window displaying a Message.

*/

class MessageViewBase
{   
public:
   /// constructor
   MessageViewBase() {}
   /// virtual destructor
   virtual ~MessageViewBase() {}
   /// update the user interface
   virtual void Update(void) = 0;
};

#endif
