//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFPrivate.h: MailFolder class hierarchy helpers
// Purpose:     misc internal helpers for MailFolder[{Cmn|CC}]
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.11.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#include "MEvent.h"

/**
   MFSuspendInteractivity: resets the folders interactive frame thus preventing
   any GUI dialogs from showing showing during the life time of this object
*/

class MFSuspendInteractivity
{
public:
   // we will DecRef() the mail folder which must be !NULL
   MFSuspendInteractivity(MailFolder *mf)
   {
      m_mf = mf;
      if ( m_mf )
      {
         m_frameOld = mf->SetInteractiveFrame(NULL);
      }
      else
      {
         FAIL_MSG( _T("NULL folder in MFInteractiveLock") );
      }
   }

   ~MFSuspendInteractivity()
   {
      if ( m_mf )
      {
         (void)m_mf->SetInteractiveFrame(m_frameOld);

         m_mf->DecRef();
      }
   }

private:
   MailFolder *m_mf;
   wxFrame *m_frameOld;
};

/**
   This class is used to restore the folders interactive frame "later", i.e.
   during the next idle loop iteration. To do this, we just send an event of
   this type using MEventManager. Although nobody processes it, the event
   object is still deleted and restores the mail folders interactive frame in
   its dtor,

   It is similar in spirit to CCEventReflector in MailFolderCC.cpp (see the
   detailed comment there) but here it doesn't even have to reflect anything
   back as it can do the job itself.
*/

class MEventFolderRestoreInterData : public MEventData
{
public:
   MEventFolderRestoreInterData(MFSuspendInteractivity *suspender)
   {
      m_suspender = suspender;
   }

   virtual ~MEventFolderRestoreInterData() { delete m_suspender; }

private:
   MFSuspendInteractivity *m_suspender;
};

/**
   This class sends MEventFolderRestoreInterData if necessary: it suspends the
   interactive notifications in its ctor and sends an event to restore them
   (using the logic described above) in its dtor
 */
class NonInteractiveLock
{
public:
   NonInteractiveLock(MailFolder *mf, bool interactive)
   {
      if ( !interactive )
      {
         /*
            We need to suppress any GUI manifestations while checking mail in
            this folder (presumably because we are called in the result of a
            background check and so the user is doing something completely
            different right now and would be very annoyed if we started popping
            up dialogs at him).

            This is not really easy to do as Ping() below will at most result
            in mm_exists() notification and hence MailFolderCC::OnMailExists()
            will be called but it doesn't do anything except sending an event
            to itself which will result in a call to OnNewMail() later.

            So, although we want to reset the interactivity during OnNewMail()
            execution, it won't start (hence it won't end) before we return and
            we need to restore the normal interactive frame after it is done.
            To do this, we set interactive frame to NULL now, wait until Ping()
            finishes (and hence [possibly] sends the MEventFolderOnNewMail
            event) and then send our own event to ourselves which tells us to
            restore the old interactive frame.
          */

         // this will set interactive frame to NULL for mf
         mf->IncRef();
         m_suspender = new MFSuspendInteractivity(mf);
      }
      else // no need to do anything
      {
         m_suspender = NULL;
      }
   }

   ~NonInteractiveLock()
   {
      // send the event to restore the interactive frame if necessary
      if ( m_suspender )
      {
         // event won't be processed at all but MEventFolderRestoreInterData
         // dtor will do the job of restoring the interactive frame
         // nevertheless
         MEventManager::Send(new MEventFolderRestoreInterData(m_suspender));
      }
   }

private:
   MFSuspendInteractivity *m_suspender;
};

/**
   MFSubSystem: define an object of this class if you want to be called
   during the mail subsystem initialization and/or shutdown
 */
class MFSubSystem
{
public:
   /// return the first initializer in the linked list or NULL if none
   static MFSubSystem *GetFirst() { return ms_initilizers; }

   /// return the next initializer in the linked list or NULL if no more
   MFSubSystem *GetNext() const { return m_next; }

   typedef bool (*InitFunction)();
   typedef void (*CleanupFunction)();

   MFSubSystem(InitFunction init, CleanupFunction cleanup)
      : m_next(ms_initilizers),
        m_init(init),
        m_cleanup(cleanup)
   {
      ms_initilizers = this;
   }

   bool Init() { return m_init ? (*m_init)() : true; }
   void CleanUp() { if ( m_cleanup ) (*m_cleanup)(); }

private:
   static MFSubSystem *ms_initilizers;

   MFSubSystem * const m_next;

   InitFunction m_init;
   CleanupFunction m_cleanup;

   DECLARE_NO_COPY_CLASS(MFSubSystem)
};


