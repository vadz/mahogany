///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MsgCmdProc.h: declaration of MsgCmdProc
// Purpose:     MsgCmdProc is a non GUI class which performs processing
//              of the commands related to the message viewer and is needed
//              because it allows us to resue the same code in both
//              wxFolderView and wxMessageViewFrame (we can't put MsgCmdProc
//              code into MessageView because it might not be even created yet
//              because many of the commands make sense even before we preview
//              the message)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     01.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     Mahogany license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MSGCMDPROC_H_
#define _MSGCMDPROC_H_

class ASMailFolder;
class MessageView;
class MFolder;
class UIdArray;

class WXDLLEXPORT wxFrame;

#include "MEvent.h"

// ----------------------------------------------------------------------------
// MsgCmdProc
// ----------------------------------------------------------------------------

class MsgCmdProc : public MEventReceiver
{
public:
   /**
      Create a MsgCmdProc object

      @param msgView the message view window we process messages for
      @parent winForDnd the window used as source for message dragging
      @return MsgCmdProc pointer which must be deleted by the caller
    */
   static MsgCmdProc *Create(MessageView *msgView,
                             wxWindow *winForDnd = NULL);

   /**
     Set the folder to use (if the folder is NULL no messages will be
     processed, so it should be set before calling ProcessCommand)
    */
   virtual void SetFolder(ASMailFolder *asmf) = 0;

   /**
     Set the frame to use for status messages and (as parent for) dialogs
    */
   virtual void SetFrame(wxFrame *frame) = 0;

   /**
     Set the window to use for DnD
    */
   virtual void SetWindowForDnD(wxWindow *winForDnd) = 0;

   /**
     Process the given command for all messages in the array

     @param id the menu command (WXMENU_MSG_XX
     @param messages the UIDs of messages to apply the command to
     @param folder the target folder for some commands (ask if NULL)
     @return true if the command was processed
    */
   virtual bool ProcessCommand(int id,
                               const UIdArray& messages,
                               MFolder *folder = NULL) = 0;

   /// get the name of our folder (only used by MMessagesDropTarget)
   virtual String GetFolderName() const = 0;

private:
   // ctor is private, only Create() can create us
};

#endif // _MSGCMDPROC_H_

