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

class ASMailFolder;

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
     Call OnListFolder() for all folders under the given one. Note that if this
     method is used, then the path in OnListFolder() will be just the folder
     path and not the full IMAP spec as when asmf->ListFolders() is called
     directly (the possibility to do the latter is kept just for the backwards
     compatibility with the code in wxSubfoldersDialog.cpp).

     @param asmf the folder to list the subfolders of
     @return true if ok, false if ListFolders() failed
    */
   bool ListAll(ASMailFolder *asmf);

   /**
     Override this method to process an mm_list() notification for one folder.

     @param path the full path to the folder (may include IMAP spec or not)
     @param delim the folder hierarchy delimiter
     @param flags the attributes (combination of ASMailFolder::ATT_XXX values)
    */
   virtual void OnListFolder(const String& path, wxChar delim, long flags) = 0;

   /**
     Override this to handle the end of the folder enumeration.
    */
   virtual void OnNoMoreFolders() = 0;


   // we redirect OnMEvent() to the methods above
   virtual bool OnMEvent(MEventData& event);

private:
   // MEventReceiver cookie for the event manager
   void *m_regCookie;

   // the IMAP spec of the folder we're listing the subfolders of or empty
   String m_specRoot;
};

#endif // _LISTRECEIVER_H_
