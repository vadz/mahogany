/*-*- c++ -*-********************************************************
 * FolderView.h : a window which shows a MailFolder                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$
 *
 *******************************************************************/

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "MEvent.h"

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
      }
   /// update the user interface
   virtual void Update(void) = 0;
   /// virtual destructor
   virtual ~FolderView()
      {
         MEventManager::Deregister(m_regCookieTreeChange);
         MEventManager::Deregister(m_regCookieFolderUpdate);
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
      else if ( ev.GetId() == MEventId_FolderUpdate)
         OnFolderUpdateEvent((MEventFolderUpdateData&)ev);
  
      return true; // continue evaluating this event
   }

protected:
   /// the derived class should close when our folder is deleted
   virtual void OnFolderDeleteEvent(const String& folderName) = 0;
   /// the derived class should update their display
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event) = 0;

private:
   void *m_regCookieTreeChange, *m_regCookieFolderUpdate;
};
#endif
