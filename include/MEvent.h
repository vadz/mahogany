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

#include "MObject.h"

/**
   MEvent ids for all kinds of events used in M. The name in the comment is the
   MEventData-derived class which is used with this event
*/
enum MEventId
{
   /// (invalid id for an event)
   MEventId_Null = -1,
   /// MEventMailData
   MEventId_NewMail = 100,
   /// MEventFolderOnNewMailData - used by MailFolderCC for private purposes
   MEventId_FolderOnNewMail = 110,
   /// MEventFolderTreeChangeData - a change in folder tree
   MEventId_FolderTreeChange = 200,
   /// MEventFolderUpdateData - there's a new folder listing
   MEventId_FolderUpdate = 400,
   /// MEventFolderExpunge - messages were expunged from the folder
   MEventId_FolderExpunge,
   /// MEventId_FolderClosed - the folder was closed (by user)
   MEventId_FolderClosed,
   /// MEventMsgStatusData - message status changed
   MEventId_MsgStatus,
   /// MEventFolderStatusData - folder status changed
   MEventId_FolderStatus,
   /// MEventASFolderResult - async operation terminated
   MEventId_ASFolderResult = 800,
   /// MEventOptionsChangeData - the program options changed
   MEventId_OptionsChange,
   /// MEventPing - causes folders to ping themselves
   MEventId_Ping,
   /// MEventNewADB - causes the ADB editor to update itself
   MEventId_NewADB,
   /// MEventData (no special data) - notifies everybody that the app closes
   MEventId_AppExit = 1000,
   /// (invalid id for an event)
   MEventId_Max
};

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

// forward decl
class MailFolder;

// FIXME: this shouldn't be needed! Opaque type?
#include "ASMailFolder.h"

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
       the folder automatically!
    */
   MailFolder *GetFolder() const { return m_Folder; }

private:
   MailFolder *m_Folder;
};

/// MEventFolderOnNewMailData - used by MailFolderCC to update itself
class MEventFolderOnNewMailData : public MEventWithFolderData
{
public:
   MEventFolderOnNewMailData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderOnNewMail, folder) { }
};

/// MEventPingData - the event asking the folders to ping
class MEventPingData : public MEventData
{
public:
   MEventPingData() : MEventData( MEventId_Ping ) {}
};

/// MEventNewADBData - the event telling ADB editor to refresh
class MEventNewADBData : public MEventData
{
public:
   MEventNewADBData(const String& adbname,
                    const String& provname)
      : MEventData(MEventId_NewADB),
        m_adbname(adbname), m_provname(provname)
   {
   }

   const String& GetAdbName() const { return m_adbname; }
   const String& GetProviderName() const { return m_provname; }

private:
   String m_adbname, m_provname;
};

/// MEventNewMailData - the event notifying the app about "new mail"
class MEventNewMailData : public MEventWithFolderData
{
public:
   /** Constructor.
       @param folder the mail folder where the new mail was detected
       @param n number of new messages
       @param messageIDs points to an array of the sequence numbers of
       the new messages in that folder, freed by the caller
   */
   MEventNewMailData(MailFolder *folder,
                     unsigned long n,
                     UIdType *messageIDs);
   ~MEventNewMailData();

   /**@name accessors */
   //@{
   /// get the number of new messages
   unsigned long GetNumber(void) const { return m_number; }
   /// get the index of the Nth new message (for calling GetMessage)
   UIdType GetNewMessageIndex(unsigned long n) const
      {
         return m_messageIDs[n];
      }
   //@}
private:
   UIdType *m_messageIDs;
   unsigned long  m_number;
};


/** MEventFolderUpdate Data - Does not carry any data apart from pointer to
    mailfolder.

    This event should have been called MEventFolderNewMailData as it is sent by
    MailFolderCC only when new mail arrives to the folder.
 */
class MEventFolderUpdateData : public MEventWithFolderData
{
public:
   // ctor
   MEventFolderUpdateData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderUpdate, folder)
      {
      }
};

/**
   MEventFolderExpungeData: notifies about message expunge
 */
class MEventFolderExpungeData : public MEventWithFolderData
{
public:
   // ctor takes the array of deleted msgnos which will be deleted by us
   MEventFolderExpungeData(MailFolder *folder,
                           wxArrayInt *delMsgnos,
                           wxArrayInt *delPositions)
      : MEventWithFolderData(MEventId_FolderExpunge, folder)
      {
         m_delMsgnos = delMsgnos;
         m_delPositions = delPositions;
      }

   // free msgno array
   virtual ~MEventFolderExpungeData()
   {
      delete m_delMsgnos;
      delete m_delPositions;
   }

   // return the number of messages deleted
   size_t GetCount() const { return m_delMsgnos ? m_delMsgnos->GetCount() : 0; }

   // return the msgno of n-th deleted item
   size_t GetItem(size_t n) const { return m_delMsgnos->Item(n); }

   // return the position in the listing of the n-th deleted item
   size_t GetItemPos(size_t n) const { return m_delPositions->Item(n); }

private:
   wxArrayInt *m_delMsgnos,
              *m_delPositions;
};

/**
 MEventFolderClosedData: notifies about folder closure
 */
class MEventFolderClosedData : public MEventWithFolderData
{
public:
   // ctor takes the array of deleted msgnos which will be deleted by us
   MEventFolderClosedData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderClosed, folder)
      {
      }
};

