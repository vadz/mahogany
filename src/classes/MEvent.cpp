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
#   include "guidef.h"
#   include "Profile.h"
#   include "MApplication.h"

#   include <wx/dynarray.h>     // for WX_DEFINE_ARRAY
#endif // USE_PCH

#include <stdarg.h>             // for va_start

#include "MEvent.h"
#include "MailFolder.h"
#include "HeaderInfo.h"

#include <list>

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

   DECLARE_NO_COPY_CLASS(MEventReceiverInfo)
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


typedef std::list<MEventData *> MEventList;

/// the list of pending events
static MEventList gs_EventList;

/// are we suspended (if > 0)?
static int gs_IsSuspended = 0;

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
         FAIL_MSG( _T("Forgot to Deregister() - will probably crash!") );

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
   static bool gs_running = false;
   if ( !gs_running && !g_busyCursorYield )
   {
      gs_running = true;
      ForceDispatchPending();
      gs_running = false;
   }
}

/* static */
void
MEventManager::ForceDispatchPending(void)
{
   // don't dispatch events while we're suspended: as Dispatch() just ignores
   // them, it means that events are simply lost if we do it
   if ( gs_IsSuspended )
      return;

   MEventLocker mutex;

   while ( !gs_EventList.empty() )
   {
      MEventData * const dataptr = gs_EventList.front();
      gs_EventList.pop_front();

      // Dispatch is safe and might cause new events:
      mutex.Unlock();
      Dispatch(dataptr);
      mutex.Lock(); // restore locked status
   }
}

/* static */
void
MEventManager::Send(MEventData * data)
{
   wxLogTrace(_T("event"), _T("Queuing event %d"), data->GetId());

   MEventLocker mutex;
   gs_EventList.push_back(data);
}

/* static */
bool MEventManager::Dispatch(MEventData *dataptr)
{
   CHECK( dataptr, false, _T("NULL event in Dispatch()") );

   if ( gs_IsSuspended )
   {
      // can't do it now!
      return false;
   }

   MEventData & data = *dataptr;
   MEventId id = data.GetId();

   wxLogTrace(_T("event"), _T("Dispatching event %d"), id);

   // make a copy of the array because some event handlers might remove
   // themselves from our list while we send the event
   MEventLocker mutex;
   MEventReceiverInfoArray receivers = gs_receivers;
   mutex.Unlock();

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

   delete dataptr;

   return true;
}

// the return value is just the pointer to the structure we add to the array
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

   {
      MEventLocker mutex;
      gs_receivers.Add(info);
   }

   return info;
}

bool MEventManager::Deregister(void *handle)
{
   MEventLocker mutex;
   int index = gs_receivers.Index((MEventReceiverInfo *)handle);

   CHECK( index != wxNOT_FOUND, false,
          _T("unregistering event handler which was not registered") );

   size_t n = (size_t)index;
   delete gs_receivers[n];
   gs_receivers.RemoveAt(n);

   return true;
}

void
MEventManager::Suspend(bool suspend)
{
   MEventLocker mutex;

   CHECK_RET( suspend || gs_IsSuspended > 0,
              _T("resuming events but not suspended") );

   gs_IsSuspended += suspend ? 1 : -1;
}

// ----------------------------------------------------------------------------
// convenience wrappers for Register() and Deregister()
// ----------------------------------------------------------------------------

/* static */
bool MEventManager::RegisterAll(MEventReceiver *who,
                                ...)
{
   va_list arglist;
   va_start(arglist, who);

   bool ok = true;

   for ( ;; )
   {
      MEventId id = (MEventId)va_arg(arglist, int);
      if ( id == MEventId_Null )
         break;

      void **pHandle = va_arg(arglist, void **);
      *pHandle = Register(*who, id);
      if ( !*pHandle )
         ok = false;
   }

   va_end(arglist);

   return ok;
}

/* static */
bool MEventManager::DeregisterAll(void **pHandle, ...)
{
   va_list arglist;
   va_start(arglist, pHandle);

   bool ok = true;
   while ( pHandle )
   {
      if ( !Deregister(*pHandle) )
         ok = false;

      pHandle = va_arg(arglist, void **);
   }

   va_end(arglist);

   return ok;
}

// ----------------------------------------------------------------------------
// different MEvent derivations
// ----------------------------------------------------------------------------

MEventOptionsChangeData::MEventOptionsChangeData(Profile *profile,
                                                 ChangeKind what)

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

MEventWithFolderData::MEventWithFolderData(MEventId id,MailFolder *mf)
   : MEventData(id)
{
   m_Folder = mf;
   SafeIncRef(m_Folder);
}

MEventWithFolderData::~MEventWithFolderData()
{
   SafeDecRef(m_Folder);
}
