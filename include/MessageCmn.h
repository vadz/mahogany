//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MessageCmn.h: cclient independent parts of MessageCC
// Purpose:     Message class is an interface and as such doesn't contain any
//              data, however we do to extract some of common fields from
//              MessageCC to make it possible to create other Message-derived
//              classes later more easily
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.05.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 by Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _MESSAGE_CMN_H_
#define _MESSAGE_CMN_H_

#include "Message.h"        // the base class

// ----------------------------------------------------------------------------
// MessageCmn adds support for common (== envelop) headers which are fast to
// retrieve and the other ones.
// ----------------------------------------------------------------------------

class MessageCmn : public Message
{
public:
protected:
   // get all headers and cache their values
   
};

#endif // _MESSAGE_CMN_H_
