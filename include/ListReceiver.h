//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   ListReceiver.h: class for handling events from ListFolders()
// Purpose:     makes using ASMailFolder::ListFolders() less painful
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.10.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _LISTRECEIVER_H_
#define _LISTRECEIVER_H_

#include "MEvent.h"

// ----------------------------------------------------------------------------
// ListEventReceiver: derive form this class to be able to easily handle
//                    ASMailFolder::Op_ListFolders events
// ----------------------------------------------------------------------------

class ListEventReceiver : public MEventReceiver
{
public:
   // default ctor/dtor
   ListEventReceiver();
   virtual ~ListEventReceiver();

   /**
     Override this method to process an mm_list() notification for one folder.

     @param path the full path to the folder
     @param delim the folder hierarchy delimiter
     @param flags the attributes (combination of ASMailFolder::ATT_XXX values)
    */
   virtual void OnListFolder(const String& path, char delim, long flags) = 0;

   /**
     Override this to handle the end of the folder enumeration.
    */
   virtual void OnNoMoreFolders() = 0;

   
   // we redirect OnMEvent() to the methods above
   virtual bool OnMEvent(MEventData& event);

private:
   // MEventReceiver cookie for the event manager
   void *m_regCookie;

};

#endif // _LISTRECEIVER_H_
