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
/**    MEvent ids for all kinds of events used in M. The name in the comment is the
       MEventData-derived class which is used with this event
*/
enum MEventId
{
   /// (invalid id for an event)
   MEventId_Null = -1,                          
   /// MEventMailData
   MEventId_NewMail = 100,                      
   /// MEventFolderTreeChangeData
   MEventId_FolderTreeChange = 200,
   /// MEventFolderUpdateData
   MEventId_FolderUpdate = 400,
   /// MEventASFolderResult
   MEventId_ASFolderResult = 800,
   /// (invalid id for an event)
   MEventId_Max                                 
};

// ----------------------------------------------------------------------------
/** The data associated with an event - more classes can be derived from this
    one.
*/
class MEventData
{
public:
   /// ctor takes the (string) id for the event
   MEventData(MEventId id = MEventId_Null) { m_eventId = id; }

   /**@name accessors */
   //@{
   /// set the id
   void SetId(MEventId id) { m_eventId = id; }
   /// retrieve the id
   MEventId GetId() const { return m_eventId; }
   //@}

   /// virtual dtor as in any base class
   virtual ~MEventData() { }
private:
   MEventId m_eventId;
};

// TODO do these event classes really belong here? In principle, no, but with
//      numeric ids we don't gain anything by moving them to separate files
//      because this one still has to be modified (to add the id) whenever a
//      new event is added. Is this a good reason to use string ids?

//forward decl
class MailFolder;

// ----------------------------------------------------------------------------
/// MEventNewMailData - the event notifying the app about "new mail"
class MEventNewMailData : public MEventData
{
public:
   /** Constructor.
       @param folder the mail folder where the new mail was detected
       @param n number o fnew messages
       @param messageIDs points to an array of the sequence numbers of
       the new messages in that folder, freed by the caller
   */
   MEventNewMailData(MailFolder *folder,
                     unsigned long n,
                     unsigned long *messageIDs)
      : MEventData(MEventId_NewMail)
      {
         m_folder = folder;
         m_number = n;
         m_messageIDs = messageIDs;
      }

   /**@name accessors */
   //@{
   /// get the folder in which there are new messages
   MailFolder *GetFolder() const { return m_folder; }
   /// get the number of new messages
   unsigned long GetNumber(void) const { return m_number; }
   /// get the index of the Nth new message (for calling GetMessage)
   unsigned long GetNewMessageIndex(unsigned long n) const
      {
         return m_messageIDs[n];
      }
   //@}   
private:
   MailFolder    *m_folder;
   unsigned long *m_messageIDs;
   unsigned long  m_number;
};


// ----------------------------------------------------------------------------
/** MEventFolderUpdate Data - Does not carry any data apart from pointer to mailfolder.*/
class MEventFolderUpdateData : public MEventData
{
public:
   /** Constructor.
   */
   MEventFolderUpdateData(MailFolder *folder)
      : MEventData(MEventId_FolderUpdate)
      {
         m_folder = folder;
      }
   /// get the folder which changed
   MailFolder *GetFolder() const { return m_folder; }
private:
   MailFolder *m_folder;
};

// ----------------------------------------------------------------------------
// MEventFolderTreeChangeData - this event is generated whenever a new folder
// is created or an existing one is deleted or renamed
// ----------------------------------------------------------------------------

class MEventFolderTreeChangeData : public MEventData
{
public:
   // what happened?
   enum ChangeKind
   {
      Create,
      Delete,
      Rename
   };

   MEventFolderTreeChangeData(const String& fullname, ChangeKind what)
      : MEventData(MEventId_FolderTreeChange), m_fullname(fullname)
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
/** MEventASFolderResult Data - Carries a ASMailFolder::Result object
    with the result of an asynchronous operation on the folder. */
class MEventASFolderResult : public MEventData
{
public:
   /** Constructor.
   */
   MEventASFolderResult(MObjectRC *result)
      : MEventData(MEventId_ASFolderResult)
      {
         m_ResultData = result;
         m_ResultData->IncRef();
      }
   ~MEventASFolderResult()
      { m_ResultData->DecRef(); }
private:
   MObjectRC  *m_ResultData;
};


// ----------------------------------------------------------------------------
// Derive from this class to be able to process events.
// ----------------------------------------------------------------------------
class MEventReceiver
{
public:
   // override this method to process the events:
   // return TRUE if it's ok to pass event to other receivers
   // or FALSE to not propagate it any more
   virtual bool OnMEvent(MEventData& event) = 0;
};

// ----------------------------------------------------------------------------
// This class allows to send events and forwards them to the receivers waiting
// for them. There is only one object of this class in the whole program and
// even this one is never used directly - all functions are static.
// ----------------------------------------------------------------------------
class MEventManager
{
public:
   MEventManager();

   /// send an event to the queue
   static void Send(MEventData * data);

   /// Dispatches all events in the queue.
   static void DispatchPending(void);
   
   // register the event receiever for the events "eventId", the returned
   // pointer is NULL if the function failed, otherwise it should be saved for
   // subsequent call to Deregister()
   static void *Register(MEventReceiver& who, MEventId eventId);

   // unregister an event receiever - the "handle" parameter is returned by
   // Register(). If the same object is registered for several different
   // events, Deregister() should be called for each (successful) call to
   // Register()
   static bool Deregister(void *handle);
protected:
   /// Dispatches a single event.
   static void Dispatch(MEventData * data);
};

#endif // MEVENT_H
