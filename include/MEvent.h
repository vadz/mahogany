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
   /// MEventFolderUpdateData -- there's a new folder listing
   MEventId_FolderUpdate = 400,
   /// MEventFolderStatusData -- folder status (e.g. no of new msgs) has changed
   MEventId_FolderStatus = 401,
   /// MEventMsgStatusData
   MEventId_MsgStatus = 402,
   /// MEventASFolderResult
   MEventId_ASFolderResult = 800,
   /// MEventOptionsChangeData
   MEventId_OptionsChange,
   /// MEventPing - causes folders to ping themselves
   MEventId_Ping,
   /// (invalid id for an event)
   MEventId_Max
};

// ----------------------------------------------------------------------------
/** The data associated with an event - more classes can be derived from this
    one.
*/
class MEventData : public MObject
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

///FIXME: this shouldn't be needed! Opaque type?
#include "ASMailFolder.h"

/** The data associated with an event - more classes can be derived from this
    one.
*/

// ----------------------------------------------------------------------------
/// MEventWithFolderData - an event data object containing a folder pointer
class MEventWithFolderData : public MEventData
{
public:
   /// ctor takes the (string) id for the event
   MEventWithFolderData(MEventId id = MEventId_Null,
                        MailFolder *folder = NULL)
      : MEventData(id) { m_Folder = folder; SafeIncRef(m_Folder); }

   /// virtual dtor as in any base class
   virtual ~MEventWithFolderData() { SafeDecRef(m_Folder); }

   /** Get the folder associated.
       If you need the folder after the MEvent object disappears, you
       need to call IncRef() on it, this function does not IncRef()
       the folder automatically! */
   MailFolder *GetFolder() const { return m_Folder; }
private:
   MailFolder *m_Folder;
};
// ----------------------------------------------------------------------------
/// MEventPingData - the event asking the folders to ping
class MEventPingData : public MEventData
{
public:
   MEventPingData() : MEventData( MEventId_Ping ) {}
};

// ----------------------------------------------------------------------------
/// MEventNewMailData - the event notifying the app about "new mail"
class MEventNewMailData : public MEventWithFolderData
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
                     unsigned long *messageIDs);
   ~MEventNewMailData();
   
   /**@name accessors */
   //@{
   /// get the number of new messages
   unsigned long GetNumber(void) const { return m_number; }
   /// get the index of the Nth new message (for calling GetMessage)
   unsigned long GetNewMessageIndex(unsigned long n) const
      {
         return m_messageIDs[n];
      }
   //@}
private:
   unsigned long *m_messageIDs;
   unsigned long  m_number;
};


// ----------------------------------------------------------------------------
/** MEventFolderUpdate Data - Does not carry any data apart from pointer to
 * mailfolder.*/
class MEventFolderUpdateData : public MEventWithFolderData
{
public:
   /** Constructor.
   */
   MEventFolderUpdateData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderUpdate, folder)
      { }
private:
   MailFolder           *m_folder;
};
// ----------------------------------------------------------------------------
/** MEventFolderStatus Data - Does not carry any data apart from pointer to
 * mailfolder.*/
class MEventFolderStatusData : public MEventWithFolderData
{
public:
   /** Constructor.
   */
   MEventFolderStatusData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderStatus, folder)
      {}
};
// ----------------------------------------------------------------------------
/** MEventMsgStatus Data - Carries folder pointer and index
    in folder listing for data that changed.*/
class MEventMsgStatusData : public MEventWithFolderData
{
public:
   /** Constructor.
   */
   MEventMsgStatusData(MailFolder *folder,
                             size_t index,
                             HeaderInfoList *listing)
      : MEventWithFolderData(MEventId_MsgStatus, folder)
      {
         m_listing = listing;
         m_listing->IncRef();
         m_index = index;
      }
   ~MEventMsgStatusData()
      {
         m_listing->DecRef();
      }
   /// Get the new listing. Don't IncRef/DecRef it, exists as long as event.
   HeaderInfoList *GetHeaders() const { return m_listing; }
   /// get the folder which changed
   size_t GetIndex() const { return m_index; }
private:
   size_t                m_index;
   class HeaderInfoList  *m_listing;
};

// ----------------------------------------------------------------------------
// MEventOptionsChangeData - this event is generated whenever some of the
// program options change. It happens when the user clicks either of the
// buttons in the options dialog: Apply, Cancel or Ok. The program is supposed
// to apply the changes immediately in response to Apply, but be prepared to
// undo them if Cancel is received. Ok should do the same as Apply, but it
// can't be followed by Cancel. The profile pointer is the profile being
// modified and generally it should be safe to ignore all changes to the
// profile if it's not a parent of the profile in use.
//
// In response to "Apply" event (only) the event handler may prevent it from
// taking place by calling Veto() on the event object.
// ----------------------------------------------------------------------------

class MEventOptionsChangeData : public MEventData
{
public:
   // what happened?
   enum ChangeKind
   {
      Invalid,
      Apply,
      Cancel,
      Ok
   };

   // ctor
   MEventOptionsChangeData(class ProfileBase *profile,
                           ChangeKind what);
   
   // what happened?
   ChangeKind GetChangeKind() const { return m_what; }

   // which profile was modified?
   class ProfileBase *GetProfile() const { return m_profile; }

   // veto the event - "Apply" will be forbidden (logging a message explaining
   // why this happened together with calling this function is probably a good
   // idea)
   void Veto() { m_vetoed = true; }

   // event is allowed if Veto() hadn't been called
   bool IsAllowed() const { return !m_vetoed; }

   // dtor releases profile
   virtual ~MEventOptionsChangeData(); 

private:
   class ProfileBase *m_profile;
   ChangeKind         m_what;
   bool               m_vetoed;
};

// ----------------------------------------------------------------------------
// MEventFolderTreeChangeData - this event is generated whenever a new folder
// is created or an existing one is deleted or renamed
// ----------------------------------------------------------------------------

class MEventFolderTreeChangeData : public MEventData
{
public:
   /// what happened?
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

   /// get the fullname of the folder
   const String& GetFolderFullName() const { return m_fullname; }

   /// get the kind of the operation which just happened
   ChangeKind GetChangeKind() const { return m_what; }

private:
   String      m_fullname;
   ChangeKind  m_what;
};

// ----------------------------------------------------------------------------
/** MEventASFolderResult Data - Carries a ASMailFolder::Result object
    with the result of an asynchronous operation on the folder. */
class MEventASFolderResultData : public MEventData
{
public:
   /** Constructor.
   */
   MEventASFolderResultData(MObjectRC *result)
      : MEventData(MEventId_ASFolderResult)
      {
         m_ResultData = result;
         m_ResultData->IncRef();
      }
   ~MEventASFolderResultData()
      { m_ResultData->DecRef(); }
   ASMailFolder::Result *GetResult(void) const
      {
         m_ResultData->IncRef();
         return ( ASMailFolder::Result *) m_ResultData;
      } 
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

   // check that we had removed ourself from the list of event
   // handlers, also needed to handle derived classes destruction correctly
   virtual ~MEventReceiver();
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
