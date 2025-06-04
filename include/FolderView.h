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
#include "MailFolder.h"         // for OpenMode

class MailFolder;
class Profile;
class ASMailFolder;
class MFolder;

class WXDLLIMPEXP_FWD_CORE wxWindow;

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

   /** @name Operations */
   //@{

   /// open the folder in the folder view
   virtual bool OpenFolder(MFolder *folder, bool readonly = false) = 0;

   /// open this folder in the folder view
   virtual void SetFolder(MailFolder *mf) = 0;

   /**
      Move to the message with this number (this refers to the on screen order,
      not the internal msgno).
    */
   virtual bool GoToMessage(MsgnoType msgno) = 0;

   /**
      Move to the next unread message, if any.

      @param takeNextIfNoUnread if true, select next msg if no more unread
      @return true if we moved to another message, false otherwise
    */
   virtual bool MoveToNextUnread(bool takeNextIfNoUnread = true) = 0;

   //@}

   /** @name Accessors */
   //@{

   /// return full folder name
   const String& GetFullName() { return m_folderName; }

   /**
      GetFolderProfile() differs from GetProfile() in two ways: first, it
      always returns a non-NULL profile as it falls back to the application
      object profile (i.e. global one) if there is no opened folder in this
      folder view and, second, it returns the pointer properly IncRef()'d so
      that the caller must DecRef() it.

      This function should be used instead of GetProfile() in all new code!

      @return profile pointer, the caller must DecRef() it
    */
   virtual Profile *GetFolderProfile() const;

   /// return a profile pointer (NOT IncRef()'d!)
   Profile *GetProfile(void) const { return m_Profile; }

   /// return pointer to async mail folder (NOT IncRef()'d!)
   ASMailFolder *GetFolder(void) const { return m_ASMailFolder; }

   /// return pointer to associated mail folder (IncRef()'d as usual)
   MailFolder *GetMailFolder() const;

   /// Return true if we have an opened folder.
   bool HasFolder() const { return m_ASMailFolder != NULL; }

   //@}

   /// event processing function
   virtual bool OnMEvent(MEventData& ev);

   /// called when our message viewer changes
   virtual void OnMsgViewerChange(wxWindow *viewerNew) = 0;

protected:
   /// the derived class should close when our folder is deleted
   virtual void OnFolderDeleteEvent(const String& folderName) = 0;
   /// the derived class should update their display
   virtual void OnFolderUpdateEvent(MEventFolderUpdateData &event) = 0;
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
        *m_regCookieFolderClosed,
        *m_regCookieASFolderResult,
        *m_regCookieMsgStatus,
        *m_regCookieAppExit;
};

/// open a new frame containing the folder view of this folder
extern bool
OpenFolderViewFrame(MFolder *folder,
                    wxWindow *parent,
                    MailFolder::OpenMode openmode = MailFolder::Normal);

/// show a dialog allowing to choose the order of headers in the folder view
extern bool ConfigureFolderViewHeaders(Profile *profile, wxWindow *parent);

#endif // FOLDERVIEW_H
