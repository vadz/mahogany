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
   /// MEventMsgStatusData
   MEventId_MsgStatus = 402,
   /// MEventASFolderResult
   MEventId_ASFolderResult = 800,
   /// MEventOptionsChangeData
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
       the folder automatically! */
   MailFolder *GetFolder() const { return m_Folder; }
private:
   MailFolder *m_Folder;
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


/** MEventFolderUpdate Data - Does not carry any data apart from pointer to
    mailfolder.
 */
class MEventFolderUpdateData : public MEventWithFolderData
{
public:
   // ctor
   MEventFolderUpdateData(MailFolder *folder)
      : MEventWithFolderData(MEventId_FolderUpdate, folder)
      { }
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
                       HeaderInfo *hi)
      : MEventWithFolderData(MEventId_MsgStatus, folder)
      {
         m_index = index;
         m_hi = hi;
      }
   ~MEventMsgStatusData();

   /// Get the changed header info
   const HeaderInfo *GetHeaderInfo() const { return m_hi; }

   /// Get the index of the changed header in the listing
   size_t GetIndex() const { return m_index; }

private:
   size_t      m_index;
   HeaderInfo *m_hi;
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
