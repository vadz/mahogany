//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MailFolderCmn.h: provide some meat on MailFolder bones
// Purpose:     MailFolderCmn is a normal base class implementing the methods
//              of MailFolder ABC for which we can write a reasonable generic
//              implementation
// Author:      Karsten Ballüder, Vadim Zeitlin
// Modified by:
// Created:     1998
// CVS-ID:      $Id$
// Copyright:   (C) 1998-2000 by Karsten Ballüder (ballueder@gmx.net)
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef MAILFOLDERCMN_H
#define MAILFOLDERCMN_H

#ifdef __GNUG__
#   pragma interface "MailFolderCmn.h"
#endif

#include "MailFolder.h" // the base class

#include "MEvent.h"     // for MEventOptionsChangeData

// define this for some additional checks of folder closing logic
#undef DEBUG_FOLDER_CLOSE

/**
   MailFolderCmn  class, common code shared by all implementations of
   the MailFolder ABC.
*/
class MailFolderCmn : public MailFolder
{
public:
   /// do status caching and call DoCountMessages() to do the real work
   virtual bool CountAllMessages(MailFolderStatus *status) const;

   /**@name Some higher level functionality implemented by the
      MailFolderCmn class on top of the other functions.
   */
   //@{
   /** Save messages to a folder identified by MFolder

       @param selections pointer to an array holding the message UIDs
       @param folder is the folder to save to, can't be NULL
       @return true if messages got saved
   */
   virtual bool SaveMessages(const UIdArray *selections, MFolder *folder);

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param folderName the name of the folder to save to
       @param isProfile if true, the folderName will be interpreted as
       a symbolic folder name, otherwise as a filename
       @return true on success
   */
   virtual bool SaveMessages(const UIdArray *selections,
                             const String& folderName);

   /** Save the messages to a folder.
       @param selections the message indices which will be converted using the current listing
       @param fileName the name of the folder to save to
       @return true on success
   */
   virtual bool SaveMessagesToFile(const UIdArray *selections,
                                   const String& fileName,
                                   MWindow *parent = NULL);

   /** Mark messages as deleted or move them to trash.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool DeleteOrTrashMessages(const UIdArray *messages);

   /** Mark messages as deleted.
       @param messages pointer to an array holding the message numbers
       @param expunge expunge messages after deletion
       @return true on success
   */
   virtual bool DeleteMessages(const UIdArray *messages, bool expunge=false);

   /** Mark messages as no longer deleted.
       @param messages pointer to an array holding the message numbers
       @return true on success
   */
   virtual bool UnDeleteMessages(const UIdArray *messages);

   /**@name Old-style functions, try to avoid. */
   //@{

   /** Set flags on a messages. Possible flag values are MSG_STAT_xxx
       @param uid mesage uid
       @param flag flag to be set, e.g. "\\Deleted"
       @param set if true, set the flag, if false, clear it
       @return always true UNSUPPORTED!
   */
  virtual bool SetMessageFlag(unsigned long uid,
                              int flag,
                              bool set = true)
      {
         return SetSequenceFlag(strutil_ultoa(uid),flag,set);
      }

   /** Delete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   virtual bool DeleteMessage(unsigned long uid);

   /** UnDelete a message.
       @param uid mesage uid
       @return always true UNSUPPORTED!
   */
   virtual inline bool UnDeleteMessage(unsigned long uid)
      { SetMessageFlag(uid,MSG_STAT_DELETED, false); return true; }

   //@}
   /** Reply to selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
       @param flags 0, or REPLY_FOLLOWUP
   */
   virtual void ReplyMessages(const UIdArray *messages,
                              const Params& params,
                              MWindow *parent = NULL);

   /** Forward selected messages.
       @param messages pointer to an array holding the message numbers
       @param parent window for dialog
       @param profile pointer for environment
   */
   virtual void ForwardMessages(const UIdArray *messages,
                                const Params& params,
                                MWindow *parent = NULL);
   //@}


   virtual HeaderInfoList *GetHeaders(void) const;

   virtual inline void GetAuthInfo(String *login, String *password) const
      { *login = m_Login; *password = m_Password; }

   /** Apply any filter rules to the folder.
       Applies the rule to all messages listed in msgs.
       @return -1 if no filter module exists, return code otherwise
   */
   virtual int ApplyFilterRules(UIdArray msgs);

   /** Update the folder to correspond to the new parameters: called from
       Options_Change MEvent handler.

       @param kind the change kind (ok/apply/cancel)
   */
   virtual void OnOptionsChange(MEventOptionsChangeData::ChangeKind kind);

