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

#include <wx/dnd.h>

class wxFolderView;

/// the clipboard/dnd format for Mahogany messages
#define MMESSAGE_FORMAT "MMessage"

/**
  MMessagesDataObject is used for the mail messages transfer. It has a
  wxFolderView pointer and the list of UIDs which allows the drop target to
  call SaveMessagesToFolder() or do anything else it wishes with these
  messages.

  NB: this only works for intraprocess dnd, passing a wxFolderView pointer
      between processes won't work!
*/
class MMessagesDataObject : public wxCustomDataObject
{
public:
   MMessagesDataObject() : wxCustomDataObject(MMESSAGE_FORMAT) { }

   MMessagesDataObject(wxFolderView *view, const UIdArray& messages);

   // accessors
   wxFolderView *GetFolderView() const { return GetMData()->view; }
   size_t GetMessageCount() const { return GetMData()->number; }
   size_t GetMessageUId(size_t n) const { return GetUIDs(GetMData())[n]; }

   UIdArray GetMessages() const;

private:
   struct Data
   {
      wxFolderView *view;
      size_t number;

      // UIdType messages[]; -- a variable sized array follows
   };

   Data *GetMData() const { return ((Data *)GetData()); }
   UIdType *GetUIDs(Data *data) const { return (UIdType *)(data + 1); }
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
   // the frame to sue for status messages
   wxFrame *m_frame;
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

private:
   MMessagesDropWhere *m_where;
};

#endif // _MDND_H_
