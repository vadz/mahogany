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
class FolderView : public EventReceiver
{
public:
   FolderView()
   {
      m_regCookie = EventManager::Register(*this, EventId_FolderTreeChange);

      ASSERT_MSG( m_regCookie, "can't reg folder view with event manager");
   }

   /// update the user interface
   virtual void Update(void) = 0;
   /// virtual destructor
   virtual ~FolderView() { EventManager::Unregister(m_regCookie); }

   /// event processing function
   virtual bool OnEvent(EventData& ev)
   {
      if ( ev.GetId() == EventId_FolderTreeChange )
      {
         EventFolderTreeChangeData& event = (EventFolderTreeChangeData &)ev;

         if ( event.GetChangeKind() == EventFolderTreeChangeData::Delete )
            OnFolderDeleteEvent(event.GetFolderFullName());
      }

      return true;
   }

protected:
   /// the derived class should close when our folder is deleted
   virtual void OnFolderDeleteEvent(const String& folderName) = 0;

private:
   void *m_regCookie;
};
#endif
