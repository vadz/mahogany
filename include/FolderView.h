/*-*- c++ -*-********************************************************
 * FolderView.h : a window which shows a MailFolder                 *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (Ballueder@gmx.net)            *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "MEvent.h"

class MailFolder;
class Profile;
class ASMailFolder;

/**
   FolderView class, a window displaying a MailFolder.
*/
class FolderView : public MEventReceiver
{
public:
   FolderView()
      {
         m_regCookieTreeChange = MEventManager::Register(*this, MEventId_FolderTreeChange);
         ASSERT_MSG( m_regCookieTreeChange, "can't reg folder view with event manager");
         m_regCookieFolderUpdate = MEventManager::Register(*this, MEventId_FolderUpdate);
         ASSERT_MSG( m_regCookieFolderUpdate, "can't reg folder view with event manager");
         m_regCookieMsgStatus = MEventManager::Register(*this, MEventId_MsgStatus);
         ASSERT_MSG( m_regCookieMsgStatus, "can't reg folder view with event manager");
         m_regCookieFolderStatus = MEventManager::Register(*this, MEventId_FolderStatus);
         ASSERT_MSG( m_regCookieFolderUpdate, "can't reg folder view with event manager");
         m_regCookieASFolderResult = MEventManager::Register(*this, MEventId_ASFolderResult);
         ASSERT_MSG( m_regCookieFolderUpdate, "can't reg folder view with event manager");
      }
   /// update the user interface
   virtual void Update(class HeaderInfoList *list = NULL) = 0;
   /// deregister event handlers
   virtual void DeregisterEvents(void)
      {
         MEventManager::Deregister(m_regCookieTreeChange);
         MEventManager::Deregister(m_regCookieFolderUpdate);
         MEventManager::Deregister(m_regCookieMsgStatus);
         MEventManager::Deregister(m_regCookieFolderStatus);
         MEventManager::Deregister(m_regCookieASFolderResult);
         m_regCookieTreeChange = NULL;
      }
   /// virtual destructor
   virtual ~FolderView()
      {
         ASSERT( m_regCookieTreeChange == NULL);

      }

   /// event processing function
   virtual bool OnMEvent(MEventData& ev)
   {
      if ( ev.GetId() == MEventId_FolderTreeChange )
      {
         MEventFolderTreeChangeData& event = (MEventFolderTreeChangeData &)ev;
         if ( event.GetChangeKind() == MEventFolderTreeChangeData::Delete )
            OnFolderDeleteEvent(event.GetFolderFullName());
      }
      else if ( ev.GetId() == MEventId_ASFolderResult )
         OnASFolderResultEvent((MEventASFolderResultData &) ev );
      else if ( ev.GetId() == MEventId_MsgStatus )
         OnMsgStatusEvent((MEventMsgStatusData&)ev );
      else if ( ev.GetId() == MEventId_FolderUpdate )
         OnFolderUpdateEvent((MEventFolderUpdateData&)ev );
      else if ( ev.GetId() == MEventId_FolderStatus )
         OnFolderStatusEvent((MEventFolderStatusData&)ev );
      return true; // continue evaluating this event
   }

   /// return full folder name
   const String& GetFullName() { return m_folderName; }

   /// return a profile pointer:
   Profile *GetProfile(void) const { return m_Profile; }

   /// return pointer to folder
   ASMailFolder * GetFolder(void) const { return m_ASMailFolder; }

protected:
   /// the derived class should close when our folder is deleted
   virtual void OnFolderDeleteEvent(const String& folderName) = 0;
   /// the derived class should update their display
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event) = 0;
   /// the derived class should update their display
   virtual void OnFolderStatusEvent(MEventFolderStatusData &event) = 0;
   /// the derived class should update their display
   virtual void OnMsgStatusEvent(MEventMsgStatusData &event) = 0;
   /// the derived class should react to the result to an asynch operation
   virtual void OnASFolderResultEvent(MEventASFolderResultData &event) = 0;

   /// the profile we use for our settings
   Profile *m_Profile;
   /// full folder name of the folder we show
   String m_folderName;
   /// the mail folder being displayed
   ASMailFolder *m_ASMailFolder;
   MailFolder *m_MailFolder;
   
private:
   void *m_regCookieTreeChange, *m_regCookieFolderUpdate,
      *m_regCookieASFolderResult, *m_regCookieMsgStatus,
      *m_regCookieFolderStatus;
};
#endif
