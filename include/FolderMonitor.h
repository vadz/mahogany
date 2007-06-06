///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   FolderMonitor.h - class which pings folders periodically
// Purpose:     declares FolderMonitor class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.11.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _FOLDERMONITOR_H_
#define _FOLDERMONITOR_H_

class MFolder;

/**
  FolderMonitor maintains the list of folders we are configured to check
  periodically and does the necessary action for each of them when its timeout
  expires from CheckNewMail() function which is supposed to be called by the
  GUI code.

  This is a singleton class: FolderMonitor::Create() is used by mApplication
  only to get the unique instance of FolderMonitor and mApplication is also
  responsible for deleting it at the end of the program.

  NB: FolderMonitor extends and replaces the old MailCollector class
 */
class FolderMonitor
{
public:
   /// the flags for CheckNewMail()
   enum
   {
      /// don't give any messages nor show any dialogs
      Silent      = 0,

      /// check all opened folders
      Opened      = 1,

      /// interactive mode: be verbose
      Interactive = 2
   };

   /**
     Check for new mail.

     @param flags determine the options
     @return true if we got any new mail, false otherwise
    */
   virtual bool CheckNewMail(int flags = 0) = 0;

   /**
     Adds or removes this folder to/from the list of folders to monitor.
     It doesn't take the ownership of the pointer passed to it.

     @param folder the folder to monitor or stop monitoring
     @return true if ok, false on error
    */
   virtual bool AddOrRemoveFolder(MFolder *folder, bool add) = 0;

   /**
     Return the smallest poll timeout for all folders we are polling.
     This is used as the global poll timer interval.

     @return smallest poll timeout in seconds
    */
   virtual long GetMinCheckTimeout(void) const = 0;

protected:
   /// only MAppBase can delete us, hence dtor is protected
   virtual ~FolderMonitor();

private:
   /// create the unique instance of FolderMonitor, may be called only once
   static FolderMonitor *Create();

   // it can call our Create() and delete us
   friend class MAppBase;
};

#endif // _FOLDERMONITOR_H_

