/*-*- c++ -*-********************************************************
 * MessageView.h : a window which shows a Message                   *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

// there is no matching .cpp file containing pragma implementation
#if 0 //def __GNUG__
#pragma interface "MessageView.h"
#endif

// ----------------------------------------------------------------------------
// MessageViewBase should be the ABC for msg viewer classes - for now it is not
// used
// ----------------------------------------------------------------------------

class MessageViewBase
{
public:
   MessageViewBase() { }
};

#endif // MESSAGEVIEW_H