   /// VZ: adding this decl as it doesn't compile otherwise
   virtual void Checkpoint(void) = 0;

   virtual MFrame *SetInteractiveFrame(MFrame *frame);
   virtual MFrame *GetInteractiveFrame() const;

   virtual void SuspendUpdates() { m_suspendUpdates++; }
   virtual void ResumeUpdates() { if ( !--m_suspendUpdates ) RequestUpdate(); }

   /** @name Delayed folder closing

       We may want to keep alive the folder for a while instead of closing it
       immediately, so we artificially bump up its reference count and
       decrement it back later when the keep alive timeout expires
    */
   //@{
   /// decrement and delete if reached 0, return TRUE if item wasn't deleted
   virtual void IncRef();
   virtual bool DecRef();
private:
   virtual bool RealDecRef();
   //@}

protected:
   /// remove the folder from our "closer" list
   virtual void Close(void);

   /// is updating currently suspended?
   bool IsUpdateSuspended() const { return m_suspendUpdates != 0; }

   /// apply filters to all new mail messages
   virtual bool FilterNewMail();

   /// move new mail to the incoming folder if necessary
   virtual bool CollectNewMail();

   /// really count messages
   virtual bool DoCountMessages(MailFolderStatus *status) const = 0;

   /** @name Config management */
   //@{
   struct MFCmnOptions
   {
      MFCmnOptions();

      bool operator!=(const MFCmnOptions& other) const;

      bool operator==(const MFCmnOptions& other) const
      {
         return !(*this != other);
      }

      /// how to sort the list of messages
      long m_ListingSortOrder;
      /// do we want to re-sort it on a status change?
      bool m_ReSortOnChange;

      /// Timer update interval for checking folder content
      int m_UpdateInterval;

      /// do we want to thread messages?
      bool m_UseThreading;

#if defined(EXPERIMENTAL_JWZ_THREADING)
#if wxUSE_REGEX
      /// the strings to use to bring subject to canonical form
      String m_SimplifyingRegex;
      String m_ReplacementString;
#else // !wxUSE_REGEX
      /// Should we remove list prefix when comparing subjects to gather them
      bool m_RemoveListPrefixGathering;
      /// Should we remove list prefix when comparing subjects to break threads
      bool m_RemoveListPrefixBreaking;
#endif // wxUSE_REGEX/!wxUSE_REGEX

      /// SHould we gather in same thread messages with same subject
      bool m_GatherSubjects;
      /// Should we break thread when subject changes
      bool m_BreakThread;

      /// Should we indent messages with missing ancestor
      bool m_IndentIfDummyNode;
#endif // EXPERIMENTAL_JWZ_THREADING

      /// do we use "To" instead of "From" for messages from oneself?
      bool m_replaceFromWithTo;
      /// if m_replaceFromWithTo, the list of the addresses for "oneself"
      wxArrayString m_ownAddresses;
   } m_Config;

   /// Use the new options from m_Config
   virtual void DoUpdate();

   /// Update m_Config info from profile.
   void UpdateConfig(void);

   /// Read options from profile into the options struct
   virtual void ReadConfig(MailFolderCmn::MFCmnOptions& config);
   //@}

   /// Constructor
   MailFolderCmn();

   /// Destructor
   ~MailFolderCmn();
   /**@name All used to build listings */
   //@{

   /// generate NewMail messages if needed
   void CheckForNewMail(HeaderInfoList *hilp);

   /** Check if this message is a "New Message" for generating new
       mail event. */
   virtual bool IsNewMessage(const HeaderInfo * hi) = 0;
   //@}

   /** @name The listing information */
   //@{
   /// our listing or NULL if not created yet
   HeaderInfoList *m_headers;

   /// The last seen new UID, to check for new mails:
   UIdType m_LastNewMsgUId;
   //@}

   /** @name Auth info */
   //@{
   /// Login for password protected mail boxes.
   String m_Login;
   /// Password for password protected mail boxes.
   String m_Password;
   //@}

   /// a timer to update information
   class MailFolderTimer *m_Timer;

private:
   /// We react to config change events.
   class MEventReceiver *m_MEventReceiver;

   /// suspend update count (updating is suspended if it is > 0)
   size_t m_suspendUpdates;

   /// the frame to which messages for this folder go by default
   MFrame *m_frame;

   friend class MfCloseEntry;
   friend class MFCmnEventReceiver;
};

#endif // MAILFOLDERCMN_H

