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
   /// schedule a message to be sent later
   virtual bool ScheduleMessage(class SendMessage *msg) = 0;
   /// move a message to the calendar to be shown again later
   virtual bool ScheduleMessage(class Message *msg) = 0;
};

#define MMODULE_INTERFACE_CALENDAR   _T("Calendar")

#endif // CALENDARMODULE_H
