// -*- c++ -*-// //// //// //// //// //// //// //// //// //// //// //// //// //
// Project:     M - cross platform e-mail GUI client
// File name:   MEvent.h - event system declarations
// Purpose:     Event classes provide a standard way for the (independent)
//              program parts to notify each other about some events. An event
//              can be sent by anybody using EventManager::Send() function and
//              can be received by any class deriving from EventReceiver if
//              it had been registered previously (EventReceiver::Register()).
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.01.99
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
// // //// //// //// //// //// //// //// //// //// //// //// //// //// //// ///

#ifndef MEVENT_H
#define MEVENT_H

// ----------------------------------------------------------------------------
// Event ids for all kinds of events used in M. The name in the comment is the
// EventData-derived class which is used with this event
// ----------------------------------------------------------------------------
enum EventId
{
   EventId_Null = -1,                          // (invalid id for an event)
   EventId_NewMail = 100,                      // EventMailData
   EventId_FolderTreeChange = 200,             // EventFolderTreeChangeData
   EventId_Max                                 // (invalid id for an event)
};

// ----------------------------------------------------------------------------
// A data associated with an event - more classes can be derived from this
// one
// ----------------------------------------------------------------------------
class EventData
{
public:
   // ctor takes the (string) id for the event
   EventData(EventId id = EventId_Null) { m_eventId = id; }

   // accessors
      // set the id
   void SetId(EventId id) { m_eventId = id; }
      // retrieve the id
   EventId GetId() const { return m_eventId; }

   // virtual dtor as in any base class
   virtual ~EventData() { }

private:
   EventId m_eventId;
};

// TODO do these event classes really belong here? In principle, no, but with
//      numeric ids we don't gain anything by moving them to separate files
//      because this one still has to be modified (to add the id) whenever a
//      new event is added. Is this a good reason to use string ids?

// ----------------------------------------------------------------------------
// EventMailData - the event notifying the app about "new mail"
// ----------------------------------------------------------------------------

class MailFolder;

class EventMailData : public EventData
{
public:
   EventMailData(MailFolder *folder,
                 unsigned long number,
                 unsigned long *messageIDs)
      : EventData(EventId_NewMail)
   {
      m_folder = folder;
      m_number = number;
      m_messageIDs = messageIDs;
   }

   // accessors
      // get the folder in which there are new messages
   MailFolder *GetFolder() const { return m_folder; }
      // get the number of new messages
   unsigned long GetNumberOfMessages() const { return m_number; }
      // get the index of the Nth new message (for calling GetMessage)
   unsigned long GetNewMessageIndex(unsigned long n) const
   {
      ASSERT( n < m_number );

      return m_messageIDs[n];
   }

private:
   MailFolder    *m_folder;
   unsigned long  m_number;
   unsigned long *m_messageIDs;
};

// ----------------------------------------------------------------------------
// EventFolderTreeChangeData - this event is generated whenever a new folder
// is created or an existing one is deleted or renamed
// ----------------------------------------------------------------------------

class EventFolderTreeChangeData : public EventData
{
public:
   // what happened?
   enum ChangeKind
   {
      Create,
      Delete,
      Rename
   };

   EventFolderTreeChangeData(const String& fullname, ChangeKind what)
      : EventData(EventId_FolderTreeChange), m_fullname(fullname)
   {
      m_what = what;
   }

   // get the fullname of the folder
   const String& GetFolderFullName() const { return m_fullname; }

   // get the kind of the operation which just happened
   ChangeKind GetChangeKind() const { return m_what; }

private:
   String      m_fullname;
   ChangeKind  m_what;
};

// ----------------------------------------------------------------------------
// Derive from this class to be able to process events.
// ----------------------------------------------------------------------------
class EventReceiver
{
public:
   // override this method to process the events:
   // return TRUE if it's ok to pass event to other receivers
   // or FALSE to not propagate it any more
   virtual bool OnEvent(EventData& event) = 0;
};

// ----------------------------------------------------------------------------
// This class allows to send events and forwards them to the receivers waiting
// for them. There is only one object of this class in the whole program and
// even this one is never used directly - all functions are static.
// ----------------------------------------------------------------------------
class EventManager
{
public:
   EventManager();

   // send an event
   static void Send(EventData& data);

   // register the event receiever for the events "eventId", the returned
   // pointer is NULL if the function failed, otherwise it should be saved for
   // subsequent call to Unregister()
   static void *Register(EventReceiver& who, EventId eventId);

   // unregister an event receiever - the "handle" parameter is returned by
   // Register(). If the same object is registered for several different
   // events, Unregister() should be called for each (successful) call to
   // Register()
   static bool Unregister(void *handle);
};

#endif // MEVENT_H
