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
#   include "Mcommon.h"
#   include "kbList.h"
#   include <wx/dynarray.h>
#endif // USE_PCH

#include "MEvent.h"

// ----------------------------------------------------------------------------
// private types
// ----------------------------------------------------------------------------

// info about one receiver
struct MEventReceiverInfo
{
   MEventReceiverInfo(MEventReceiver& who, MEventId eventId) : receiver(who)
      { id = eventId; }

   MEventReceiver& receiver;
   MEventId        id;
};

// array of all registered receivers
WX_DEFINE_ARRAY(MEventReceiverInfo *, MEventReceiverInfoArray);

// ----------------------------------------------------------------------------
// global variables (we don't make them static member vars of MEventManager to
// reduce compilation dependencies)
// ----------------------------------------------------------------------------

// the one and only event manager - it can be safely created statically
// because it doesn't allocate memory nor does any other dangerous things in
// its ctor
static MEventManager gs_eventManager;

// all registered event handlers
static MEventReceiverInfoArray gs_receivers;


KBLIST_DEFINE(MEventList, MEventData);

/// the list of pending events
static MEventList gs_EventList;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------


MEventManager::MEventManager()
{
}

void
MEventManager::DispatchPending(void)
{
   MEventData *dataptr = NULL;
   
   while(! gs_EventList.empty() )
   {
      dataptr = gs_EventList.pop_front();
      Dispatch(dataptr);
      delete dataptr;
   }
}

void
MEventManager::Send(MEventData * data)
{
   gs_EventList.push_back(data);
}

void MEventManager::Dispatch(MEventData * dataptr)
{
   MEventData & data = *dataptr;
   
   MEventId id = data.GetId();

   // make a copy of the array because some event handlers might remove
   // themselves from our list while we send the event
   MEventReceiverInfoArray receivers = gs_receivers;
   size_t count = receivers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      MEventReceiverInfo *info = receivers[n];
      
      // check that the object didn't go away!
      if ( gs_receivers.Index(info) == wxNOT_FOUND )
         continue;

      if ( info->id == id )
      {
         // notify this one
         if ( !info->receiver.OnMEvent(data) )
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
void *MEventManager::Register(MEventReceiver& who, MEventId eventId)
{
   MEventReceiverInfo *info = new MEventReceiverInfo(who, eventId);

#ifdef DEBUG
   // check that we don't register the same object twice
   size_t count = gs_receivers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      MEventReceiverInfo *info = gs_receivers[n];
      if ( info->id == eventId && &(info->receiver) == &who )
      {
         FAIL_MSG( "Registering the same handler twice in "
                   "MEventManager::Register()" );
      }
   }
#endif

   gs_receivers.Add(info);

   return info;
}

bool MEventManager::Deregister(void *handle)
{
   int index = gs_receivers.Index((MEventReceiverInfo *)handle);

   CHECK( index != wxNOT_FOUND, false,
          "unregistering event handler which was not registered" );

   size_t n = (size_t)index;
   delete gs_receivers[n];
   gs_receivers.Remove(n);

   return true;
}
