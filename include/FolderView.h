///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   FolderView.h - interface of the folder view class
// Author:      Karsten Ballüder
// Modified by:
// Created:     1997
// CVS-ID:      $Id$
// Copyright:   (C) 1997-2000 by Karsten Ballüder (Ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "MEvent.h"

class MailFolder;
class Profile;
class ASMailFolder;
class wxWindow;

/**
   FolderView class, a window displaying a MailFolder.
*/
class FolderView : public MEventReceiver
{
public:
   FolderView();

   /// deregister event handlers
   virtual void DeregisterEvents(void);

   /// virtual destructor
   virtual ~FolderView();

   /// event processing function
   virtual bool OnMEvent(MEventData& ev);

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
   /// update the folderview
   virtual void OnFolderStatusEvent(MEventFolderStatusData &event) = 0;
   /// update the folderview
   virtual void OnFolderExpungeEvent(MEventFolderExpungeData &event) = 0;
   /// close the folderview
   virtual void OnFolderClosedEvent(MEventFolderClosedData &event) = 0;
   /// the derived class should update their display
   virtual void OnMsgStatusEvent(MEventMsgStatusData &event) = 0;
   /// the derived class should react to the result of an async operation
   virtual void OnASFolderResultEvent(MEventASFolderResultData &event) = 0;
   /// we remember that we were open when the app exists
   virtual void OnAppExit();

   /// the profile we use for our settings
   Profile *m_Profile;

   /// full folder name of the folder we show
   String m_folderName;

   /// the mail folder being displayed
   ASMailFolder *m_ASMailFolder;

   /// the mail folder associated with m_ASMailFolder
   MailFolder *m_MailFolder;

private:
   void *m_regCookieTreeChange,
        *m_regCookieFolderUpdate,
        *m_regCookieFolderExpunge,
        *m_regCookieFolderStatus,
        *m_regCookieFolderClosed,
        *m_regCookieASFolderResult,
        *m_regCookieMsgStatus,
        *m_regCookieAppExit;
};

/// open a new frame containing the folder view of this folder
extern bool OpenFolderViewFrame(const String& name, wxWindow *parent);

/// show a dialog allowing to choose the order of headers in the folder view
extern bool ConfigureFolderViewHeaders(class Profile *profile, wxWindow *parent);

#endif // FOLDERVIEW_H