/**
   MEventMsgStatus Data - carries folder pointer and HeaderInfo object with
   its index in the folder list
 */

class HeaderInfo;
class MEventMsgStatusData : public MEventWithFolderData
{
public:
   // ctor
   MEventMsgStatusData(MailFolder *folder,
                       size_t index,
                       const HeaderInfo& hi);

   ~MEventMsgStatusData();

   /// Get the changed header info
   const HeaderInfo *GetHeaderInfo() const { return m_hi; }

   /// Get the index of the changed header in the listing
   size_t GetIndex() const { return m_index; }

private:
   size_t m_index;

   class HeaderInfo *m_hi;
};

/**
   MEventFolderStatusData is sent when the number of messages in folder
   (or the number of new, unread or other interesting categories of messages)
   changes. It is processed by the folder tree. When this event is received,
   you can query MfStatusCache for the number of messages in the folder.

   The difference between this event and FolderUpdate one is that the latter is
   only sent for an open folder and only if new mail appears in it (similarly,
   FolderExpunge event is only sent when messages are expunged) while this
   event is sent even if the folder is not opened and in both cases.
 */
class MEventFolderStatusData : public MEventData
{
public:
   MEventFolderStatusData(const String& folderName)
      : MEventData(MEventId_FolderStatus),
        m_folderName(folderName)
      {
      }

   // get the full name of the folder which was updated
   const String& GetFolderName() const { return m_folderName; }

private:
   String m_folderName;
};

/**
   MEventOptionsChangeData - this event is generated whenever some of the
   program options change. It happens when the user clicks either of the
   buttons in the options dialog: Apply, Cancel or Ok. The program is supposed
   to apply the changes immediately in response to Apply, but be prepared to
   undo them if Cancel is received. Ok should do the same as Apply, but it
   can't be followed by Cancel. The profile pointer is the profile being
   modified and generally it should be safe to ignore all changes to the
   profile if it's not a parent of the profile in use.

   In response to "Apply" event (only) the event handler may prevent it from
   taking place by calling Veto() on the event object.
 */

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
   MEventOptionsChangeData(class Profile *profile,
                           ChangeKind what);

   // what happened?
   ChangeKind GetChangeKind() const { return m_what; }

   // which profile was modified?
   class Profile *GetProfile() const { return m_profile; }

   // veto the event - "Apply" will be forbidden (logging a message explaining
   // why this happened together with calling this function is probably a good
   // idea)
   void Veto() { m_vetoed = true; }

   // event is allowed if Veto() hadn't been called
   bool IsAllowed() const { return !m_vetoed; }

   // dtor releases profile
   virtual ~MEventOptionsChangeData();

private:
   class Profile *m_profile;
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
      Create,     // folder "fullname" was created
      Delete,     //                       deleted
      Rename,     //                       renamed
      CreateUnder // some folders were created under folder "fullname"
   };

   MEventFolderTreeChangeData(const String& fullname,
                              ChangeKind what,
                              const String& newname = "")
      : MEventData(MEventId_FolderTreeChange),
        m_fullname(fullname),
        m_newname(newname)
      {
         m_what = what;
      }

   /// get the fullname of the folder
   const String& GetFolderFullName() const { return m_fullname; }

   /// get the new folder name (for Rename operation only)
   const String& GetNewFolderName() const { return m_newname; }

   /// get the kind of the operation which just happened
   ChangeKind GetChangeKind() const { return m_what; }

private:
   String      m_fullname,
               m_newname;
   ChangeKind  m_what;
};

/** MEventASFolderResult Data - Carries a ASMailFolder::Result object
    with the result of an asynchronous operation on the folder.
 */
class MEventASFolderResultData : public MEventData
{
public:
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


/**
   Derive from this class to be able to process events.
 */
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

/**
   This class allows to send events and forwards them to the receivers waiting
   for them. There is only one object of this class in the whole program and
   even this one is never used directly - all functions are static.
 */

class MEventManager
{
public:
   MEventManager();

   /// send an event to the queue, it will be processed some time later
   static void Send(MEventData *data);

   /// dispatches the event immediately, return false if suspended
   static bool Dispatch(MEventData *data);

   /// dispatches all events in the queue.
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

   // convenience wrapper for Register() above: register the same object for
   // several events putting the cookies into provided pointers and stopping
   // at MEventId_Null, i.e. you should call it like this:
   //
   //    Register(this, MEventId_1, &handle, MEventId_Null);
   //
   // return TRUE if all succeeded
   static bool RegisterAll(MEventReceiver *who,
                           /* MEventId eventId, void **pHandle, */
                           ...);

   // convenience wrapper for Deregister(): deregisters all handles and NULLs
   // them, stops at the NULL pointer
   static bool DeregisterAll(void **pHandle, ...);

   /// Temporarily suspend (enable/disable) event dispatching:
   static void Suspend(bool suspended = TRUE);
};

// ----------------------------------------------------------------------------
// MEventManagerSuspender: temporarily suspend processing the events
// ----------------------------------------------------------------------------

class MEventManagerSuspender
{
public:
   MEventManagerSuspender() { MEventManager::Suspend(true); }
   ~MEventManagerSuspender() { MEventManager::Suspend(false); }
};

#endif // MEVENT_H
