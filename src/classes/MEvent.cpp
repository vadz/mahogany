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
#include "Profile.h"
#include "MailFolder.h"
#include "MApplication.h"

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

/// are we currently dispatching events?
static bool gs_IsDispatching = false;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// MEventReceiver
// ----------------------------------------------------------------------------


// check that we had removed ourself from the list of event handlers
MEventReceiver::~MEventReceiver()
{
#ifdef DEBUG
   size_t count = gs_receivers.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      MEventReceiverInfo *info = gs_receivers[n];
      if ( &(info->receiver) == this )
      {
         FAIL_MSG( "Forgot to Deregister() - will probably crash!" );

         break;
      }
   }
#endif // DEBUG
}

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------

MEventManager::MEventManager()
{
}

/* static */
void
MEventManager::DispatchPending(void)
{
   if(gs_IsDispatching)
      return;
   gs_IsDispatching = true;
   MEventData *dataptr = NULL;
   
   while(! gs_EventList.empty() )
   {
      dataptr = gs_EventList.pop_front();
      Dispatch(dataptr);
      delete dataptr;
   }
   gs_IsDispatching = false;
}

/* static */
void
MEventManager::Send(MEventData * data)
{
   MEventLocker mutex;
   gs_EventList.push_back(data);
}

/* static */
void MEventManager::Dispatch(MEventData * dataptr)
{
   MEventLocker mutex;

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

// the return value is just the pointer to the structure we add to the array
// (it's opaque for the caller)
void *MEventManager::Register(MEventReceiver& who, MEventId eventId)
{
   MEventLocker mutex;

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
   MEventLocker mutex;

   int index = gs_receivers.Index((MEventReceiverInfo *)handle);

   CHECK( index != wxNOT_FOUND, false,
          "unregistering event handler which was not registered" );

   size_t n = (size_t)index;
   delete gs_receivers[n];
   gs_receivers.Remove(n);

   return true;
}

// ----------------------------------------------------------------------------
// different MEvent derivations
// ----------------------------------------------------------------------------

MEventOptionsChangeData::MEventOptionsChangeData(class ProfileBase
                                                 *profile, ChangeKind
                                                 what)

   : MEventData(MEventId_OptionsChange)
{
   SafeIncRef(profile);

   m_profile = profile;
   m_what = what;
   m_vetoed = false;
}

MEventOptionsChangeData::~MEventOptionsChangeData()
{
   SafeDecRef(m_profile);
}

MEventNewMailData::MEventNewMailData(MailFolder *folder,
                                     unsigned long n,
                                     unsigned long *messageIDs)
   : MEventData(MEventId_NewMail)
{
   m_folder = folder;
   m_folder->IncRef();
   m_number = n;
   m_messageIDs = new unsigned long [n];
   for(size_t i = 0; i < n; i++)
      m_messageIDs[i] = messageIDs[i];
}

MEventNewMailData::~MEventNewMailData()
{
   delete [] m_messageIDs;
   m_folder->DecRef();
}

