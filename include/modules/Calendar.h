/*-*- c++ -*- *******************************************************
 * Calendar - a Calendar module for Mahogany                        *
 *                                                                  *
 * (C) 2000 by Karsten Ballüder (Ballueder@gmx.net)                 *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef CALENDARMODULE_H
#define CALENDARMODULE_H

#include "MModule.h"

/**
   This is the interface for the Mahogany scoring module.
   It just provides one function which calculates a score function
   from a HeaderInfo pointer.
*/
class MModule_Calendar : public MModule
{
public:
   virtual bool ScheduleMessage(class SendMessageCC *msg) = 0;
};

#define MMODULE_INTERFACE_CALENDAR   "Calendar"

#endif // CALENDARMODULE_H
