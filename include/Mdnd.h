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

/* there is no matching Mdnd.cpp yet
#ifdef __GNUG__
#   pragma interface "Mdnd.h"
#endif
*/

#include <wx/dataobj.h>

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
   MMessagesDataObject() : wxCustomDataObject(MMESSAGE_FORMAT)
   {
   }

   MMessagesDataObject(wxFolderView *view, const UIdArray& messages)
      : wxCustomDataObject(MMESSAGE_FORMAT)
   {
      // we store the wxFolderView pointer first followed bythe number of
      // messages - and then all messages after it
      size_t len = sizeof(MMessagesDataObject::Data) +
                     messages.GetCount()*sizeof(UIdType);
      void *buf = new char[len];
      Data *data = (Data *)buf;
      data->view = view;
      data->number = messages.GetCount();

      UIdType *p = GetUIDs(data);
      for ( size_t n = 0; n < data->number; n++ )
      {
         p[n] = messages[n];
      }

      TakeData(len, data);
   }

   // accessors
   wxFolderView *GetFolderView() const { return GetMData()->view; }
   size_t GetMessageCount() const { return GetMData()->number; }
   size_t GetMessageUId(size_t n) const { return GetUIDs(GetMData())[n]; }

   UIdArray GetMessages() const
   {
      UIdArray messages;
      size_t count = GetMessageCount();
      messages.Alloc(count);

      for ( size_t n = 0; n < count; n++ )
      {
         messages.Add(GetMessageUId(n));
      }

      return messages;
   }

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

#endif // _MDND_H_
