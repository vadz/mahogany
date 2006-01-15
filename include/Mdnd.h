///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   Mdnd.h - drag and drop related classes
// Purpose:     data objects for DND
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.04.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MDND_H_
#define _MDND_H_

#ifdef __GNUG__
   #pragma interface "Mdnd.h"
#endif

#include "MailFolder.h"

#include <wx/dnd.h>

#if wxUSE_DND

class MsgCmdProc;
class MailFolder;

/// the clipboard/dnd format for Mahogany messages
#define MMESSAGE_FORMAT _T("MMessage")

/**
  MMessagesDataObject is used for the mail messages transfer. It has a
  MsgCmdProc pointer (on which it will call ProcessCommand(DROP)) and the list
  of UIDs which allows the drop target to call SaveMessagesToFolder() or do
  anything else it wishes with these messages.

  NB: this only works for intraprocess dnd, passing a MsgCmdProc pointer
      between processes won't work!
*/
class MMessagesDataObject : public wxCustomDataObject
{
public:
   MMessagesDataObject() : wxCustomDataObject(MMESSAGE_FORMAT) { }

   MMessagesDataObject(MsgCmdProc *msgProc,
                       MailFolder *folder,
                       const UIdArray& messages);

   virtual ~MMessagesDataObject();

   // accessors
   MsgCmdProc *GetMsgCmdProc() const { return GetMData()->msgProc; }

   /// return the inc-refed folder pointer
   MailFolder *GetFolder() const
   {
      MailFolder *mf = GetMData()->folder;
      SafeIncRef(mf);
      return mf;
   }

   size_t GetMessageCount() const { return GetMData()->number; }
   size_t GetMessageUId(size_t n) const { return GetUIDs(GetMData())[n]; }

   UIdArray GetMessages() const;

private:
   struct Data
   {
      MailFolder *folder;
      MsgCmdProc *msgProc;
      size_t number;

      // UIdType messages[]; -- a variable sized array follows
   };

   Data *GetMData() const { return ((Data *)GetData()); }
   UIdType *GetUIDs(Data *data) const { return (UIdType *)(data + 1); }

   DECLARE_NO_COPY_CLASS(MMessagesDataObject)
};

/** MMessagesDropWhere is a helper class for MMessagesDropTarget, it has only
    one pure virtual function which must be overridden to return the folder we
    should drop messages to.
 */
class MMessagesDropWhere
{
public:
   /// return the folder which should be used to copy messages to (may be NULL)
   virtual MFolder *GetFolder(wxCoord x, wxCoord y) const = 0;

   /// called if the messages were successfully copied
   virtual void Refresh();

   virtual ~MMessagesDropWhere();
};

/** MMessagesDropTargetBase is a common base class for drop targets and is not
    used directly itself */
class MMessagesDropTargetBase : public wxDropTarget
{
public:
   MMessagesDropTargetBase(wxWindow *win);

   /// overridden base class virtuals
   virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def);
   virtual void OnLeave();
   virtual wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);

   /// the method to override in the derived classes
   virtual wxDragResult OnMsgDrop(wxCoord x, wxCoord y,
                                  MMessagesDataObject *data,
                                  wxDragResult def) = 0;

protected:
   // get the frame to use for status messages
   wxFrame *GetFrame() const { return m_frame; }

private:
   wxFrame *m_frame;

   DECLARE_NO_COPY_CLASS(MMessagesDropTargetBase)
};

/** MMessagesDropTarget is the drop target object to work with
    MMessagesDataObjects. It needs a window to register itself with and an
    object of class MMessagesDropWhere which is used to retrieve the folder to
    drop messages to (and which will be deleted by the drop target)
 */
class MMessagesDropTarget : public MMessagesDropTargetBase
{
public:
   /// ctor
   MMessagesDropTarget(MMessagesDropWhere *where, wxWindow *win);

   /// overridden base class virtuals
   virtual wxDragResult OnMsgDrop(wxCoord x, wxCoord y,
                                  MMessagesDataObject *data, wxDragResult def);

   /// will delete the MMessagesDropWhere object
   virtual ~MMessagesDropTarget();

protected:
   MMessagesDropWhere *GetDropWhere() const { return m_where; }

private:
   MMessagesDropWhere *m_where;

   DECLARE_NO_COPY_CLASS(MMessagesDropTarget)
};

#endif // wxUSE_DND

#endif // _MDND_H_
