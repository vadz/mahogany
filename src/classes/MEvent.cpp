// -*- c++ -*-/////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MEvent.cpp - implementation of event subsystem
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.01.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"

#ifndef  USE_PCH
   #include "Mcommon.h"

   #include <wx/dynarray.h>
#endif // USE_PCH

#include "MEvent.h"

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

// info about one receiver
struct EventReceiverInfo
{
   EventReceiverInfo(EventReceiver& who, EventId eventId) : receiver(who)
      { id = eventId; }

   EventReceiver& receiver;
   EventId        id;
};

// array of all registered receivers
WX_DEFINE_ARRAY(EventReceiverInfo *, EventReceiverInfoArray);

// ----------------------------------------------------------------------------
// global variables (we don't make them static member vars of EventManager to
// reduce compilation dependencies)
// ----------------------------------------------------------------------------

// the one and only event manager - it can be safely created statically
// because it doesn't allocate memory nor does any other dangerous things in
// its ctor
static EventManager gs_eventManager;

// all registered event handlers
static EventReceiverInfoArray gs_receivers;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------

EventManager::EventManager()
{
}

void EventManager::Send(EventData& data)
{
   EventId id = data.GetId();

   // make a copy of the array because some event handlers might remove
   // themselves from our list while we send the event
   EventReceiverInfoArray receivers = gs_receivers;
   size_t count = receivers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      EventReceiverInfo *info = receivers[n];
      
      // check that the object didn't go away!
      if ( gs_receivers.Index(info) == wxNOT_FOUND )
         continue;

      if ( info->id == id )
      {
         // notify this one
         if ( !info->receiver.OnEvent(data) )
         {
            // the handler decided to stop the event propagation
            break;
         }
         //else: continue to search other receivers for this event
      }
   }
}

// the return value is justthe pointer to the structure we add to the array
// (it's opaque for the caller)
void *EventManager::Register(EventReceiver& who, EventId eventId)
{
   EventReceiverInfo *info = new EventReceiverInfo(who, eventId);

#ifdef DEBUG
   // check that we don't register the same object twice
   size_t count = gs_receivers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      EventReceiverInfo *info = gs_receivers[n];
      if ( info->id == eventId && &(info->receiver) == &who )
      {
         FAIL_MSG( "Registering the same handler twice in "
                   "EventManager::Register()" );
      }
   }
#endif

   gs_receivers.Add(info);

   return info;
}

bool EventManager::Unregister(void *handle)
{
   int index = gs_receivers.Index((EventReceiverInfo *)handle);

   CHECK( index != wxNOT_FOUND, false,
          "unregistering event handler which was not registered" );

   size_t n = (size_t)index;
   delete gs_receivers[n];
   gs_receivers.Remove(n);

   return true;
}

