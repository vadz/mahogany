///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   interface/MDialogs.h: some common exported dialog functions
// Purpose:     functions here can be used from Python to interface with user
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.12.03
// CVS-ID:      $Id$
// Copyright:   (c) 2003 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _M_INTERFACE_MDIALOGS_H_
#define _M_INTERFACE_MDIALOGS_H_

namespace MDialogs
{

/**
   Show a message box.

   @param message the string to show
   @return true if ok was pressed, false if cancel
 */
extern bool Message(const char *message);

/**
   Show the message in the status bar of the main frame.

   @param message the string to show
 */
extern void Status(const char *message);

} // namespace MDialogs

#endif // _M_INTERFACE_MDIALOGS_H_

